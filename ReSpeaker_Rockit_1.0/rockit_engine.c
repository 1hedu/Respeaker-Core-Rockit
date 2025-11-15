#include "rockit_engine.h"
#include "paraphonic.h"
#include "filter_svf.h"
#include "patch_storage.h"
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include "wavetables.h"

// Q1.15 helpers
static inline int16_t qmul_q15(int16_t a, int16_t b){ 
    int32_t t=(int32_t)a*(int32_t)b; 
    t+=(1<<14); 
    return (int16_t)(t>>15); 
}
static inline int16_t sat16(int32_t x){ 
    if(x>32767) return 32767; 
    if(x<-32768) return -32768; 
    return (int16_t)x; 
}
static inline int16_t lut8_to_q15(uint8_t s){ 
    return ((int16_t)s-128)<<7; 
}

// Wave types - 16 matching original Rockit manual
typedef enum {
    W_SINE=0, W_SQUARE=1, W_SAW=2, W_TRI=3,
    W_MORPH1=4, W_MORPH2=5, W_MORPH3=6, W_MORPH4=7,
    W_MORPH5=8, W_MORPH6=9, W_MORPH7=10, W_MORPH8=11,
    W_MORPH9=12, W_HARDSYNC=13, W_NOISE=14, W_RAW_SQUARE=15
} wave_t;

// Envelope states
typedef enum { ENV_IDLE=0, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE } env_t;

// Per-voice morph state for time-varying waveforms (matches original Rockit)
typedef struct {
    uint8_t morph_timer;        // Sample counter for morph speed
    uint8_t morph_index;        // 0-255 morph position
    uint16_t morph_index_16;    // 0-65535 for longer morph cycles
    uint8_t morph_state;        // State machine for complex morphs (MORPH_8, MORPH_4)
    uint8_t phase_shifter;      // Phase offset for MORPH_2
    uint8_t phase_shift_timer;  // Timer for phase shifting
    uint16_t lfsr;              // Per-voice LFSR for noise
} morph_state_t;

// Anti-aliasing: mipmap selection based on MIDI note
static inline uint8_t get_mipmap_index(uint8_t note, uint8_t *blend_pos) {
    *blend_pos = note & 0x03;  // note % 4
    return note >> 2;           // note / 4
}

// Blend between adjacent mipmaps to prevent aliasing
static inline uint8_t blend_mipmaps(const uint8_t table[][256], uint8_t mipmap, uint8_t blend_pos, uint8_t phase_idx) {
    uint16_t sample;
    
    switch(blend_pos) {
        case 0:  // 50% current + 50% below
            if(mipmap == 0) return table[0][phase_idx];
            sample = table[mipmap][phase_idx] + table[mipmap-1][phase_idx];
            return sample >> 1;
        case 1:  // 75% current + 25% below
            if(mipmap == 0) return table[0][phase_idx];
            sample = (table[mipmap][phase_idx] * 3) + table[mipmap-1][phase_idx];
            return sample >> 2;
        case 2:  // 75% current + 25% above
            if(mipmap >= 31) return table[31][phase_idx];
            sample = (table[mipmap][phase_idx] * 3) + table[mipmap+1][phase_idx];
            return sample >> 2;
        case 3:  // 50% current + 50% above
            if(mipmap >= 31) return table[31][phase_idx];
            sample = table[mipmap][phase_idx] + table[mipmap+1][phase_idx];
            return sample >> 1;
    }
    return table[mipmap][phase_idx];
}

