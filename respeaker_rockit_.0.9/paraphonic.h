/*
 * Rockit Paraphonic Voice Allocation Module - ReSpeaker Port
// ... license text ...
 */

#pragma once
#include <stdint.h>
#include <string.h>

// Voice allocation modes
typedef enum {
    MODE_MONOPHONIC = 0,
    MODE_LOW_NOTE,
    MODE_LAST_NOTE,
    MODE_ROUND_ROBIN,
    MODE_HIGH_NOTE,
    MODE_COUNT
} voice_mode_t;

// Voice structure
typedef struct {
    uint8_t note;
    uint8_t velocity;
    uint8_t active;
} voice_t;

// Paraphonic state
typedef struct {
    voice_t voices[3];              // 3 voices
    uint8_t note_stack[16];         // Stack of pressed notes
    uint8_t velocity_stack[16];     // Corresponding velocities
    uint8_t stack_size;
    voice_mode_t mode;
    uint8_t rr_next_voice;          // Round robin pointer
    uint8_t three_voice_mode;       // 0=2 voices, 1=3 voices
} paraphonic_state_t;

static paraphonic_state_t para_state;

// Forward declarations
static void allocate_voices_monophonic(void);
static void allocate_low_note_priority(void);
static void allocate_high_note_priority(void);
static void allocate_last_note_priority(void);
static void allocate_round_robin(void);
static void allocate_voices(void);

/*
 * Initialize paraphonic system
 */
void paraphonic_init(void) {
    memset(&para_state, 0, sizeof(para_state));
    para_state.mode = MODE_LAST_NOTE;  // Last Note is usually a better default than Round Robin
    para_state.three_voice_mode = 1;     // Default: 3 voices enabled
}

/*
 * Get current voice assignments (for synth engine)
 */
void paraphonic_get_voices(voice_t* out_voices) {
    memcpy(out_voices, para_state.voices, sizeof(para_state.voices));
}

/*
 * Get voice mode
 */
voice_mode_t paraphonic_get_mode(void) {
    return para_state.mode;
}

/*
 * Set voice mode
 */
void paraphonic_set_mode(voice_mode_t mode) {
    if (mode < MODE_COUNT) {
        para_state.mode = mode;
        allocate_voices();
    }
}

/*
 * Set 3-voice mode
 */
void paraphonic_set_three_voice_mode(uint8_t enabled) {
    para_state.three_voice_mode = enabled ? 1 : 0;
    allocate_voices();
}

/*
 * Handle incoming MIDI note on
 */
void paraphonic_note_on(uint8_t note, uint8_t velocity) {
    // Check if already in stack (for retriggering)
    for (uint8_t i = 0; i < para_state.stack_size; i++) {
        if (para_state.note_stack[i] == note) {
            // Already present, do nothing for now, or move to end for Last Note Priority
            // Keeping it simple: just return
            return;
        }
    }
    
    // Add to note stack if not full
    if (para_state.stack_size < 16) {
        para_state.note_stack[para_state.stack_size] = note;
        para_state.velocity_stack[para_state.stack_size] = velocity;
        para_state.stack_size++;
    }
    
    allocate_voices();
}

/*
 * Handle incoming MIDI note off
 */
void paraphonic_note_off(uint8_t note) {
    // Remove from note stack
    for (uint8_t i = 0; i < para_state.stack_size; i++) {
        if (para_state.note_stack[i] == note) {
            // Shift stack down
            for (uint8_t j = i; j < para_state.stack_size - 1; j++) {
                para_state.note_stack[j] = para_state.note_stack[j + 1];
                para_state.velocity_stack[j] = para_state.velocity_stack[j + 1];
            }
            para_state.stack_size--;
            break;
        }
    }
    
    allocate_voices();
}

/*
 * Core voice allocation logic
 */
static void allocate_voices(void) {
    // Clear all voices first
    for (uint8_t i = 0; i < 3; i++) {
        para_state.voices[i].active = 0;
    }
    
    // If no notes, we're done
    if (para_state.stack_size == 0) {
        return;
    }
    
    switch (para_state.mode) {
        case MODE_MONOPHONIC:
            allocate_voices_monophonic();
            break;
        case MODE_LOW_NOTE:
            allocate_low_note_priority();
            break;
        case MODE_LAST_NOTE:
            allocate_last_note_priority();
            break;
        case MODE_ROUND_ROBIN:
            allocate_round_robin();
            break;
        case MODE_HIGH_NOTE:
            allocate_high_note_priority();
            break;
        default:
            break;
    }
}

