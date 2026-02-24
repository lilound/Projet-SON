#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// Inclusion de tes classes personnalisées
#include "Peak.h"
#include "MyDsp.h"
#include "Additive.h"

// --- ÉTATS DU SYSTÈME ---
bool modeDiagnostic = true;   // Commence par le diagnostic
bool modeCorrection = false;  // S'active après ou via un bouton

// --- OBJETS AUDIO ---
// Entrée/Sortie Physique
AudioInputI2S            in;
AudioOutputI2S           out;
AudioControlSGTL5000     audioShield;

// Objets pour le DIAGNOSTIC (pe_v3.ino)
MyDsp                    myDsp; 

// Connexions Audio
// Chemin 1 : Diagnostic
AudioConnection          patchCordDiag1(myDsp, 0, out, 0);
AudioConnection          patchCordDiag2(myDsp, 1, out, 1);

// Chemin 2 : Correction 
AudioConnection          patchCordCorr1(in, 0, myDsp, 0);


// --- VARIABLES GLOBALES (Issues des deux codes) ---

// Filtres pour la correction
PeakFilter filtres[7] = {
  PeakFilter(AUDIO_SAMPLE_RATE_EXACT),
  PeakFilter(AUDIO_SAMPLE_RATE_EXACT),
  PeakFilter(AUDIO_SAMPLE_RATE_EXACT),
  PeakFilter(AUDIO_SAMPLE_RATE_EXACT),
  PeakFilter(AUDIO_SAMPLE_RATE_EXACT),
  PeakFilter(AUDIO_SAMPLE_RATE_EXACT),
  PeakFilter(AUDIO_SAMPLE_RATE_EXACT)
};

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
  audioShield.volume(0.5);

  // Initialisation Diagnostic
  myDsp.setEar(earMode);
  myDsp.setMute(false);
  tempsDebutPalier = millis();
  
}

// ================================================================
// GESTION DES MESSAGES
// ================================================================
void loop() {
  // --- 1. LECTURE DES COMMANDES PYTHON ---
  if (Serial.available() > 0) {
    String commande = Serial.readStringUntil('\n');
    commande.trim(); // Nettoie les données reçues

    if (commande == "START_DIAG") {
      myDsp.setDiagnostic(true);
      modeDiagnostic = true;
      modeCorrection = false;
      myDsp.setMute(false); // Active le générateur
    } 
    else if (commande == "START_CORR") {
      modeDiagnostic = false;
      modeCorrection = true;
      myDsp.setMute(true);  // Coupe le générateur

    }
    else if (commande == "STOP") {
      myDsp.setDiagnostic(false);
      modeDiagnostic = false;
      modeCorrection = false;
      myDsp.setMute(true);
    }
    // Si la commande contient la liste de points (ex: "10,20,30...")
    else if (commande.indexOf(',') > 0) {
      int startIndex = 0;
        for (int i = 0; i < 7; i++) {
          int endIndex = commande.indexOf(',', startIndex);
          if (endIndex == -1) endIndex = commande.length();
          float gain = commande.substring(startIndex, endIndex).toFloat();
          float freq = 125.0 * pow(2, i);
          myDsp.setFilter(i, gain, freq, freq * 0.5); // Mise à jour du filtre i
          startIndex = endIndex + 1;
        }
    }
  }
  

  // --- 2. EXÉCUTION DES MODES ---
  if (modeDiagnostic) {
    loopDiagnostic();
  } 
  if (modeCorrection) {
      for (int i = 0; i < 7; i++) {
        myDsp.setFilter(i, 0.0, frequencesStandard[i], 1.0);
      }
    }
  
}


// ================================================================
// DIAGNOSTIC
// ================================================================
void loopDiagnostic() {
  float freqActuelle = frequencesStandard[indexFreq];

  if (!enPauseEntreFrequences) {
    myDsp.setFreq(freqActuelle);
    myDsp.setFilter(0,0.0f, freqActuelle, 1.0f);
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
    Serial.println("--- PASSAGE EN MODE CORRECTION ---");

    audioShield.inputSelect(AUDIO_INPUT_MIC);
    audioShield.micGain(20); // Remet le gain micro nécessaire pour la correction
    audioShield.volume(0.5);  // Monte le volume général comme dans mic.ino
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
  myDsp.setMute(true);
}