// Wavetable sampling with TIME-VARYING MORPHING (matches original Rockit firmware)
// morph: pointer to per-voice morph state (updated each sample for time-varying behavior)
// env_state: current envelope state for MORPH_9
static inline int16_t wavetable_sample(uint32_t phase, wave_t w, uint8_t midi_note, morph_state_t *morph, env_t env_state){
    uint8_t blend_pos;
    uint8_t mipmap = get_mipmap_index(midi_note, &blend_pos);
    uint8_t i = (uint8_t)(phase >> 24);
    uint8_t sample_u8;
    uint16_t temp16;
    int16_t stemp;

    if(mipmap > 31) mipmap = 31;

    switch(w){
        case W_SINE:
            sample_u8 = G_AUC_SIN_LUT[i];
            break;

        case W_SQUARE:
            sample_u8 = blend_mipmaps(G_AUC_SQUARE_WAVETABLE_LUT, mipmap, blend_pos, i);
            break;

        case W_SAW:
            sample_u8 = blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, i);
            break;

        case W_TRI:
            sample_u8 = blend_mipmaps(G_AUC_TRIANGLE_WAVETABLE_LUT, mipmap, blend_pos, i);
            break;

        case W_MORPH1:  // Square morphing with inverted ramp (offset 180°)
            if(morph->morph_timer == 0) {
                morph->morph_index++;
                morph->morph_timer = 15;  // MORPH_1_TIME_PERIOD
            }
            morph->morph_timer--;

            temp16 = blend_mipmaps(G_AUC_SQUARE_WAVETABLE_LUT, mipmap, blend_pos, i) * morph->morph_index;
            temp16 += blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, (uint8_t)(i + 127)) * (255 - morph->morph_index);
            sample_u8 = temp16 >> 8;
            break;

        case W_MORPH2:  // Triangle with phase-shifting ramp
            if(morph->morph_timer == 0) {
                morph->morph_index++;
                morph->morph_timer = 10;  // MORPH_2_TIME_PERIOD
            }
            morph->morph_timer--;

            if(morph->phase_shift_timer == 0) {
                morph->phase_shifter++;
                morph->phase_shift_timer = 50;  // PHASE_SHIFT_TIMER_2
            }
            morph->phase_shift_timer--;

            temp16 = blend_mipmaps(G_AUC_TRIANGLE_WAVETABLE_LUT, mipmap, blend_pos, i) * morph->morph_index;
            temp16 += blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, (uint8_t)(i + morph->phase_shifter)) * (255 - morph->morph_index);
            sample_u8 = temp16 >> 8;
            break;

        case W_MORPH3:  // Triangle minus reversed square (phase difference)
            if(morph->morph_timer == 0) {
                morph->morph_index++;
                morph->morph_timer = 50;
            }
            morph->morph_timer--;

            stemp = (int16_t)blend_mipmaps(G_AUC_TRIANGLE_WAVETABLE_LUT, mipmap, blend_pos, i) -
                    (int16_t)blend_mipmaps(G_AUC_SQUARE_WAVETABLE_LUT, mipmap, blend_pos, (uint8_t)(i - morph->morph_index));
            sample_u8 = (stemp > 127) ? 255 : ((stemp < -128) ? 0 : (128 + stemp));
            break;

        case W_MORPH4:  // Sawtooth minus reversed sawtooth (bidirectional morph)
            if(morph->morph_timer == 0) {
                if(morph->morph_state == 0) {
                    morph->morph_index++;
                    if(morph->morph_index == 255) morph->morph_state = 1;
                } else {
                    morph->morph_index--;
                    if(morph->morph_index == 0) morph->morph_state = 0;
                }
                morph->morph_timer = 250;
            }
            morph->morph_timer--;

            stemp = (int16_t)blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, i) -
                    (int16_t)blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, (uint8_t)(i - morph->morph_index));
            sample_u8 = (stemp > 127) ? 255 : ((stemp < -128) ? 0 : (128 + stemp));
            break;

        case W_MORPH5:  // Enveloped sine followed by enveloped square
        case W_MORPH6:  // Enveloped ramp followed by enveloped square
            if(morph->morph_timer == 0) {
                morph->morph_index_16++;
                morph->morph_timer = (w == W_MORPH5) ? 10 : 50;
            }
            morph->morph_timer--;

            if(morph->morph_index_16 == 383) morph->morph_index_16 = 0;

            temp16 = 0;
            if(morph->morph_index_16 < 255) {
                uint8_t base = (w == W_MORPH5) ? G_AUC_SIN_LUT[i] : blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, i);
                temp16 = (base * blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, morph->morph_index_16 & 0xFF)) >> 8;
            }
            if(morph->morph_index_16 > 128 && morph->morph_index_16 < 383) {
                uint8_t idx = morph->morph_index_16 - 128;
                temp16 += (blend_mipmaps(G_AUC_SQUARE_WAVETABLE_LUT, mipmap, blend_pos, i) * (255 - G_AUC_SIN_LUT[idx])) >> 8;
            }
            sample_u8 = (temp16 >> 1) & 0xFF;
            break;

        case W_MORPH7:  // Variable pulse width square (PWM)
            if(morph->morph_timer == 0) {
                morph->morph_index++;
                morph->morph_timer = 25;
            }
            morph->morph_timer--;

            stemp = (int16_t)blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, i) -
                    (int16_t)blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, (uint8_t)(i - morph->morph_index));
            sample_u8 = (stemp > 127) ? 255 : ((stemp < -128) ? 0 : (128 + stemp));
            break;

        case W_MORPH8:  // Triangle→Noise→Narrowing pulse sequence
            if(morph->morph_timer == 0) {
                morph->morph_index++;
                morph->morph_timer = 5;
            }
            morph->morph_timer--;

            switch(morph->morph_state) {
                case 0:  // Triangle for ~20ms
                    sample_u8 = blend_mipmaps(G_AUC_TRIANGLE_WAVETABLE_LUT, mipmap, blend_pos, i);
                    if(morph->morph_index == 255) morph->morph_state = 1;
                    break;
                case 1:  // Noise for ~20ms
                    if((i & 0x0F) == 0) {
                        uint16_t bit = ((morph->lfsr >> 15) ^ (morph->lfsr >> 13) ^ (morph->lfsr >> 12) ^ (morph->lfsr >> 10)) & 1;
                        morph->lfsr = (morph->lfsr << 1) | bit;
                    }
                    sample_u8 = morph->lfsr & 0xFF;
                    if(morph->morph_index == 255) morph->morph_state = 2;
                    break;
                case 2:  // Narrowing pulse
                case 3:  // Hold narrow pulse
                    stemp = (int16_t)blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, i) -
                            (int16_t)blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, (uint8_t)(i - morph->morph_index));
                    sample_u8 = (stemp > 127) ? 255 : ((stemp < -128) ? 0 : (128 + stemp));
                    if(morph->morph_state == 2 && morph->morph_index == 255) morph->morph_state = 3;
                    break;
                default:
                    sample_u8 = 128;
            }
            break;

        case W_MORPH9:  // Envelope-following waveform
            switch(env_state) {
                case ENV_ATTACK:
                    sample_u8 = blend_mipmaps(G_AUC_TRIANGLE_WAVETABLE_LUT, mipmap, blend_pos, i);
                    break;
                case ENV_DECAY:
                case ENV_SUSTAIN:
                    stemp = (int16_t)blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, i) -
                            (int16_t)blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, (uint8_t)(i - 127));
                    sample_u8 = (stemp > 127) ? 255 : ((stemp < -128) ? 0 : (128 + stemp));
                    break;
                default:  // Release
                    if(morph->morph_timer == 0) {
                        morph->morph_index++;
                        morph->morph_timer = 10;
                    }
                    morph->morph_timer--;
                    stemp = (int16_t)blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, i) -
                            (int16_t)blend_mipmaps(G_AUC_RAMP_WAVETABLE_LUT, mipmap, blend_pos, (uint8_t)(i - morph->morph_index));
                    sample_u8 = (stemp > 127) ? 255 : ((stemp < -128) ? 0 : (128 + stemp));
            }
            break;

        case W_HARDSYNC:  // Hard sync
            sample_u8 = blend_mipmaps(G_AUC_HARDSYNC_2_WAVETABLE_LUT, mipmap >> 1, 0, i >> 1);
            break;

        case W_NOISE:  // LFSR-based pseudo-random noise
            if((i & 0x0F) == 0) {
                uint16_t bit = ((morph->lfsr >> 15) ^ (morph->lfsr >> 13) ^ (morph->lfsr >> 12) ^ (morph->lfsr >> 10)) & 1;
                morph->lfsr = (morph->lfsr << 1) | bit;
            }
            sample_u8 = morph->lfsr & 0xFF;
            break;

        case W_RAW_SQUARE:  // Raw aliasing square
            sample_u8 = (i < 128) ? 255 : 0;
            break;

        default:
            sample_u8 = G_AUC_SIN_LUT[i];
            break;
    }

    return ((int16_t)sample_u8 - 128) << 7;
}

