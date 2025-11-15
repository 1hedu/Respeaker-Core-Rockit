# Rockit Synth Web UI

A web-based interface for controlling the Rockit synthesizer via TCP MIDI on the ReSpeaker.

## Features

- **Visual Keyboard**: 2-octave playable keyboard (C3-C5) with mouse/touch support
- **Computer Keyboard Support**: Play notes using your QWERTY keyboard
- **Filter Controls**: Cutoff and Resonance sliders
- **Envelope Controls**: Attack, Decay, Release sliders
- **Oscillator Controls**: Mix slider and Sub-oscillator toggle
- **LFO Controls**: Depth slider
- **Voice Mode Controls**: Mono/Para and 2-voice/3-voice toggles
- **Master Volume**: Overall volume control
- **Panic Button**: Emergency all-notes-off

## Installation

1. **Install Flask** (if not already installed):
   ```bash
   pip3 install flask
   ```

2. **Copy files to ReSpeaker**:
   ```bash
   scp synth_webui.py root@<respeaker-ip>:/root/
   scp -r templates root@<respeaker-ip>:/root/
   ```

3. **Make sure your Rockit synth is running** with TCP MIDI enabled on port 50000

## Usage

1. **Start the web server**:
   ```bash
   python3 synth_webui.py
   ```

2. **Open browser** and navigate to:
   ```
   http://<respeaker-ip>:8080
   ```

3. **Start playing!**
   - Click/touch keys on the visual keyboard
   - Use your computer keyboard (keys: a,w,s,e,d,f,t,g,y,h,u,j,k,o,l,p,;,')
   - Adjust sliders to control synth parameters
   - Toggle buttons for on/off controls

## MIDI CC Mapping

| Control | CC# | Range |
|---------|-----|-------|
| LFO Depth | 1 | 0-127 |
| Master Volume | 7 | 0-127 |
| Filter Cutoff | 74 | 0-127 |
| Filter Resonance | 71 | 0-127 |
| Osc Mix | 72 | 0-127 |
| Attack | 73 | 0-127 |
| Decay | 75 | 0-127 |
| Release | 70 | 0-127 |
| Sub-Osc | 76 | 0-63=Off, 64-127=On |
| Mono/Para | 102 | 0-63=Mono, 64-127=Para |
| Voice Count | 103 | 0-63=2-voice, 64-127=3-voice |

## Computer Keyboard Layout

```
 W E   T Y U   O P
A S D F G H J K L ; '
```

Maps to chromatic notes starting from C3.

## Running as a Service (Optional)

To auto-start the web UI on boot, create a systemd service:

```bash
sudo nano /etc/systemd/system/rockit-webui.service
```

Content:
```ini
[Unit]
Description=Rockit Synth Web UI
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/root
ExecStart=/usr/bin/python3 /root/synth_webui.py
Restart=always

[Install]
WantedBy=multi-user.target
```

Then:
```bash
sudo systemctl daemon-reload
sudo systemctl enable rockit-webui
sudo systemctl start rockit-webui
```

## Troubleshooting

- **Can't connect**: Make sure the Rockit synth is running with `--tcp-midi` flag
- **No sound**: Check that audio output is working and volume is up
- **UI not loading**: Verify Flask is installed and port 8080 is not blocked
- **Notes stuck on**: Click the PANIC button to reset all notes

## Architecture

```
Browser (Port 8080) → Flask Web Server → TCP MIDI (Port 50000) → Rockit Synth
```

The web UI sends 3-byte raw MIDI messages over TCP to localhost:50000, where the Rockit synth listens and processes them.
