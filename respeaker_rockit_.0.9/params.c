#include "params.h"

static int16_t V[P_COUNT];

const param_spec_t PARAM_SPECS[P_COUNT] = {
  // Oscillators
  [P_OSC1_WAVE]     = {"osc1_wave",   0, 3,   2},
  [P_OSC2_WAVE]     = {"osc2_wave",   0, 3,   3},
  [P_OSC_MIX]       = {"osc_mix",     0, 127, 64},
  [P_TUNE]          = {"tune",      -64, 64,  0},
  [P_FINE]          = {"fine",      -64, 64,  0},
  [P_SUBOSC]        = {"subosc",      0, 1,   0},  // 0:off 1:on
  
  // Envelope
  [P_ENV_ATTACK]    = {"attack",      0, 127, 4},
  [P_ENV_DECAY]     = {"decay",       0, 127, 20},
  [P_ENV_SUSTAIN]   = {"sustain",     0, 127, 100},
  [P_ENV_RELEASE]   = {"release",     0, 127, 40},
  
  // Filter
  [P_FILTER_CUTOFF]    = {"flt_cutoff",    0, 127, 64},   // 50Hz - 12kHz
  [P_FILTER_RESONANCE] = {"flt_res",       0, 127, 0},    // Q: 0.5 - 20
  [P_FILTER_ENV_AMT]   = {"flt_env_amt",   0, 127, 64},   // Envelope modulation amount
  [P_FILTER_MODE]      = {"flt_mode",      0, 3,   0},    // 0:LP 1:HP 2:BP 3:Notch
  
  // LFO
  [P_LFO_RATE]      = {"lfo_rate",    0, 127, 32},
  [P_LFO_DEPTH]     = {"lfo_depth",   0, 127, 0},
  [P_LFO_DEST]      = {"lfo_dest",    0, 4,   0},    // 0:off 1:pitch 2:mix 3:filter 4:pwm
  [P_LFO_SHAPE]     = {"lfo_shape",   0, 3,   0},    // 0:sine 1:tri 2:ramp 3:square
  
  // Global
  [P_GLIDE_TIME]    = {"glide_ms",    0, 127, 0},
  [P_MASTER_VOL]    = {"volume",      0, 127, 100},
};

void params_init(void){ 
  for(int i=0; i<P_COUNT; i++) 
    V[i] = PARAM_SPECS[i].def; 
}

void params_set(param_id_t id, int16_t val){
  if(id < 0 || id >= P_COUNT) return;
  if(val < PARAM_SPECS[id].min) val = PARAM_SPECS[id].min;
  if(val > PARAM_SPECS[id].max) val = PARAM_SPECS[id].max;
  V[id] = val;
}

int16_t params_get(param_id_t id){ 
  if(id < 0 || id >= P_COUNT) return 0; 
  return V[id]; 
}
