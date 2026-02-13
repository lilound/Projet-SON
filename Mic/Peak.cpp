#include "Peak.h"
#include <math.h>

PeakFilter::PeakFilter(float SR) : _SR(SR), x1(0), x2(0), y1(0), y2(0) {}

void PeakFilter::setup(float Lfx, float fx, float B_hz) {
    float A  = pow(10.0f, Lfx / 40.0f); // dB → coefficient linéaire
    float w0 = 2.0f * M_PI * fx / _SR;
    float Q  = fx / B_hz;               // largeur de bande en Hz → Q
    float alpha = sin(w0) / (2.0f * Q);

    float cosw0 = cos(w0);

    float b0n = 1 + alpha*A;
    float b1n = -2*cosw0;
    float b2n = 1 - alpha*A;
    float a0n = 1 + alpha/A;
    float a1n = -2*cosw0;
    float a2n = 1 - alpha/A;

    b0 = b0n / a0n;
    b1 = b1n / a0n;
    b2 = b2n / a0n;
    a1 = a1n / a0n;
    a2 = a2n / a0n;
}

float PeakFilter::tick(float input) {
    float output = b0*input + b1*x1 + b2*x2
                   - a1*y1 - a2*y2;

    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;

    return output;
}
