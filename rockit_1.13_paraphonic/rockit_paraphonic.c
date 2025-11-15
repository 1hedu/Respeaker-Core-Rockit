/*
 * Rockit Paraphonic Voice Allocation Module - Enhanced Version
 * Version 2.6 - November 2025
 * 
 * LATEST FIX (v2.6):
 * - Single note now plays on BOTH DCO1 & DCO2 (temporary mono)
 * - Prevents stuck notes when only one key is pressed
 * 
 * NEW FEATURES:
 * - Mono/Para toggle via CC 103 or button combo
 * - SELECT + SYNC buttons together cycle through modes
 * - All 3 para modes + mono mode
 * - LED feedback for current mode
 * - EF-101D enable/disable via CC 104
 * 
 * Modes:
 * - Monophonic (classic Rockit behavior)
 * - Low Note Priority
 * - Last Note Priority
 * - Round Robin
 *
 * Controls:
 * - CC 103: 0-63 = Mono, 64-127 = Paraphonic
 * - CC 102: Para mode select (0-42=Low, 43-84=Last, 85-127=RR)
 * - SELECT + SYNC buttons: Cycle modes (MonoÃ¢â€ â€™LowÃ¢â€ â€™LastÃ¢â€ â€™RRÃ¢â€ â€™Mono...)
 *
 * GPLv3 License - Compatible with Rockit 1.12
 */

#include "pgmspace.h"
#include "eight_bit_synth_main.h"
#include "midi.h"
#include "led_switch_handler.h"
#include "rockit_paraphonic.h"

// Voice allocation modes
/* VoiceMode enum defined in header; removed duplicate. */

// Voice assignment structure  
typedef struct {
    unsigned char note;
    unsigned char velocity;
    unsigned char active;
} Voice;

// Paraphonic state
typedef struct {
    Voice voices[3];                 // 0=OSC_1, 1=OSC_2, 2=EF-101D
    unsigned char note_stack[16];    // Stack of pressed notes
    unsigned char velocity_stack[16];
    unsigned char stack_size;
    VoiceMode mode;
    unsigned char rr_next_voice;     // Round robin pointer
    unsigned char last_ef101_note;   // Track for note-off
    unsigned char ef101d_enabled;    // NEW: Is EF-101D connected?
    unsigned char sub_osc_mode;      // NEW: OSC_2 plays one octave lower
} ParaphonicState;

static ParaphonicState para_state;

// Forward declarations
static void allocate_voices_monophonic(g_setting *p_global_setting);
static void allocate_low_note_priority(g_setting *p_global_setting);
static void allocate_high_note_priority(g_setting *p_global_setting);
static void allocate_last_note_priority(g_setting *p_global_setting);
static void allocate_round_robin(g_setting *p_global_setting);
static void clear_voice(unsigned char voice_idx, g_setting *p_global_setting);
static void update_mode_leds(void);
static unsigned char apply_subosc_offset(unsigned char note);

/*
 * Initialize paraphonic system
 * Starts in Round Robin paraphonic mode
 */
void paraphonic_init(void) {
    para_state.mode = MODE_MONOPHONIC;  // Default: classic Rockit monophonic mode
    para_state.stack_size = 0;
    para_state.rr_next_voice = 0;
    para_state.last_ef101_note = 0;
    para_state.ef101d_enabled = 0;  // Default: No EF-101D (2 voices)
    para_state.sub_osc_mode = 0;     // Default: Normal OSC_2 operation
    
    for (unsigned char i = 0; i < 3; i++) {
        para_state.voices[i].note = 0;
        para_state.voices[i].velocity = 0;
        para_state.voices[i].active = 0;
    }
    
    for (unsigned char i = 0; i < 16; i++) {
        para_state.note_stack[i] = 0;
        para_state.velocity_stack[i] = 0;
    }
    
    update_mode_leds();
}

/*
 * Handle incoming MIDI note on
 */
