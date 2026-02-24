#include <Audio.h>
#include "Peak.h"


AudioInputI2S in;
AudioOutputI2S out;
AudioControlSGTL5000 audioShield;
AudioRecordQueue queue; // Pour récupérer les données
AudioPlayQueue play;  // Pour renvoyer les données traitées
AudioConnection patchCord1(in, 0, queue, 0); // branche l'entrée micro (canal gauche, index 0) vers la "file d'attente" de traitement (queue)
AudioConnection patchCord2(play, 0, out, 0); // branche la sortie du traitement (play) vers le casque gauche (out, index 0)
AudioConnection patchCord3(play, 0, out, 1); // idem à droite

PeakFilter filtre1(AUDIO_SAMPLE_RATE_EXACT);
PeakFilter filtre2(AUDIO_SAMPLE_RATE_EXACT);

int buttonState = 0;

void setup() {
  Serial.begin(9600);
  pinMode(0, INPUT);
  AudioMemory(12); // Alloue 12 blocs de mémoire pour transporter et stocker les données audio entre les composants
  audioShield.enable();
  audioShield.inputSelect(AUDIO_INPUT_MIC);
  audioShield.micGain(20); // in dB NE PAS METTRE EN COMMENTAIRE OU SURDITE INEVITABLE
  audioShield.volume(0.8);
  filtre1.setup(-90.0f, 10000.0f, 10000.0f);
  //filtre2.setup(18.0f, 2000.0f, 400.0f);
  queue.begin(); // Démarrer la capture
}

void loop() {
  // Vérifier si un bloc de 128 échantillons est prêt
  if (queue.available() >= 1) {
    int16_t *inbuffer = queue.readBuffer(); 
    int16_t *outBuffer = play.getBuffer(); 

    buttonState = digitalRead(0);
    if (outBuffer != NULL) {
      for (int i = 0; i < 128; i++) {
        // Conversion entier -> float (-1.0 à 1.0 car on normalise la valeur)
        float sample = (float)inbuffer[i] / 32768.0f;
        float filteredSample;

        if (buttonState)
        {
          // Application du filtre
          filteredSample = filtre1.tick(sample);
        }
        else {
          filteredSample = sample;
        }


        // Conversion float -> entier non normalisé avec sécurité pour que ça dépasse pas la val max possible
        float scaled = filteredSample * 32767.0f;
        if (scaled > 32767.0f) scaled = 32767.0f;
        if (scaled < -32768.0f) scaled = -32768.0f;
        
        outBuffer[i] = (int16_t)scaled;
      
      play.playBuffer(); // Envoi vers la sortie physique
      }
    queue.freeBuffer(); // Libérer la mémoire
    }
  }
}