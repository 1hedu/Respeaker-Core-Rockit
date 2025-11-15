# Rockit 1.13 Paraphonic Synth - ReSpeaker Core Port

Port of the Rockit 1.13 paraphonic synthesizer firmware from ATmega644P to ReSpeaker Core v1.0 (MIPS platform).

## Overview

This project brings the feature-rich Rockit paraphonic synthesizer to the ReSpeaker Core v1.0, enabling it to function as a standalone hardware synthesizer with TCP MIDI input and web-based control interface.

### Features

- **3-voice paraphonic synthesis** with configurable modes (mono/paraphonic)
- **16 oscillator waveforms**: Sine, Square, Saw, Triangle, 9 morphing waves, Hard Sync, Noise, Raw Square
- **16 LFO shapes** with 6 modulation destinations
- **State-variable filter** with 4 modes (LP, HP, BP, Notch)
- **ADSR envelope** with filter envelope modulation
- **Sub-oscillator** for added low-end
- **Glide/portamento** with adjustable time
- **TCP MIDI server** on port 50000 for remote control
- **Web UI** for parameter control via browser

## Hardware

- **Platform**: Seeed ReSpeaker Core v1.0
- **CPU**: MediaTek MT7688AN (MIPS 24KEc @ 580 MHz)
- **Audio**: Onboard codec with 1/4" line output
- **OS**: OpenWrt Chaos Calmer

## Quick Start

### Prerequisites
- ReSpeaker Core v1.0 with network access
- Host computer for cross-compilation (Linux recommended)

### Building

See **[CROSS_COMPILE_GUIDE.md](CROSS_COMPILE_GUIDE.md)** for detailed cross-compilation instructions.

Quick summary:
```bash
# Set up OpenWrt SDK environment
export SDK_PATH=~/OpenWrt-SDK-15.05.1-ramips-mt7688_gcc-4.8-linaro_uClibc-0.9.33.2.Linux-x86_64
export STAGING_DIR=$SDK_PATH/staging_dir
export PATH=$STAGING_DIR/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin:$PATH

# Compile
cd ReSpeaker_Rockit_1.0
make

# Deploy
scp respeaker_rockit root@respeaker.local:/root/
```

### Running

```bash
# On ReSpeaker device
ssh root@respeaker.local
cd /root
./respeaker_rockit hw:0,0 --tcp-midi
```

The synth will start and listen for MIDI on TCP port 50000.

### Web Interface

1. Copy `rockit_complete.html` to a web server or open locally
2. Update the IP address in the HTML to point to your ReSpeaker
3. Open in browser to control all parameters

## Project Structure

```
.
├── ReSpeaker_Rockit_1.0/        # Current firmware (WORKING)
│   ├── main.c                    # Entry point, ALSA setup, MIDI server
│   ├── rockit_engine.c           # Synth engine (oscillators, envelope, filter)
│   ├── rockit_engine.h
│   ├── params.c                  # Parameter management
│   ├── params.h
│   ├── paraphonic.h              # Voice allocation
│   ├── wavetables.c              # Waveform lookup tables
│   ├── wavetables.h
│   ├── filter_svf.c              # State-variable filter
│   ├── filter_svf.h
│   ├── socket_midi_raw.c         # TCP MIDI server
│   ├── socket_midi_raw.h
│   ├── avr_compat.h              # AVR compatibility shims
│   └── Makefile
│
├── rockit_1.13_paraphonic.tar.gz # Original ATmega644P firmware
├── rockit_complete.html          # Web-based control interface
├── CROSS_COMPILE_GUIDE.md        # Detailed build instructions
└── README.md                     # This file
```

## MIDI Implementation

### Control Changes (CC)

