#include "rockit_engine.h"
#include "paraphonic.h"
#include "filter_svf.h"
#include <string.h>
#include <stdint.h>
#include <math.h>
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

// Wave types
typedef enum { W_SINE=0, W_TRI=1, W_RAMP=2, W_SQUARE=3 } wave_t;

static inline int16_t wavetable_sample(uint32_t phase, wave_t w){
    uint8_t i = (uint8_t)(phase >> 24);
    switch(w){
        case W_TRI:    return ((int16_t)G_AUC_TRIANGLE_SIMPLE_WAVETABLE_LUT[i]-128)<<7;
        case W_RAMP:   return ((int16_t)G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[i]-128)<<7;
        case W_SQUARE: return ((int16_t)G_AUC_SQUARE_SIMPLE_WAVETABLE_LUT[i]-128)<<7;
        default:       return ((int16_t)G_AUC_SINE_SIMPLE_WAVETABLE_LUT[i]-128)<<7;
    }

    return 0;
}



// LFO
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

static inline int16_t lfo_wave(uint32_t ph, uint8_t shape){
    uint8_t i=(uint8_t)(ph>>24);
    switch(shape){
        case 1: return lut8_to_q15((i<128)?(i<<1):(255-((i-128)<<1))); // tri
        case 2: return lut8_to_q15(i); // ramp
        case 3: return (i<128)?32767:-32768; // square
        default:return lut8_to_q15(G_AUC_SIN_LUT[i]);
    }
    
    return 0;
}


// Envelope states
typedef enum { ENV_IDLE=0, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE } env_t;

// Per-voice state
typedef struct {
    uint8_t active;
    uint8_t note;
    uint32_t ph1, inc1, ph2, inc2;
    
    // Envelope
    env_t env;
    int16_t env_q;
    uint32_t t, atk, dec, rel;
    int16_t sus_q;
    
    // Glide
    uint32_t inc_target, inc_cur;
} voice_state_t;

static voice_state_t V[3];
static lfo_t L1;
static svf_t flt;
static int g_sr=48000;

// Freq LUT
static const uint16_t FREQ[128]={
 8,9,9,10,10,11,12,12,13,14,15,15,16,17,18,19,21,22,23,24,26,28,29,31,33,35,37,39,41,44,46,49,52,55,58,62,
 65,69,73,78,82,87,92,98,104,110,117,123,131,139,147,156,165,175,185,196,208,220,233,247,262,277,294,311,
 330,349,370,392,415,440,466,494,523,554,587,622,659,698,740,784,831,880,932,988,1047,1109,1175,1245,1319,
 1397,1480,1568,1661,1760,1865,1976,2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,3951,4186,4435,
 4699,4978,5274,5588,5920,6272,6645,7040,7459,7902,8372,8870,9397,9956,10548,11175,11840,12544
};

static inline uint32_t note_to_inc(uint8_t note){ 
    return hz_to_inc((float)FREQ[note&0x7F], g_sr); 
}

static inline float semi(float st){ 
    return powf(2.0f, st/12.0f); 
}

static void apply_env_params(voice_state_t* v){
    #define MAPMS(x) ( (1u + (unsigned)(x)*4999u/127u) )
    v->atk = (uint32_t)( (uint64_t)MAPMS(params_get(P_ENV_ATTACK)) * g_sr / 1000 );
    v->dec = (uint32_t)( (uint64_t)MAPMS(params_get(P_ENV_DECAY))  * g_sr / 1000 );
    v->rel = (uint32_t)( (uint64_t)MAPMS(params_get(P_ENV_RELEASE))* g_sr / 1000 );
    v->sus_q = (int16_t)(params_get(P_ENV_SUSTAIN) * 257);
    #undef MAPMS
}