/*
 * Monophonic Mode: Most recent note plays
 */
static void allocate_voices_monophonic(void) {
    uint8_t note = para_state.note_stack[para_state.stack_size - 1];
    uint8_t velocity = para_state.velocity_stack[para_state.stack_size - 1];
    
    para_state.voices[0].note = note;
    para_state.voices[0].velocity = velocity;
    para_state.voices[0].active = 1;
}

/*
 * Low Note Priority: Always play the N lowest notes
 */
static void allocate_low_note_priority(void) {
    // Sort notes (bubble sort)
    uint8_t sorted_notes[16];
    uint8_t sorted_vel[16];
    
    memcpy(sorted_notes, para_state.note_stack, para_state.stack_size);
    memcpy(sorted_vel, para_state.velocity_stack, para_state.stack_size);
    
    for (uint8_t i = 0; i < para_state.stack_size - 1; i++) {
        for (uint8_t j = 0; j < para_state.stack_size - i - 1; j++) {
            if (sorted_notes[j] > sorted_notes[j + 1]) {
                uint8_t temp = sorted_notes[j];
                sorted_notes[j] = sorted_notes[j + 1];
                sorted_notes[j + 1] = temp;
                
                temp = sorted_vel[j];
                sorted_vel[j] = sorted_vel[j + 1];
                sorted_vel[j + 1] = temp;
            }
        }
    }
    
    // Assign to voices
    uint8_t max_voices = para_state.three_voice_mode ? 3 : 2;
    uint8_t voices_to_assign = (para_state.stack_size < max_voices) ? 
                                para_state.stack_size : max_voices;
    
    for (uint8_t i = 0; i < voices_to_assign; i++) {
        para_state.voices[i].note = sorted_notes[i];
        para_state.voices[i].velocity = sorted_vel[i];
        para_state.voices[i].active = 1;
    }
}

/*
 * High Note Priority: Always play the N highest notes
 */
static void allocate_high_note_priority(void) {
    // Sort notes in descending order
    uint8_t sorted_notes[16];
    uint8_t sorted_vel[16];
    
    memcpy(sorted_notes, para_state.note_stack, para_state.stack_size);
    memcpy(sorted_vel, para_state.velocity_stack, para_state.stack_size);
    
    for (uint8_t i = 0; i < para_state.stack_size - 1; i++) {
        for (uint8_t j = 0; j < para_state.stack_size - i - 1; j++) {
            if (sorted_notes[j] < sorted_notes[j + 1]) {  // Descending
                uint8_t temp = sorted_notes[j];
                sorted_notes[j] = sorted_notes[j + 1];
                sorted_notes[j + 1] = temp;
                
                temp = sorted_vel[j];
                sorted_vel[j] = sorted_vel[j + 1];
                sorted_vel[j + 1] = temp;
            }
        }
    }
    
    // Assign to voices
    uint8_t max_voices = para_state.three_voice_mode ? 3 : 2;
    uint8_t voices_to_assign = (para_state.stack_size < max_voices) ? 
                                para_state.stack_size : max_voices;
    
    for (uint8_t i = 0; i < voices_to_assign; i++) {
        para_state.voices[i].note = sorted_notes[i];
        para_state.voices[i].velocity = sorted_vel[i];
        para_state.voices[i].active = 1;
    }
}

/*
 * Last Note Priority: Most recent notes get voices
 */
static void allocate_last_note_priority(void) {
    uint8_t max_voices = para_state.three_voice_mode ? 3 : 2;
    uint8_t voices_to_assign = (para_state.stack_size < max_voices) ? 
                                para_state.stack_size : max_voices;
    
    // Assign most recent notes (from end of stack)
    for (uint8_t i = 0; i < voices_to_assign; i++) {
        uint8_t stack_idx = para_state.stack_size - 1 - i;
        para_state.voices[i].note = para_state.note_stack[stack_idx];
        para_state.voices[i].velocity = para_state.velocity_stack[stack_idx];
        para_state.voices[i].active = 1;
    }
}