void paraphonic_note_on(unsigned char note, unsigned char velocity, g_setting *p_global_setting) {
    // Add to note stack if not full
    if (para_state.stack_size < 16) {
        para_state.note_stack[para_state.stack_size] = note;
        para_state.velocity_stack[para_state.stack_size] = velocity;
        para_state.stack_size++;
    }
    
    // Sync flags - behavior depends on mode
    if (para_state.mode == MODE_MONOPHONIC) {
        // MONO: Always trigger envelopes (classic Rockit behavior)
        g_uc_lfo_midi_sync_flag = 1;
        g_uc_adsr_midi_sync_flag = 1;
        g_uc_filter_envelope_sync_flag = 1;
        g_uc_oscillator_midi_sync_flag = 1;
    } else {
        // PARAPHONIC: Only trigger envelopes on FIRST note
        // Additional notes shouldn't retrigger shared filter/amp!
        if (para_state.stack_size == 1) {
            // This is the first note - trigger everything
            g_uc_lfo_midi_sync_flag = 1;
            g_uc_adsr_midi_sync_flag = 1;
            g_uc_filter_envelope_sync_flag = 1;
            g_uc_oscillator_midi_sync_flag = 1;
        }
        // If stack_size > 1, we're adding notes to an existing chord
        // Don't retrigger envelopes!
    }
    
    // Allocate voices based on current mode
    paraphonic_allocate_voices(p_global_setting);
}

/*
 * Handle incoming MIDI note off
 */
void paraphonic_note_off(unsigned char note, g_setting *p_global_setting) {
    // Remove from note stack
    for (unsigned char i = 0; i < para_state.stack_size; i++) {
        if (para_state.note_stack[i] == note) {
            // Shift stack down
            for (unsigned char j = i; j < para_state.stack_size - 1; j++) {
                para_state.note_stack[j] = para_state.note_stack[j + 1];
                para_state.velocity_stack[j] = para_state.velocity_stack[j + 1];
            }
            para_state.stack_size--;
            break;
        }
    }
    
    // Reallocate voices based on remaining notes
    paraphonic_allocate_voices(p_global_setting);
    
    // Turn off key press flag if no notes remain
    if (para_state.stack_size == 0) {
        g_uc_key_press_flag = 0;
    }
}

/*
 * Set voice allocation mode
 */
void paraphonic_set_mode(VoiceMode mode, g_setting *p_global_setting) {
    para_state.mode = mode;
    paraphonic_allocate_voices(p_global_setting);
    update_mode_leds();
}

/*
 * Get current mode (for display/debugging)
 */
VoiceMode paraphonic_get_mode(void) {
    return para_state.mode;
}

/* Get sub-osc mode state */
unsigned char paraphonic_get_subosc_mode(void) {
    return para_state.sub_osc_mode;
}

/*
 * Cycle to next mode (for button control)
 * Order: Mono Ã¢â€ â€™ Low Ã¢â€ â€™ Last Ã¢â€ â€™ RR Ã¢â€ â€™ High Ã¢â€ â€™ Mono...
 */
void paraphonic_cycle_mode(g_setting *p_global_setting) {
    para_state.mode = (para_state.mode + 1) % 5;
    paraphonic_allocate_voices(p_global_setting);
    update_mode_leds();
}

/*
 * Core voice allocation logic
 */
void paraphonic_allocate_voices(g_setting *p_global_setting) {
    // Clear all voices first
    for (unsigned char i = 0; i < 3; i++) {
        clear_voice(i, p_global_setting);
    }
    
    // If no notes, we're done
    if (para_state.stack_size == 0) {
        return;
    }
    
    switch (para_state.mode) {
        case MODE_MONOPHONIC:
            allocate_voices_monophonic(p_global_setting);
            break;
            
        case MODE_LOW_NOTE:
            allocate_low_note_priority(p_global_setting);
            break;
            
        case MODE_LAST_NOTE:
            allocate_last_note_priority(p_global_setting);
            break;
            
        case MODE_ROUND_ROBIN:
            allocate_round_robin(p_global_setting);
            break;
            
        case MODE_HIGH_NOTE:
            allocate_high_note_priority(p_global_setting);
            break;
    }
    
    // Make sure note on flag is set
    if (para_state.stack_size > 0) {
        g_uc_key_press_flag = 1;
    }
}

/*
 * Monophonic Mode: Classic Rockit behavior
 * Most recent note plays on both oscillators (detuned unison or sub-osc)
 */
static void allocate_voices_monophonic(g_setting *p_global_setting) {
    // Just play most recent note on OSC_1 (OSC_2 follows via detune or sub-osc)
    unsigned char note = para_state.note_stack[para_state.stack_size - 1];
    unsigned char velocity = para_state.velocity_stack[para_state.stack_size - 1];
    
    para_state.voices[0].note = note;
    para_state.voices[0].velocity = velocity;
    para_state.voices[0].active = 1;
    
    p_global_setting->auc_midi_note_index[OSC_1] = note;
    p_global_setting->uc_note_velocity = velocity;
    
    // OSC_2 will follow OSC_1 via normal Rockit detuning in calculate_pitch.c
    // OR if sub-osc mode is enabled, we set it one octave down here
    if (para_state.sub_osc_mode) {
        // Sub-osc mode: Set OSC_2 directly to one octave below
        p_global_setting->auc_midi_note_index[OSC_2] = apply_subosc_offset(note);
    }
    // If sub-osc is off, calculate_pitch.c handles the detune
    
    // Voice 3 (EF-101D) stays silent in mono mode
}

