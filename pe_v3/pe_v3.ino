#include <Audio.h>
#include "MyDsp.h"

MyDsp myDsp;
AudioOutputI2S out;
AudioControlSGTL5000 audioShield;
AudioConnection patchCord1(myDsp, 0, out, 0);
AudioConnection patchCord2(myDsp, 1, out, 1);

// --- CONFIGURATION AUDIOGRAMME ---
float frequencesStandard[] = {125, 250, 500, 1000, 2000, 4000, 8000};
int indexFreq = 0;
int totalFreq = 7;

// --- RÉGLAGES DU DIAGNOSTIC ---
float seuilDiagnosticdB = 40.0; // <--- MODIFIEZ CETTE VALEUR ICI (ex: 25.0 pour plus de sensibilité)
float dbPerteHL = 0.0; 
unsigned long tempsDebutPalier = 0;
bool enPauseEntreFrequences = false;

int buttonState = 0;
int oldButtonState = 0;
int earMode = 0; 

struct Resultat {
  float frequence;
  float perteDB;
};
Resultat sourdG[7], sourdD[7];
int indG = 0, indD = 0;

struct Intervalle {
  float debut;
  float fin;
};
Intervalle trousPrecisG[10], trousPrecisD[10];
int nbTrousG = 0, nbTrousD = 0;

// --- PROTOTYPES ---
float dbToLin(float dbPerte);
void enregistrerResultat(float f, float db);
void preparerFrequenceSuivante();
void passerAOreilleSuivante();
void afficherResultats();
void lancerDiagnosticZones(Resultat* resultats, int nbRes, int ear, Intervalle* trous, int* nbTrous);

void setup() {
  pinMode(0, INPUT);
  AudioMemory(16);
  audioShield.enable();
  audioShield.volume(0.5); 
  
  myDsp.setEar(earMode);
  myDsp.setMute(false);
  tempsDebutPalier = millis();
  Serial.begin(9600);
  Serial.print("Diagnostic configuré pour seuil >= "); Serial.print(seuilDiagnosticdB); Serial.println(" dB HL");
}

void loop() {
  float freqActuelle = frequencesStandard[indexFreq];

  if (!enPauseEntreFrequences) {
    myDsp.setFreq(freqActuelle);
    myDsp.setFilter(15.0, freqActuelle, 100.0);
    audioShield.volume(dbToLin(dbPerteHL)); 
    
    // Sortie Plotter
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

// --- LOGIQUE DE DIAGNOSTIC AVEC VARIABLE ---
void lancerDiagnosticZones(Resultat* res, int nbRes, int ear, Intervalle* trous, int* nbTrous) {
  myDsp.setEar(ear);
  for (int i = 0; i < nbRes; i++) {
    // On utilise ici la variable globale pour le test
    if (res[i].perteDB >= seuilDiagnosticdB) {
      Serial.print("!!! Zone suspecte détectée à "); Serial.print(res[i].frequence); Serial.println(" Hz");
      
      float fBase = res[i].frequence / 1.414;
      float fHaut = res[i].frequence * 1.414;
      float debutTrou = 0;
      bool dansLeTrou = false;
      
      // On teste à un volume constant (le seuil trouvé) pour voir la largeur du trou
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

// --- FONCTIONS DE GESTION (STABLES) ---
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
    while(1); 
  }
}

void enregistrerResultat(float f, float db) {
  if (earMode == 0) { sourdD[indD++] = {f, db}; } 
  else { sourdG[indG++] = {f, db}; }
}

void afficherResultats() {
  // On envoie une ligne de début pour que Python sache quand commencer à lire
  Serial.println("START_DATA");
  for(int i=0; i<totalFreq; i++) {
    Serial.print(frequencesStandard[i]); Serial.print(",");
    Serial.print(sourdD[i].perteDB); Serial.print(",");
    Serial.println(sourdG[i].perteDB);
  }
  Serial.println("END_DATA");

}
