#ifndef MYDSP_H
#define MYDSP_H

#include "Arduino.h"
#include "AudioStream.h"
#include "Audio.h"
#include "Additive.h"
#include "AudioStream.h"
#include "Peak.h"

class MyDsp : public AudioStream {
public:
  // Le constructeur doit être déclaré ici, mais défini dans le .cpp
  MyDsp();
  virtual void update(void);
  void setFreq(float f);
  void setFilter(int id, float gain, float centerFreq, float bandwidth);  
  void setEar(int mode); // pour gérer sur quelle oreille on envoie : canal 0 : le gauche, canal 1 : le droit, canal 2 : les 2
  void setMute(bool mute); // couper le son pour de vrai
  void setDiagnostic(bool diagnostic); // couper le son pour de vrai

  private:
  audio_block_t *inputQueueArray[1]; // Tableau pour stocker l'entrée
  bool _mute = false;
  bool _isDiagnostic = false; // Pour basculer entre source additive et micro
  Additive source; // Générateur de son
  PeakFilter filters[7]; // Tableau de 7 filtres
  int earMode;

};

#endif
