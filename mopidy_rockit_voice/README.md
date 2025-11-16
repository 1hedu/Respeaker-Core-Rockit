# Mopidy-Rockit-Voice

Voice control for Rockit paraphonic synthesizer via MIDI commands.

## Overview

This Mopidy extension enables voice control of the Rockit synth running on ReSpeaker Core. It's based on `mopidy-hallo` but modified to send MIDI commands instead of playing music files.

**Key benefit**: Keeps the microphone continuously claimed by Mopidy, avoiding the ALSA device release/reclaim cycle that causes audio noise.

## How It Works

1. **Continuous Microphone Capture**: Mopidy frontend thread runs continuously, keeping the ReSpeaker mic claimed
2. **Wake Word Detection**: Listens for "respeaker" wake word using PocketSphinx
3. **Speech Recognition**: Records and recognizes voice command
4. **MIDI Translation**: Converts voice command to MIDI and sends to Rockit synth (TCP port 50000)

## Supported Voice Commands

| Command | Action | MIDI Message |
|---------|--------|--------------|
| "play [note]" | Play a note | Note On + Note Off |
| "stop" | All notes off | CC 123 (All Notes Off) |
| "louder" | Increase volume | CC 7 = 110 (~85%) |
| "quieter" | Decrease volume | CC 7 = 64 (50%) |
| "filter up" | Increase cutoff | CC 74 = 100 |
| "filter down" | Decrease cutoff | CC 74 = 30 |

**Note names supported** (for "play" command):
- C, D, E, F, G, A, B (plays middle octave)

## Installation

```bash
cd mopidy_rockit_voice
python setup.py install
```

Or for development:
```bash
python setup.py develop
```

## Configuration

Add to your Mopidy config (`/root/.config/mopidy/mopidy.conf`):

```ini
[rockit_voice]
enabled = true
midi_host = localhost
midi_port = 50000
```

## Usage

1. **Start Rockit synth**:
   ```bash
   cd /root
   ./respeaker_rockit --tcp-midi &
   ```

2. **Start Mopidy** (with rockit_voice extension):
   ```bash
   mopidy --config /root/.config/mopidy/mopidy.conf
   ```

3. **Say the wake word**: "respeaker"

4. **Give a command**:
   - "play note C"
   - "louder"
   - "filter up"
   - "stop"

## Extending Commands

To add new voice commands, edit `__init__.py`:

```python
# Add to self.commands in __init__
self.commands = {
    'your phrase': self._cmd_your_handler,
}

# Add handler method
def _cmd_your_handler(self, text):
    """Your command description"""
    # Send MIDI CC, Note, etc.
    self._send_cc(74, 127)  # Example: cutoff to max
```

### MIDI Reference for Rockit

See [README.md](../README.md) for complete MIDI CC mapping. Key controls:

- **CC 7**: Master Volume (0-127)
- **CC 74**: Filter Cutoff (0-127)
- **CC 71**: Resonance (0-127)
- **CC 72**: Oscillator Mix (0-127)
- **CC 73**: Attack (0-127)
- **CC 75**: Decay (0-127)
- **CC 123**: All Notes Off

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│ Mopidy Process                                          │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────────────────────────────────────┐          │
│  │ RockitVoiceFrontend (Pykka Actor)        │          │
│  │                                           │          │
│  │  Thread:                                 │          │
│  │    ┌──────────────────────────┐          │          │
│  │    │ ReSpeaker Microphone     │          │          │
│  │    │ - wakeup("respeaker")    │ ◄────────┼───────── Mic claimed
│  │    │ - listen()               │          │          continuously!
│  │    │ - recognize()            │          │          │
│  │    └──────────────────────────┘          │          │
│  │            │                             │          │
│  │            │ voice command               │          │
│  │            ▼                             │          │
│  │    ┌──────────────────────────┐          │          │
│  │    │ Command Processing       │          │          │
│  │    │ - match known phrases    │          │          │
│  │    │ - extract parameters     │          │          │
│  │    └──────────────────────────┘          │          │
│  │            │                             │          │
│  │            │ MIDI bytes                  │          │
│  │            ▼                             │          │
│  │    ┌──────────────────────────┐          │          │
│  │    │ TCP Socket               │          │          │
│  │    │ localhost:50000          │──────────┼───────►  │
│  │    └──────────────────────────┘          │          │
│  │                                           │          │
│  └──────────────────────────────────────────┘          │
│                                                         │
│  Also manages: alsamixer "Headphone" control           │
└─────────────────────────────────────────────────────────┘
                                                 │
                                                 │ MIDI
                                                 ▼
                                    ┌────────────────────────┐
                                    │ Rockit Synth           │
                                    │ (respeaker_rockit)     │
                                    │ TCP MIDI port 50000    │
                                    └────────────────────────┘
```

## Benefits

**Solves the noise issue**:
- Previously: Mopidy stops → releases Headphone control → releases mic device → codec changes state → noise → voice script reclaims mic → noise stops
- Now: Mopidy keeps BOTH Headphone control AND mic claimed → no release → no noise!

**Unified process**:
- Single Mopidy process handles both music playback and voice control
- No separate Python scripts to manage
- Integrates with existing Mopidy setup

## Troubleshooting

**Wake word not detected**:
- Check if ReSpeaker library is installed: `python -c "from respeaker import Microphone"`
- Verify mic is working: `arecord -d 2 test.wav && aplay test.wav`

**MIDI commands not working**:
- Confirm Rockit synth is running: `ps | grep respeaker_rockit`
- Test MIDI server: `echo -ne '\x90\x3C\x64' | nc localhost 50000`

**Mopidy doesn't load extension**:
- Run `mopidy deps` to check for missing dependencies
- Check config: `mopidy config` and verify `[rockit_voice]` section
- View logs: `mopidy -vv` for verbose output

## Credits

- Based on **mopidy-hallo** by ReSpeaker/Yihui Xiong
- Rockit synthesizer by Matt Heins / HackMe Electronics
- Modified for Rockit MIDI control by 1hedu

## License

Apache License, Version 2.0 (same as mopidy-hallo)
