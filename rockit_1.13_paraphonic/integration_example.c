/*
 * CONCRETE INTEGRATION EXAMPLE
 * 
 * This shows exactly what to add to your Rockit code.
 * Based on Rockit 1.12 architecture.
 */

/******************************************************************************
 * STEP 1: Add to beginning of eight_bit_synth_main.c
 ******************************************************************************/

#include "rockit_paraphonic.h"

// Track last note sent to EF-101D for note-off messages
static uint8_t last_ef101_note = 0;

/******************************************************************************
 * STEP 2: Implement hardware interface functions
 * Add these somewhere in eight_bit_synth_main.c
 ******************************************************************************/

/*
 * Set DCO1 to play a specific MIDI note
 */
void set_dco1_note(uint8_t note, uint8_t velocity) {
    if (note == 0 || velocity == 0) {
        // Silence DCO1 by setting amplitude to 0
        // (Rockit likely has a variable like osc1_amp or similar)
        osc1_amplitude = 0;
        return;
    }
    
    // Rockit uses wavetable synthesis with phase accumulators
    // The frequency is controlled by phase_increment
    
    // Convert MIDI note to frequency (A4 = 440Hz is MIDI note 69)
    // freq = 440 * 2^((note - 69) / 12)
    
    // Use Rockit's existing note table if available, or calculate:
    // Assuming Rockit has a frequency lookup table called 'note_freq_table'
    uint16_t frequency = note_freq_table[note];
    
    // Set the phase increment for wavetable playback
    // Rockit probably has something like:
    osc1_phase_inc = (frequency * OSC_PHASE_MULTIPLIER) / SAMPLE_RATE;
    
    // Set amplitude based on velocity (scale 0-127 to your amp range)
    osc1_amplitude = (velocity * MAX_OSC_AMPLITUDE) / 127;
}

/*
 * Set DCO2 to play a specific MIDI note  
 */
void set_dco2_note(uint8_t note, uint8_t velocity) {
    if (note == 0 || velocity == 0) {
        osc2_amplitude = 0;
        return;
    }
    
    uint16_t frequency = note_freq_table[note];
    osc2_phase_inc = (frequency * OSC_PHASE_MULTIPLIER) / SAMPLE_RATE;
    osc2_amplitude = (velocity * MAX_OSC_AMPLITUDE) / 127;
}

/*
 * Send MIDI note to EF-101D via Respeaker
 */
void send_midi_note_out(uint8_t note, uint8_t velocity) {
    // Rockit 1.12 has MIDI output functions
    // Something like midi_send() or uart_send()
    
    if (note == 0 || velocity == 0) {
        // Send note-off
        midi_tx_byte(0x80 | (midi_channel - 1)); // Status: Note Off
        midi_tx_byte(last_ef101_note);           // Note number
        midi_tx_byte(0);                         // Velocity
    } else {
        // Send note-on
        midi_tx_byte(0x90 | (midi_channel - 1)); // Status: Note On
        midi_tx_byte(note);                      // Note number  
        midi_tx_byte(velocity);                  // Velocity
        
        last_ef101_note = note; // Remember for note-off
    }
}

/******************************************************************************
 * STEP 3: Modify main() function
 * Add paraphonic initialization
 ******************************************************************************/

int main(void) {
    // Existing Rockit initialization code...
    init_hardware();
    init_midi();
    init_oscillators();
    init_filter();
    // ... etc
    
    // ADD THIS:
    paraphonic_init();
    
    sei(); // Enable interrupts
    
    // Main loop
    while(1) {
        // Existing main loop code
        check_knobs();
        check_buttons();
        update_lfo();
        // ... etc
    }
    
    return 0;
}

/******************************************************************************
 * STEP 4: Modify MIDI receive handler
 * Route MIDI messages to paraphonic system
 ******************************************************************************/

/*
 * This is probably in a MIDI interrupt or called from main loop
 * Look for where Rockit processes incoming MIDI
 */
