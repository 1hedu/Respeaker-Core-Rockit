
#pragma once
#include <stdint.h>
typedef struct { float ic1eq, ic2eq; float g, k; int sample_rate; } svf_t;
void svf_init(svf_t* f, int sample_rate);
void svf_set_cutoff(svf_t* f, float hz);
void svf_set_q(svf_t* f, float Q);

// SVF filter modes - all use same state update
static inline float svf_process_lp(svf_t* f, float v0){
  float v1 = (f->g * (v0 - f->ic2eq) + f->ic1eq) / (1.0f + f->g * (f->g + f->k));
  float v2 = f->ic2eq + f->g * v1;
  f->ic1eq = 2.0f * v1 - f->ic1eq;
  f->ic2eq = 2.0f * v2 - f->ic2eq;
  return v2;  // Lowpass output
}

static inline float svf_process_hp(svf_t* f, float v0){
  float v1 = (f->g * (v0 - f->ic2eq) + f->ic1eq) / (1.0f + f->g * (f->g + f->k));
  float v2 = f->ic2eq + f->g * v1;
  f->ic1eq = 2.0f * v1 - f->ic1eq;
  f->ic2eq = 2.0f * v2 - f->ic2eq;
  return v0 - f->k * v1 - v2;  // Highpass output
}

static inline float svf_process_bp(svf_t* f, float v0){
  float v1 = (f->g * (v0 - f->ic2eq) + f->ic1eq) / (1.0f + f->g * (f->g + f->k));
  float v2 = f->ic2eq + f->g * v1;
  f->ic1eq = 2.0f * v1 - f->ic1eq;
  f->ic2eq = 2.0f * v2 - f->ic2eq;
  return v1;  // Bandpass output
}

static inline float svf_process_notch(svf_t* f, float v0){
  float v1 = (f->g * (v0 - f->ic2eq) + f->ic1eq) / (1.0f + f->g * (f->g + f->k));
  float v2 = f->ic2eq + f->g * v1;
  f->ic1eq = 2.0f * v1 - f->ic1eq;
  f->ic2eq = 2.0f * v2 - f->ic2eq;
  return v0 - f->k * v1;  // Notch output (v0 - BP)
}
