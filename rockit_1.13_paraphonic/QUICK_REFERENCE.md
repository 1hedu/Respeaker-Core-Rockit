# Rockit Paraphonic - Quick Reference & Example Patches

## MIDI CC Quick Reference

**Mode Selection (CC 102)**
- 0-42: Low Note Priority
- 43-84: Last Note Priority  
- 85-127: Round Robin

## Voice Assignment Chart

```
MIDI Note Input â†’ Voice Allocation â†’ Hardware Output

Voice 1: DCO1 (Internal Rockit oscillator 1)
Voice 2: DCO2 (Internal Rockit oscillator 2)
Voice 3: EF-101D (External via MIDI â†’ Respeaker â†’ CV)
                             â†“
                    All voices sum internally in Rockit
                             â†“
                    Pre-delay â†’ Filter â†’ VCA â†’ Output
                             â†“
                    Booster/Tube â†’ Chorus â†’ Done!
```

## Example Patches & Use Cases

### Patch 1: Juno-Style Pad (Round Robin)
**Mode**: Round Robin
**Settings**:
- Filter: Low-pass, moderate resonance
- Filter envelope: Slow attack, long decay
- Pre-delay: 30-50ms, low mix
- Chorus: Moderate depth, slow rate

**Playing**: Hold chord, let round-robin create movement
**Result**: Each note cycles through voices, creating organic variation through pre-delay and chorus spread

---

### Patch 2: Bass Stack (Low Note Priority)
**Mode**: Low Note Priority  
**Settings**:
- Filter: Low-pass, low resonance
- DCO1: Square wave
- DCO2: Square wave, detuned slightly
- EF-101D: Sub-oscillator character
- Pre-delay: Off or minimal
- Tube: Before booster, drive moderate

**Playing**: Play bass lines, can add upper notes without affecting bass
**Result**: Thick, 3-oscillator bass that stays locked to lowest notes

---

### Patch 3: Lead Synth (Last Note Priority)
**Mode**: Last Note Priority
**Settings**:
- Filter: Band-pass, high resonance
- Filter envelope: Fast attack, medium decay
- Pre-delay: 80-120ms, 20% mix
- Chorus: Light depth, faster rate
- Tube: After booster, drive high

**Playing**: Melodic lines with occasional chord stabs
**Result**: Lead voice always plays your most recent notes, older notes fade naturally

---

### Patch 4: Polyrhythmic Sequence (Round Robin)
**Mode**: Round Robin
**Settings**:
- Filter: High-pass, moderate resonance  
- LFO â†’ Filter cutoff
- Pre-delay: 150ms, 40% mix
- Arpeggiator: ON (if using Rockit's arp in drone mode)

**Playing**: Hold 3-4 notes
**Result**: Each voice creates its own delay tail, rhythmic chaos from pre-delay feedback

---

### Patch 5: Chord Memory (Low Note Priority)
**Mode**: Low Note Priority
**Settings**:
- Drone mode: ON
- Filter envelope: Very slow attack/decay
- Pre-delay: 200ms, 50% mix
- Chorus: Deep, slow

**Result**: Ambient pad machine - holds lowest chord voicing with massive delay/chorus wash

---

## Mode Comparison for Common Playing Styles

| Playing Style | Recommended Mode | Why |
|--------------|------------------|-----|
| Bass lines | Low Note | Keeps bass stable |
| Lead melodies | Last Note | Most recent = most important |
| Arpeggios | Round Robin | Creates movement |
| Chord stabs | Low Note | Consistent voicing |
| Fast runs | Round Robin | Distributes load |
| Drone/ambient | Low Note | Stable harmonic structure |

## Pre-Delay Sweet Spots

The pre-delay placement (before filter) is KEY to this design:

**Short (20-60ms)**
- Subtle thickening
- Pseudo-chorus effect
- Maintains clarity

**Medium (60-150ms)**  
- Rhythmic doubling
- Call-and-response feel
- Filter sweeps create movement

**Long (150-300ms)**
- Pseudo-reverb
- Heavy texture
- Notes swim together

**Feedback Trick**: 
If your delay pedal has feedback, set it to 20-30% for sustained pad sounds. The filter will process the delays, creating evolving timbres.

## EF-101D Tuning Tips

The EF-101D as sub-oscillator:
1. **Standard Tuning**: Same octave as DCO1/DCO2
2. **Sub Octave**: -12 semitones (if Respeaker can transpose)
3. **Fifth**: +7 semitones for power chord effect
4. **Detune**: Slightly off-tune for analog drift vibe

**CV Calibration**: 
Make sure your Respeaker DAC is properly calibrated for 1V/octave. Test with a tuner on each voice to ensure they're tracking together.

## Troubleshooting Common Issues

**"Notes are muddy/smeared"**
- This is actually a FEATURE with pre-delay!
- Reduce pre-delay mix
- Use shorter delay time
- Try High-Pass filter mode

**"EF-101D out of tune"**
- Calibrate Respeaker DAC output
- Check I2C communication
- Verify 1V/octave scaling

**"Voices cutting out"**
- Check note stack isn't overflowing (16 note limit)
- Verify MIDI note-offs are being sent
- May need to clear stuck notes on mode change

**"Round Robin not rotating"**
- Only rotates on NEW notes
- Try releasing all keys between notes
- Check that note-on velocity > 0

## Performance Tips

**Mode Switching During Performance**:
- Map CC 102 to a MIDI controller knob
- Or add a footswitch if you modify the hardware
- Switching modes real-time creates dramatic texture changes

**Velocity Sensitivity**:
- The velocity parameter is passed through but not used by default
- Modify `set_dco*_note()` to scale amplitude by velocity
- Creates dynamic expression

**MIDI Thru Chain**:
Version 1.12 has MIDI thru - you can chain additional gear:
```
MIDI Controller â†’ Rockit â†’ Respeaker â†’ Another synth
```

## Future Mods to Consider

1. **Unison Mode**: All 3 voices play same note, slightly detuned
2. **Voice Panning**: Send DCO1 left, DCO2 right, EF-101D center (requires stereo pre-delay mod)
3. **Per-Voice Filtering**: Different filter settings per voice (major hardware mod)
4. **Polyphonic Mode**: Independent envelopes per voice (requires significant firmware changes)

## Selling Points for Your Build

When you list this synth, emphasize:
- âœ… 3-voice paraphonic (rare in mono synths)
- âœ… Three different playing modes
- âœ… Juno-inspired chorus architecture
- âœ… Pre-filter delay (not common)
- âœ… Tube overdrive with switchable order
- âœ… Custom firmware development
- âœ… Analog+digital hybrid character

---

## Contact & Credits

Paraphonic firmware by: [Your Name]
Based on: HackMe Rockit by Matt Heins
Hardware mods: Inspired by classic Juno architecture
License: GPLv3

For questions about this paraphonic implementation:
[Your contact info]
