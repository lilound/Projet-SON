#ifndef SINE_H_INCLUDED
#define SINE_H_INCLUDED

#include "SineTable.h"
#include "Phasor.h"

class Additive{
public:
  Additive(int SR);
  
  void setFrequency(float f);
  void setGain(float g);
  float tick();
private:
  SineTable sineTable;
  Phasor phasor1;
  float gain;
  int samplingRate;
};


#endif  // SINE_H_INCLUDED
