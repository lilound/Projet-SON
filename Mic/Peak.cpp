#include "Peak.h"
#include <math.h>

PeakFilter::PeakFilter(float SR) : _SR(SR), x1(0), x2(0), y1(0), y2(0) {}

void PeakFilter::setup(float Lfx, float fx, float B) {
    float T = 1.0 / _SR;
    float wx = 2.0 * M_PI * fx;
    
    // Ton prewarp spécifique
    float Bw = B * T / sin(wx * T); 
    
    // Calcul du gain linéaire
    float g = pow(10.0, fabs(Lfx) / 20.0);
    
    float a1_val = M_PI * Bw;
    float b1_val = g * a1_val;
    
    float b1s, a1s;
    if (Lfx > 0) {
        b1s = b1_val;
        a1s = a1_val;
    } else {
        b1s = a1_val;
        a1s = b1_val;
    }

    // Transformation Bilinéaire vers le plan Z
    float denominator = 1.0 + a1s; 
    b0 = (1.0 + b1s) / denominator;
    b1 = (1.0 - b1s) / denominator;
    b2 = 0; // Inutilisé pour l'ordre 1
    a1 = (1.0 - a1s) / denominator;
    a2 = 0; // Inutilisé pour l'ordre 1
}

float PeakFilter::tick(float input) {
    // Équation de différence pour un filtre du 1er ordre
    float output = b0 * input + b1 * x1 - a1 * y1;

    // Mise à jour de la mémoire (un seul échantillon de retard suffit)
    x1 = input;
    y1 = output;

    return output;
}
