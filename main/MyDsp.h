#ifndef MYDSP_H
#define MYDSP_H

#include "Arduino.h"
#include "AudioStream.h"
#include "Audio.h"
#include "Additive.h"

class MyDsp : public AudioStream {
public:
  MyDsp();
  virtual void update(void);
  
  // Contrôle du générateur de bips (Diagnostic)
  void setFreq(float f);
  
  // Gestion des modes et routing
  void setEar(int mode);        // 0: Gauche, 1: Droit, 2: Les deux
  void setMute(bool mute);      // Bypass ou Silence
  void setDiagnostic(bool diag); // true: Bips, false: Micro
  
  // Simulation d'acouphène
  void setAcouphene(bool actif, float freq, float age);

  // Cette méthode peut rester vide dans le .cpp car Filters.h gère tout
  void setFilter(int id, float gain, float centerFreq, float bandwidth);  

private:
  audio_block_t *inputQueueArray[1]; 
  
  // États logiques
  bool _mute = false;
  bool _isDiagnostic = false; 
  bool acoupheneActif = false;
  int earMode;

  // Générateurs de sons (Additive.h)
  Additive source;            // Pour les bips du diagnostic
  Additive oscilloAcouphene;  // Pour le sifflement permanent
};

#endif