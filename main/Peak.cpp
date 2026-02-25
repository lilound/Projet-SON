#include "Peak.h"
#include <math.h>

PeakFilter::PeakFilter(float SR) : _SR(SR), x1(0), x2(0), y1(0), y2(0) {}

void PeakFilter::setup(float Lfx, float fx, float Q)
{
    // Lfx = gain en dB, fx = fréquence centrale, Q = facteur de qualité
    float A  = pow(10.0f, Lfx / 40.0f);   // Convertit dB en amplitude
    float w0 = 2.0f * PI * fx / _SR;      // Fréquence normalisée
    float alpha = sin(w0) / (2.0f * Q);   // Largeur de bande
    float cosw = cos(w0);

    // Coefficients biquad Peak EQ standard
    float a0 = 1.0f + alpha / A;
    b0 = 1.0f + alpha * A;
    b1 = -2.0f * cosw;
    b2 = 1.0f - alpha * A;
    a1 = -2.0f * cosw;
    a2 = 1.0f - alpha / A;

    // Normalisation
    b0 /= a0; b1 /= a0; b2 /= a0;
    a1 /= a0; a2 /= a0;
}

float PeakFilter::tick(float input) {
    // Équation de différence (Biquad)
    float output = b0*input + b1*x1 + b2*x2 - a1*y1 - a2*y2;
    
    // Mise à jour de la mémoire
    x2 = x1; x1 = input;
    y2 = y1; y1 = output;
    
    return output;
}