#include "MyDsp.h"
#include "Additive.h"

MyDsp::MyDsp() : AudioStream(1, inputQueueArray), source(44100), earMode(2), oscilloAcouphene(44100){
  _isDiagnostic = false;
  _mute = true; 
  acoupheneActif = false;
  // Note : On n'initialise plus filters[i] ici car on utilise Filters.h
}

void MyDsp::setFreq(float f) { source.setFrequency(f); }

// Cette méthode devient optionnelle car Filters.h gère l'égalisation
void MyDsp::setFilter(int id, float gain, float centerFreq, float bandwidth) {
  // Optionnel : vide ou redirection vers myFilters si vous avez accès à l'instance
}

void MyDsp::setEar(int mode) { earMode = mode; }
void MyDsp::setMute(bool mute) { _mute = mute; }
void MyDsp::setDiagnostic(bool diagnostic) { _isDiagnostic = diagnostic; }

void MyDsp::setAcouphene(bool actif, float freq, float age) {
    acoupheneActif = actif;
    if (actif) {
        oscilloAcouphene.setFrequency(freq);
        float nouveauGain = 0.001;
        oscilloAcouphene.setGain(nouveauGain);
    }
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
    
    // 1. Choix de la source principale
    if (_isDiagnostic) {
      sample = source.tick(); // Les bips du test
    } else if (inBlock) {
      sample = (float)inBlock->data[i] / 32768.0f; // Le micro
    }

    // 2. Gestion du Bypass (Mute)
    // Si _mute est vrai, on laisse passer le son brut (Bypass vers Filters.h)
    // Si _mute est faux, on traite
    float processedSample = sample;

    // 3. Ajout de l'acouphène (Sifflement constant)
    if (acoupheneActif) {
        // On multiplie par un facteur très petit (0.002) 
        // directement ici pour être CERTAIN que c'est pris en compte
        processedSample += (oscilloAcouphene.tick() * 0.0015f); 
    }
    
    // 4. Limitation et Conversion
    processedSample = max(-1.0f, min(1.0f, processedSample));
    int16_t val = (int16_t)(processedSample * 32767.0f);

    // 5. Envoi aux canaux
    // (Le bloc Filters.h recevra ce signal pour appliquer l'égalisation finale)
    outBlockL->data[i] = (earMode == 0 || earMode == 2) ? val : 0;
    outBlockR->data[i] = (earMode == 1 || earMode == 2) ? val : 0;
  }

  if (inBlock) release(inBlock);
  transmit(outBlockL, 0);
  release(outBlockL);
  transmit(outBlockR, 1);
  release(outBlockR);
}