static void voice_trigger(voice_state_t* v, uint8_t note){
    v->note = note;
    v->active = 1;
    v->env = ENV_ATTACK;
    v->t = 0;
    
    int16_t coarse = params_get(P_TUNE);
    int16_t fine = params_get(P_FINE);
    float st = coarse*0.5f + fine*(1.0f/64.0f);
    float hz = (float)FREQ[note&0x7F] * semi(st);
    uint32_t inc = hz_to_inc(hz, g_sr);
    
    v->inc1 = v->inc_cur = v->inc_target = inc;
    v->inc2 = (params_get(P_SUBOSC) ? (inc>>1) : inc);
    
    apply_env_params(v);
}

static void voice_release(voice_state_t* v){ 
    if(!v->active) return; 
    v->env = ENV_RELEASE; 
    v->t = 0; 
}

static int16_t voice_tick(voice_state_t* v){
    if(!v->active) return 0;
    
    // Glide
    uint32_t glide_ms = (uint32_t)(params_get(P_GLIDE_TIME)*20);
    if(glide_ms > 0){
        int32_t delta = (int32_t)v->inc_target - (int32_t)v->inc_cur;
        // FIX: Ensure calculation of step uses signed intermediate, but keep result small
        int32_t step_size = (int32_t)((int64_t)delta * 1000 / ((glide_ms * (uint32_t)g_sr) + 1));

        if(delta > 0 && step_size > 0) v->inc_cur += step_size;
        else if(delta < 0 && step_size < 0) v->inc_cur += step_size;
        else if(delta < 0 && step_size > 0) v->inc_cur -= step_size;
        else if(delta > 0 && step_size < 0) v->inc_cur -= step_size; // Should not happen with current formula, but safe
        else v->inc_cur = v->inc_target;

    } else { 
        v->inc_cur = v->inc_target; 
    }
    
    v->inc1 = v->inc_cur;
    v->inc2 = (params_get(P_SUBOSC) ? (v->inc_cur>>1) : v->inc_cur);
    
    // Envelope
    switch(v->env){
        case ENV_ATTACK:
            if(v->atk==0){ v->env_q=32767; v->env=ENV_DECAY; v->t=0; break; }
            // FIX: Prevent overflow on v->t++
            if (v->t > v->atk) { v->t = v->atk; } 

            v->env_q = (int16_t)((int32_t)v->t * 32767 / (int32_t)v->atk);
            if(v->t++ >= v->atk){ v->env_q=32767; v->env=ENV_DECAY; v->t=0; }
            break;
        case ENV_DECAY:{
            if(v->dec==0){ v->env_q=v->sus_q; v->env=ENV_SUSTAIN; v->t=0; break; }
            // FIX: Prevent overflow on v->t++
            if (v->t > v->dec) { v->t = v->dec; } 

            int32_t delta = 32767 - v->sus_q;
            int32_t down = (int32_t)v->t * delta / (int32_t)v->dec;
            int32_t lvl = 32767 - down; 
            if(lvl < v->sus_q) lvl = v->sus_q;
            v->env_q=(int16_t)lvl;
            if(v->t++ >= v->dec){ v->env_q=v->sus_q; v->env=ENV_SUSTAIN; v->t=0; }
        } break;
        case ENV_SUSTAIN: 
            v->env_q=v->sus_q; 
            break;
        case ENV_RELEASE:{
            if(v->rel==0){ v->env_q=0; v->env=ENV_IDLE; v->active=0; v->t=0; break; }
            // REMOVED: int32_t initial_env_q = (v->t == 0) ? 32767 : v->env_q; // Fixes unused variable warning
            
            // FIX: Prevent overflow on v->t++
            if (v->t > v->rel) { v->t = v->rel; }

            // Using sustained level as release start is more accurate for real synths
            int32_t down = (int32_t)v->t * v->sus_q / (int32_t)v->rel; 
            int32_t lvl = v->sus_q - down; 
            if(lvl<0) lvl=0;
            v->env_q=(int16_t)lvl;
            if(v->t++ >= v->rel){ v->env_q=0; v->env=ENV_IDLE; v->active=0; v->t=0; }
        } break;
        default: 
            v->env_q=0; 
            break;
    }
    
    // Oscillators - read waveforms from parameters
    int16_t mix_q = (int16_t)(params_get(P_OSC_MIX) * 257);
    wave_t wave1 = (wave_t)params_get(P_OSC1_WAVE);
    wave_t wave2 = (wave_t)params_get(P_OSC2_WAVE);
    int16_t s1 = wavetable_sample(v->ph1, wave1);
    int16_t s2 = wavetable_sample(v->ph2, wave2);
    int16_t a = qmul_q15((int16_t)(32767 - mix_q), s1);
    int16_t b = qmul_q15(mix_q, s2);
    int16_t s = a + b;
    
    v->ph1 += v->inc1; 
    v->ph2 += v->inc2;
    
    return qmul_q15(s, v->env_q);
}

