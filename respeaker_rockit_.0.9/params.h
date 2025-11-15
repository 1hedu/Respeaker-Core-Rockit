#pragma once
#include <stdint.h>

typedef enum {
  // Oscillators
  P_OSC1_WAVE, 
  P_OSC2_WAVE, 
  P_OSC_MIX, 
  P_TUNE, 
  P_FINE,
  P_SUBOSC,
  
  // Envelope
  P_ENV_ATTACK, 
  P_ENV_DECAY, 
  P_ENV_SUSTAIN, 
  P_ENV_RELEASE,
  
  // Filter
  P_FILTER_CUTOFF,
  P_FILTER_RESONANCE,
  P_FILTER_ENV_AMT,
  P_FILTER_MODE,  // 0:LP 1:HP 2:BP 3:Notch
  
  // LFO
  P_LFO_RATE, 
  P_LFO_DEPTH, 
  P_LFO_DEST,    // 0:off 1:pitch 2:mix 3:filter 4:pwm
  P_LFO_SHAPE,   // 0:sine 1:tri 2:ramp 3:square
  
  // Global
  P_GLIDE_TIME,
  P_MASTER_VOL,
  
  P_COUNT
} param_id_t;

typedef struct { 
  const char *name; 
  int16_t min, max, def; 
} param_spec_t;

extern const param_spec_t PARAM_SPECS[P_COUNT];

void params_init(void);
void params_set(param_id_t id, int16_t value);
int16_t params_get(param_id_t id);
