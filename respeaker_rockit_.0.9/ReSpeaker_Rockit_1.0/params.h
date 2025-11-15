#pragma once
#include <stdint.h>

typedef enum {
  // Oscillators (0-9 waveforms matching original Rockit)
  P_OSC1_WAVE,    // 0:Sine 1:Tri 2:Saw 3:Square 4:Para 5:HS1 6:HS2 7:Mult1 8:Mult2
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
  
  // LFO 1 (16 waveforms matching original Rockit)
  P_LFO1_RATE, 
  P_LFO1_DEPTH, 
  P_LFO1_DEST,    // 0:Amp 1:Filt 2:FiltQ 3:FiltEnv 4:Pitch 5:Detune
  P_LFO1_SHAPE,   // 0-15: Sine,Sq,Saw,Tri,Morph1-9,HS,Noise,RawSq
  
  // LFO 2
  P_LFO2_RATE, 
  P_LFO2_DEPTH, 
  P_LFO2_DEST,    // 0:Mix 1:Filt 2:FiltQ 3:LFO1Rate 4:LFO1Amt 5:FiltAtk
  P_LFO2_SHAPE,
  
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