/*
 * Low Note Priority: Always play the 3 lowest notes
 */
static void allocate_low_note_priority(g_setting *p_global_setting) {
    // Sort notes (bubble sort - simple for small arrays)
    unsigned char sorted_notes[16];
    unsigned char sorted_vel[16];
    
    for (unsigned char i = 0; i < para_state.stack_size; i++) {
        sorted_notes[i] = para_state.note_stack[i];
        sorted_vel[i] = para_state.velocity_stack[i];
    }
    
    for (unsigned char i = 0; i < para_state.stack_size - 1; i++) {
        for (unsigned char j = 0; j < para_state.stack_size - i - 1; j++) {
            if (sorted_notes[j] > sorted_notes[j + 1]) {
                unsigned char temp_note = sorted_notes[j];
                unsigned char temp_vel = sorted_vel[j];
                sorted_notes[j] = sorted_notes[j + 1];
                sorted_vel[j] = sorted_vel[j + 1];
                sorted_notes[j + 1] = temp_note;
                sorted_vel[j + 1] = temp_vel;
            }
        }
    }
    
    // Special case: single note plays on both DCO1 and DCO2 (temporary mono)
    if (para_state.stack_size == 1) {
        unsigned char note = sorted_notes[0];
        unsigned char velocity = sorted_vel[0];
        
        // Both internal oscillators play the same note
        para_state.voices[0].note = note;
        para_state.voices[0].velocity = velocity;
        para_state.voices[0].active = 1;
        para_state.voices[1].note = note;
        para_state.voices[1].velocity = velocity;
        para_state.voices[1].active = 1;
        
        p_global_setting->auc_midi_note_index[OSC_1] = note;
        p_global_setting->auc_midi_note_index[OSC_2] = apply_subosc_offset(note);
        p_global_setting->uc_note_velocity = velocity;
        
        // EF-101D silent in single note mode
        if (para_state.ef101d_enabled && para_state.last_ef101_note != 0xFF) {
            put_midi_message_in_outgoing_fifo(MESSAGE_TYPE_NOTE_OFF, para_state.last_ef101_note, 0);
            para_state.last_ef101_note = 0xFF;
        }
    } else {
        // Normal paraphonic operation
        unsigned char max_voices = para_state.ef101d_enabled ? 3 : 2;
        unsigned char voices_to_assign = (para_state.stack_size < max_voices) ? para_state.stack_size : max_voices;
        
        for (unsigned char i = 0; i < voices_to_assign; i++) {
            para_state.voices[i].note = sorted_notes[i];
            para_state.voices[i].velocity = sorted_vel[i];
            para_state.voices[i].active = 1;
            
            // Route to hardware
            if (i == 0) {
                p_global_setting->auc_midi_note_index[OSC_1] = sorted_notes[i];
                p_global_setting->uc_note_velocity = sorted_vel[i];
            } else if (i == 1) {
                p_global_setting->auc_midi_note_index[OSC_2] = apply_subosc_offset(sorted_notes[i]);
            } else if (i == 2 && para_state.ef101d_enabled) {
                put_midi_message_in_outgoing_fifo(MESSAGE_TYPE_NOTE_ON, sorted_notes[i], sorted_vel[i]);
                para_state.last_ef101_note = sorted_notes[i];
            }
        }
    }
}

/*
 * High Note Priority: Always play the 2 highest notes
 */
