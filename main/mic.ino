#include <Audio.h>
#include "Peak.h"
#include "MyDsp.h"

AudioInputI2S            in;
MyDsp                    myDsp;    // Votre processeur d'effets
AudioOutputI2S           out;
AudioControlSGTL5000     audioShield;

// Branchement : Micro -> MyDsp -> Sorties (L et R)
AudioConnection          patch1(in, 0, myDsp, 0);
AudioConnection          patch2(myDsp, 0, out, 0);
AudioConnection          patch3(myDsp, 1, out, 1);

void setup() {
  Serial.begin(9600);
  pinMode(0, INPUT);
  AudioMemory(12); // Alloue 12 blocs de mémoire pour transporter et stocker les données audio entre les composants
  audioShield.enable();
  audioShield.inputSelect(AUDIO_INPUT_MIC);
  audioShield.micGain(20); // in dB NE PAS METTRE EN COMMENTAIRE OU SURDITE INEVITABLE
  audioShield.volume(0.8);

  // Configuration initiale du filtre
  myDsp.setFilter(15.0f, 1000.0f, 1.0f); 
  myDsp.setEar(2); // Son sur les deux oreilles
}

void loop() {
  int buttonState = digitalRead(0);

  myDsp.setMute(!buttonState); 
  
  delay(10); 
}