/*
 * Round Robin: Cycle through voices as notes arrive
 * FIX: Simplifed allocation logic to directly assign from the note stack
 */
static void allocate_round_robin(void) {
    uint8_t max_voices = para_state.three_voice_mode ? 3 : 2;
    uint8_t voices_to_assign = (para_state.stack_size < max_voices) ? 
                                para_state.stack_size : max_voices;
    
    if (para_state.stack_size == 0) return;

    // Use a temporary array to track which notes from the stack are currently assigned
    uint8_t assigned_notes[3] = {0};
    uint8_t assigned_count = 0;

    // Preserve playing voices if the playing note is still in the stack
    for(uint8_t i = 0; i < max_voices; ++i) {
        uint8_t current_note = para_state.voices[i].note;
        if(para_state.voices[i].active) {
            // Check if this note is still held
            for(uint8_t k = 0; k < para_state.stack_size; ++k) {
                if(para_state.note_stack[k] == current_note) {
                    assigned_notes[i] = current_note;
                    assigned_count++;
                    break;
                }
            }
        }
    }

    // Assign *newest* notes to available voices using round robin logic
    for(uint8_t i = 0; i < voices_to_assign; ++i) {
        uint8_t stack_idx = para_state.stack_size - 1 - i;
        uint8_t note = para_state.note_stack[stack_idx];
        uint8_t velocity = para_state.velocity_stack[stack_idx];
        
        // Check if this note is already assigned (in assigned_notes array)
        uint8_t already_assigned = 0;
        for(uint8_t j = 0; j < max_voices; ++j) {
            if(assigned_notes[j] == note) {
                already_assigned = 1;
                break;
            }
        }
        
        if (!already_assigned) {
            // Find a voice to steal using the round robin pointer
            uint8_t voice_to_steal = para_state.rr_next_voice % max_voices;
            
            para_state.voices[voice_to_steal].note = note;
            para_state.voices[voice_to_steal].velocity = velocity;
            para_state.voices[voice_to_steal].active = 1;
            assigned_notes[voice_to_steal] = note; // Update temporary array

            para_state.rr_next_voice = (para_state.rr_next_voice + 1) % max_voices;
        }
    }
    
    // Final assignment check (ensure active flag is set only for assigned notes)
    for(uint8_t i = 0; i < max_voices; ++i) {
        uint8_t note = para_state.voices[i].note;
        uint8_t is_held = 0;
        for(uint8_t k = 0; k < para_state.stack_size; ++k) {
            if(para_state.note_stack[k] == note) {
                is_held = 1;
                break;
            }
        }
        para_state.voices[i].active = is_held;
    }
}


/*
 * Handle MIDI CC for mode switching
 */
void paraphonic_handle_cc(uint8_t cc, uint8_t value) {
    switch (cc) {
        case 102:  // Mono/Para (0-63=Mono, 64-127=Para)
            if (value < 64) {
                paraphonic_set_mode(MODE_MONOPHONIC);
            } else if (para_state.mode == MODE_MONOPHONIC) {
                paraphonic_set_mode(MODE_LAST_NOTE);
            }
            break;
            
        case 103:  // 3-voice enable
            paraphonic_set_three_voice_mode(value >= 64);
            break;
            
        case 104:  // Cycle para modes
            if (para_state.mode != MODE_MONOPHONIC) {
                voice_mode_t next = para_state.mode + 1;
                if (next == MODE_MONOPHONIC || next >= MODE_COUNT) next = MODE_LOW_NOTE;
                paraphonic_set_mode(next);
            } else {
                paraphonic_set_mode(MODE_LOW_NOTE);
            }
            break;
            
        case 105:  // 3-voice toggle
            paraphonic_set_three_voice_mode(value >= 64);
            break;
    }
}

/*
 * Get mode name (for debugging/display)
 */
const char* paraphonic_get_mode_name(void) {
    switch (para_state.mode) {
        case MODE_MONOPHONIC:  return "Mono";
        case MODE_LOW_NOTE:    return "Low Note";
        case MODE_LAST_NOTE:   return "Last Note";
        case MODE_ROUND_ROBIN: return "Round Robin";
        case MODE_HIGH_NOTE:   return "High Note";
        default:               return "Unknown";
    }
}