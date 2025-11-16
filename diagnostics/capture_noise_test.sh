#!/bin/bash
# Capture Noise Test - Simplified for BusyBox
# Direct investigation of microphone noise issue

echo "════════════════════════════════════════════════════════════════"
echo "  Microphone Noise Investigation"
echo "  $(date)"
echo "════════════════════════════════════════════════════════════════"
echo ""

CARD=0
OUTPUT_DIR="./noise_test_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTPUT_DIR"

echo "Output directory: $OUTPUT_DIR"
echo ""

# ============================================================================
# Helper Functions
# ============================================================================

snapshot_mixers() {
    local label="$1"
    local file="$OUTPUT_DIR/mixer_${label}.txt"

    echo "Taking mixer snapshot: $label"
    {
        echo "════════════════════════════════════════════════════════════════"
        echo "  Mixer Snapshot: $label"
        echo "  $(date)"
        echo "════════════════════════════════════════════════════════════════"
        echo ""
        amixer -c "$CARD"
    } > "$file"
    echo "  Saved: $file"
}

check_capture_device() {
    echo "Capture device status:"
    if [ -f /proc/asound/card0/pcm0c/sub0/status ]; then
        cat /proc/asound/card0/pcm0c/sub0/status
    else
        echo "  Status file not found"
    fi
    echo ""
}

# ============================================================================
# Test Sequence
# ============================================================================

echo "▶ INITIAL STATE"
echo "────────────────────────────────────────────────────────────────"
echo ""

snapshot_mixers "01_initial"
check_capture_device

echo ""
echo "▶ TEST 1: Capture Level Sweep"
echo "────────────────────────────────────────────────────────────────"
echo ""
echo "Testing if capture volume affects the noise..."
echo ""

# Get current capture volume
INITIAL_VOL=$(amixer -c "$CARD" sget Capture 2>/dev/null | grep -oP '\[\K[0-9]+(?=%\])' | head -1)
echo "Initial capture volume: ${INITIAL_VOL}%"
echo ""

# Test at 0% (your hypothesis)
echo "Setting capture to 0%..."
amixer -c "$CARD" sset Capture 0% 2>/dev/null
sleep 1
snapshot_mixers "02_capture_0pct"
echo ""
read -p "Is the noise gone at 0% capture? (y/n) " -n 1 -r NOISE_AT_ZERO
echo ""
echo "Response: $NOISE_AT_ZERO" > "$OUTPUT_DIR/noise_at_zero.txt"
echo ""

# Restore volume for other tests
if [ -n "$INITIAL_VOL" ]; then
    echo "Restoring capture to ${INITIAL_VOL}%..."
    amixer -c "$CARD" sset Capture "${INITIAL_VOL}%" 2>/dev/null
fi
echo ""

echo "▶ TEST 2: Mopidy Interaction"
echo "────────────────────────────────────────────────────────────────"
echo ""

read -p "Is Mopidy RUNNING now? (y/n) " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo ""
    echo "Taking snapshot with Mopidy running..."
    snapshot_mixers "03_mopidy_running"
    check_capture_device
    echo ""

    read -p "Now STOP Mopidy and press Enter when noise starts..." -r
    echo ""

    echo "Taking snapshot after Mopidy stopped..."
    snapshot_mixers "04_mopidy_stopped_noise"
    check_capture_device
    echo ""

    read -p "Does noise exist now? (y/n) " -n 1 -r NOISE_EXISTS
    echo ""
    echo "Noise present: $NOISE_EXISTS" > "$OUTPUT_DIR/noise_after_stop.txt"
    echo ""

    if [[ $NOISE_EXISTS =~ ^[Yy]$ ]]; then
        echo "▶ Finding what changed..."
        echo "──────────────────────────────────────────────────────────────"
        echo ""

        echo "Mixer differences:"
        diff "$OUTPUT_DIR/mixer_03_mopidy_running.txt" "$OUTPUT_DIR/mixer_04_mopidy_stopped_noise.txt" || true
        echo ""
    fi
else
    echo "Skipping Mopidy test - Mopidy not running"
fi
echo ""

echo "▶ TEST 3: Headphone Control Investigation"
echo "────────────────────────────────────────────────────────────────"
echo ""
echo "Mopidy uses 'Headphone' control. Let's test it directly..."
echo ""

echo "Current Headphone state:"
amixer -c "$CARD" sget Headphone 2>/dev/null || echo "Headphone control not found!"
echo ""

read -p "Try toggling Headphone mute? (y/n) " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo ""
    echo "Muting Headphone..."
    amixer -c "$CARD" sset Headphone mute 2>/dev/null
    sleep 1
    snapshot_mixers "05_headphone_muted"
    echo ""

    read -p "Did this affect the noise? (better/worse/same) " -r EFFECT
    echo "Headphone mute effect: $EFFECT" > "$OUTPUT_DIR/headphone_mute_effect.txt"
    echo ""

    echo "Unmuting Headphone..."
    amixer -c "$CARD" sset Headphone unmute 2>/dev/null
    sleep 1
    snapshot_mixers "06_headphone_unmuted"
    echo ""
fi

echo "▶ TEST 4: Looking for Loopback/Monitor Controls"
echo "────────────────────────────────────────────────────────────────"
echo ""

echo "Searching for loopback-related controls..."
amixer -c "$CARD" scontrols > "$OUTPUT_DIR/all_controls.txt"

echo "All controls saved to: $OUTPUT_DIR/all_controls.txt"
echo ""

echo "Potentially relevant controls:"
grep -i -E '(loop|monitor|sidetone|bypass|input.*playback|mic.*playback|adc.*dac)' "$OUTPUT_DIR/all_controls.txt" || echo "None found"
echo ""

# Check specific WM8960 controls that might exist
echo "Checking common WM8960 loopback controls..."
for ctrl in "Left Input Mixer Boost" "Right Input Mixer Boost" "Mono Output Mixer" "PCM Playback"; do
    if amixer -c "$CARD" sget "$ctrl" >/dev/null 2>&1; then
        echo "  Found: $ctrl"
        amixer -c "$CARD" sget "$ctrl"
        echo ""
    fi
done

echo "▶ FINAL STATE"
echo "────────────────────────────────────────────────────────────────"
echo ""
snapshot_mixers "99_final"

echo ""
echo "════════════════════════════════════════════════════════════════"
echo "  TEST COMPLETE"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "Results saved to: $OUTPUT_DIR"
echo ""
echo "Files created:"
ls -lh "$OUTPUT_DIR"
echo ""

echo "Summary of findings:"
echo "  1. Noise at 0% capture: $(cat $OUTPUT_DIR/noise_at_zero.txt 2>/dev/null || echo 'Not tested')"
if [ -f "$OUTPUT_DIR/noise_after_stop.txt" ]; then
    echo "  2. Noise after Mopidy stop: $(cat $OUTPUT_DIR/noise_after_stop.txt)"
fi
if [ -f "$OUTPUT_DIR/headphone_mute_effect.txt" ]; then
    echo "  3. Headphone mute effect: $(cat $OUTPUT_DIR/headphone_mute_effect.txt)"
fi
echo ""

echo "Review mixer snapshots to see what changed:"
echo "  diff $OUTPUT_DIR/mixer_03_mopidy_running.txt $OUTPUT_DIR/mixer_04_mopidy_stopped_noise.txt"
echo ""
