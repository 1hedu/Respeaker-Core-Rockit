# Voice Control Setup Guide

## The Problem We're Solving

**Current situation**:
```
Mopidy running → Claims "Headphone" mixer
Mopidy stops   → Releases "Headphone" AND mic device
                → WM8960 codec changes state
                → AUDIO NOISE! 📢
voice_midi_controller.py starts → Reclaims mic
                → Noise stops (but wake word doesn't work properly)
```

**New solution with mopidy-rockit-voice**:
```
Mopidy starts → Claims BOTH "Headphone" AND microphone
              → Keeps them continuously
              → No release cycle = NO NOISE! ✅
              → Voice commands work properly ✅
```

## Installation

### 1. Install the Extension

On your ReSpeaker device:

```bash
cd /root/Respeaker-Core-Rockit/mopidy_rockit_voice
python setup.py install

# Or for development (changes reflected without reinstall):
python setup.py develop
```

### 2. Configure Mopidy

Edit your Mopidy config (wherever you showed it from earlier):

```bash
# Find your config location first
mopidy config

# Then edit it
vi /root/.config/mopidy/mopidy.conf
# (or wherever it actually is on your system)
```

Add this section:

```ini
[rockit_voice]
enabled = true
midi_host = localhost
midi_port = 50000
```

Your complete config should look something like:

```ini
[audio]
mixer = alsamixer

[alsamixer]
enabled = true
card = 0
control = Headphone

[rockit_voice]
enabled = true
midi_host = localhost
midi_port = 50000

# ... rest of your existing config ...
```

### 3. Verify Installation

Check if Mopidy recognizes the extension:

```bash
mopidy deps

# Should show:
# ...
# Mopidy-Rockit-Voice v0.1.0 ...
```

## Usage

### Start Everything

**Option 1: Mopidy handles everything**

```bash
# Start Rockit synth
cd /root
./respeaker_rockit --tcp-midi &

# Start Mopidy (which includes voice control)
mopidy --config /path/to/mopidy.conf

# That's it! Mopidy keeps the mic claimed continuously
```

**Option 2: Use your existing start_rockit.sh + Mopidy**

Edit `start_rockit.sh` to NOT kill Mopidy:

```bash
# Make sure this line stays commented:
# NOTE: DO NOT killall python - this kills Mopidy which causes audio noise!
```

Then:

```bash
# Terminal 1: Start Mopidy
mopidy --config /path/to/mopidy.conf

# Terminal 2: Start Rockit (without killing Mopidy)
./start_rockit.sh
```

### Use Voice Commands

1. **Say the wake word**: "respeaker"
   - LEDs should show "thinking" animation

2. **Give a command**:
   - "**play C**" - Plays middle C
   - "**play note E**" - Plays E
   - "**louder**" - Increase volume to ~85%
   - "**quieter**" - Decrease volume to 50%
   - "**filter up**" - Increase cutoff
   - "**filter down**" - Decrease cutoff
   - "**stop**" - All notes off

3. **LEDs react**:
   - Thinking (blue spinning) while listening
   - Speaking (green) while processing
   - Off when done

## Testing

### Check if Voice Extension is Running

```bash
# View Mopidy logs
mopidy -vv

# Should see:
# INFO     [RockitVoiceFrontend] Rockit Voice Control started. Listening for wake word...
# INFO     [RockitVoiceFrontend] MIDI target: localhost:50000
```

### Test MIDI Connection

```bash
# Manual MIDI test (while Rockit synth is running)
echo -ne '\x90\x3C\x64' | nc localhost 50000

# Should play middle C (note 60)
```

### Test Wake Word

```bash
# In Mopidy logs (run with mopidy -vv), you should see:
# When you say "respeaker":
INFO     [RockitVoiceFrontend] Wake word detected!

# When you say a command like "play C":
INFO     [RockitVoiceFrontend] Recognized: "play C"
INFO     [RockitVoiceFrontend] Matched command: play
INFO     [RockitVoiceFrontend] Playing note 60
```

## Extending Commands

Want to add more voice commands? Edit `mopidy_rockit_voice/__init__.py`:

### Example: Add "bass boost" command

