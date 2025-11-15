
#include "filter_svf.h"
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
void svf_init(svf_t* f, int sample_rate){
  f->ic1eq=0.0f; f->ic2eq=0.0f; f->g=0.0f; f->k=1.0f; f->sample_rate=sample_rate>0?sample_rate:48000;
}
void svf_set_cutoff(svf_t* f, float hz){
  if(hz < 10.0f) hz = 10.0f;
  float nyq = 0.45f * (float)f->sample_rate;
  if(hz > nyq) hz = nyq;
  f->g = tanf((float)M_PI * hz / (float)f->sample_rate);
}
void svf_set_q(svf_t* f, float Q){
  if(Q < 0.3f) Q = 0.3f;
  if(Q > 20.0f) Q = 20.0f;
  f->k = 1.0f / Q;
}
