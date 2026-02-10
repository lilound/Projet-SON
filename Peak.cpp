#include "Peak.h"
#include <math.h>

PeakFilter::PeakFilter(float SR) : _SR(SR), x1(0), x2(0), y1(0), y2(0) {}

void PeakFilter::setup(float Lfx, float fx, float B) {
    float g = pow(10.0, abs(Lfx) / 40.0);
    float w0 = 2.0 * PI * fx / _SR;
    float alpha = sin(w0) * sinh(log(2.0) / 2.0 * B * w0 / sin(w0));

    float a0;
    if (Lfx >= 0) { // Boost
        b0 = 1.0 + alpha * g;
        b1 = -2.0 * cos(w0);
        b2 = 1.0 - alpha * g;
        a0 = 1.0 + alpha / g;
    } else { // Cut
        b0 = 1.0 + alpha / g;
        b1 = -2.0 * cos(w0);
        b2 = 1.0 - alpha / g;
        a0 = 1.0 + alpha * g;
    }
    
    // Normalisation des coefficients
    b0 /= a0; b1 /= a0; b2 /= a0;
    a1 = (-2.0 * cos(w0)) / a0;
    a2 = (1.0 - (Lfx >= 0 ? alpha/g : alpha*g)) / a0;
}

float PeakFilter::tick(float input) {
    // Équation de différence (Biquad)
    float output = b0*input + b1*x1 + b2*x2 - a1*y1 - a2*y2;
    
    // Mise à jour de la mémoire
    x2 = x1; x1 = input;
    y2 = y1; y1 = output;
    
    return output;
}