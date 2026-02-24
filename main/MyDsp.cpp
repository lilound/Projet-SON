#include "MyDsp.h"

MyDsp::MyDsp() : AudioStream(1, inputQueueArray), source(44100), earMode(2) {
  // Réglage par défaut du filtre
  filter.setup(0.0, 1000.0, 100.0); 
}

void MyDsp::setFreq(float f) { source.setFrequency(f); }

void MyDsp::setFilter(float gain, float centerFreq, float bandwidth) {
  filter.setup(gain, centerFreq, bandwidth);
}

void MyDsp::setEar(int mode) { earMode = mode; }

void MyDsp::setMute(bool mute) { _mute = mute; }


void MyDsp::update(void) {
  audio_block_t* inBlock = receiveReadOnly(0); 
  audio_block_t* outBlockL = allocate();
  audio_block_t* outBlockR = allocate();

  if (!outBlockL || !outBlockR) {
    if (inBlock) release(inBlock);
    if (outBlockL) release(outBlockL);
    if (outBlockR) release(outBlockR);
    return;
  }

  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    // 1. On récupère le son du micro TOUJOURS
    float sample = 0;
    if (inBlock) {
      sample = (float)inBlock->data[i] / 32768.0f;
    }

    float processedSample;
    // 2. Si _mute est FAUX, on applique le filtre. 
    // Si _mute est VRAI, on laisse passer le son intact (Bypass).
    if (!_mute) {
      processedSample = filter.tick(sample);
    } else {
      processedSample = sample; 
    }
    
    // 3. Limitation et Conversion
    processedSample = max(-1.0f, min(1.0f, processedSample));
    int16_t val = (int16_t)(processedSample * 32767.0f);

    // 4. Envoi aux canaux
    outBlockL->data[i] = (earMode == 0 || earMode == 2) ? val : 0;
    outBlockR->data[i] = (earMode == 1 || earMode == 2) ? val : 0;
  }

  if (inBlock) release(inBlock);
  transmit(outBlockL, 0);
  release(outBlockL);
  transmit(outBlockR, 1);
  release(outBlockR);
}