# Installing Keywords for Voice Control

## Quick Install

On your ReSpeaker device:

```bash
cd /root/Respeaker-Core-Rockit

# Pull latest changes
git pull

# Copy keywords to ReSpeaker library
cp Mopidy-Rockit-Voice/keywords.txt /usr/lib/python2.7/site-packages/respeaker/pocketsphinx-data/keywords.txt

# Reinstall extension
cd Mopidy-Rockit-Voice
python setup.py install

# Restart Mopidy
/etc/init.d/mopidy restart

# Watch logs
logread -f | grep mopidy
```

## Voice Commands

After saying **"respeaker"** wake word, you can say:

### Play Notes
- **"play c"** - Play middle C
- **"play e"** - Play E
- **"stop"** - Stop all notes

### Control Parameters
- **"cutoff up"** - Increase filter cutoff by ~15%
- **"cutoff down"** - Decrease filter cutoff
- **"cutoff max"** - Set cutoff to maximum (127)
- **"cutoff zero"** - Set cutoff to minimum (0)
- **"cutoff half"** - Set cutoff to 50% (64)

### Supported Parameters
- **volume** (CC 7) - Master volume
- **attack** (CC 73) - Envelope attack time
- **decay** (CC 75) - Envelope decay time
- **release** (CC 70) - Envelope release time
- **sustain** (CC 86) - Envelope sustain level
- **cutoff** (CC 74) - Filter cutoff frequency
- **resonance** (CC 71) - Filter resonance/Q
- **envelope** (CC 85) - Filter envelope amount
- **mix** (CC 72) - Oscillator mix
- **glide** (CC 90) - Portamento time

### Modifiers
- **up** - Increase by ~15% (20 MIDI units)
- **down** - Decrease by ~15% (20 MIDI units)
- **max** - Set to maximum (127)
- **min** / **zero** - Set to minimum (0)
- **half** - Set to 50% (64)
- **on** - Set to ~80% (100)
- **off** - Set to 0

## Examples

```
You say: "respeaker"
LEDs: Turn green (listening)

You say: "cutoff up"
System: Increases cutoff from 64 to 84
LEDs: Spin green (processing), then off

You say: "respeaker"
You say: "volume max"
System: Sets volume to 127
```

## Troubleshooting

**Wake word works but commands not recognized:**
- Check keywords.txt was copied: `cat /usr/lib/python2.7/site-packages/respeaker/pocketsphinx-data/keywords.txt`
- Should see all command words listed
- Restart Mopidy after copying

**Commands recognized but MIDI not working:**
- Check Rockit synth is running: `ps | grep respeaker_rockit`
- Test MIDI manually: `echo -ne '\xB0\x4A\x7F' | nc localhost 50000`

**Want to add more keywords:**
1. Edit `/usr/lib/python2.7/site-packages/respeaker/pocketsphinx-data/keywords.txt`
2. Add line: `yourword /1e-20/`
3. Restart Mopidy

---

For more details, see main [README.md](README.md)