// LFO structure
typedef struct{ 
    uint32_t ph, inc; 
    uint8_t shape; 
    int16_t depth_q; 
} lfo_t;

static inline uint32_t hz_to_inc(float hz, int sr){ 
    double k=(hz*4294967296.0)/(double)sr; 
    if(k<0)k=0; 
    if(k>4294967295.0)k=4294967295.0; 
    return (uint32_t)k; 
}

// LFO waveform generation - 16 shapes
static inline int16_t lfo_wave(uint32_t ph, uint8_t shape){
    uint8_t i=(uint8_t)(ph>>24);
    static uint16_t lfsr = 0xACE1;
    
    switch(shape){
        case 0:  // Sine
            return lut8_to_q15(G_AUC_SIN_LUT[i]);
        case 1:  // Square
            return (i<128)?32767:-32768;
        case 2:  // Ramp/Saw
            return lut8_to_q15(i);
        case 3:  // Triangle
            return lut8_to_q15((i<128)?(i<<1):(255-((i-128)<<1)));
        case 4:
        case 5:
        case 6:
            return lut8_to_q15(G_AUC_TRIANGLE_SIMPLE_WAVETABLE_LUT[i]);
        case 7:
        case 8:
        case 9:
            return lut8_to_q15(G_AUC_SQUARE_SIMPLE_WAVETABLE_LUT[i]);
        case 10: // Reverse ramp
            return lut8_to_q15(255-i);
        case 11:
        case 12:
            return lut8_to_q15(G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[i]);
        case 13: // Hard sync
            return lut8_to_q15(G_AUC_HARDSYNC_2_SIMPLE_WAVETABLE_LUT[i>>1]);
        case 14: // Noise (LFSR)
            if((i & 0x0F) == 0) {
                uint16_t bit = ((lfsr >> 15) ^ (lfsr >> 13) ^ (lfsr >> 12) ^ (lfsr >> 10)) & 1;
                lfsr = (lfsr << 1) | bit;
            }
            return (int16_t)(lfsr & 0xFFFF) - 16384;
        case 15: // Raw square
            return (i<128)?32767:-32768;
        default:
            return lut8_to_q15(G_AUC_SIN_LUT[i]);
    }
}