| CC # | Parameter | Range | Description |
|------|-----------|-------|-------------|
| 1 | LFO Depth | 0-127 | Modulation wheel |
| 7 | Master Volume | 0-127 | Overall output level |
| 70 | Release | 0-127 | Envelope release time |
| 71 | Resonance | 0-127 | Filter resonance/Q |
| 72 | OSC Mix | 0-127 | OSC1 ← → OSC2 balance |
| 73 | Attack | 0-127 | Envelope attack time |
| 74 | Cutoff | 0-127 | Filter cutoff frequency |
| 75 | Decay | 0-127 | Envelope decay time |
| 76 | Sub Osc | 0-127 | Sub-oscillator on/off (64+ = on) |
| 80 | OSC1 Wave | 0-127 | Waveform (value >> 3 = 0-15) |
| 81 | OSC2 Wave | 0-127 | Waveform (value >> 3 = 0-15) |
| 82 | Tune | 0-127 | Coarse tuning (semitones) |
| 83 | Fine | 0-127 | Fine tuning (cents) |
| 84 | Filter Mode | 0-127 | LP/HP/BP/Notch (value >> 5) |
| 85 | Filter Env | 0-127 | Envelope → Filter amount |
| 86 | Sustain | 0-127 | Envelope sustain level |
| 87 | LFO1 Rate | 0-127 | LFO1 frequency |
| 88 | LFO1 Shape | 0-127 | LFO1 waveform (value >> 3) |
| 89 | LFO1 Dest | 0-127 | LFO1 destination (value >> 4) |
| 90 | Glide | 0-127 | Portamento time |
| 91-94 | LFO2 controls | 0-127 | Rate, Depth, Shape, Dest |
| 102 | Mode | 0-127 | Mono/Para toggle |
| 103 | Voices | 0-127 | 2/3-voice toggle |

### Note Messages
- **Note On**: MIDI note number 0-127, velocity 0-127
- **Note Off**: MIDI note number 0-127

## CLI Commands

When running interactively, type commands at the prompt:

```
NOTE <0-127>      - Trigger note (stays on)
OFF <0-127>       - Release note
ALLOFF            - All notes off (panic)
CUTOFF <0-127>    - Set filter cutoff
RESO <0-127>      - Set resonance
VOL <0-127>       - Set master volume
MIX <0-127>       - Set oscillator mix
ATTACK <0-127>    - Set attack time
DECAY <0-127>     - Set decay time
RELEASE <0-127>   - Set release time
HELP              - Show commands
```

## Fixes Applied (v1.0)

This version includes critical fixes from the original port attempt:

1. **PROGMEM Syntax Error** - Removed incompatible AVR-specific keywords from wavetables
2. **MIDI CC Mapping** - Updated CC handler to match web UI (CC 70-94 range)
3. **16 Waveform Support** - Added all waveforms from original Rockit 1.13
4. **params_init() Bug** - Added missing initialization call (was causing zero volume)

## Performance

- **CPU Usage**: ~25% at 48kHz sample rate
- **Latency**: <10ms note-to-audio
- **Polyphony**: 3 voices (paraphonic mode)
- **Memory**: ~8MB RSS

## Known Issues

- LFO routing partially implemented (destinations work but some modulations are placeholders)
- Sub-oscillator uses simple -1 octave sine wave (original had more complex implementation)
- Some morph waveforms reuse base waveforms (requires additional wavetables for full morphing)

## Future Enhancements

- [ ] Complete LFO modulation routing
- [ ] Add arpeggiator (present in original firmware)
- [ ] Implement preset system
- [ ] Add drone/loop mode
- [ ] Optimize wavetable access for better performance
- [ ] Add MIDI learn for CC mapping

## Credits

- **Original Rockit Firmware**: Matt Heins / HackMe Electronics (2011-2015)
- **ReSpeaker Port**: Based on Rockit 1.13 paraphonic edition
- **Platform**: Seeed Studio ReSpeaker Core v1.0

## License

This project inherits the GPL v3 license from the original Rockit firmware.

```
Rockit is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
```

See [LICENSE](LICENSE) file for details.

## Support

For issues specific to this ReSpeaker port, please open an issue on GitHub.

For questions about the original Rockit firmware, see http://hackmelectronics.com/

---

**Status**: ✅ Working (2025-01-15)
**Tested On**: ReSpeaker Core v1.0 (MT7688) with OpenWrt Chaos Calmer
