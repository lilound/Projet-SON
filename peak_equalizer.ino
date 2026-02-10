#include <Audio.h>
#include "MyDsp.h"

// Objets Audio
MyDsp myDsp;
AudioInputI2S input;          // On ajoute l'entrée Jack
AudioOutputI2S out;
AudioControlSGTL5000 audioShield;

// Connexions : Entrée Jack -> Ton DSP -> Sortie Casque (L et R)
AudioConnection patchCord0(input, 0, myDsp, 0); 
AudioConnection patchCord1(myDsp, 0, out, 0);
AudioConnection patchCord2(myDsp, 0, out, 1);

void setup() {
  AudioMemory(12); // On donne un peu plus de mémoire pour être tranquille
  audioShield.enable();
  audioShield.volume(0.5);
  audioShield.inputSelect(AUDIO_INPUT_LINEIN); // Utilise l'entrée Jack "Line In"

  // On règle ton filtre une fois au début
  // Paramètres : (Gain en dB, Fréquence en Hz, Largeur de bande en Hz)
  myDsp.setParameters(10.0, 1000.0, 200.0); 
}

void loop() {
  // On laisse vide pour l'instant, le filtrage se fait tout seul en arrière-plan
}