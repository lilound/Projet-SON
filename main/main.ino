#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "Filters.h"
#include "MyDsp.h"
#include "Additive.h"

// --- ÉTATS DU SYSTÈME ---
bool modeDiagnostic = false;   
bool modeCorrection = false; 

// --- OBJETS AUDIO ---
AudioInputI2S            in;
AudioOutputI2S           out;
AudioControlSGTL5000     audioShield;

MyDsp                    myDsp; 
Filters                  myFilters; // Objet Faust (7 bandes, stéréo)

// --- CONNEXIONS AUDIO ---
AudioConnection          patchCordIn(in, 0, myDsp, 0); 
AudioConnection          patchCordToFiltL(myDsp, 0, myFilters, 0);
AudioConnection          patchCordToFiltR(myDsp, 1, myFilters, 1);
AudioConnection          patchCordOutL(myFilters, 0, out, 0);
AudioConnection          patchCordOutR(myFilters, 1, out, 1);

// --- VARIABLES GLOBALES ---
float frequencesStandard[] = {125, 250, 500, 1000, 2000, 4000, 8000};
int indexFreq = 0;
int totalFreq = 7;
float seuilDiagnosticdB = 40.0; 
float dbPerteHL = 0.0; 
float derniereDbEnvoyee = -100.0; 
unsigned long tempsDebutPalier = 0;
bool enPauseEntreFrequences = false;
unsigned long momentFinPause = 0;

int buttonState = 0;
int oldButtonState = 0;
int earMode = 2; 

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
void mettreAJourFiltresSimulation(String commande);
void ajouterAcouphene(String commande);

// ================================================================
// SETUP
// ================================================================
void setup() {
  Serial.begin(9600);
  pinMode(0, INPUT_PULLDOWN); // Sécurise le bouton physique
   
  audioShield.enable();
  audioShield.inputSelect(AUDIO_INPUT_MIC);
  audioShield.micGain(30); 
  audioShield.volume(0.5);

  myDsp.setEar(earMode);
  myDsp.setMute(true); 
  
  AudioMemory(40);

  // Initialisation des filtres Faust à 0dB
  for(int i=0; i<7; i++) {
    myFilters.setParamValue(("/Filters/level_0" + String(i)).c_str(), 0.0);
    myFilters.setParamValue(("/Filters/level_1" + String(i)).c_str(), 0.0);
  }
}

// ================================================================
// LOOP PRINCIPALE
// ================================================================
void loop() {
  if (Serial.available() > 0) {
    String commande = Serial.readStringUntil('\n');
    commande.trim();

    if (commande == "START_DIAG") {
      for(int i=0; i<7; i++) {
        myFilters.setParamValue(("/Filters/level_0" + String(i)).c_str(), 0.0);
        myFilters.setParamValue(("/Filters/level_1" + String(i)).c_str(), 0.0);
      }
      myDsp.setDiagnostic(true);
      modeDiagnostic = true;
      modeCorrection = false;
      myDsp.setMute(false); 
      indexFreq = 0;
      dbPerteHL = 0.0;
      derniereDbEnvoyee = -100.0;
      earMode = 0;
      myDsp.setEar(earMode);
      enPauseEntreFrequences = false;
      tempsDebutPalier = millis();
    } 
    else if (commande == "STOP") {
        myDsp.setDiagnostic(false);
        mettreAJourFiltresSimulation("0,0,0,0,0,0,0");
        modeDiagnostic = false;
        modeCorrection = false;
        myDsp.setMute(true);
        myDsp.setAcouphene(false, 0, 0);
        earMode = 2;
        myDsp.setEar(earMode);
        audioShield.volume(0.5);
        Serial.println("ABORT_DIAG");
    }
    else if (commande.indexOf(',') > 0) {
      if (commande.indexOf(';') < 0){
        mettreAJourFiltresSimulation(commande); 
        modeDiagnostic = false;
        modeCorrection = true;
        myDsp.setMute(false); 
      }
      else { 
        ajouterAcouphene(commande);
        modeDiagnostic = false;
        modeCorrection = true;
        myDsp.setMute(false); 
      }
    }
  }

  // Exécution des modes
  if (modeDiagnostic) {
    loopDiagnostic();
  } 
  else if (modeCorrection) {
    myDsp.setDiagnostic(false); 
    myDsp.setMute(false); 
  }
  else {
    myDsp.setDiagnostic(false);
    myDsp.setMute(true); 
  }
}

