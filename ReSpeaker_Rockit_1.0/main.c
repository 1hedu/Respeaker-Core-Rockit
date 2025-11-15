#include <alsa/asoundlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include "rockit_engine.h"
#include "socket_midi_raw.h"
#include "patch_storage.h"

static volatile int run=1; 
static snd_pcm_t* h=NULL;

static void onint(int s){
    (void)s;
    run=0;
}

// FIX: Replaced *_alloca with *_malloc/*_free functions to fix linker error.
static int setup(const char*d, int r, snd_pcm_uframes_t p){
    int e; 
    snd_pcm_hw_params_t *hw = NULL; 
    snd_pcm_sw_params_t *sw = NULL;
    
    if((e=snd_pcm_open(&h,d,SND_PCM_STREAM_PLAYBACK,0))<0){
        fprintf(stderr,"open %s: %s\n",d,snd_strerror(e));
        return -1;
    }
    
    // Allocate hardware parameters structure
    if (snd_pcm_hw_params_malloc(&hw) < 0) {
        fprintf(stderr,"hw params malloc failed\n");
        snd_pcm_close(h);
        return -1;
    }

    snd_pcm_hw_params_any(h,hw);
    snd_pcm_hw_params_set_access(h,hw,SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(h,hw,SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate(h,hw,r,0);
    snd_pcm_hw_params_set_channels(h,hw,2);
    
    snd_pcm_uframes_t bs=p*4;
    snd_pcm_hw_params_set_period_size(h,hw,p,0);
    snd_pcm_hw_params_set_buffer_size_near(h,hw,&bs);
    
    if((e=snd_pcm_hw_params(h,hw))<0){
        fprintf(stderr,"hw: %s\n",snd_strerror(e));
        snd_pcm_hw_params_free(hw);
        snd_pcm_close(h);
        return -1;
    }
    
    // Allocate software parameters structure
    if (snd_pcm_sw_params_malloc(&sw) < 0) {
        fprintf(stderr,"sw params malloc failed\n");
        snd_pcm_hw_params_free(hw);
        snd_pcm_close(h);
        return -1;
    }

    snd_pcm_sw_params_current(h,sw);
    snd_pcm_sw_params_set_start_threshold(h,sw,p);
    snd_pcm_sw_params_set_avail_min(h,sw,p);
    
    if((e=snd_pcm_sw_params(h,sw))<0){
        fprintf(stderr,"sw: %s\n",snd_strerror(e));
        snd_pcm_sw_params_free(sw);
        snd_pcm_hw_params_free(hw);
        snd_pcm_close(h);
        return -1;
    }
    
    // Free the parameter structures before exit
    snd_pcm_hw_params_free(hw);
    snd_pcm_sw_params_free(sw);
    
    return 0;
}

static void cc_handler(uint8_t cc, uint8_t val){
    rockit_handle_cc(cc, val);
}

static void note_on_cb(uint8_t note){ 
    rockit_note_on(note); 
}

static void note_off_cb(uint8_t note){ 
    rockit_note_off(note); 
}

// Track which notes are currently held via CLI
static uint8_t cli_notes_held[128] = {0};

// --- IMPROVED CLI Handler with note holding ---
static void handle_cli_input() {
    char input_line[64];
    char command[16];
    int value;

    // Make stdin non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    
    if (fgets(input_line, sizeof(input_line), stdin) != NULL) {
        // Restore blocking mode
        fcntl(STDIN_FILENO, F_SETFL, flags);
        
        if (sscanf(input_line, "%15s %d", command, &value) >= 1) {
            
            // Convert command to uppercase
            for (int i = 0; command[i]; i++) {
                command[i] = toupper(command[i]);
            }

            // Clamp MIDI values to 0-127
            if (value < 0) value = 0;
            if (value > 127) value = 127;

            // Handle commands
            if (strcmp(command, "CUTOFF") == 0 || strcmp(command, "FLT") == 0) {
                rockit_handle_cc(74, (uint8_t)value);
                fprintf(stderr, "CLI: Set Cutoff to %d\n", value);
            } else if (strcmp(command, "RESO") == 0 || strcmp(command, "Q") == 0) {
                rockit_handle_cc(71, (uint8_t)value);
                fprintf(stderr, "CLI: Set Resonance to %d\n", value);
            } else if (strcmp(command, "VOL") == 0 || strcmp(command, "VOLUME") == 0) {
                rockit_handle_cc(7, (uint8_t)value);
                fprintf(stderr, "CLI: Set Volume to %d\n", value);
            } else if (strcmp(command, "MIX") == 0) {
                rockit_handle_cc(72, (uint8_t)value);
                fprintf(stderr, "CLI: Set OSC Mix to %d\n", value);
            } else if (strcmp(command, "ATTACK") == 0 || strcmp(command, "ATK") == 0) {
                rockit_handle_cc(73, (uint8_t)value);
                fprintf(stderr, "CLI: Set Attack to %d\n", value);
            } else if (strcmp(command, "DECAY") == 0 || strcmp(command, "DEC") == 0) {
                rockit_handle_cc(75, (uint8_t)value);
                fprintf(stderr, "CLI: Set Decay to %d\n", value);
            } else if (strcmp(command, "RELEASE") == 0 || strcmp(command, "REL") == 0) {
                rockit_handle_cc(70, (uint8_t)value);
                fprintf(stderr, "CLI: Set Release to %d\n", value);
            } else if (strcmp(command, "NOTE") == 0 || strcmp(command, "ON") == 0 || strcmp(command, "N") == 0) {
                // Turn on note and track it
                rockit_note_on((uint8_t)value);
                cli_notes_held[value] = 1;
                fprintf(stderr, "CLI: Note On %d (stays on until OFF)\n", value);
            } else if (strcmp(command, "OFF") == 0) {
                // Turn off specific note
                rockit_note_off((uint8_t)value);
                cli_notes_held[value] = 0;
                fprintf(stderr, "CLI: Note Off %d\n", value);
            } else if (strcmp(command, "ALLOFF") == 0 || strcmp(command, "PANIC") == 0) {
                // Turn off all notes
                for(int i = 0; i < 128; i++) {
                    if(cli_notes_held[i]) {
                        rockit_note_off(i);
                        cli_notes_held[i] = 0;
                    }
                }
                fprintf(stderr, "CLI: All notes off\n");
            } else if (strcmp(command, "HELP") == 0 || strcmp(command, "?") == 0) {
                fprintf(stderr, "\nCLI Commands:\n");
                fprintf(stderr, "  NOTE <0-127>      - Turn note on (stays on!)\n");
                fprintf(stderr, "  OFF <0-127>       - Turn note off\n");
                fprintf(stderr, "  ALLOFF            - Turn all notes off\n");
                fprintf(stderr, "  CUTOFF <0-127>    - Filter cutoff\n");
                fprintf(stderr, "  RESO <0-127>      - Filter resonance\n");
                fprintf(stderr, "  VOL <0-127>       - Master volume\n");
                fprintf(stderr, "  MIX <0-127>       - Oscillator mix\n");
                fprintf(stderr, "  ATTACK <0-127>    - Envelope attack\n");
                fprintf(stderr, "  DECAY <0-127>     - Envelope decay\n");
                fprintf(stderr, "  RELEASE <0-127>   - Envelope release\n");
                fprintf(stderr, "  HELP              - Show this help\n\n");
            } else {
                fprintf(stderr, "CLI: Unknown command '%s' (type HELP)\n", command);
            }
        }
    } else {
        // Restore blocking mode even if no input
        fcntl(STDIN_FILENO, F_SETFL, flags);
    }
}
// --- END IMPROVED CLI ---

int main(int argc,char**argv){
    const char*dev = "default"; // Default audio device name
    int rate = 48000;
    snd_pcm_uframes_t per = 256;

    signal(SIGINT, onint);

    rockit_engine_t e;
    rockit_engine_init(&e);

    // Initialize patch storage system (creates /tmp/rockit_patches directory)
    patch_storage_init();

    // --- ARGUMENT PARSING LOOP ---
    for(int ai=1; ai<argc; ++ai){
        
        // 1. Check for the MIDI flag first
        if(strcmp(argv[ai], "--alsa")==0 || strcmp(argv[ai], "--tcp-midi")==0){ 
            socket_midi_raw_start(50000, note_on_cb, note_off_cb, cc_handler); 
            
            // --- CC COMMANDS ---
            fprintf(stderr,"RAW MIDI TUNNEL (127.0.0.1:50000) Active.\\n");
            fprintf(stderr,"  Input is 3-byte Raw MIDI (Status, Data1, Data2).\\n");
            fprintf(stderr,"  CC 102: Mono/Para (0-63=Mono, 64-127=Para)\\n");
            fprintf(stderr,"  CC 103: 3-voice (0-63=2-voice, 64-127=3-voice)\\n");
            fprintf(stderr,"  CC 104: Cycle para modes (Low→Last→RR→High)\\n");
            fprintf(stderr,"  CC 105: 3-voice toggle\\n");
            fprintf(stderr,"  CC 76:  Sub-osc (0-63=Off, 64-127=On)\\n");
            fprintf(stderr,"  CC 1:   LFO Depth\\n");
            fprintf(stderr,"  CC 7:   Master Volume\\n");
            fprintf(stderr,"  CC 74:  Filter Cutoff\\n");
            fprintf(stderr,"  CC 71:  Filter Resonance\\n");
            fprintf(stderr,"  CC 72:  Osc Mix\\n");
            fprintf(stderr,"  CC 73:  Attack\\n");
            fprintf(stderr,"  CC 75:  Decay\\n");
            fprintf(stderr,"  CC 70:  Release\\n\\n");
        }

        // 2. Check for the device name override flag
        else if(strcmp(argv[ai], "-d")==0 && ai+1 < argc){
            dev = argv[ai+1];
            ai++; // Consume the next argument (the device name)
        }
        
        // 3. Otherwise, if the current device is still 'default', set it to this argument
        else if (strcmp(dev, "default") == 0) {
            dev = argv[ai];
        }
    }
    // --- END ARGUMENT PARSING LOOP ---
    
    // Setup audio PCM with the determined device name
    if(setup(dev, rate, per) < 0) return 1;

    // Allocate and CLEAR audio buffer to prevent garbage noise on startup
    int16_t *buf = (int16_t*)calloc(per * 2, sizeof(int16_t));
    if (!buf) {
        fprintf(stderr, "Error: Failed to allocate audio buffer\n");
        return 1;
    }
    
    fprintf(stderr,"==============================================\n");
    fprintf(stderr,"Rockit Paraphonic Synth - ReSpeaker Edition\n");
    fprintf(stderr,"==============================================\n");
    fprintf(stderr,"Audio: %s @ %d Hz, period %lu\n", dev, rate, (unsigned long)per);
    fprintf(stderr,"Tip: use 'hw:0,0' for 1/4\" line out if default doesn't work\n\n");
    
    fprintf(stderr,"Starting audio engine...\n");

    // Reset ALSA state to clear any garbage from previous runs
    snd_pcm_drop(h);      // Drop any pending frames from previous session
    snd_pcm_prepare(h);   // Prepare for new audio stream

    // DEBUG: Print initial parameter state
    fprintf(stderr,"\n=== DEBUG: Initial Parameter State ===\n");
    fprintf(stderr,"Master Vol: %d\n", params_get(P_MASTER_VOL));
    fprintf(stderr,"Filter Cutoff: %d\n", params_get(P_FILTER_CUTOFF));
    fprintf(stderr,"Filter Resonance: %d\n", params_get(P_FILTER_RESONANCE));
    fprintf(stderr,"Filter Mode: %d\n", params_get(P_FILTER_MODE));
    fprintf(stderr,"OSC1 Wave: %d\n", params_get(P_OSC1_WAVE));
    fprintf(stderr,"OSC2 Wave: %d\n", params_get(P_OSC2_WAVE));
    fprintf(stderr,"OSC Mix: %d\n", params_get(P_OSC_MIX));
    fprintf(stderr,"Tune: %d\n", params_get(P_TUNE));
    fprintf(stderr,"Drone Mode: %d\n", params_get(P_DRONE_MODE));
    fprintf(stderr,"LFO1 Depth: %d\n", params_get(P_LFO1_DEPTH));
    fprintf(stderr,"LFO2 Depth: %d\n", params_get(P_LFO2_DEPTH));
    fprintf(stderr,"Env Attack: %d, Decay: %d, Sustain: %d, Release: %d\n",
        params_get(P_ENV_ATTACK), params_get(P_ENV_DECAY),
        params_get(P_ENV_SUSTAIN), params_get(P_ENV_RELEASE));
    fprintf(stderr,"=====================================\n\n");

    fprintf(stderr,"Type 'HELP' for commands. Notes stay on until you turn them OFF!\n\n");

    int debug_frame_count = 0;
    while(run){
        rockit_engine_render(&e, buf, per, rate);

        // DEBUG: Print first few samples to see if audio is being generated
        if(debug_frame_count < 3) {
            int16_t max_sample = 0;
            for(size_t i = 0; i < per * 2; i++) {
                if(buf[i] > max_sample || buf[i] < -max_sample) {
                    max_sample = buf[i] > 0 ? buf[i] : -buf[i];
                }
            }
            fprintf(stderr,"DEBUG Frame %d: Max sample amplitude = %d (out of 32767)\n",
                    debug_frame_count, max_sample);
            debug_frame_count++;
        }

        snd_pcm_sframes_t w = snd_pcm_writei(h, buf, per);
        if(w<0) snd_pcm_prepare(h);
        
        // Check for CLI input
        handle_cli_input();
    }
    
    fprintf(stderr,"\nShutting down...\n");

    // Properly drain and stop ALSA to prevent state issues on next startup
    snd_pcm_drop(h);       // Drop pending frames immediately
    snd_pcm_drain(h);      // Wait for remaining data to be played
    snd_pcm_close(h);      // Close the device

    free(buf);
    return 0;
}