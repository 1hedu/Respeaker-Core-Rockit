/*
 * Rockit Paraphonic Voice Allocation Module - Enhanced Header
 * Version 2.0
 */

#ifndef ROCKIT_PARAPHONIC_H
#define ROCKIT_PARAPHONIC_H

#include "eight_bit_synth_main.h"

// Voice allocation modes (external use)
typedef enum {
    MODE_MONOPHONIC = 0,
    MODE_LOW_NOTE = 1,
    MODE_LAST_NOTE = 2,
    MODE_ROUND_ROBIN = 3,
    MODE_HIGH_NOTE = 4
} VoiceMode;

// Public API functions
void paraphonic_init(void);
void paraphonic_note_on(unsigned char note, unsigned char velocity, g_setting *p_global_setting);
void paraphonic_note_off(unsigned char note, g_setting *p_global_setting);
void paraphonic_set_mode(VoiceMode mode, g_setting *p_global_setting);
void paraphonic_allocate_voices(g_setting *p_global_setting);
void paraphonic_cycle_mode(g_setting *p_global_setting);  // NEW: For button control
VoiceMode paraphonic_get_mode(void);  // NEW: Query current mode
unsigned char paraphonic_get_subosc_mode(void);  // NEW: Query sub-osc state

// MIDI CC handlers
void paraphonic_handle_mode_cc(unsigned char value, g_setting *p_global_setting);           // CC 102: Mono/Juno toggle
void paraphonic_handle_enable_cc(unsigned char value, g_setting *p_global_setting);         // CC 103: Paraphonic on/off
void paraphonic_handle_paraphonic_mode_cc(unsigned char value, g_setting *p_global_setting); // CC 104: Paraphonic mode selection
void paraphonic_handle_ef101d_cc(unsigned char value, g_setting *p_global_setting);          // CC 105: EF-101D enable

#endif // ROCKIT_PARAPHONIC_H