static void allocate_high_note_priority(g_setting *p_global_setting) {
    // Sort notes (bubble sort - simple for small arrays)
    unsigned char sorted_notes[16];
    unsigned char sorted_vel[16];
    
    for (unsigned char i = 0; i < para_state.stack_size; i++) {
        sorted_notes[i] = para_state.note_stack[i];
        sorted_vel[i] = para_state.velocity_stack[i];
    }
    
    // Sort in descending order (highest first)
    for (unsigned char i = 0; i < para_state.stack_size - 1; i++) {
        for (unsigned char j = 0; j < para_state.stack_size - i - 1; j++) {
            if (sorted_notes[j] < sorted_notes[j + 1]) {  // Note: < for descending
                unsigned char temp_note = sorted_notes[j];
                unsigned char temp_vel = sorted_vel[j];
                sorted_notes[j] = sorted_notes[j + 1];
                sorted_vel[j] = sorted_vel[j + 1];
                sorted_notes[j + 1] = temp_note;
                sorted_vel[j + 1] = temp_vel;
            }
        }
    }
    
    // Special case: single note plays on both DCO1 and DCO2 (temporary mono)
    if (para_state.stack_size == 1) {
        unsigned char note = sorted_notes[0];
        unsigned char velocity = sorted_vel[0];
        
        // Both internal oscillators play the same note
        para_state.voices[0].note = note;
        para_state.voices[0].velocity = velocity;
        para_state.voices[0].active = 1;
        para_state.voices[1].note = note;
        para_state.voices[1].velocity = velocity;
        para_state.voices[1].active = 1;
        
        p_global_setting->auc_midi_note_index[OSC_1] = note;
        p_global_setting->auc_midi_note_index[OSC_2] = apply_subosc_offset(note);
        p_global_setting->uc_note_velocity = velocity;
        
        // EF-101D silent in single note mode
        if (para_state.ef101d_enabled && para_state.last_ef101_note != 0xFF) {
            put_midi_message_in_outgoing_fifo(MESSAGE_TYPE_NOTE_OFF, para_state.last_ef101_note, 0);
            para_state.last_ef101_note = 0xFF;
        }
    } else {
        // Normal paraphonic operation
        unsigned char max_voices = para_state.ef101d_enabled ? 3 : 2;
        unsigned char voices_to_assign = (para_state.stack_size < max_voices) ? para_state.stack_size : max_voices;
        
        for (unsigned char i = 0; i < voices_to_assign; i++) {
            para_state.voices[i].note = sorted_notes[i];
            para_state.voices[i].velocity = sorted_vel[i];
            para_state.voices[i].active = 1;
            
            // Route to hardware
            if (i == 0) {
                p_global_setting->auc_midi_note_index[OSC_1] = sorted_notes[i];
                p_global_setting->uc_note_velocity = sorted_vel[i];
            } else if (i == 1) {
                p_global_setting->auc_midi_note_index[OSC_2] = apply_subosc_offset(sorted_notes[i]);
            } else if (i == 2 && para_state.ef101d_enabled) {
                put_midi_message_in_outgoing_fifo(MESSAGE_TYPE_NOTE_ON, sorted_notes[i], sorted_vel[i]);
                para_state.last_ef101_note = sorted_notes[i];
            }
        }
    }
}

/*
 * Last Note Priority: Most recent notes get voices
 */
static void allocate_last_note_priority(g_setting *p_global_setting) {
    // Special case: single note plays on both DCO1 and DCO2 (temporary mono)
    if (para_state.stack_size == 1) {
        unsigned char note = para_state.note_stack[0];
        unsigned char velocity = para_state.velocity_stack[0];
        
        // Both internal oscillators play the same note
        para_state.voices[0].note = note;
        para_state.voices[0].velocity = velocity;
        para_state.voices[0].active = 1;
        para_state.voices[1].note = note;
        para_state.voices[1].velocity = velocity;
        para_state.voices[1].active = 1;
        
        p_global_setting->auc_midi_note_index[OSC_1] = note;
        p_global_setting->auc_midi_note_index[OSC_2] = apply_subosc_offset(note);
        p_global_setting->uc_note_velocity = velocity;
        
        // EF-101D silent in single note mode
        if (para_state.ef101d_enabled && para_state.last_ef101_note != 0xFF) {
            put_midi_message_in_outgoing_fifo(MESSAGE_TYPE_NOTE_OFF, para_state.last_ef101_note, 0);
            para_state.last_ef101_note = 0xFF;
        }
    } else {
        // Normal paraphonic operation
        unsigned char max_voices = para_state.ef101d_enabled ? 3 : 2;
        unsigned char voices_to_assign = (para_state.stack_size < max_voices) ? para_state.stack_size : max_voices;
        
        // Assign most recent notes (from end of stack)
        for (unsigned char i = 0; i < voices_to_assign; i++) {
            unsigned char stack_idx = para_state.stack_size - 1 - i;
            para_state.voices[i].note = para_state.note_stack[stack_idx];
            para_state.voices[i].velocity = para_state.velocity_stack[stack_idx];
            para_state.voices[i].active = 1;
            
            // Route to hardware
            if (i == 0) {
                p_global_setting->auc_midi_note_index[OSC_1] = para_state.note_stack[stack_idx];
                p_global_setting->uc_note_velocity = para_state.velocity_stack[stack_idx];
            } else if (i == 1) {
                p_global_setting->auc_midi_note_index[OSC_2] = apply_subosc_offset(para_state.note_stack[stack_idx]);
            } else if (i == 2 && para_state.ef101d_enabled) {
                put_midi_message_in_outgoing_fifo(MESSAGE_TYPE_NOTE_ON, 
                                                 para_state.note_stack[stack_idx], 
                                                 para_state.velocity_stack[stack_idx]);
                para_state.last_ef101_note = para_state.note_stack[stack_idx];
            }
        }
    }
}

