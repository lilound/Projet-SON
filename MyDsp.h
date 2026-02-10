#ifndef my_dsp_h_
#define my_dsp_h_

#include "AudioStream.h"
#include "Peak.h"

class MyDsp : public AudioStream {
public:
    MyDsp() : AudioStream(1, new audio_block_t*[1]) {}
    virtual void update(void);
    void setParameters(float gain, float freq, float bw);

private:
    PeakFilter peak;
};

#endif