void rockit_engine_init(rockit_engine_t *e){
    (void)e; 
    params_init(); 
    memset(V, 0, sizeof(V));
    
    // Initialize paraphonic allocator
    paraphonic_init();
    
    // LFO setup
    L1.ph=0; 
    L1.shape=(uint8_t)params_get(P_LFO_SHAPE); 
    L1.depth_q=0;
    L1.inc = hz_to_inc(2.0f, g_sr);
    
    // Filter setup
    svf_init(&flt, 48000);
    svf_set_cutoff(&flt, 1000.0f);
    svf_set_q(&flt, 1.0f);

    // Set sensible defaults for audible output
    params_set(P_MASTER_VOL, 100);      // Set volume to ~80%
    params_set(P_FILTER_CUTOFF, 100);   // Open filter considerably
    params_set(P_FILTER_RESONANCE, 20); // Moderate resonance
    params_set(P_FILTER_MODE, 0);       // Lowpass
    params_set(P_FILTER_ENV_AMT, 64);   // Medium envelope amount
    params_set(P_ENV_ATTACK, 5);        // Fast attack
    params_set(P_ENV_DECAY, 40);        // Medium decay
    params_set(P_ENV_SUSTAIN, 100);     // High sustain
    params_set(P_ENV_RELEASE, 30);      // Medium release
    params_set(P_OSC1_WAVE, 2);         // Sawtooth
    params_set(P_OSC2_WAVE, 3);         // Square
    params_set(P_OSC_MIX, 64);          // Even mix
    params_set(P_TUNE, 64);             // Center tune
    params_set(P_FINE, 64);             // Center fine
    params_set(P_SUBOSC, 0);            // Off
    params_set(P_LFO_RATE, 32);         // Moderate rate
    params_set(P_LFO_DEPTH, 0);         // Off by default
    params_set(P_LFO_SHAPE, 0);         // Sine
    params_set(P_LFO_DEST, 0);          // Off
    params_set(P_GLIDE_TIME, 0);        // No glide
}

void rockit_note_on(uint8_t note){
    // FIX: Remove hardcoded velocity and let paraphonic handle it
    paraphonic_note_on(note, 100);  
    
    // Update voice states from paraphonic allocator
    voice_t voices[3];
    paraphonic_get_voices(voices);
    
    for(int i=0; i<3; i++){
        // Check if the allocator says this voice should be active AND it should play a new note
        // The check 'V[i].note != voices[i].note' handles both new note and note retrigger
        if(voices[i].active && (V[i].note != voices[i].note || V[i].env == ENV_IDLE)){
            voice_trigger(&V[i], voices[i].note);
        }
    }
}

void rockit_note_off(uint8_t note){
    paraphonic_note_off(note);
    
    // Update voice states from paraphonic allocator
    voice_t voices[3];
    paraphonic_get_voices(voices);
    
    // Release voices that are no longer playing one of the currently held notes
    for(int i=0; i<3; i++){
        // Check if the synth voice is playing the note that was just released, OR 
        // if the voice is currently active, but the allocator says it should now be idle.
        if(V[i].active && V[i].note == note && !voices[i].active){
             voice_release(&V[i]);
        }
    }
}

