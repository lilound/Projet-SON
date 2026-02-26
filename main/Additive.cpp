#include <cmath>

#include "Additive.h"

#define SINE_TABLE_SIZE 16384

Additive::Additive(int SR) : 
sineTable(SINE_TABLE_SIZE),
phasor1(SR),
gain(1.0),
samplingRate(SR){}

void Additive::setFrequency(float f){
  phasor1.setFrequency(f);

}
    
void Additive::setGain(float g){
  gain = g;
}
    
float Additive::tick(){
  int index1 = phasor1.tick()*SINE_TABLE_SIZE;
  //int index2 = int(index1*1.5)%SINE_TABLE_SIZE;
  //return sineTable.tick(index1) + sineTable.tick(index2)*gain*0.5;
  return sineTable.tick(index1);
}