/*
 * Round Robin: Cycle through voices as notes arrive
 */
static void allocate_round_robin(g_setting *p_global_setting) {
    if (para_state.stack_size > 0) {
        unsigned char note = para_state.note_stack[para_state.stack_size - 1];
        unsigned char velocity = para_state.velocity_stack[para_state.stack_size - 1];
        
        // Special case: single note plays on both DCO1 and DCO2 (temporary mono)
        if (para_state.stack_size == 1) {
            // Both internal oscillators play the same note
            para_state.voices[0].note = note;
            para_state.voices[0].velocity = velocity;
            para_state.voices[0].active = 1;
            para_state.voices[1].note = note;
            para_state.voices[1].velocity = velocity;
            para_state.voices[1].active = 1;
            
            p_global_setting->auc_midi_note_index[OSC_1] = note;
            p_global_setting->auc_midi_note_index[OSC_2] = apply_subosc_offset(note);
            p_global_setting->uc_note_velocity = velocity;
            
            // Reset round robin counter for next time
            para_state.rr_next_voice = 0;
            
            // EF-101D silent in single note mode
            if (para_state.ef101d_enabled && para_state.last_ef101_note != 0xFF) {
                put_midi_message_in_outgoing_fifo(MESSAGE_TYPE_NOTE_OFF, para_state.last_ef101_note, 0);
                para_state.last_ef101_note = 0xFF;
            }
            return;
        }
        
        // Normal paraphonic operation
        unsigned char max_voices = para_state.ef101d_enabled ? 3 : 2;
        
        // Assign to next voice in rotation
        unsigned char voice_idx = para_state.rr_next_voice % max_voices;
        para_state.voices[voice_idx].note = note;
        para_state.voices[voice_idx].velocity = velocity;
        para_state.voices[voice_idx].active = 1;
        
        // Route to hardware
        if (voice_idx == 0) {
            p_global_setting->auc_midi_note_index[OSC_1] = note;
            p_global_setting->uc_note_velocity = velocity;
        } else if (voice_idx == 1) {
            p_global_setting->auc_midi_note_index[OSC_2] = apply_subosc_offset(note);
        } else if (voice_idx == 2 && para_state.ef101d_enabled) {
            put_midi_message_in_outgoing_fifo(MESSAGE_TYPE_NOTE_ON, note, velocity);
            para_state.last_ef101_note = note;
        }
        
        // Advance round robin pointer
        para_state.rr_next_voice = (para_state.rr_next_voice + 1) % max_voices;
        
        // Also assign other held notes to remaining voices
        if (para_state.stack_size >= 2) {
            unsigned char assigned = 1;
            for (signed char i = para_state.stack_size - 2; i >= 0 && assigned < max_voices; i--) {
                for (unsigned char v = 0; v < max_voices && assigned < max_voices; v++) {
                    if (!para_state.voices[v].active) {
                        para_state.voices[v].note = para_state.note_stack[i];
                        para_state.voices[v].velocity = para_state.velocity_stack[i];
                        para_state.voices[v].active = 1;
                        
                        if (v == 0) {
                            p_global_setting->auc_midi_note_index[OSC_1] = para_state.note_stack[i];
                            p_global_setting->uc_note_velocity = para_state.velocity_stack[i];
                        } else if (v == 1) {
                            p_global_setting->auc_midi_note_index[OSC_2] = apply_subosc_offset(para_state.note_stack[i]);
                        } else if (v == 2 && para_state.ef101d_enabled) {
                            put_midi_message_in_outgoing_fifo(MESSAGE_TYPE_NOTE_ON, 
                                                             para_state.note_stack[i], 
                                                             para_state.velocity_stack[i]);
                            para_state.last_ef101_note = para_state.note_stack[i];
                        }
                        assigned++;
                        break;
                    }
                }
            }
        }
    }
}

