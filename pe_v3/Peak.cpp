#include "Peak.h"
#include <math.h>

PeakFilter::PeakFilter(float SR) : _SR(SR), x1(0), x2(0), y1(0), y2(0) {}

void PeakFilter::setup(float Lfx, float fx, float B) {
    // 1. db2linear
    float g = pow(10.0, fabs(Lfx) / 20.0);
    
    // 2. Préparation des variables (wx et prewarping comme dans ton code)
    float T = 1.0 / _SR;
    float wx = 2.0 * M_PI * fx;
    float Bw = B * T / sin(wx * T); // Ton prewarp
    
    // 3. Calcul des coefficients du filtre Biquad (Transformation bilinéaire)
    float alpha = sin(wx * T) * sinh(log(2.0) / 2.0 * Bw * (wx * T) / sin(wx * T));
    float cosw0 = cos(wx * T);
    float a0;

    if (Lfx >= 0) { // BOOST
        b0 = 1.0 + alpha * g;
        b1 = -2.0 * cosw0;
        b2 = 1.0 - alpha * g;
        a0 = 1.0 + alpha / g;
        a1 = -2.0 * cosw0 / a0;
        a2 = (1.0 - alpha / g) / a0;
    } else { // CUT
        b0 = 1.0 + alpha / g;
        b1 = -2.0 * cosw0;
        b2 = 1.0 - alpha / g;
        a0 = 1.0 + alpha * g;
        a1 = -2.0 * cosw0 / a0;
        a2 = (1.0 - alpha * g) / a0;
    }

    // Normalisation finale des b
    b0 /= a0; b1 /= a0; b2 /= a0;
}

float PeakFilter::tick(float input) {
    // Équation de différence (Direct Form 1)
    // C'est ici que le son est réellement modifié
    float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

    // Mise à jour de la mémoire pour le prochain échantillon
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;

    return output;
}
