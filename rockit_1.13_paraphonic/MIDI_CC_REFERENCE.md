# Rockit Paraphonic - MIDI CC Control Reference

## Current CC Mapping (Final)

### CC 102: Mono/Juno Mode Toggle
**Controls OSC_2 behavior in MONOPHONIC mode only**

- **Value 0-63:** Classic Rockit Mono
  - OSC_1 plays the note
  - OSC_2 follows with DETUNE knob (fat detuned unison)
  
- **Value 64-127:** Juno Sub-Osc Mode  
  - OSC_1 plays the note
  - OSC_2 plays **one octave lower** (like Juno sub-osc)
  - Detune knob disabled (OSC_2 locked to -12 semitones)

**Note:** This only affects MONOPHONIC mode. In paraphonic modes, OSC_2 always plays independent notes.

### CC 103: Paraphonic Enable/Disable
**Master on/off for paraphonic voice allocation**

- **Value 0-63:** Disabled  
  - Classic Rockit monophonic operation
  - CC 102 controls Mono/Juno toggle
  
- **Value 64-127:** Enabled
  - Paraphonic voice allocation active
  - Use CC 104 to select voice mode

### CC 104: Paraphonic Mode Selection
**Selects voice allocation algorithm (only when CC 103 ≥ 64)**

- **Value 0-31:** Low Note Priority
  - Always plays the 3 lowest notes pressed
  
- **Value 32-63:** Last Note Priority  
  - Plays the 3 most recently pressed notes
  
- **Value 64-95:** Round Robin (Default)
  - Cycles voices for each new note
  
- **Value 96-127:** High Note Priority
  - Always plays the 3 highest notes pressed

### CC 105: EF-101D External Synth Enable
**Enables optional third voice via MIDI out**

- **Value 0-63:** 2-voice operation
  - OSC_1 and OSC_2 only
  
- **Value 64-127:** 3-voice operation
  - OSC_1, OSC_2, and EF-101D (MIDI out)

---

## Quick Setup Examples

### Classic Rockit Sound
```
CC 103 = 0   (Paraphonic off)
CC 102 = 0   (Normal mono with detune)
```
Turn DETUNE knob for fat unison leads.

### Juno-Style Bass
```
CC 103 = 0   (Paraphonic off)  
CC 102 = 127 (Sub-osc mode)
```
OSC_2 automatically one octave below. Adjust OSC_MIX to taste.

### 2-Voice Paraphonic
```
CC 103 = 127 (Paraphonic on)
CC 104 = 64  (Round robin)
CC 105 = 0   (2 voices)
```
Play 2-note chords/melodies.

### 3-Voice with EF-101D
```
CC 103 = 127 (Paraphonic on)
CC 104 = 64  (Round robin)
CC 105 = 127 (3 voices)
```
Requires external MIDI synth on EF-101D out.

### Sub-Osc Paraphonic (Experimental)
```
CC 103 = 127 (Paraphonic on)
CC 104 = 0   (Low note priority)
CC 102 = 127 (Sub-osc enabled)
```
Each voice doubled one octave down. Organ-like!

---

## Important Notes

### Detune Knob Behavior
- **Mono mode, CC 102 < 64:** Detune knob works (OSC_2 offsets from OSC_1)
- **Mono mode, CC 102 ≥ 64:** Detune disabled (OSC_2 locked to -12 semitones)
- **Paraphonic modes:** Detune knob doesn't apply (voices play independent notes)

### Sub-Osc in Paraphonic
Yes, you can enable sub-osc (CC 102 ≥ 64) while in paraphonic mode! Each voice gets its own sub-octave. This creates interesting octave-spread chords.

### Startup Defaults
- Monophonic mode (CC 103 < 64)
- Normal detune (CC 102 < 64)
- 2 voices (CC 105 < 64)

### Saving Settings
CC values are NOT saved in patches - they're live performance controls via MIDI only.

---

## Troubleshooting

**"Sub-osc not working in mono"**
- Make sure CC 103 < 64 (paraphonic off)
- Set CC 102 ≥ 64
- Detune knob won't affect it - OSC_2 is locked to -12

**"Can't select LFO2"**  
- This was a bug in empty patches - fixed in this version

**"Detune doesn't work in paraphonic"**
- By design! In paraphonic, OSC_1 and OSC_2 play different notes
- Use sub-osc mode (CC 102 ≥ 64) for octave doubling instead

**"EF-101D not working"**
- Check MIDI out is connected to external synth
- Set CC 105 ≥ 64
- EF-101D only gets notes in paraphonic modes with 3+ notes

---

## MIDI Implementation Chart

| CC  | Function | Range | Default |
|-----|----------|-------|---------|
| 102 | Mono/Juno | 0-127 | < 64 (Mono) |
| 103 | Para Enable | 0-127 | < 64 (Off) |
| 104 | Para Mode | 0-127 | 64-95 (RR) |
| 105 | EF-101D | 0-127 | < 64 (Off) |

All other CCs function as per original Rockit (LFO, filter, etc).