typedef struct {
    uint8_t active;
    uint8_t note;
    uint32_t ph1, inc1, ph2, inc2;
    env_t env;
    int16_t env_q;
    uint32_t t, atk, dec, rel;
    int16_t sus_q;
    uint32_t inc_target, inc_cur;
    morph_state_t morph1, morph2;  // Separate morph state for OSC1 and OSC2
} voice_state_t;

static voice_state_t V[3];
static lfo_t L1, L2;  // Two LFOs!
static svf_t flt;
static int g_sr = 48000;

// Track tuning params to avoid expensive recalculation every sample
static int16_t g_last_tune = -999;
static uint8_t g_tuning_dirty = 1;

static const uint16_t FREQ[128]={
 8,9,9,10,10,11,12,12,13,14,15,15,16,17,18,19,21,22,23,24,26,28,29,31,33,35,37,39,41,44,46,49,52,55,58,62,
 65,69,73,78,82,87,92,98,104,110,117,123,131,139,147,156,165,175,185,196,208,220,233,247,262,277,294,311,
 330,349,370,392,415,440,466,494,523,554,587,622,659,698,740,784,831,880,932,988,1047,1109,1175,1245,1319,
 1397,1480,1568,1661,1760,1865,1976,2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,3951,4186,4435,
 4699,4978,5274,5588,5920,6272,6645,7040,7459,7902,8372,8870,9397,9956,10548,11175,11840,12544
};

static inline float semi(int st){
    return powf(2.0f, st/12.0f);
}

static inline uint32_t ms_to_samples(float ms, int sr){
    return (uint32_t)((ms*0.001f)*(float)sr);
}

// Calculate phase increment with detune matching original Rockit firmware
// tune_param: 0-127, center at 64, ±16 semitones range (matches original OSC_DETUNE)
static inline uint32_t calc_phase_inc(uint8_t note, int tune_param, int sr) {
    float semitone_offset;

    if(tune_param == 0) {
        // No tuning offset - use base frequency
        semitone_offset = 0.0f;
    } else {
        // Match original Rockit: 0-127 input, center at 64
        // Map to ±16 semitones: (value - 64) / 4.0
        semitone_offset = (tune_param - 64) / 4.0f;
    }

    float hz = (float)FREQ[note&0x7F] * semi((int)semitone_offset);
    return hz_to_inc(hz, sr);
}

static void voice_trigger(voice_state_t *v, uint8_t note, int sr){
    v->active = 1;
    v->note = note;
    // Don't reset phase to avoid clicks - let it continue
    // v->ph1 = 0;
    // v->ph2 = 0;

    // OSC1: Base tuning (no detune offset)
    v->inc1 = calc_phase_inc(note, 0, sr);

    // OSC2: With detune (±16 semitones centered at 64)
    int tune = params_get(P_TUNE);
    v->inc_target = calc_phase_inc(note, tune, sr);

    // Glide/Portamento: If glide is OFF, jump immediately. If ON, glide from current to target.
    int glide_param = params_get(P_GLIDE_TIME);
    if(glide_param == 0) {
        // No glide - jump immediately to new frequency
        v->inc_cur = v->inc_target;
        v->inc2 = v->inc_target;
    }
    // else: keep inc_cur at previous value, glide will interpolate in voice_tick

    v->env = ENV_ATTACK;
    v->env_q = 0;
    v->t = 0;

    float a_ms = ((float)params_get(P_ENV_ATTACK)/127.0f)*2000.0f;
    float d_ms = ((float)params_get(P_ENV_DECAY)/127.0f)*2000.0f;
    float r_ms = ((float)params_get(P_ENV_RELEASE)/127.0f)*2000.0f;

    v->atk = ms_to_samples(a_ms, sr);
    v->dec = ms_to_samples(d_ms, sr);
    v->rel = ms_to_samples(r_ms, sr);
    v->sus_q = ((int16_t)params_get(P_ENV_SUSTAIN)*32767)/127;

    // Initialize morph state for time-varying waveforms
    // Seed LFSR with unique value per voice (avoid all voices having same noise)
    v->morph1.lfsr = 0xACE1 + (note << 8);
    v->morph2.lfsr = 0x5EED + (note << 8);
    // Don't reset other morph state - let it continue for smooth morphing across notes
}

