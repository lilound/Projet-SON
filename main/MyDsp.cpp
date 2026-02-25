#include "MyDsp.h"
#include "Additive.h"

// Un seul constructeur propre
MyDsp::MyDsp() : AudioStream(1, inputQueueArray), source(44100), earMode(2), oscilloAcouphene(44100){
  _isDiagnostic = false;
  _mute = true; 
  acoupheneActif = false;
  for(int i=0; i<7; i++)
    filters[i].setup(0.0, 1000.0, 1.5);  // Q = 1
}

void MyDsp::setFreq(float f) { source.setFrequency(f); }

// La méthode avec l'ID pour les 7 filtres
void MyDsp::setFilter(int id, float gain, float centerFreq, float bandwidth) {
  if(id >= 0 && id < 7) filters[id].setup(gain, centerFreq, bandwidth);
}

void MyDsp::setEar(int mode) { earMode = mode; }

void MyDsp::setMute(bool mute) { _mute = mute; }
void MyDsp::setDiagnostic(bool diagnostic) { _isDiagnostic = diagnostic; }
void MyDsp::setAcouphene(bool actif, float freq = 0, float age = 20) {
    acoupheneActif = actif;
    if (actif) {
        oscilloAcouphene.setFrequency(freq);
        float nouveauGain = 0.02 + ((age - 20) * 0.002);
        // Sécurité : on plafonne le gain à 0.15 pour ne pas abîmer les oreilles
        if (nouveauGain > 0.15) nouveauGain = 0.15;
        oscilloAcouphene.setGain(nouveauGain);
    }
}

float MyDsp::tick(float input) {
    float sortie = input; // Le son micro passé par tes filtres
    
    if (acoupheneActif) {
        // On additionne le sifflement d'Additive au signal
        sortie += oscilloAcouphene.tick();
    }
    
    return sortie;
}

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
    
    // 1. Choix de la source
    if (_isDiagnostic) {
      sample = source.tick();
    } else if (inBlock) {
      sample = (float)inBlock->data[i] / 32768.0f;
    }

    float processedSample = sample;

    // 2. Application des filtres en cascade (Bypass si _mute est vrai)
    if (!_mute) {
      for(int j = 0; j < 7; j++) {
        processedSample = filters[j].tick(processedSample);
      }
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
