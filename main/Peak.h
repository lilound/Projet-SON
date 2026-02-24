#ifndef PEAK_H
#define PEAK_H

#include <Arduino.h>

class PeakFilter {
public:
    PeakFilter(float SR = 44100.0);
    void setup(float Lfx, float fx, float B);
    float tick(float input); // Traitement échantillon par échantillon

private:
    float _SR;
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2; // États (mémoire du filtre)
};

#endif