static void voice_release(voice_state_t *v){
    if(v->env == ENV_IDLE) return;
    v->env = ENV_RELEASE;
    v->t = 0;
}

static int16_t voice_tick(voice_state_t *v, int sr, int tune, int mix){
    if(!v->active) return 0;

    // OSC1: Always at base tuning (no detune)
    // Keep inc1 as set in voice_trigger

    // OSC2: Update detune live if TUNE param changed
    if(g_tuning_dirty){
        v->inc2 = calc_phase_inc(v->note, tune, sr);
        v->inc_target = v->inc2;
    }

    // Glide (affects OSC2 only)
    float glide = (float)params_get(P_GLIDE_TIME)/127.0f;
    if(glide > 0.01f){
        uint32_t glide_rate = (uint32_t)(glide * 100.0f * (float)sr / 1000.0f);
        if(glide_rate < 1) glide_rate = 1;

        if(v->inc_cur < v->inc_target){
            uint32_t delta = (v->inc_target - v->inc_cur) / glide_rate;
            if(delta < 1) delta = 1;
            v->inc_cur += delta;
            if(v->inc_cur > v->inc_target) v->inc_cur = v->inc_target;
        } else if(v->inc_cur > v->inc_target){
            uint32_t delta = (v->inc_cur - v->inc_target) / glide_rate;
            if(delta < 1) delta = 1;
            v->inc_cur -= delta;
            if(v->inc_cur < v->inc_target) v->inc_cur = v->inc_target;
        }
        // Apply glide to OSC2
        v->inc2 = v->inc_cur;
    }
    
    // Oscillators with anti-aliasing and TIME-VARYING MORPHING
    wave_t w1 = (wave_t)params_get(P_OSC1_WAVE);
    wave_t w2 = (wave_t)params_get(P_OSC2_WAVE);
    if(w1 > 15) w1 = W_SINE;
    if(w2 > 15) w2 = W_SINE;

    // Pass morph state pointers for time-varying waveforms, and envelope state for MORPH_9
    int16_t s1 = wavetable_sample(v->ph1, w1, v->note, &v->morph1, v->env);
    int16_t s2 = wavetable_sample(v->ph2, w2, v->note, &v->morph2, v->env);

    // Use modulated mix (can be modulated by LFO2)
    int32_t osc = ((127-mix)*s1 + mix*s2) / 127;

    v->ph1 += v->inc1;
    v->ph2 += v->inc2;
    
    // Envelope
    switch(v->env){
        case ENV_ATTACK:
            if(v->atk == 0){
                v->env_q = 32767;
                v->env = ENV_DECAY;
                v->t = 0;
            } else {
                v->env_q = (int16_t)((32767UL * v->t) / v->atk);
                v->t++;
                if(v->t >= v->atk){
                    v->env = ENV_DECAY;
                    v->t = 0;
                }
            }
            break;
        case ENV_DECAY:
            if(v->dec == 0){
                v->env_q = v->sus_q;
                v->env = ENV_SUSTAIN;
            } else {
                int16_t delta = 32767 - v->sus_q;
                v->env_q = 32767 - (int16_t)((delta * v->t) / v->dec);
                v->t++;
                if(v->t >= v->dec){
                    v->env = ENV_SUSTAIN;
                }
            }
            break;
        case ENV_SUSTAIN:
            v->env_q = v->sus_q;
            break;
        case ENV_RELEASE:
            if(v->rel == 0){
                v->env_q = 0;
                v->active = 0;
                v->env = ENV_IDLE;
            } else {
                int16_t start = v->env_q;
                v->env_q = start - (int16_t)((start * v->t) / v->rel);
                v->t++;
                if(v->t >= v->rel || v->env_q <= 0){
                    v->env_q = 0;
                    v->active = 0;
                    v->env = ENV_IDLE;
                }
            }
            break;
        default:
            v->env_q = 0;
            v->active = 0;
            break;
    }
    
    return qmul_q15((int16_t)osc, v->env_q);
}

void rockit_engine_init(rockit_engine_t *e){
    (void)e;

    // CRITICAL: Initialize all parameters to their default values
    params_init();

    memset(V, 0, sizeof(V));
    memset(&L1, 0, sizeof(L1));
    memset(&L2, 0, sizeof(L2));

    // Initialize filter with default sample rate
    svf_init(&flt, 48000);

    // Initialize paraphonic system
    paraphonic_init();
}

