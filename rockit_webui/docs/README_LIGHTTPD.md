# Rockit Synth Web UI - Lighttpd Version

**Ultra-lightweight setup using your existing lighttpd server!**

Since ReSpeaker already has lighttpd installed, we can leverage it to serve a static HTML file. This is the most efficient approach.

## 🎯 Architecture

```
Browser → lighttpd (port 80) → rockit.html
                              ↓ (JavaScript AJAX)
                     midi_bridge.py (port 8090)
                              ↓ (TCP MIDI)
                     Rockit Synth (port 50000)
```

## 📦 Files

1. **rockit.html** - Static HTML/CSS/JS (served by lighttpd)
2. **midi_bridge.py** - Tiny Python bridge (2KB, stdlib only)

## 🚀 Installation

### Step 1: Find lighttpd document root

```bash
# On ReSpeaker, check config
cat /etc/lighttpd/lighttpd.conf | grep document-root

# Usually it's one of:
# /www
# /var/www  
# /srv/http
```

Let's assume it's `/www` (most common on OpenWrt).

### Step 2: Deploy HTML

```bash
# Copy HTML to web root
scp rockit.html root@<respeaker-ip>:/www/

# Or if different root:
scp rockit.html root@<respeaker-ip>:/var/www/
```

### Step 3: Deploy MIDI bridge

```bash
# Copy Python bridge script
scp midi_bridge.py root@<respeaker-ip>:/root/

# On ReSpeaker, make it executable
chmod +x /root/midi_bridge.py
```

### Step 4: Start services

```bash
# Terminal 1: Start Rockit synth
./respeaker_rockit --tcp-midi &

# Terminal 2: Start MIDI bridge
python /root/midi_bridge.py &
```

### Step 5: Access the UI

Open browser to:
```
http://<respeaker-ip>/rockit.html
```

That's it! Lighttpd serves the HTML, JavaScript talks to the MIDI bridge, bridge forwards to synth.

## 🔧 Auto-Start on Boot

Create `/etc/init.d/rockit-synth`:

```bash
#!/bin/sh /etc/rc.common

START=99
STOP=10

start() {
    # Start synth
    /root/respeaker_rockit --tcp-midi &
    
    # Start MIDI bridge
    python /root/midi_bridge.py &
}

stop() {
    killall respeaker_rockit
    killall midi_bridge.py
}
```

Then enable:
```bash
chmod +x /etc/init.d/rockit-synth
/etc/init.d/rockit-synth enable
/etc/init.d/rockit-synth start
```

## 📊 Storage Usage

- **rockit.html**: ~11KB
- **midi_bridge.py**: ~2KB
- **Total**: ~13KB (0.04% of 32MB flash!)

## 🎛️ Features

All the same features as before:
- Visual keyboard (2 octaves)
- Filter, Envelope, Oscillator, LFO controls
- Voice modes
- Master volume
- Panic button
- Mobile/touch support

## 🔍 Troubleshooting

**HTML not loading:**
```bash
# Check lighttpd is running
ps | grep lighttpd

# Check document root
cat /etc/lighttpd/lighttpd.conf | grep document-root

# Restart lighttpd if needed
/etc/init.d/lighttpd restart
```

**MIDI bridge connection error:**
```bash
# Check bridge is running
ps | grep midi_bridge

# Check it's listening
netstat -ln | grep 8090

# Check synth is running
ps | grep respeaker_rockit
netstat -ln | grep 50000
```

**CORS issues:**
- MIDI bridge already includes CORS headers
- Should work from any browser

## 🆚 Why This Is Better

| Approach | HTML Server | Python Size | Total Footprint |
|----------|-------------|-------------|-----------------|
| Minimal Python | Python stdlib | 17KB | 17KB |
| Flask | Flask | 100KB+ | 120KB+ |
| **Lighttpd** | Already installed! | 2KB | **13KB** ✅ |

**Winner:** Lighttpd approach is smallest and fastest!

## 💡 Advanced: Multiple Synths

Since the HTML is static, you can control multiple synths by:

1. Run multiple synth instances on different MIDI ports
2. Run multiple MIDI bridges on different HTTP ports
3. Create multiple HTML files pointing to different bridges

## 📝 Notes

- lighttpd serves static files incredibly efficiently
- MIDI bridge only uses Python stdlib (BaseHTTPServer)
- No Flask, no external dependencies
- Perfect for embedded systems like ReSpeaker
- Can access from any device on network
- Works on mobile browsers too

## 🎹 Quick Commands Reference

```bash
# Start everything
./respeaker_rockit --tcp-midi &
python midi_bridge.py &

# Stop everything  
killall respeaker_rockit midi_bridge.py

# Check status
ps | grep -E "respeaker|midi_bridge"
netstat -ln | grep -E "8090|50000"
```

Access at: `http://<respeaker-ip>/rockit.html`