// ================================================================
// LOGIQUE DU TEST AUDITIF
// ================================================================
void loopDiagnostic() {
  // 1. Lecture STOP prioritaire
  if (Serial.available() > 0) {
    String commande = Serial.readStringUntil('\n');
    commande.trim();
    if (commande == "STOP") {
        myDsp.setDiagnostic(false);
        modeDiagnostic = false;
        myDsp.setMute(true);
        earMode = 2;
        myDsp.setEar(earMode);
        Serial.println("ABORT_DIAG");
        return;
    }
  }

  // 2. Gestion de la pause
  if (enPauseEntreFrequences) {
    if (millis() >= momentFinPause) {
      enPauseEntreFrequences = false;
      derniereDbEnvoyee = -100.0; 
      myDsp.setMute(false); 
      tempsDebutPalier = millis();
    } else {
      return; 
    }
  }

  // 3. Audio & Envoi PC
  float freqActuelle = frequencesStandard[indexFreq];
  myDsp.setFreq(freqActuelle);
  audioShield.volume(dbToLin(dbPerteHL)); 

  if (dbPerteHL != derniereDbEnvoyee) {
    derniereDbEnvoyee = dbPerteHL;
    tempsDebutPalier = millis();
  }

  // 4. Bouton
  buttonState = digitalRead(0);
  if (buttonState == HIGH && oldButtonState == LOW) {
    myDsp.setMute(true);
    enregistrerResultat(freqActuelle, dbPerteHL);
    preparerFrequenceSuivante();
    oldButtonState = HIGH;
  } 
  else if (buttonState == LOW) {
    oldButtonState = LOW;
    if (millis() - tempsDebutPalier > 2000) { 
      dbPerteHL += 10.0; 
      if (dbPerteHL > 70.0) { 
        enregistrerResultat(freqActuelle, 70.0);
        preparerFrequenceSuivante();
      }
      tempsDebutPalier = millis();
    }
  }
}

// ================================================================
// FONCTIONS FILTRES & SIMULATION
// ================================================================
void mettreAJourFiltresSimulation(String commande) {
  unsigned int startIndex = 0;
  
  for (int i = 0; i < 7; i++) {
    int endIndex = commande.indexOf(',', startIndex);
    if (endIndex == -1) endIndex = commande.length();
    float gain = commande.substring(startIndex, endIndex).toFloat();
    if (gain < 0){
      gain = gain/3;
    }

    myFilters.setParamValue(("/Filters/level_0" + String(i)).c_str(), gain);
    myFilters.setParamValue(("/Filters/level_1" + String(i)).c_str(), gain);
    
    startIndex = endIndex + 1;
    if (startIndex >= commande.length()) break;
  }
  Serial.println("FILTRES_OK");
}

void ajouterAcouphene(String commande){ 
  int iAge = commande.indexOf(';');
  if (iAge < 0) return;
  float age = commande.substring(0, iAge).toFloat();

  int iFreqAc = commande.indexOf(';', iAge + 1);
  if (iFreqAc < 0) return;
  float freqAcouphene = commande.substring(iAge + 1, iFreqAc).toFloat();

  String gains = commande.substring(iFreqAc + 1);
  mettreAJourFiltresSimulation(gains);
  
  modeCorrection = true; 
  modeDiagnostic = false;
  myDsp.setMute(false);
  
  if (freqAcouphene > 0) {
    myDsp.setAcouphene(true, freqAcouphene, age);
    Serial.println("ACOUPHENE_ON");
  } else {
    myDsp.setAcouphene(false, 0, 0);
  }
}

// ================================================================
// UTILITAIRES
// ================================================================
float dbToLin(float dbPerte) {
  float gainBase = -60.0; 
  float dbReel = gainBase + dbPerte;
  float amplitudeMax = 0.15; 
  return amplitudeMax * pow(10.0, dbReel / 20.0);
}

void enregistrerResultat(float f, float db) {
  if (earMode == 0) { sourdD[indD++] = {f, db}; } 
  else { sourdG[indG++] = {f, db}; }
}

void preparerFrequenceSuivante() {
  myDsp.setMute(true);
  enPauseEntreFrequences = true;
  momentFinPause = millis() + 600; 
  indexFreq++;
  if (indexFreq >= totalFreq) {
    passerAOreilleSuivante();
  } else {
    dbPerteHL = 0.0; 
  }
}

void passerAOreilleSuivante() {
  if (earMode == 0) {
    earMode = 1;    
    myDsp.setEar(earMode);
    indexFreq = 0;
    dbPerteHL = 0.0;
    enPauseEntreFrequences = false;
    myDsp.setMute(false);
    tempsDebutPalier = millis();
  } else {
    afficherResultats();
    Serial.flush(); 
    modeDiagnostic = false;
    modeCorrection = false;
    earMode = 2;
    myDsp.setEar(earMode);
    myDsp.setMute(true); 
    audioShield.volume(0.5);
  }
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
