#include "MyDsp.h"

void MyDsp::setParameters(float gain, float freq, float bw) {
    peak.setup(gain, freq, bw);
}

void MyDsp::update(void) {
    audio_block_t *block = receiveReadOnly(0); // On reçoit le son de l'entrée jack
    if (!block) return;

    audio_block_t *outBlock = allocate();
    if (outBlock) {
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            // Conversion int16 (-32768 à 32767) vers float (-1.0 à 1.0)
            float sample = block->data[i] / 32767.0f;
            
            // Application du filtre Peak EQ
            float filteredSample = peak.tick(sample);
            
            // Saturation de sécurité et conversion retour vers int16
            filteredSample = max(-1.0f, min(1.0f, filteredSample));
            outBlock->data[i] = (int16_t)(filteredSample * 32767.0f);
        }
        transmit(outBlock, 0);
        release(outBlock);
    }
    release(block);
}