void rockit_engine_render(rockit_engine_t *e, int16_t *out, size_t frames, int sr){
    g_sr = sr;

    // Check if tuning param changed (ONCE per render call, not per sample!)
    int16_t tune = params_get(P_TUNE);
    g_tuning_dirty = (tune != g_last_tune);
    if(g_tuning_dirty){
        g_last_tune = tune;
    }

    // LFO1 parameters
    float lfo1_hz = 0.01f + ((float)params_get(P_LFO1_RATE)/127.0f)*20.0f;
    L1.inc = hz_to_inc(lfo1_hz, sr);
    L1.shape = params_get(P_LFO1_SHAPE) & 0x0F;
    L1.depth_q = ((int16_t)params_get(P_LFO1_DEPTH)*32767)/127;

    // LFO2 parameters
    float lfo2_hz = 0.01f + ((float)params_get(P_LFO2_RATE)/127.0f)*20.0f;
    L2.inc = hz_to_inc(lfo2_hz, sr);
    L2.shape = params_get(P_LFO2_SHAPE) & 0x0F;
    L2.depth_q = ((int16_t)params_get(P_LFO2_DEPTH)*32767)/127;

    // Filter parameters - Exponential scaling like original Rockit
    // Maps 0-127 to 20Hz-20kHz with exponential curve for musical response
    int cutoff_param = params_get(P_FILTER_CUTOFF);
    float cutoff_norm = cutoff_param / 127.0f;  // 0.0 to 1.0
    float cutoff_hz = 20.0f * powf(1000.0f, cutoff_norm);  // 20Hz to 20kHz exponential

    float q = 0.5f + (params_get(P_FILTER_RESONANCE)/127.0f) * 19.5f;
    svf_set_cutoff(&flt, cutoff_hz);
    svf_set_q(&flt, q);

    // Get filter mode for later use in the loop
    int filter_mode = params_get(P_FILTER_MODE);

    // LIVE ENVELOPE PARAMETER UPDATES - Read envelope params and update all active voices
    // This allows real-time parameter changes while notes are held (like real synths)
    float a_ms = ((float)params_get(P_ENV_ATTACK)/127.0f)*2000.0f;
    float d_ms = ((float)params_get(P_ENV_DECAY)/127.0f)*2000.0f;
    float r_ms = ((float)params_get(P_ENV_RELEASE)/127.0f)*2000.0f;
    uint32_t atk_samples = ms_to_samples(a_ms, sr);
    uint32_t dec_samples = ms_to_samples(d_ms, sr);
    uint32_t rel_samples = ms_to_samples(r_ms, sr);
    int16_t sus_q = ((int16_t)params_get(P_ENV_SUSTAIN)*32767)/127;

    // Update envelope parameters for all active voices
    for(int v=0; v<3; v++){
        if(V[v].active){
            V[v].atk = atk_samples;
            V[v].dec = dec_samples;
            V[v].rel = rel_samples;
            V[v].sus_q = sus_q;
        }
    }

    // ==== DRONE MODE ====
    // Continuous note-on with manual pitch/amplitude control (from original Rockit)
    // - ENV_ATTACK controls pitch (0-127 = MIDI notes 0-127)
    // - ENV_SUSTAIN controls amplitude (envelope forced to sustain level)
    static uint8_t prev_drone_mode = 0;
    static uint8_t drone_triggered = 0;
    uint8_t drone_mode = params_get(P_DRONE_MODE);

    if(drone_mode) {
        // Drone mode is active
        uint8_t drone_note = params_get(P_ENV_ATTACK);  // Use attack knob for pitch (0-127)

        // On drone mode activation, trigger a note
        if(!prev_drone_mode) {
            rockit_note_on(drone_note);
            drone_triggered = 1;
        }

        // Keep the drone playing by forcing envelope to sustain state
        // The sustain level is already controlled by ENV_SUSTAIN parameter above
        for(int v=0; v<3; v++){
            if(V[v].active){
                // Force envelope to sustain state for continuous sound
                V[v].env = ENV_SUSTAIN;
                // Sustain level is already set from sus_q above
            }
        }
    } else {
        // Drone mode deactivated
        if(prev_drone_mode && drone_triggered) {
            // Release all voices when leaving drone mode
            for(int v=0; v<3; v++){
                if(V[v].active){
                    V[v].env = ENV_RELEASE;
                }
            }
            drone_triggered = 0;
        }
    }
    prev_drone_mode = drone_mode;

    for(size_t i=0; i<frames; i++){
        // ==== LFO MODULATION SYSTEM (matches original Rockit routing) ====

        // Generate LFO waveforms (0-255 unsigned)
        uint8_t lfo1_wave = (uint8_t)((lfo_wave(L1.ph, L1.shape) + 32768) >> 8);
        uint8_t lfo2_wave = (uint8_t)((lfo_wave(L2.ph, L2.shape) + 32768) >> 8);

        // LFO 1 Modulation Routing
        int lfo1_dest = params_get(P_LFO1_DEST);
        int lfo1_depth = params_get(P_LFO1_DEPTH);
        int16_t lfo1_mod = 0;  // Modulation amount to add to parameter
        if(lfo1_depth > 0) {
            int16_t mod = (int16_t)lfo1_wave - 128;  // Center to bipolar (-128 to +127)
            lfo1_mod = (mod * lfo1_depth) >> 7;      // Scale by depth, divide by 128
        }

        // LFO 2 Modulation Routing
        int lfo2_dest = params_get(P_LFO2_DEST);
        int lfo2_depth = params_get(P_LFO2_DEPTH);
        int16_t lfo2_mod = 0;
        if(lfo2_depth > 0) {
            int16_t mod = (int16_t)lfo2_wave - 128;
            lfo2_mod = (mod * lfo2_depth) >> 7;
        }

        // Apply LFO modulation to destinations (matching original Rockit)
        // LFO1 destinations: 0:Amp, 1:Filter, 2:FilterQ, 3:FilterEnv, 4:Pitch, 5:Detune
        // LFO2 destinations: 0:Mix, 1:Filter, 2:FilterQ, 3:LFO1Rate, 4:LFO1Depth, 5:FilterAtk

        int16_t modulated_vol = params_get(P_MASTER_VOL);
        int16_t modulated_mix = params_get(P_OSC_MIX);
        int16_t modulated_tune = tune;  // Already loaded from outer scope

        // LFO 1 routing (only safe per-sample modulations - no filter!)
        switch(lfo1_dest) {
            case 0: modulated_vol += lfo1_mod; break;       // Amplitude
            case 1: break;  // Filter Cutoff - DISABLED (can't update coefficients per-sample)
            case 2: break;  // Filter Q - DISABLED (can't update coefficients per-sample)
            case 3: break;  // Filter Env Amount - not implemented yet
            case 4: break;  // Pitch Shift - global pitch bend, complex
            case 5: modulated_tune += lfo1_mod; break;      // Detune
        }

        // LFO 2 routing (only safe per-sample modulations - no filter!)
        switch(lfo2_dest) {
            case 0: modulated_mix += lfo2_mod; break;       // OSC Mix
            case 1: break;  // Filter Cutoff - DISABLED (can't update coefficients per-sample)
            case 2: break;  // Filter Q - DISABLED (can't update coefficients per-sample)
            case 3: break;  // LFO1 Rate - meta-modulation, complex
            case 4: break;  // LFO1 Depth - meta-modulation, complex
            case 5: break;  // Filter Attack - not implemented yet
        }

        // Clamp modulated values to valid ranges
        if(modulated_vol < 0) modulated_vol = 0;
        if(modulated_vol > 127) modulated_vol = 127;
        if(modulated_mix < 0) modulated_mix = 0;
        if(modulated_mix > 127) modulated_mix = 127;
        if(modulated_tune < 0) modulated_tune = 0;
        if(modulated_tune > 127) modulated_tune = 127;

        // Apply exponential curve to modulated volume (LFO tremolo affects volume curve)
        float vol_norm_mod = modulated_vol / 127.0f;
        int16_t vol_q_mod = (int16_t)(32767.0f * vol_norm_mod * vol_norm_mod);

        // Tick LFO phase accumulators
        L1.ph += L1.inc;
        L2.ph += L2.inc;
        
        // Voice summing with proper scaling
        int32_t mix = 0;
        int active_voices = 0;
        for(int v=0; v<3; v++){
            if(V[v].active){
                // Pass modulated tune and mix to voice_tick
                mix += voice_tick(&V[v], sr, modulated_tune, modulated_mix);
                active_voices++;
            }
        }

        // Scale by voice count to prevent clipping
        if(active_voices > 1){
            mix = mix / active_voices;
        }

        // Convert to float for filter and apply correct filter mode
        float sf = (float)sat16(mix) / 32768.0f;
        switch(filter_mode) {
            case 0: sf = svf_process_lp(&flt, sf); break;  // Lowpass
            case 1: sf = svf_process_hp(&flt, sf); break;  // Highpass
            case 2: sf = svf_process_bp(&flt, sf); break;  // Bandpass
            case 3: sf = svf_process_notch(&flt, sf); break;  // Notch
            default: sf = svf_process_lp(&flt, sf); break;
        }
        int16_t filtered = (int16_t)(sf * 32768.0f);

        // Apply modulated master volume
        int16_t v16 = qmul_q15(filtered, vol_q_mod);
        
        // STEREO OUTPUT
        out[2*i+0] = v16; 
        out[2*i+1] = v16;
    }
}

