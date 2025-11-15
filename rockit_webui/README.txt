╔═══════════════════════════════════════════════════════════════╗
║                   ROCKIT SYNTH WEB UI PACKAGE                 ║
║                         Complete Bundle                        ║
╚═══════════════════════════════════════════════════════════════╝

📦 PACKAGE CONTENTS:

  📁 lighttpd_version/        ⭐ RECOMMENDED FOR RESPEAKER
     - rockit.html            Static HTML UI (15KB)
     - midi_bridge.py         MIDI forwarder (3KB)
     - start_rockit_lighttpd.sh

  📁 minimal_version/         For systems without web server
     - synth_webui_minimal.py All-in-one server (17KB)
     - start_rockit_minimal.sh

  📁 flask_version/           For systems with extra storage
     - synth_webui.py         Flask server
     - templates/             HTML templates
     - start_rockit_webui.sh
     - setup_webui.sh

  📁 docs/                    Documentation
     - WHICH_VERSION.md       Choose which version to use
     - README_LIGHTTPD.md     Lighttpd setup guide
     - README_MINIMAL.md      Minimal version guide
     - ROCKIT_WEBUI_README.md Flask version guide

  📄 START_HERE.txt           Quick start guide


🚀 QUICK START:

1. READ: START_HERE.txt (you are here!)
2. DECIDE: Check docs/WHICH_VERSION.md to pick your version
3. DEPLOY: Follow the instructions for your chosen version

For ReSpeaker with lighttpd (most common):
  → Use lighttpd_version/ ⭐


🎹 FEATURES (All Versions):

  ✅ Visual keyboard (2 octaves, C3-C5)
  ✅ Filter controls (cutoff, resonance)
  ✅ Envelope (attack, decay, release)
  ✅ Oscillator (mix, sub-oscillator)
  ✅ LFO depth control
  ✅ Voice modes (mono/para, 2/3-voice)
  ✅ Master volume
  ✅ Panic button
  ✅ Mobile/touch support


📊 SIZE COMPARISON:

  Lighttpd:  13KB  ⭐ Smallest, fastest
  Minimal:   17KB  Single file, no deps
  Flask:    100KB+ Best for customization


🔧 REQUIREMENTS:

  All versions:
  - Rockit synth running with --tcp-midi flag
  - Python (comes with ReSpeaker)

  Lighttpd version:
  - lighttpd web server (pre-installed on ReSpeaker)

  Flask version:
  - Flask package (pip install flask)


📡 ARCHITECTURE:

Browser sends AJAX requests → MIDI bridge → TCP MIDI (port 50000) → Synth

The web UI never directly touches the synth. It just sends MIDI CC
and note messages over TCP to port 50000 where the synth listens.


💡 TIPS:

- Start with lighttpd version if you have a ReSpeaker
- Use minimal version for quick deployment on any system
- All versions have identical features and UI
- Can access from any device on your network
- Works great on mobile browsers


🆘 TROUBLESHOOTING:

1. Web UI won't load
   → Check that web server is running
   → Verify correct port and IP address

2. UI loads but no sound
   → Check synth is running: ps | grep respeaker_rockit
   → Verify TCP port: netstat -ln | grep 50000
   → Try panic button then play notes again

3. Notes stuck on
   → Click the PANIC button in the UI
   → Or restart the synth

4. Still having issues?
   → Read the detailed docs in docs/ folder
   → Check that midi_bridge.py is running (lighttpd version)


📝 NOTES:

- This package works with the Rockit synthesizer engine
- Make sure your synth is compiled and running with --tcp-midi
- All MIDI communication happens over TCP on localhost:50000
- The web UI is just a pretty frontend that sends MIDI messages


🎵 ENJOY YOUR ROCKIT SYNTH! 🎹