void rockit_handle_cc(uint8_t cc, uint8_t val){
    // Pass to paraphonic handler first
    paraphonic_handle_cc(cc, val);
    
    // Then handle other CCs
    switch(cc){
        // LFO
        case 1:  params_set(P_LFO_DEPTH, val); break;
        case 87: params_set(P_LFO_RATE, val); break;
        case 88: params_set(P_LFO_SHAPE, val >> 5); break;   // 0-3 from 0,32,64,96
        case 89: params_set(P_LFO_DEST, val >> 5); break;    // 0-4 from 0,32,64,96,127
        
        // Master
        case 7:  params_set(P_MASTER_VOL, val); break;
        
        // Filter
        case 74: params_set(P_FILTER_CUTOFF, val); break;
        case 71: params_set(P_FILTER_RESONANCE, val); break;
        case 84: params_set(P_FILTER_MODE, val >> 5); break; // 0-3 from 0,32,64,96
        case 85: params_set(P_FILTER_ENV_AMT, val); break;
        
        // Oscillators
        case 72: params_set(P_OSC_MIX, val); break;
        case 76: params_set(P_SUBOSC, (val>=64)?1:0); break;
        case 80: params_set(P_OSC1_WAVE, val & 0x03); break; // 0-3
        case 81: params_set(P_OSC2_WAVE, val & 0x03); break; // 0-3
        case 82: params_set(P_TUNE, val); break;
        case 83: params_set(P_FINE, val); break;
        
        // Envelope (Amplitude)
        case 73: params_set(P_ENV_ATTACK, val); break;
        case 75: params_set(P_ENV_DECAY, val); break;
        case 86: params_set(P_ENV_SUSTAIN, val); break;
        case 70: params_set(P_ENV_RELEASE, val); break;
        
        // Global
        case 90: params_set(P_GLIDE_TIME, val); break;
        
        default: break;
    }
}

void rockit_engine_render(rockit_engine_t *e, int16_t *out, size_t frames, int sample_rate){
    (void)e; 
    g_sr = sample_rate;
    
    // Update LFO
    float lfo_hz = 0.1f + (params_get(P_LFO_RATE)/127.0f)*10.0f;
    L1.inc = hz_to_inc(lfo_hz, g_sr);
    L1.shape = (uint8_t)params_get(P_LFO_SHAPE);
    L1.depth_q = (int16_t)(params_get(P_LFO_DEPTH)*128);
    
    // Update filter
    float cutoff_hz = 50.0f + (params_get(P_FILTER_CUTOFF)/127.0f) * 12000.0f;
    svf_set_cutoff(&flt, cutoff_hz);
    float Q = 0.5f + (params_get(P_FILTER_RESONANCE)/127.0f) * 10.0f;
    svf_set_q(&flt, Q);
    
    int16_t vol_q = (int16_t)(params_get(P_MASTER_VOL)*257);
    uint8_t lfo_dest = (uint8_t)params_get(P_LFO_DEST);
    
    for(size_t i=0; i<frames; i++){
        int16_t l = lfo_wave(L1.ph, L1.shape); 
        L1.ph += L1.inc;
        
        int32_t mix = 0;
        int active_voices = 0;
        for(int v=0; v<3; v++){
            // Apply LFO to destination
            if(lfo_dest == 1){ // pitch vibrato
                uint32_t base = V[v].inc_target ? V[v].inc_target : note_to_inc(V[v].note);
                // FIX: Use 32767 for Q1.15 range instead of 30, for full depth range.
                int32_t det = ((int32_t)base * l * L1.depth_q) / (32767 * 32767); 
                V[v].inc_target = (uint32_t)((int32_t)base + det);
            }
            
            int16_t s = voice_tick(&V[v]);
            if(V[v].active){
                mix += s;
                active_voices++;
            }
        }
        
        // Scale down mix to prevent clipping with multiple voices
        if(active_voices > 1){
            mix = mix / active_voices;
        }
        
        // Apply filter
        // FIX: The original code used voice_tick, but the combined signal must be filtered.
        float sf = (float)sat16(mix) / 32768.0f;
        sf = svf_process_lp(&flt, sf);
        int16_t filtered = (int16_t)(sf * 32768.0f);
        
        int16_t v16 = qmul_q15(filtered, vol_q);
        out[2*i+0] = v16; 
        out[2*i+1] = v16;
    }
}