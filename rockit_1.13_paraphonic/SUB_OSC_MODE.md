# Sub-Oscillator Mode - New Feature!

## What It Does

OSC_2 can now play **one octave lower** than OSC_1, just like a Juno sub-oscillator!

This works in ALL modes:
- **Monophonic:** OSC_1 + sub-osc one octave down
- **Paraphonic:** Each voice gets its own sub-osc

## How To Enable

**MIDI CC 105:**
- Value 0-63: Normal OSC_2 operation (detune works in mono)
- Value 64-127: Sub-Osc mode (OSC_2 one octave lower)

## Behavior by Mode

### Monophonic Mode (Default)
**Sub-Osc OFF (CC 105 < 64):**
- OSC_1 plays the note
- OSC_2 follows with detune knob (classic Rockit)
- Fat, detuned unison sound
- Detune knob is ACTIVE

**Sub-Osc ON (CC 105 >= 64):**
- OSC_1 plays the note
- OSC_2 plays exactly one octave lower
- Thick, bass-heavy sound like a Juno
- Detune knob is BYPASSED (sub-osc tracks perfectly)

### Paraphonic Modes
**Sub-Osc OFF:**
- Detune knob doesn't work (by design)
- OSC_2 plays independent notes for polyphony

**Sub-Osc ON:**
- OSC_2 still plays independent notes
- BUT each note is one octave lower
- Creates a doubled, octave-spread paraphonic sound

## Why Detune Doesn't Work in Paraphonic

In paraphonic modes, OSC_1 and OSC_2 are playing **different MIDI notes** for polyphony. The detune knob is designed to offset OSC_2 from OSC_1 when they're playing the *same* note (monophonic).

In paraphonic, they're already playing different notes, so detune doesn't make sense. Sub-osc mode gives you a different flavor - all voices get a sub-octave.

## MIDI CC Summary

**Paraphonic Control CCs:**
- **CC 102:** Voice mode selection (0-4)
  - 0-25: Monophonic
  - 26-50: Low Note Priority
  - 51-76: Last Note Priority
  - 77-101: Round Robin (default paraphonic)
  - 102-127: High Note Priority

- **CC 103:** Paraphonic enable/disable
  - 0-63: Disabled (classic Rockit mono)
  - 64-127: Enabled (use mode from CC 102)

- **CC 104:** EF-101D external synth enable
  - 0-63: 2 voices (OSC_1, OSC_2)
  - 64-127: 3 voices (OSC_1, OSC_2, EF-101D)

- **CC 105:** Sub-Oscillator mode (NEW!)
  - 0-63: Normal OSC_2 operation
  - 64-127: Sub-osc (one octave down)

## Sound Examples

**Classic Juno Sub-Osc Bass:**
1. Set to Monophonic mode (CC 102 = 0)
2. Enable sub-osc (CC 105 = 127)
3. Set OSC MIX to blend OSC_1 and OSC_2
4. Play low bass notes
5. Result: Thick, doubled octave bass

**Paraphonic Organ:**
1. Set to Low Note Priority (CC 102 = 40)
2. Enable sub-osc (CC 105 = 127)
3. Play 2-3 note chords
4. Result: Each note has its octave below, organ-like

**Fat Mono Lead:**
1. Monophonic mode (CC 102 = 0)
2. Sub-osc OFF (CC 105 = 0)
3. Adjust DETUNE knob for chorusing
4. Result: Classic detuned unison lead

## Technical Details

The sub-osc feature:
- Subtracts 12 semitones (one octave) from the note
- Applied to all OSC_2 assignments in paraphonic voice allocation
- In monophonic mode, directly sets OSC_2 to note-12
- Bypasses the normal detune calculation when enabled

## Compatibility

- **EF-101D:** Works independently - sub-osc only affects OSC_1 and OSC_2
- **Detune:** Works in monophonic when sub-osc is OFF
- **All paraphonic modes:** Support sub-osc
- **Patches:** Sub-osc state is controlled via MIDI, not saved in patches

## Tips

1. **For classic Rockit sound:** Keep sub-osc OFF
2. **For modern bass:** Enable sub-osc in mono mode
3. **For experimental:** Try sub-osc in paraphonic modes
4. **Mix control:** Use OSC_MIX knob to balance OSC_1 and OSC_2

Enjoy the new sonic possibilities!
