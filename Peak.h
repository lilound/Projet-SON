#ifndef PEAK_H
#define PEAK_H

#include <Arduino.h>

class PeakFilter {
public:
    PeakFilter(float SR = 44100.0);
    // Lfx: Gain (dB), fx: Fréquence (Hz), B: Bande passante (Hz)
    void setup(float Lfx, float fx, float B);
    float tick(float input);

private:
    float _SR;
    // Coefficients du filtre
    float b0, b1, b2, a1, a2;
    // Mémoire du filtre (les "états")
    float x1, x2, y1, y2;
};

#endif
