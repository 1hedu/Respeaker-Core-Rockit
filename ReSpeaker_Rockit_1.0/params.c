#include "params.h"

static int16_t V[P_COUNT];

const param_spec_t PARAM_SPECS[P_COUNT] = {
  // Oscillators (0-15: 16 waveforms matching original Rockit)
  [P_OSC1_WAVE]     = {"osc1_wave",   0, 15,  2},  // 0-15: Sine,Square,Saw,Tri,Morph1-9,HardSync,Noise,RawSquare
  [P_OSC2_WAVE]     = {"osc2_wave",   0, 15,  3},
  [P_OSC_MIX]       = {"osc_mix",     0, 127, 64},
  [P_TUNE]          = {"tune",        0, 127, 64},  // Detune OSC2: 64=center, ±16 semitones (matches original Rockit)
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
  
  // LFO 1 (16 waveforms matching original Rockit)
  [P_LFO1_RATE]      = {"lfo1_rate",    0, 127, 32},
  [P_LFO1_DEPTH]     = {"lfo1_depth",   0, 127, 0},
  [P_LFO1_DEST]      = {"lfo1_dest",    0, 5,   0},   // 6 destinations
  [P_LFO1_SHAPE]     = {"lfo1_shape",   0, 15,  0},   // 16 waveforms
  
  // LFO 2
  [P_LFO2_RATE]      = {"lfo2_rate",    0, 127, 32},
  [P_LFO2_DEPTH]     = {"lfo2_depth",   0, 127, 0},
  [P_LFO2_DEST]      = {"lfo2_dest",    0, 5,   0},   // 6 destinations
  [P_LFO2_SHAPE]     = {"lfo2_shape",   0, 15,  0},   // 16 waveforms
  
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