void process_midi_message(void) {
    // Rockit probably has something like:
    uint8_t status = midi_rx_buffer[0];
    uint8_t data1 = midi_rx_buffer[1];
    uint8_t data2 = midi_rx_buffer[2];
    
    // Check if message is for our channel
    if ((status & 0x0F) == (midi_channel - 1)) {
        // ADD THIS:
        paraphonic_midi_handler(status, data1, data2);
    }
    
    // REMOVE or COMMENT OUT old monophonic note handling
    // (The paraphonic handler replaces it)
    /*
    if ((status & 0xF0) == 0x90 && data2 > 0) {
        // Old code: set_note(data1);
    }
    */
    
    // Keep any other MIDI handling (CC, program change, etc.)
}

/******************************************************************************
 * STEP 5: Add mode switching button (OPTIONAL)
 * This adds a front-panel button to cycle through modes
 ******************************************************************************/

// Add this to your button checking code
void check_mode_button(void) {
    static VoiceMode current_mode = MODE_ROUND_ROBIN;
    static uint8_t last_button_state = 0;
    
    // Read button (adjust pin for your hardware)
    uint8_t button_state = PINA & (1 << PA7); // Example pin
    
    // Detect button press (rising edge)
    if (button_state && !last_button_state) {
        // Cycle to next mode
        current_mode = (current_mode + 1) % 3;
        paraphonic_set_mode(current_mode);
        
        // Optional: Flash LED to indicate mode
        // LED flash pattern: 1 flash = Low, 2 = Last, 3 = RoundRobin
        for (uint8_t i = 0; i <= current_mode; i++) {
            PORTB |= (1 << PB0);  // LED on
            _delay_ms(100);
            PORTB &= ~(1 << PB0); // LED off  
            _delay_ms(100);
        }
    }
    
    last_button_state = button_state;
}

// Call this from your main loop:
// check_mode_button();

/******************************************************************************
 * REFERENCE: Rockit Variable Names (may vary in your version)
 ******************************************************************************/

/*
 * Common Rockit variable names to look for:
 * 
 * Oscillators:
 * - osc1_phase, osc2_phase
 * - osc1_phase_inc, osc2_phase_inc  
 * - osc1_amplitude, osc2_amplitude
 * - osc1_wavetable, osc2_wavetable
 * 
 * MIDI:
 * - midi_channel
 * - midi_rx_buffer[]
 * - note_freq_table[]
 * 
 * Audio:
 * - sample_rate (probably 31250 Hz)
 * - audio_output
 * 
 * If your variable names are different, just replace them above!
 */

/******************************************************************************
 * Makefile Changes
 ******************************************************************************/

/*
Add to your Makefile CSRCS line:

CSRCS = eight_bit_synth_main.c \
        interrupt_routines.c \
        rockit_paraphonic.c

Make sure rockit_paraphonic.c and rockit_paraphonic.h are in the same 
directory as your other source files.
*/

/******************************************************************************
 * Expected Code Size Impact
 ******************************************************************************/

/*
Flash (program memory): ~2KB additional
RAM (variables): ~100 bytes additional
Stack: ~50 bytes worst case

ATMEGA164PA has:
- 16KB flash (plenty of room)
- 1KB RAM (should be fine)

If you're close to limits, you can optimize by:
1. Reducing note_stack size from 16 to 8
2. Removing one voice mode you don't need
3. Using uint8_t instead of uint16_t where possible
*/

/******************************************************************************
 * TESTING CHECKLIST
 ******************************************************************************/

/*
â–¡ Code compiles without errors
â–¡ Synth powers on normally
â–¡ Mode switches via CC 102
â–¡ Low Note mode: plays 3 lowest notes correctly
â–¡ Last Note mode: plays 3 most recent notes correctly  
â–¡ Round Robin mode: cycles through voices
â–¡ EF-101D tracks pitch via MIDI out
â–¡ Notes turn off cleanly (no stuck notes)
â–¡ Pre-delay creates desired texture
â–¡ Chorus spreads stereo image
*/

/******************************************************************************
 * DEBUGGING TIPS
 ******************************************************************************/

/*
If notes stick:
- Check that note-off messages call paraphonic_note_off()
- Verify clear_voice() actually silences oscillators
- Add debug LED to show when notes received

If EF-101D doesn't track:
- Use MIDI monitor to verify note-out messages
- Check Respeaker I2C DAC is working
- Verify CV output with multimeter (should be 1V/octave)

If voices don't allocate correctly:
- Add debug output showing which voice gets which note
- Check stack_size isn't exceeding limits
- Verify mode is set correctly
*/
