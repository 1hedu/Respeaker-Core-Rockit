# Rockit Synth Web UI - Minimal Edition

**Zero-dependency web interface for ReSpeaker Core v1.0**

This version uses ONLY Python standard library - no Flask, no pip installs, no extra storage needed!

## ✅ Why This Version?

- **Works on vanilla ReSpeaker** with no modifications
- **Single Python file** - all HTML embedded
- **Zero dependencies** - uses Python's built-in http.server
- **Tiny footprint** - fits easily in 32MB flash
- **Same features** as the Flask version

## 🚀 Quick Start

```bash
# 1. Copy files to ReSpeaker
scp synth_webui_minimal.py root@<respeaker-ip>:/root/
scp start_rockit_minimal.sh root@<respeaker-ip>:/root/

# 2. On ReSpeaker, make startup script executable
chmod +x start_rockit_minimal.sh

# 3. Start everything
./start_rockit_minimal.sh

# 4. Open browser
http://<respeaker-ip>:8080
```

## 🎹 Features

- **Visual keyboard** - 2 octaves (C3-C5)
- **Filter** - Cutoff, Resonance
- **Envelope** - Attack, Decay, Release
- **Oscillator** - Mix, Sub-oscillator toggle
- **LFO** - Depth control
- **Voice modes** - Mono/Para, 2/3-voice
- **Master volume**
- **Panic button**

## 📊 Storage Usage

- `synth_webui_minimal.py`: ~15KB
- `start_rockit_minimal.sh`: ~1KB
- Total: **~16KB** (0.05% of 32MB flash!)

## 🔧 Manual Start

If you prefer to start components separately:

```bash
# Terminal 1: Start synth
./respeaker_rockit --tcp-midi

# Terminal 2: Start web UI  
python synth_webui_minimal.py
```

## 🌐 MIDI CC Mapping

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

## 🐛 Troubleshooting

**Web UI won't start:**
- Check Python is available: `which python`
- Try `python2` or `python3` instead

**Can't connect to synth:**
- Verify synth is running: `ps | grep respeaker_rockit`
- Check TCP port: `netstat -ln | grep 50000`

**Browser can't reach page:**
- Find ReSpeaker IP: `ip addr show br-lan`
- Check firewall (unlikely on OpenWrt default)

## 📡 Architecture

```
Browser → HTTP (port 8080) → Python http.server → TCP MIDI (port 50000) → Rockit Synth
```

Everything runs locally on the ReSpeaker. The web UI is just a client that sends MIDI commands over TCP.

## 🆚 Flask Version vs Minimal Version

| Feature | Flask Version | Minimal Version |
|---------|--------------|-----------------|
| Dependencies | Flask required | None |
| Files | 3 files | 1 file |
| Storage | ~100KB+ | ~16KB |
| ReSpeaker v1.0 | May not fit | ✅ Fits perfectly |
| Features | Full | Full |

## 🎛️ Optional: Auto-Start on Boot

Add to `/etc/rc.local` (before `exit 0`):

```bash
cd /root
./start_rockit_minimal.sh &
```

Or create a proper init script if you prefer.

## 📝 Notes

- The HTML UI is embedded in the Python file as a string
- No external files needed beyond the single .py file
- Uses Python 2 compatible syntax for maximum compatibility
- Works with OpenWrt's default Python installation
