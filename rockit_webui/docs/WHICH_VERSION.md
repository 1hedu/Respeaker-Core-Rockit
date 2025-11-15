# Which Web UI Should You Use?

## 🎯 Quick Answer

**For ReSpeaker with lighttpd installed (most common):**
→ Use `rockit.html` + `midi_bridge.py` ⭐ BEST OPTION

**For vanilla systems without web server:**
→ Use `synth_webui_minimal.py` ✅

**For systems with extra storage:**
→ Use `synth_webui.py` (Flask version)

## 📊 Detailed Comparison

### Lighttpd Version (`rockit.html` + `midi_bridge.py`) ⭐

**Pros:**
- ✅ Uses existing lighttpd server (already on ReSpeaker!)
- ✅ Smallest footprint (~13KB total)
- ✅ Static HTML = fastest loading
- ✅ MIDI bridge is tiny (2KB, stdlib only)
- ✅ Professional separation: web server + API bridge
- ✅ Most efficient architecture

**Cons:**
- ⚠️ Requires lighttpd to be installed (but it usually is!)
- ⚠️ Slightly more setup (2 files instead of 1)

**Use when:**
- You have lighttpd installed (check: `which lighttpd`)
- You want the most professional setup
- You want the smallest footprint
- **This is the best option for ReSpeaker!**

### Minimal Version (`synth_webui_minimal.py`)

**Pros:**
- ✅ Zero dependencies - uses only Python stdlib
- ✅ Single file - easy to deploy
- ✅ Tiny footprint (~17KB total)
- ✅ Works on vanilla ReSpeaker Core v1.0
- ✅ Python 2 compatible

**Cons:**
- ⚠️ HTML embedded in Python string (harder to edit)
- ⚠️ Basic HTTP server (no advanced features)
- ⚠️ Slightly larger than lighttpd version

**Use when:**
- Lighttpd is NOT installed
- You want absolute simplicity (single file)
- You need quick deployment

### Flask Version (`synth_webui.py` + `templates/`)

**Pros:**
- ✅ Clean separation of HTML and Python
- ✅ Easy to customize UI
- ✅ Professional framework
- ✅ Better error handling

**Cons:**
- ⚠️ Requires Flask installation (`pip install flask`)
- ⚠️ Multiple files to manage
- ⚠️ May not fit on ReSpeaker v1.0 without SD card

**Use when:**
- You have extra storage (SD card)
- You're on a different Linux system
- You want to heavily customize the UI
- Flask is already installed

## 🎛️ Feature Comparison

| Feature | Lighttpd | Minimal | Flask |
|---------|----------|---------|-------|
| Keyboard | ✅ | ✅ | ✅ |
| All MIDI Controls | ✅ | ✅ | ✅ |
| Visual Design | ✅ | ✅ | ✅ |
| Mobile Support | ✅ | ✅ | ✅ |
| Dependencies | lighttpd (pre-installed) | None | Flask |
| File Count | 2 | 1 | 2+ |
| Size | **13KB** ⭐ | 17KB | ~100KB+ |
| Architecture | Professional | Simple | Framework |

**Bottom line:** All versions have identical UI and features!

## 🚀 Installation Guide

### For Lighttpd Version (⭐ RECOMMENDED)

```bash
# 1. Check if lighttpd is installed
which lighttpd

# 2. Find document root
cat /etc/lighttpd/lighttpd.conf | grep document-root
# Usually: /www or /var/www

# 3. Copy files
scp rockit.html root@<ip>:/www/
scp midi_bridge.py root@<ip>:/root/
scp start_rockit_lighttpd.sh root@<ip>:/root/

# 4. On ReSpeaker
chmod +x start_rockit_lighttpd.sh
./start_rockit_lighttpd.sh

# 5. Open browser
http://<respeaker-ip>/rockit.html
```

### For Flask Version

```bash
# Copy to device
scp synth_webui.py root@<ip>:/root/
scp -r templates root@<ip>:/root/

# On device
pip install flask  # or pip3
chmod +x start_rockit_webui.sh
./start_rockit_webui.sh
```

## 🔍 How to Check Available Space

```bash
# On ReSpeaker
df -h /

# If you see < 5MB free, use minimal version
# If you see > 20MB free, either version is fine
```

## 💡 Recommendation

**If lighttpd is installed → Use the lighttpd version!** ⭐

It's the smallest, fastest, and most professional setup. Since most ReSpeakers have lighttpd pre-installed, this is usually the best choice.

**If no web server → Use the minimal version.**

It works everywhere and requires zero setup.

**Need heavy customization → Use Flask.**

But only if you have extra storage and don't mind installing dependencies.

## 📝 Files Summary

**Lighttpd Setup:** ⭐
- `rockit.html` - Static HTML UI
- `midi_bridge.py` - TCP MIDI forwarder (2KB)
- `start_rockit_lighttpd.sh` - Startup script
- `README_LIGHTTPD.md` - Documentation

**Flask Setup:**
- `synth_webui.py` - Flask web server
- `templates/index.html` - UI template
- `start_rockit_webui.sh` - Startup script
- `setup_webui.sh` - Installs Flask
- `ROCKIT_WEBUI_README.md` - Documentation

Both connect the same way: Browser → HTTP → TCP MIDI → Synth
