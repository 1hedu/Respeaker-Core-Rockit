# Audio Diagnostics Suite

Comprehensive diagnostic tools for investigating the Mopidy/Microphone interaction issue on ReSpeaker Core.

## The Problem

Based on observations and research:

1. **Mopidy releases microphone**: When Mopidy is stopped (not paused), it releases the ALSA audio device
2. **Noise occurs**: After Mopidy releases the device, a noise/blaring sound occurs
3. **ReSpeaker library reclaim**: Running scripts using the ReSpeaker Python library (with PyAudio) would reclaim the mic and stop the noise
4. **Wake word issues**: After reclaiming, wake word detection would trigger but commands wouldn't be processed properly

### Known Issues from Research

- **Mopidy ALSA release bug**: There's a documented issue where Mopidy doesn't always properly release the ALSA device on stop ([Mopidy Discourse #3372](https://discourse.mopidy.com/t/mopidy-doesnt-release-sound-card-on-stop/3372))
- **ReSpeaker library**: Uses PyAudio + PocketSphinx for wake word detection, searches for devices with 'respeaker' in the name
- **Workaround**: Need to use ALSA dmix for device sharing, or ensure proper device initialization

## Important Notes

**BusyBox Environment**: Some scripts use commands not available in BusyBox (like `fuser`, `ps -a`). New scripts below are BusyBox-compatible.

**About mic_capture_test.sh**: This script records audio samples from the microphone - it does NOT play audio through Mopidy. When it prompts "Press Enter when Mopidy is PLAYING", it means Mopidy should be playing music (to test if recording works while Mopidy uses the audio device). If Mopidy never plays in your setup, use the simpler `capture_noise_test.sh` instead.

## Diagnostic Tools

### NEW: `capture_noise_test.sh` - Simplified Noise Investigation (BusyBox-compatible)

**Purpose**: Direct, interactive investigation of the microphone noise issue with minimal dependencies.

**What it tests**:
- **Capture volume hypothesis**: Does setting capture to 0% eliminate noise?
- **Mopidy interaction**: Takes mixer snapshots before/after Mopidy stop
- **Headphone control**: Tests if muting the "Headphone" control affects noise
- **Loopback detection**: Searches for mixer controls that might route mic to output

**Usage**:
```bash
cd diagnostics
chmod +x capture_noise_test.sh
./capture_noise_test.sh
```

**Interactive prompts**:
- Asks if Mopidy is running
- Prompts you to stop Mopidy and observe noise
- Tests different mixer settings
- Records your observations

**Output**: Creates `noise_test_YYYYMMDD_HHMMSS/` with mixer snapshots and diff comparisons

**When to use**: This is the **easiest starting point** for investigation!

---

### NEW: `mopidy_mixer_investigation.sh` - Deep Dive into Mixer Controls (BusyBox-compatible)

**Purpose**: Comprehensive analysis of ALSA mixer controls, focusing on the Mopidy "Headphone" control relationship.

**What it shows**:
- All ALSA mixer controls (simple and detailed views)
- Headphone control state (what Mopidy controls)
- All capture/microphone controls
- Search for loopback/monitoring paths
- WM8960 codec-specific information
- Process check (BusyBox-compatible method)

**Usage**:
```bash
cd diagnostics
chmod +x mopidy_mixer_investigation.sh
./mopidy_mixer_investigation.sh
```

**Interactive test**: Optionally takes snapshots before/after you stop Mopidy to show exactly what changed.

**When to use**: When you need detailed information about all available mixer controls and their current states.

---

### 1. `alsa_state_check.sh` - System State Snapshot

**Purpose**: Comprehensive one-time check of the entire ALSA audio subsystem state.

**What it checks**:
- All ALSA devices and capabilities
- Processes using audio devices (via `fuser` and `lsof`)
- PCM device status from `/proc/asound`
- All mixer controls and current levels
- ALSA configuration files (`asound.conf`, `.asoundrc`)
- Mopidy status and configuration
- Kernel modules and device permissions
- Recent audio-related kernel messages

**Usage**:
```bash
cd diagnostics
chmod +x alsa_state_check.sh
./alsa_state_check.sh

# Save to log file
./alsa_state_check.sh | tee logs/state_$(date +%Y%m%d_%H%M%S).log
```

**When to run**:
- Before starting any tests (baseline)
- After Mopidy stops (to see what changed)
- After running ReSpeaker library scripts
- When investigating why wake word stopped working

---

### 2. `audio_monitor.sh` - Real-time Device Monitoring

**Purpose**: Continuously monitor ALSA device state changes in real-time.

**What it monitors**:
- Which processes are holding `/dev/snd` devices
- PCM device states (RUNNING, PREPARED, CLOSED)
- Capture volume levels
- Relevant processes (mopidy, python, arecord, etc.)

**Usage**:
```bash
cd diagnostics
chmod +x audio_monitor.sh

# Interactive mode (clears screen, updates every 2s)
./audio_monitor.sh

# Log mode (append to file, no screen clear)
./audio_monitor.sh --log

# Run once and exit
./audio_monitor.sh --once
```

**When to run**:
- Run in background terminal while reproducing the issue
- During Mopidy start/stop cycles
- While testing ReSpeaker library scripts

---

### 3. `mic_capture_test.sh` - Microphone Test Suite

**Purpose**: Systematically test microphone capture at different volumes and scenarios to identify when noise occurs.

**Tests performed**:
1. **Baseline**: Record at current volume settings
2. **Volume sweep**: Test at 0%, 25%, 50%, 75%, 100% to find noise threshold
3. **Mopidy interaction**: Record before/during/after Mopidy stop
4. **Device state**: Monitor device ownership during recording

**Usage**:
```bash
cd diagnostics
chmod +x mic_capture_test.sh

# Test default card (card 0)
./mic_capture_test.sh

# Test specific card
./mic_capture_test.sh 1
```

**Output**:
- Creates `mic_tests_YYYYMMDD_HHMMSS/` directory
- Multiple `.wav` files for different test scenarios
- `device_state_log.txt` showing device ownership changes

**What to check**:
1. Listen to the recordings: `aplay mic_tests_*/01_baseline_*.wav`
2. Compare recordings before/after Mopidy stop
3. Check if volume=0 eliminates noise
4. Review device state logs

---

### 4. `pyaudio_mic_test.py` - PyAudio Device Claim Test

**Purpose**: Simulate what the ReSpeaker Python library does when it claims the microphone using PyAudio.

**Features**:
- Lists all audio devices (highlights ReSpeaker devices)
- Auto-detects ReSpeaker devices (same logic as respeaker_python_library)
- Opens audio stream and captures data
- Shows RMS levels to verify data is being captured
- Interactive menu for different test scenarios

**Usage**:
```bash
cd diagnostics

# If PyAudio not installed
pip install pyaudio  # or pip2 install pyaudio for Python 2

# Run the test
python pyaudio_mic_test.py
```

**Interactive options**:
1. Test capture with auto-detected ReSpeaker device (10s)
2. Test capture with specific device index
3. Extended duration test (custom duration)
4. Re-list all devices

**When to run**:
- After Mopidy stops and noise occurs
- To verify if claiming the device stops the noise
- To test if the device can be opened while noise is happening

---

## Investigation Workflow

### **RECOMMENDED: Quick Start Investigation**

```bash
cd diagnostics

# Step 1: Run the simplified noise test (easiest!)
./capture_noise_test.sh

# This will:
# - Test if capture volume=0 stops the noise (your hypothesis!)
# - Prompt you to stop Mopidy and observe
# - Take mixer snapshots to see what changed
# - Search for loopback controls

# Step 2: If you need more details
./mopidy_mixer_investigation.sh

# This shows all mixer controls and helps identify
# what the "Headphone" control (used by Mopidy) does
```

**What to look for**:
1. Does noise go away at 0% capture? → Confirms your hypothesis
2. What mixer values changed when Mopidy stopped? → Check the diff output
3. Are there loopback/monitor controls? → Might be routing mic to output
4. Does muting "Headphone" affect noise? → Shows if output affects input

---

### Scenario 1: Fresh Investigation

```bash
# Terminal 1: Baseline state
./alsa_state_check.sh | tee logs/01_baseline.log

# Terminal 1: Start monitoring
./audio_monitor.sh --log

# Terminal 2: Start Mopidy (if not running)
systemctl start mopidy  # or however you start it

# Terminal 2: Play something, then STOP (not pause)

# Terminal 3: Check state after stop
./alsa_state_check.sh | tee logs/02_after_mopidy_stop.log

# Terminal 3: Run capture tests
./mic_capture_test.sh

# Compare logs
diff logs/01_baseline.log logs/02_after_mopidy_stop.log
```

### Scenario 2: Testing the "Reclaim" Hypothesis

```bash
# Terminal 1: Monitor in real-time
./audio_monitor.sh

# Terminal 2: Cause the issue (stop Mopidy)

# Terminal 2: Observe noise, then run PyAudio test
python pyaudio_mic_test.py
# Choose option 1 to test with ReSpeaker device

# Observe in Terminal 1 if device state changes
# Check if noise stops during PyAudio capture
```

### Scenario 3: Volume Level Testing

```bash
# Run the capture test suite
./mic_capture_test.sh

# This will:
# 1. Record baseline
# 2. Test at volume 0% (does noise go away?)
# 3. Test at various levels
# 4. Prompt you to interact with Mopidy

# Listen to results
cd mic_tests_*/
aplay 01_baseline_*.wav
aplay 02_sweep_vol0.wav    # Does vol=0 eliminate noise?
aplay 03_with_mopidy_playing.wav
aplay 04_with_mopidy_stopped.wav
```

---

## What to Look For

### In `alsa_state_check.sh` output:

**Section 2 (Device Ownership)**:
- Which process is holding the capture device?
- Does the device show as "in use" or free?

**Section 3 (PCM Status)**:
- Is the capture device in `RUNNING`, `PREPARED`, or `CLOSED` state?
- Check `hw_params` - are they set or unset?

**Section 4 (Mixer Levels)**:
- What is the capture volume level?
- Are there multiple capture controls?

**Section 6 (Mopidy Config)**:
- What ALSA device is Mopidy using?
- Is it using `default`, `hw:X,Y`, or `dmix`?

### In `audio_monitor.sh` output:

- **State transitions**: Watch for state changes from CLOSED → PREPARED → RUNNING
- **Process changes**: When does Mopidy release the device? Does another process grab it?
- **Timestamp correlation**: Match state changes with when noise starts/stops

### In `mic_capture_test.sh` results:

**Audio files**:
- Do recordings at volume=0 have noise?
- Is there a volume threshold where noise appears?
- Do recordings differ before/after Mopidy stop?

**RMS levels**:
- Normal speech/silence: RMS ~100-500
- Loud noise: RMS >1000
- Dead silence: RMS <10

### In `pyaudio_mic_test.py` output:

**Device listing**:
- Is ReSpeaker device detected?
- What's the device index?

**During capture**:
- Does the noise stop when PyAudio opens the stream?
- Are RMS levels reasonable or showing noise?
- Can the device be opened at all?

---

## Common Findings & Solutions

### Finding: Device shows as "in use" but no process listed
**Possible cause**: Device in PREPARED state but not RUNNING
**Solution**: May need to reset ALSA state or reboot

### Finding: Capture volume at 100%, noise present
**Test**: Lower to 0% or 50%
**Hypothesis from user**: "i think if i turn down mic capture all the way... it goes away"

### Finding: Mopidy using `hw:0,0` directly
**Issue**: Exclusive access, blocks other apps
**Solution**: Configure Mopidy to use `dmix` device for sharing

### Finding: PyAudio can't open device "Device or resource busy"
**Cause**: Another process holding exclusive access
**Solution**: Find and stop the blocking process

### Finding: After PyAudio closes, noise returns
**Cause**: Device left in weird state
**Solution**: Need proper ALSA reset/initialization sequence

---

## Logs Directory Structure

Recommended organization:

```
diagnostics/
├── logs/
│   ├── 01_baseline.log
│   ├── 02_after_mopidy_stop.log
│   ├── 03_after_pyaudio_reclaim.log
│   └── audio_monitor_YYYYMMDD_HHMMSS.log
├── mic_tests_YYYYMMDD_HHMMSS/
│   ├── 01_baseline_vol50.wav
│   ├── 02_sweep_vol0.wav
│   ├── 03_with_mopidy_playing.wav
│   ├── 04_with_mopidy_stopped.wav
│   └── device_state_log.txt
└── [scripts]
```

---

## Next Steps After Diagnosis

Once you've identified the pattern:

1. **If noise correlates with capture volume**:
   - Test if volume=0 or mute eliminates it
   - Configure system to auto-mute capture when not in use

2. **If noise occurs when device is "released"**:
   - Test ALSA dmix configuration for device sharing
   - Ensure Mopidy uses dmix, not direct `hw:` access
   - Keep a "dummy" capture process running to hold the device

3. **If ReSpeaker library fixes it**:
   - Analyze what PyAudio initialization does differently
   - May need to reset/reinitialize ALSA after Mopidy stops
   - Consider having a persistent capture stream

4. **If wake word fails after reclaim**:
   - Check if audio data is flowing correctly
   - Verify PocketSphinx is receiving valid samples
   - May need to restart wake word service, not just reclaim device

---

## Additional Tools

If needed, additional diagnostic commands:

```bash
# Show all ALSA PCM devices
cat /proc/asound/pcm

# Show detailed card info
cat /proc/asound/card0/codec#0  # if exists

# Test playback device
speaker-test -c 2 -t wav -l 1

# Verify capture is working
arecord -d 5 test.wav && aplay test.wav

# Check for buffer overruns
dmesg | grep -i xrun

# See what GStreamer (used by Mopidy) thinks
gst-inspect-1.0 alsasrc
```

---

## Notes

- All scripts designed for **OpenWrt / ReSpeaker Core** environment
- Requires root or audio group membership
- Some tools may not be available (like `lsof`) - scripts handle gracefully
- Recordings are **not** saved by `pyaudio_mic_test.py` (use `mic_capture_test.sh` for that)

---

Created: 2025-11-15
For: Mopidy/Microphone noise investigation on ReSpeaker Core Rockit project
