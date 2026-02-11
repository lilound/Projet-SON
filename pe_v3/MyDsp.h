#ifndef MYDSP_H
#define MYDSP_H

#include "Arduino.h"
#include "AudioStream.h"
#include "Audio.h"
#include "Additive.h"
#include "Peak.h"

class MyDsp : public AudioStream {
public:
  MyDsp();
  virtual void update(void);
  void setFreq(float f);
  void setFilter(float gain, float centerFreq, float bandwidth);
  void setEar(int mode); // pour gérer sur quelle oreille on envoie : canal 0 : le gauche, canal 1 : le droit, canal 2 : les 2
  void setMute(bool mute); // couper le son pour de vrai

  private:

  bool _mute = false;
  Additive source; // Générateur de son
  PeakFilter filter; // Notre filtre Peak EQ
  int earMode;
};

#endif
