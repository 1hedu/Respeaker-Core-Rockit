# Rockit Paraphonic Firmware - Integration Guide

## Overview
This paraphonic voice allocation module adds 3-voice capability to the HackMe Rockit synth, distributing notes across:
- **Voice 1**: DCO1 (internal)
- **Voice 2**: DCO2 (internal)  
- **Voice 3**: EF-101D (via MIDI out to Respeaker for CV conversion)

## Voice Allocation Modes

### 1. Low Note Priority
- Always plays the 3 lowest pressed notes
- Perfect for bass lines and chord voicings where you want consistent low end
- Example: Playing C-E-G-B plays C-E-G

### 2. Last Note Priority
- Plays the 3 most recently pressed notes
- Great for melodic playing where recent notes are most important
- Example: Playing C-E-G-B plays E-G-B

### 3. Round Robin
- Each new note cycles to the next voice
- Creates interesting movement and texture
- Best for fast arpeggios and rhythmic patterns

## Integration Steps

### Step 1: Add Files to Your Project
Add these files to your Rockit source directory:
- `rockit_paraphonic.c`
- `rockit_paraphonic.h`

Update your Makefile to include `rockit_paraphonic.c` in compilation.

### Step 2: Implement Hardware Interface Functions

In your main Rockit code (likely `eight_bit_synth_main.c`), implement these three functions:

```c
#include "rockit_paraphonic.h"

// Set DCO1 frequency based on MIDI note
void set_dco1_note(uint8_t note, uint8_t velocity) {
    if (note == 0 || velocity == 0) {
        // Turn off DCO1
        osc1_amplitude = 0;
        return;
    }
    
    // Convert MIDI note to frequency
    // Rockit uses phase accumulators for oscillators
    uint32_t frequency = midi_note_to_frequency(note);
    osc1_phase_increment = frequency_to_phase_increment(frequency);
    osc1_amplitude = velocity_to_amplitude(velocity);
}

// Set DCO2 frequency based on MIDI note
void set_dco2_note(uint8_t note, uint8_t velocity) {
    if (note == 0 || velocity == 0) {
        // Turn off DCO2
        osc2_amplitude = 0;
        return;
    }
    
    uint32_t frequency = midi_note_to_frequency(note);
    osc2_phase_increment = frequency_to_phase_increment(frequency);
    osc2_amplitude = velocity_to_amplitude(velocity);
}

// Send MIDI note to output (for EF-101D via Respeaker)
void send_midi_note_out(uint8_t note, uint8_t velocity) {
    if (note == 0 || velocity == 0) {
        // Send note off
        midi_send_byte(0x80 | midi_channel); // Note off
        midi_send_byte(last_ef101_note);     // Last note played
        midi_send_byte(0);
    } else {
        // Send note on
        midi_send_byte(0x90 | midi_channel); // Note on
        midi_send_byte(note);
        midi_send_byte(velocity);
        last_ef101_note = note;
    }
}
```

### Step 3: Initialize Paraphonic System

In your `main()` function, after hardware initialization:

```c
int main(void) {
    // Existing Rockit initialization
    hardware_init();
    midi_init();
    
    // Initialize paraphonic system
    paraphonic_init();
    
    // Main loop
    while(1) {
        // Your existing code
    }
}
```

### Step 4: Route MIDI Messages to Paraphonic Handler

In your existing MIDI receive handler (likely in an interrupt or main loop):

```c
// Your existing MIDI receive code
void handle_midi_message(void) {
    uint8_t status = midi_read_status();
    uint8_t data1 = midi_read_data1();
    uint8_t data2 = midi_read_data2();
    
    // Route to paraphonic handler
    paraphonic_midi_handler(status, data1, data2);
    
    // Keep any other MIDI handling you need
}
```

## Mode Selection

### Via MIDI CC
Send MIDI CC 102 (configurable in code) with values:
- **0-42**: Low Note Priority
- **43-84**: Last Note Priority
- **85-127**: Round Robin

### Via Front Panel (Optional)
Add a button that cycles through modes:

```c
void mode_button_handler(void) {
    static VoiceMode current_mode = MODE_ROUND_ROBIN;
    
    current_mode = (current_mode + 1) % 3;
    paraphonic_set_mode(current_mode);
    
    // Update LED or display to show mode
}
```

## Helper Functions You'll Need

### MIDI Note to Frequency Conversion
```c
uint32_t midi_note_to_frequency(uint8_t note) {
    // A4 (MIDI 69) = 440 Hz
    // Each semitone is 2^(1/12) ratio
    // You might already have this in your Rockit code
    
    // For 8-bit integer math, use lookup table:
    const uint32_t freq_table[128] = {
        // Fill with pre-calculated frequencies
        // Or use existing Rockit frequency table
    };
    
    return freq_table[note];
}
```

### Frequency to Phase Increment
```c
uint32_t frequency_to_phase_increment(uint32_t frequency) {
    // Rockit uses phase accumulators
    // Phase increment = (frequency * 2^32) / sample_rate
    // Adjust based on your actual sample rate (likely 31250 Hz or similar)
    
    return (frequency * 137439) >> 10; // Example calculation
}
```

## Testing

### Test 1: Low Note Priority
1. Set mode to Low Note (CC 102 value 0)
2. Play a C major triad (C-E-G)
3. All three should sound
4. Add B above - C-E-G still playing
5. Add A below - A-C-E should now play

### Test 2: Last Note Priority
1. Set mode to Last Note (CC 102 value 50)
2. Play C-E-G slowly
3. Only the last 3 notes pressed should sound
4. Play additional notes - oldest notes drop off

### Test 3: Round Robin
1. Set mode to Round Robin (CC 102 value 100)
2. Play rapid notes
3. Each new note should go to the next voice in rotation
4. Creates cycling pattern across the 3 oscillators

## Troubleshooting

**Problem**: Notes stick or don't turn off
- Check that `set_dco1_note(0, 0)` actually silences the oscillator
- Verify MIDI note-off messages are being received

**Problem**: EF-101D not tracking
- Check MIDI cable from Rockit OUT to Respeaker
- Verify Respeaker DAC is outputting CV correctly
- Check that `send_midi_note_out()` is actually sending MIDI

**Problem**: Voices overlap or sound muddy
- This is expected! Pre-delay will cause note smearing
- Adjust delay mix to taste
- Try different voice modes for different textures

## Advanced Modifications

### Change Voice Assignment
Edit the hardware routing in `paraphonic_allocate_voices()` functions if you want different voice assignments.

### Add Velocity Response
The velocity parameter is passed through - use it to control amplitude, filter, or other parameters per voice.

### Portamento
Add portamento between note changes by smoothing the phase increment changes in your `set_dco*_note()` functions.

## Code Size
Estimated additional flash memory usage: ~2KB
Estimated additional RAM usage: ~100 bytes

Should fit comfortably in ATMEGA164PA (16KB flash, 1KB RAM).

## License
GPLv3 - Compatible with original Rockit firmware
