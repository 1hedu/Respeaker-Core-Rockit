#pragma once
#include <stdint.h>

typedef enum {
  // Oscillators (16 waveforms matching original Rockit)
  P_OSC1_WAVE,    // 0-15: Sine,Sq,Saw,Tri,Morph1-9,HardSync,Noise,RawSq
  P_OSC2_WAVE,
  P_OSC_MIX,
  P_TUNE,         // Detune OSC2: 0-127, center 64, ±16 semitones
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
  P_FILTER_MODE,  // 0:LP 1:BP 2:HP 3:Notch (original Rockit order)
  
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
  P_DRONE_MODE,   // 0:off 1:on - Continuous note with manual pitch/volume control

  // Arpeggiator (for drone mode)
  P_ARP_PATTERN,  // 0-15: 16 different arpeggio patterns
  P_ARP_SPEED,    // 0-127: Arpeggiator speed
  P_ARP_LENGTH,   // 1-8: Number of steps in pattern to play
  P_ARP_GATE,     // 0-127: Note gate time (percentage of step)

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
