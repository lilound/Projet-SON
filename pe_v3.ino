#include <Audio.h>
#include "MyDsp.h"

MyDsp myDsp;
AudioOutputI2S out;
AudioControlSGTL5000 audioShield;

AudioConnection patchCord1(myDsp, 0, out, 0);
AudioConnection patchCord2(myDsp, 1, out, 1);

int buttonState = 0;
int oldButtonState = 0;
float freq = 125.0;
int init_count = millis();
bool passer = false;
int ind = 0;

//Gauche ou droite ? 
int earMode = 0 ;

// surdité 
float sourdG[20], sourdD[20];
int indG = 0, indD = 0;

// pointeurs pour les fonctions 
float* sourdActuel; 
int* indActuel;

// si il faut pousser le diagnostique (certaines fréquences ne sont pas entendues):
bool enDiagnostic = false;
int indexTrouEnCours = 0;
float freqDepartDiag;

//à comprendre plus tard 
struct Intervalle {
  float debut;
  float fin;
};

Intervalle trousPrecisG[10], trousPrecisD[10];
int nbTrousG = 0, nbTrousD = 0;

void setup() {
  pinMode(0, INPUT);
  AudioMemory(16);
  audioShield.enable();
  audioShield.volume(0.025); 
  // Initialisation des pointeurs sur la Gauche au départ
  sourdActuel = sourdG;
  indActuel = &indG;
  myDsp.setEar(earMode);
}

void loop() {
  // --- SÉCURITÉ ANTI-CRASH ---
  if (freq > 17000.0 || freq < 20.0) { 
    freq = 125.0;
    if (earMode == 0){
      earMode = 1;
      myDsp.setEar(earMode);
      // switch des pointeurs pour enregistrer à droite 
      sourdActuel = sourdD;
      indActuel = &indD;
    }
    else if (earMode == 1){
      // dcp : si jamais on est arrivé à 16000 et que liste sourd non vide : on lance fonction de tests et on recup resultats pour savoir l'intervale précis de surdité
      

      lancerDiagnostic(sourdG, indG, 0, trousPrecisG, &nbTrousG); // 0 pour Gauche
      lancerDiagnostic(sourdD, indD, 1, trousPrecisD, &nbTrousD); // 1 pour Droite
      myDsp.setMute(true);
    
      // Affichage des intervalles réels
      Serial.println("\n========= RÉSULTATS PRÉCIS =========");
      for(int i=0; i<nbTrousG; i++) {
          Serial.print("Trou Oreille G : "); Serial.print(trousPrecisG[i].debut); 
          Serial.print("Hz - "); Serial.print(trousPrecisG[i].fin); Serial.println("Hz");
      }
      for(int i=0; i<nbTrousD; i++) {
          Serial.print("Trou Oreille D : "); Serial.print(trousPrecisD[i].debut); 
          Serial.print("Hz - "); Serial.print(trousPrecisD[i].fin); Serial.println("Hz");
      }
      while(1);
    }
    
  }
  myDsp.setFreq(freq);
  // On règle aussi le filtre pour qu'il suive la fréquence (sinon on n'entend rien)
  myDsp.setFilter(15.0, freq, 100.0);

  delay(10);
  if ((millis() - init_count) >= 6000){
    sourdActuel[*indActuel] = freq; // On remplit le tableau pointé
    Serial.println(sourdActuel[*indActuel]);
    (*indActuel)++;
    freq *= 2.0; 
    passer = true;
    init_count = millis(); 
  }
  buttonState = digitalRead(0);
  //Serial.println(buttonState);
  if (buttonState and !oldButtonState) {
    Serial.print("la fréquence "); Serial.print(freq); Serial.println(" est entendue.");
    init_count = millis(); 
    myDsp.setMute(true);
    oldButtonState = 1;
    passer = true;
  }

  else if (passer){
    //Serial.print(freq); Serial.println(" pas entendue."); 
    passer = false; 
    myDsp.setMute(true);
    delay(1000);
    myDsp.setMute(false);
    oldButtonState = 0;
    freq *= 2.0;
    audioShield.volume(0.025); 
    
  }
  
}

void lancerDiagnostic(float* listeSourd, int nombreSourd, int ear, Intervalle* resultats, int* nbResultats) {
  myDsp.setEar(ear);
  
  for (int i = 0; i < nombreSourd; i++) {
    float fCentrale = listeSourd[i];
    float fBas = fCentrale / 2.0; // On repart de la dernière fréquence entendue
    float fHaut = fCentrale;
    
    Serial.print("\n--- Analyse fine autour de "); Serial.print(fCentrale); Serial.println(" Hz ---");
    
    float debutTrou = 0;
    bool dansLeTrou = false;

    // Balayage fin : on divise l'intervalle en 10 paliers
    float pas = (fHaut - fBas) / 10.0;
    
    for (float f = fBas; f <= fHaut; f += pas) {
      myDsp.setFreq(f);
      myDsp.setFilter(20.0, f, 40.0); // Les FILTRES EN SÉRIE rendent ce pic super précis
      
      unsigned long startLocal = millis();
      bool entendu = false;
      
      // On attend 2 secondes que l'utilisateur appuie
      while (millis() - startLocal < 5000) {
        if (digitalRead(0) == HIGH) {
          entendu = true;
          myDsp.setMute(true);
          break;
        }
      }

      if (!entendu && !dansLeTrou) {
        debutTrou = f;
        dansLeTrou = true;
        Serial.print("Début du silence détecté à : "); Serial.println(f);
      } 
      else if (entendu && dansLeTrou) {
        resultats[*nbResultats] = {debutTrou, f};
        (*nbResultats)++;
        dansLeTrou = false;
        Serial.print("Fin du silence détectée à : "); Serial.println(f);
      }
      
      // Petit silence entre deux tests pour ne pas saturer l'oreille
      
      myDsp.setMute(true);
      delay(1000);
      myDsp.setMute(false);
      audioShield.volume(0.025);
    }
  }
}
