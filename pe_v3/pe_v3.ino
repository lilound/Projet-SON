#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// Inclusion de tes classes personnalisées
#include "Peak.h"
#include "MyDsp.h"

// --- ÉTATS DU SYSTÈME ---
bool modeDiagnostic = true;   // Commence par le diagnostic
bool modeCorrection = false;  // S'active après ou via un bouton

// --- OBJETS AUDIO ---
// Entrée/Sortie Physique
AudioInputI2S            in;
AudioOutputI2S           out;
AudioControlSGTL5000     audioShield;

// Objets pour la CORRECTION (mic.ino)
AudioRecordQueue         queue; 
AudioPlayQueue           play;  

// Objets pour le DIAGNOSTIC (pe_v3.ino)
MyDsp                    myDsp; 

// Connexions Audio
// Chemin 1 : Diagnostic (Générateur -> Sortie)
AudioConnection          patchCordDiag1(myDsp, 0, out, 0);
AudioConnection          patchCordDiag2(myDsp, 1, out, 1);

// Chemin 2 : Correction (Micro -> Queue -> Traitement -> Play -> Sortie)
AudioConnection          patchCordCorr1(in, 0, queue, 0);
AudioConnection          patchCordCorr2(play, 0, out, 0);
AudioConnection          patchCordCorr3(play, 0, out, 1);

// --- VARIABLES GLOBALES (Issues des deux codes) ---

// Filtres pour la correction
PeakFilter filtre1(AUDIO_SAMPLE_RATE_EXACT);

// Variables de diagnostic
float frequencesStandard[] = {125, 250, 500, 1000, 2000, 4000, 8000};
int indexFreq = 0;
int totalFreq = 7;
float seuilDiagnosticdB = 40.0; 
float dbPerteHL = 0.0; 
unsigned long tempsDebutPalier = 0;
bool enPauseEntreFrequences = false;

int buttonState = 0;
int oldButtonState = 0;
int earMode = 0; 

struct Resultat { float frequence; float perteDB; };
Resultat sourdG[7], sourdD[7];
int indG = 0, indD = 0;

struct Intervalle { float debut; float fin; };
Intervalle trousPrecisG[10], trousPrecisD[10];
int nbTrousG = 0, nbTrousD = 0;

// --- PROTOTYPES ---
float dbToLin(float dbPerte);
void enregistrerResultat(float f, float db);
void preparerFrequenceSuivante();
void passerAOreilleSuivante();
void afficherResultats();
void lancerDiagnosticZones(Resultat* resultats, int nbRes, int ear, Intervalle* trous, int* nbTrous);

// ================================================================
// SETUP
// ================================================================
void setup() {
  Serial.begin(9600);
  pinMode(0, INPUT);
  
  // Alloue de la mémoire pour les deux modes
  AudioMemory(20); 
  
  audioShield.enable();
  audioShield.inputSelect(AUDIO_INPUT_MIC);
  audioShield.micGain(20); 
  audioShield.volume(0.8);

  // Initialisation des filtres de correction
  filtre1.setup(-150.0f, 10000.0f, 10000.0f);

  // Initialisation Diagnostic
  myDsp.setEar(earMode);
  myDsp.setMute(false);
  tempsDebutPalier = millis();
  
  if(modeDiagnostic) {
    Serial.println("MODE DIAGNOSTIC ACTIF");
    queue.end(); // On ne capture pas le micro pendant le diag
  }
}

// ================================================================
// LOOP (SÉPARÉ PAR VARIABLES)
// ================================================================
void loop() {
  //Serial.println("Passage au mode diagnostic hihi");
  
  // --- BRANCHE DIAGNOSTIC ---
  if (modeDiagnostic) {
    loopDiagnostic();
    //Serial.println("Passage au mode correction hihi");    
  }

  // --- BRANCHE CORRECTION ---
  if (modeCorrection) {
    loopCorrection();
  }
}

// ================================================================
// LOGIQUE DE MIC.INO (CORRECTION)
// ================================================================
void loopCorrection() {
  // 1. Vérifier si au moins un bloc de 128 échantillons est disponible dans la file d'attente
  if (queue.available() >= 1) {
    
    // 2. Récupérer les pointeurs vers les buffers (Entrée et Sortie)
    int16_t *inbuffer = queue.readBuffer();  // Lit le buffer entrant
    int16_t *outBuffer = play.getBuffer();   // Récupère un buffer vide pour la sortie

    // 3. Sécurité : On ne traite que si le buffer de sortie est bien alloué
    if (outBuffer != NULL) {
      for (int i = 0; i < 128; i++) {
        
        // --- TRAITEMENT IDENTIQUE À MIC.INO ---
        
        // Conversion entier (int16) -> float normalisé (-1.0 à 1.0)
        float sample = (float)inbuffer[i] / 32768.0f;

        // Application du filtre Peak (filtre1)
        float filteredSample = filtre1.tick(sample);

        // Conversion float -> entier non normalisé
        float scaled = filteredSample * 32767.0f;

        // Sécurité anti-clipping (écrêtage) pour éviter les bruits numériques violents
        if (scaled > 32767.0f) scaled = 32767.0f;
        if (scaled < -32768.0f) scaled = -32768.0f;
        
        // Stockage dans le buffer de sortie
        outBuffer[i] = (int16_t)scaled;
      }

      // 4. Envoi effectif du buffer traité vers le matériel (casque/HP)
      play.playBuffer(); 
    }

    // 5. IMPORTANT : Libérer le buffer d'entrée pour la prochaine capture
    queue.freeBuffer(); 
  }
}