void rockit_note_on(uint8_t note){
    // Use paraphonic allocator
    paraphonic_note_on(note, 100);
    
    // Update voice states from paraphonic allocator
    voice_t voices[3];
    paraphonic_get_voices(voices);
    
    for(int i=0; i<3; i++){
        // Trigger if allocator says active AND (new note OR voice was idle)
        if(voices[i].active && (V[i].note != voices[i].note || V[i].env == ENV_IDLE)){
            voice_trigger(&V[i], voices[i].note, g_sr);
        }
    }
}

void rockit_note_off(uint8_t note){
    // Use paraphonic allocator
    paraphonic_note_off(note);
    
    // Update voice states from paraphonic allocator
    voice_t voices[3];
    paraphonic_get_voices(voices);
    
    // Release voices that should no longer be active
    for(int i=0; i<3; i++){
        if(V[i].active && V[i].note == note && !voices[i].active){
            voice_release(&V[i]);
        }
    }
}

void rockit_handle_cc(uint8_t cc, uint8_t value){
    // Pass to paraphonic handler first
    paraphonic_handle_cc(cc, value);

    // Standard MIDI CC mapping (compatible with web UI and v0.9)
    switch(cc){
        // LFO 1 (primary)
        case 1:  params_set(P_LFO1_DEPTH, value); break;             // Mod wheel
        case 87: params_set(P_LFO1_RATE, value); break;
        case 88: params_set(P_LFO1_SHAPE, value >> 3); break;        // 0-15 from 0-127
        case 89: params_set(P_LFO1_DEST, value >> 4); break;         // 0-7 from 0-127

        // LFO 2 (extended controls - no UI yet, placeholders for future)
        case 95: params_set(P_LFO2_RATE, value); break;
        case 96: params_set(P_LFO2_DEPTH, value); break;
        case 97: params_set(P_LFO2_SHAPE, value >> 3); break;        // 0-15 from 0-127
        case 98: params_set(P_LFO2_DEST, value >> 4); break;         // 0-7 from 0-127

        // Master
        case 7:  params_set(P_MASTER_VOL, value); break;

        // Filter
        case 74: params_set(P_FILTER_CUTOFF, value); break;
        case 71: params_set(P_FILTER_RESONANCE, value); break;
        case 84: params_set(P_FILTER_MODE, value >> 5); break;       // 0-3 from 0,32,64,96
        case 85: params_set(P_FILTER_ENV_AMT, value); break;

        // Oscillators
        case 72: params_set(P_OSC_MIX, value); break;
        case 76: params_set(P_SUBOSC, (value>=64)?1:0); break;
        case 80: params_set(P_OSC1_WAVE, value >> 3); break;         // 0-15 from 0-127 (16 waveforms)
        case 81: params_set(P_OSC2_WAVE, value >> 3); break;         // 0-15 from 0-127
        case 82: params_set(P_TUNE, value); break;                   // 0-127, center 64, ±16 semitones

        // Envelope (Amplitude)
        case 73: params_set(P_ENV_ATTACK, value); break;
        case 75: params_set(P_ENV_DECAY, value); break;
        case 86: params_set(P_ENV_SUSTAIN, value); break;
        case 70: params_set(P_ENV_RELEASE, value); break;

        // Global
        case 90: params_set(P_GLIDE_TIME, value); break;
        case 91: params_set(P_DRONE_MODE, value >= 64 ? 1 : 0); break;  // Toggle: 0-63=off, 64-127=on

        // Patch Save/Recall (using simple text file storage instead of EEPROM)
        case 92: {  // Save Patch: value 0-127 maps to patch 0-15
            uint8_t patch_num = value >> 3;  // Divide by 8: 0-7 = patch 0, 8-15 = patch 1, etc.
            if(patch_save(patch_num) == 0) {
                fprintf(stderr, "✓ Saved to patch %d (CC92 value=%d)\n", patch_num, value);
            }
            break;
        }
        case 93: {  // Recall Patch: value 0-127 maps to patch 0-15
            uint8_t patch_num = value >> 3;
            if(patch_recall(patch_num) == 0) {
                fprintf(stderr, "✓ Recalled patch %d (CC93 value=%d)\n", patch_num, value);
            }
            break;
        }

        default: break;
    }
}