```python
# In __init__ method, add to self.commands:
self.commands = {
    'play': self._cmd_play_note,
    'stop': self._cmd_stop_notes,
    'louder': self._cmd_volume_up,
    'quieter': self._cmd_volume_down,
    'filter up': self._cmd_filter_up,
    'filter down': self._cmd_filter_down,
    'bass boost': self._cmd_bass_boost,  # NEW
}

# Add the handler method:
def _cmd_bass_boost(self, text):
    """Enable sub-oscillator and boost low frequencies"""
    logger.info('Bass boost activated!')
    # Sub Osc On (CC 76, value >= 64)
    self._send_cc(76, 100)
    # Cutoff down a bit
    self._send_cc(74, 60)
```

Then reinstall:

```bash
cd mopidy_rockit_voice
python setup.py develop  # If using develop mode
# Or: python setup.py install

# Restart Mopidy
killall mopidy
mopidy --config /path/to/mopidy.conf
```

### MIDI CC Reference

See main [README.md](README.md) for complete list. Key ones:

| CC | Parameter | Range |
|----|-----------|-------|
| 7 | Master Volume | 0-127 |
| 70 | Release | 0-127 |
| 71 | Resonance | 0-127 |
| 72 | OSC Mix | 0-127 |
| 73 | Attack | 0-127 |
| 74 | Cutoff | 0-127 |
| 75 | Decay | 0-127 |
| 76 | Sub Osc | 64+ = on |
| 123 | All Notes Off | 0 |

## Troubleshooting

### "ReSpeaker library not available"

```bash
# Install respeaker library
pip install respeaker

# Or if it was part of vanilla ReSpeaker OS, check:
pip list | grep respeaker
```

### Wake word doesn't work

```bash
# Test microphone
arecord -d 3 test.wav && aplay test.wav

# Check if something else is using the mic
cat /proc/asound/card0/pcm0c/sub0/status
# Should show: state: RUNNING, owner_pid: <mopidy's PID>

# Check Mopidy process
ps | grep mopidy
```

### MIDI not working

```bash
# Is Rockit synth running?
ps | grep respeaker_rockit

# Can you connect to TCP MIDI?
nc -v localhost 50000
# (Ctrl+C to exit)

# Check Mopidy logs for MIDI errors
mopidy -vv 2>&1 | grep -i midi
```

### Noise still happens

If you still get noise:

1. **Check that Mopidy is actually running when you test**:
   ```bash
   ps | grep mopidy
   ```

2. **Verify the extension loaded**:
   ```bash
   mopidy deps | grep rockit
   ```

3. **Check if mic is claimed by Mopidy**:
   ```bash
   cat /proc/asound/card0/pcm0c/sub0/status
   # owner_pid should be Mopidy's PID
   ```

4. **Run the diagnostic**:
   ```bash
   cd diagnostics
   ./capture_noise_test.sh
   ```

## Comparison: Before vs After

### Before (with separate voice_midi_controller.py)

```
┌─────────────┐         ┌──────────────────┐
│   Mopidy    │         │ voice_midi_      │
│             │         │ controller.py    │
│ Claims:     │         │                  │
│ - Headphone │         │ Claims:          │
└─────────────┘         │ - Microphone     │
       ↓                └──────────────────┘
   When stopped:               ↓
   Releases both        Tries to claim mic
       ↓                       ↓
   Codec changes         Sometimes works,
   state → NOISE!        sometimes doesn't
```

### After (with mopidy-rockit-voice)

```
┌──────────────────────────────────┐
│         Mopidy Process           │
│                                  │
│  ┌────────────┐  ┌─────────────┐│
│  │ Music      │  │ Voice       ││
│  │ Playback   │  │ Control     ││
│  │            │  │             ││
│  │ Claims:    │  │ Claims:     ││
│  │ -Headphone │  │ -Microphone ││
│  └────────────┘  └─────────────┘│
│                                  │
│  Both stay claimed continuously  │
│  No release cycle = NO NOISE! ✅  │
└──────────────────────────────────┘
```

## Next Steps

1. **Test basic functionality**: Install, configure, test wake word
2. **Add your own commands**: Modify the code to add MIDI commands you want
3. **Integrate with your workflow**: Decide if you want Mopidy always running, or start/stop it
4. **Report findings**: Let me know if the noise issue is actually solved! 🎯

---

Created: 2025-11-16
For: Solving Mopidy/Microphone noise issue on ReSpeaker Core Rockit
