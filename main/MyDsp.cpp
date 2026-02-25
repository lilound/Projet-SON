#include "MyDsp.h"

// Un seul constructeur propre
MyDsp::MyDsp() : AudioStream(1, inputQueueArray), source(44100), earMode(2) {
  _isDiagnostic = false;
  _mute = true; 
  for(int i=0; i<7; i++) filters[i].setup(0.0, 1000.0, 100.0); 
}

void MyDsp::setFreq(float f) { source.setFrequency(f); }

// La méthode avec l'ID pour les 7 filtres
void MyDsp::setFilter(int id, float gain, float centerFreq, float bandwidth) {
  if(id >= 0 && id < 7) filters[id].setup(gain, centerFreq, bandwidth);
}

void MyDsp::setEar(int mode) { earMode = mode; }

void MyDsp::setMute(bool mute) { _mute = mute; }
void MyDsp::setDiagnostic(bool diagnostic) { _isDiagnostic = diagnostic; }

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
    float sample = 0;
    
    // 1. On récupère la source (Générateur ou Micro)
    if (_isDiagnostic) {
      sample = source.tick();
    } else if (inBlock) {
      sample = (float)inBlock->data[i] / 32768.0f;
    }

    // 2. On initialise TOUJOURS avec la source
    float processedSample = sample; 

    // 3. Application de la logique Mute / Bypass
    if (!_mute) {
      // MODE FILTRÉ (Diagnostic actif ou Correction active)
      for(int j = 0; j < 7; j++) {
        processedSample = filters[j].tick(processedSample);
      }
    } 
    else if (_isDiagnostic) {
      // MODE MUTE en Diagnostic : Silence total entre les bips
      processedSample = 0.0f; 
    }
    // Note : Si !_mute est faux et !_isDiagnostic est faux, 
    // processedSample garde sa valeur 'sample' (Bypass micro direct).

    // 4. Limitation et Conversion
    processedSample = max(-1.0f, min(1.0f, processedSample));
    int16_t val = (int16_t)(processedSample * 32767.0f);

    outBlockL->data[i] = (earMode == 0 || earMode == 2) ? val : 0;
    outBlockR->data[i] = (earMode == 1 || earMode == 2) ? val : 0;
  }

  if (inBlock) release(inBlock);
  transmit(outBlockL, 0);
  release(outBlockL);
  transmit(outBlockR, 1);
  release(outBlockR);
}