// ================================================================
// LOGIQUE DE PE_V3.INO (DIAGNOSTIC)
// ================================================================
void loopDiagnostic() {
  float freqActuelle = frequencesStandard[indexFreq];

  if (!enPauseEntreFrequences) {
    myDsp.setFreq(freqActuelle);
    myDsp.setFilter(15.0, freqActuelle, 100.0);
    audioShield.volume(dbToLin(dbPerteHL)); 
    
    Serial.print(freqActuelle); Serial.print(" ");
    Serial.println(-dbPerteHL); 
  }

  buttonState = digitalRead(0);

  if (buttonState && !oldButtonState) {
    myDsp.setMute(true);
    enregistrerResultat(freqActuelle, dbPerteHL);
    preparerFrequenceSuivante();
    oldButtonState = 1;
  } 
  else if (!buttonState) {
    if (oldButtonState == 1) oldButtonState = 0;
    if (millis() - tempsDebutPalier > 4000) {
      dbPerteHL += 10.0; 
      if (dbPerteHL > 61.0) { 
        enregistrerResultat(freqActuelle, 70.0);
        preparerFrequenceSuivante();
      }
      tempsDebutPalier = millis();
    }
  }
}

// ================================================================
// FONCTIONS UTILITAIRES (Diagnostic)
// ================================================================

void passerAOreilleSuivante() {
  if (earMode == 0) {
    lancerDiagnosticZones(sourdD, indD, 0, trousPrecisD, &nbTrousD);
    earMode = 1;
    myDsp.setEar(earMode);
    indexFreq = 0;
    dbPerteHL = 0.0;
    enPauseEntreFrequences = false;
    myDsp.setMute(false);
    tempsDebutPalier = millis();
  } else {
    lancerDiagnosticZones(sourdG, indG, 1, trousPrecisG, &nbTrousG);
    afficherResultats();
    
    // FIN DU DIAGNOSTIC -> PASSAGE AUTOMATIQUE EN CORRECTION
    modeDiagnostic = false;
    modeCorrection = true;
    myDsp.setMute(true); // Coupe le générateur
    queue.begin();       // Lance la capture micro
    Serial.println("--- PASSAGE EN MODE CORRECTION ---");

    audioShield.inputSelect(AUDIO_INPUT_MIC);
    audioShield.micGain(20); // Remet le gain micro nécessaire pour la correction
    audioShield.volume(0.8);  // Monte le volume général comme dans mic.ino
    queue.begin();           // Relance la capture
    }
}

// Garder ici toutes tes autres fonctions : dbToLin, enregistrerResultat, lancerDiagnosticZones, etc.
// (Elles restent identiques à ton code d'origine)

float dbToLin(float dbPerte) {
  float gainBase = -60.0; 
  float dbReel = gainBase + dbPerte;
  float amplitudeMax = 0.15; 
  return amplitudeMax * pow(10.0, dbReel / 20.0);
}

void preparerFrequenceSuivante() {
  myDsp.setMute(true);
  enPauseEntreFrequences = true;
  delay(2000); 
  indexFreq++;
  if (indexFreq >= totalFreq) {
    passerAOreilleSuivante();
  } else {
    dbPerteHL = 0.0; 
    enPauseEntreFrequences = false;
    myDsp.setMute(false);
    tempsDebutPalier = millis();
  }
}

void enregistrerResultat(float f, float db) {
  if (earMode == 0) { sourdD[indD++] = {f, db}; } 
  else { sourdG[indG++] = {f, db}; }
}

void afficherResultats() {
  Serial.println("START_DATA");
  for(int i=0; i<totalFreq; i++) {
    Serial.print(frequencesStandard[i]); Serial.print(",");
    Serial.print(sourdD[i].perteDB); Serial.print(",");
    Serial.println(sourdG[i].perteDB);
  }
  Serial.println("END_DATA");
}

void lancerDiagnosticZones(Resultat* res, int nbRes, int ear, Intervalle* trous, int* nbTrous) {
  myDsp.setEar(ear);
  for (int i = 0; i < nbRes; i++) {
    if (res[i].perteDB >= seuilDiagnosticdB) {
      float fBase = res[i].frequence / 1.414;
      float fHaut = res[i].frequence * 1.414;
      float debutTrou = 0;
      bool dansLeTrou = false;
      audioShield.volume(dbToLin(res[i].perteDB)); 

      for (float f = fBase; f <= fHaut; f *= 1.1) {
        myDsp.setFreq(f);
        myDsp.setMute(false);
        unsigned long start = millis();
        bool entendu = false;
        while(millis() - start < 2000) {
          if(digitalRead(0) == HIGH) { entendu = true; break; }
        }
        if (!entendu && !dansLeTrou) {
          debutTrou = f; dansLeTrou = true;
        } else if (entendu && dansLeTrou) {
          trous[*nbTrous] = {debutTrou, f};
          (*nbTrous)++;
          dansLeTrou = false;
        }
        myDsp.setMute(true);
        delay(200);
      }
    }
  }
}