/*
 * Clear a voice (turn off oscillator)
 */
static void clear_voice(unsigned char voice_idx, g_setting *p_global_setting) {
    para_state.voices[voice_idx].active = 0;
    para_state.voices[voice_idx].note = 0;
    para_state.voices[voice_idx].velocity = 0;
    
    // DON'T change the oscillator note here!
    // In monophonic mode, we want the ADSR release to play out naturally
    // The oscillator should keep playing the last note while envelope releases
    // Only for EF-101D external voice do we send explicit note-off
    if (voice_idx == 2 && para_state.last_ef101_note != 0) {
        // For EF-101D, send note-off
        put_midi_message_in_outgoing_fifo(MESSAGE_TYPE_NOTE_OFF, para_state.last_ef101_note, 0);
    }
}

/*
 * Update 7-segment display to show current mode
 * Briefly shows mode number, then returns to patch number
 * 0 = Monophonic, 1 = Low Note, 2 = lAst, 3 = Round Robin
 */
static void update_mode_leds(void) {
    // Hold display for ~2 seconds (timer counts down in slow interrupt ~5Hz = 10 ticks)
    paraphonic_hold_mode_display(para_state.mode, 10);
}

/*
 * MIDI CC handlers
 */

// CC 102: Mono/Juno Toggle (Sub-Oscillator)
// < 64 = Mono (normal detune), >= 64 = Juno (sub-osc one octave down)
void paraphonic_handle_mode_cc(unsigned char value, g_setting *p_global_setting) {
    if (value < 64) {
        para_state.sub_osc_mode = 0;  // Normal mono with detune
    } else {
        para_state.sub_osc_mode = 1;  // Juno sub-osc mode
        // Force detune to center (0 offset) so it doesn't interfere with sub-osc
        // OSC_DETUNE center is 128 (0x80) - no offset
        p_global_setting->auc_synth_params[OSC_DETUNE] = 128;
    }
    // Reallocate if currently in mono mode to apply change
    if (para_state.mode == MODE_MONOPHONIC) {
        paraphonic_allocate_voices(p_global_setting);
    }
}

// CC 103: Enable/Disable Paraphonic
// < 64 = Monophonic (classic Rockit)
// >= 64 = Paraphonic (use mode from CC 104)
void paraphonic_handle_enable_cc(unsigned char value, g_setting *p_global_setting) {
    if (value < 64) {
        // Monophonic mode
        para_state.mode = MODE_MONOPHONIC;
    } else {
        // Paraphonic mode - default to Round Robin if not set
        if (para_state.mode == MODE_MONOPHONIC) {
            para_state.mode = MODE_ROUND_ROBIN;
        }
    }
    paraphonic_allocate_voices(p_global_setting);
    update_mode_leds();
}

// CC 104: Paraphonic Mode Selection (when paraphonic enabled)
void paraphonic_handle_paraphonic_mode_cc(unsigned char value, g_setting *p_global_setting) {
    // Only works when in paraphonic mode (not mono)
    if (para_state.mode == MODE_MONOPHONIC) {
        return;
    }
    
    // 4 paraphonic modes
    if (value < 43) {
        paraphonic_set_mode(MODE_LOW_NOTE, p_global_setting);
    } else if (value < 85) {
        paraphonic_set_mode(MODE_LAST_NOTE, p_global_setting);
    } else if (value < 107) {
        paraphonic_set_mode(MODE_ROUND_ROBIN, p_global_setting);
    } else {
        paraphonic_set_mode(MODE_HIGH_NOTE, p_global_setting);
    }
}

// CC 105: Toggle EF-101D (third voice)
void paraphonic_handle_ef101d_cc(unsigned char value, g_setting *p_global_setting) {
    if (value < 64) {
        para_state.ef101d_enabled = 0;  // 2 voices (no EF-101D)
    } else {
        para_state.ef101d_enabled = 1;  // 3 voices (with EF-101D)
    }
    // Reallocate voices with new voice count
    paraphonic_allocate_voices(p_global_setting);
}

// REMOVED: Old duplicate sub-osc handler - now integrated into CC 102

// Helper: Apply sub-osc offset if enabled
static unsigned char apply_subosc_offset(unsigned char note) {
    if (para_state.sub_osc_mode && note >= 12) {
        return note - 12;  // One octave down
    }
    return note;
}
