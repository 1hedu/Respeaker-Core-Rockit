
#pragma once
#include <stdint.h>
typedef struct { float ic1eq, ic2eq; float g, k; int sample_rate; } svf_t;
void svf_init(svf_t* f, int sample_rate);
void svf_set_cutoff(svf_t* f, float hz);
void svf_set_q(svf_t* f, float Q);
static inline float svf_process_lp(svf_t* f, float v0){
  float v1 = (f->g * (v0 - f->ic2eq) + f->ic1eq) / (1.0f + f->g * (f->g + f->k));
  float v2 = f->ic2eq + f->g * v1;
  f->ic1eq = 2.0f * v1 - f->ic1eq;
  f->ic2eq = 2.0f * v2 - f->ic2eq;
  return v2;
}
