#include "MyDsp.h"

MyDsp::MyDsp() : AudioStream(2, new audio_block_t*[2]), source(44100), earMode(0) {
  // Réglage par défaut : Fréquence 440Hz, Boost de 15dB à 1000Hz
  filter.setup(15.0, 1000.0, 100.0); 
}

void MyDsp::setFreq(float f) { source.setFrequency(f); }

void MyDsp::setFilter(float gain, float centerFreq, float bandwidth) {
  filter.setup(gain, centerFreq, bandwidth);
}

void MyDsp::setEar(int mode) { earMode = mode; }

void MyDsp::setMute(bool mute) { _mute = mute; }


void MyDsp::update(void) {
  audio_block_t* outBlockL = allocate();
  audio_block_t* outBlockR = allocate();
  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    // 1. On génère le son (source)
    float rawSample = _mute ? 0 : source.tick(); // On coupe à la source
    // 2. On applique le filtre (traitement)
    float filteredSample = filter.tick(rawSample);
    
    // 3. Protection contre la saturation et conversion
    filteredSample = max(-1.0f, min(1.0f, filteredSample));
    int16_t val = (int16_t)(filteredSample * 32767.0f);
    // Envoi vers le bloc Gauche (canal 0)
    if (outBlockL) outBlockL->data[i] = (earMode == 2 || earMode == 0) ? val : 0;
    
    // Envoi vers le bloc Droit (canal 1)
    if (outBlockR) outBlockR->data[i] = (earMode == 2 || earMode == 1) ? val : 0;
  }
    
  if (outBlockL){
    transmit(outBlockL, 0);
    release(outBlockL);
  }
  if (outBlockR){
    transmit(outBlockR, 1);
    release(outBlockR);
  }
}
