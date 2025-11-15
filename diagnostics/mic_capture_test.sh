#!/bin/bash
# Microphone Capture Level Testing Script
# Tests different capture volumes and checks for noise issues

echo "════════════════════════════════════════════════════════════════"
echo "  Microphone Capture Test Suite"
echo "  $(date)"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Configuration
CARD="${1:-0}"  # Default to card 0
OUTPUT_DIR="./mic_tests_$(date +%Y%m%d_%H%M%S)"
DURATION=3      # Recording duration in seconds
SAMPLE_RATE=48000

# Create output directory
mkdir -p "$OUTPUT_DIR"
echo "Output directory: $OUTPUT_DIR"
echo ""

# ============================================================================
# Helper Functions
# ============================================================================

# Function to check if device is in use
check_device_available() {
    local card=$1
    echo "Checking if capture device is available..."

    fuser /dev/snd/pcmC${card}D0c 2>&1
    if [ $? -eq 0 ]; then
        echo "WARNING: Device is currently in use!"
        echo "Processes using capture device:"
        fuser -v /dev/snd/pcmC${card}D0c 2>&1
        echo ""
        read -p "Continue anyway? (y/N) " -n 1 -r
        echo ""
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    else
        echo "✓ Capture device appears to be free"
    fi
    echo ""
}

# Function to set capture volume
set_capture_volume() {
    local card=$1
    local volume=$2

    echo "Setting capture volume to $volume%..."

    # Try common control names
    amixer -c "$card" sset Capture "${volume}%" 2>/dev/null || \
    amixer -c "$card" sset Mic "${volume}%" 2>/dev/null || \
    amixer -c "$card" sset 'Mic Boost' "${volume}%" 2>/dev/null

    echo ""
}

# Function to get current capture volume
get_capture_volume() {
    local card=$1

    amixer -c "$card" sget Capture 2>/dev/null | grep -oP '\[\K[0-9]+' | head -1 || \
    amixer -c "$card" sget Mic 2>/dev/null | grep -oP '\[\K[0-9]+' | head -1 || \
    echo "?"
}

# Function to record audio
record_test() {
    local card=$1
    local volume=$2
    local filename="$3"

    echo "Recording ${DURATION}s at ${volume}% capture volume..."
    echo "  Output: $filename"

    arecord -D hw:${card},0 \
        -f S16_LE \
        -r ${SAMPLE_RATE} \
        -c 2 \
        -d ${DURATION} \
        "$filename" 2>&1 | grep -v "^Recording"

    if [ $? -eq 0 ]; then
        echo "✓ Recording complete"

        # Basic analysis
        analyze_recording "$filename"
    else
        echo "✗ Recording failed!"
    fi
    echo ""
}

# Function to analyze recording
analyze_recording() {
    local filename="$1"

    echo "  Analysis:"

    # File size
    local size=$(ls -lh "$filename" | awk '{print $5}')
    echo "    Size: $size"

    # Check if file contains actual data (not all zeros/silence)
    local nonzero=$(xxd "$filename" | grep -v "0000 0000 0000 0000" | wc -l)
    if [ "$nonzero" -gt 10 ]; then
        echo "    Content: Contains audio data"
    else
        echo "    Content: Appears silent/empty"
    fi

    # Calculate approximate RMS if sox is available
    if command -v sox >/dev/null 2>&1; then
        local stats=$(sox "$filename" -n stat 2>&1)
        local rms=$(echo "$stats" | grep "RMS.*amplitude" | awk '{print $3}')
        local max=$(echo "$stats" | grep "Maximum amplitude" | awk '{print $3}')
        echo "    RMS level: $rms"
        echo "    Max amplitude: $max"
    fi
}

# ============================================================================
# Main Test Sequence
# ============================================================================

echo "▶ TEST CONFIGURATION"
echo "────────────────────────────────────────────────────────────────"
echo "Card: $CARD"
echo "Duration: ${DURATION}s per test"
echo "Sample rate: ${SAMPLE_RATE}Hz"
echo "Output: $OUTPUT_DIR"
echo ""

echo "▶ INITIAL STATE"
echo "────────────────────────────────────────────────────────────────"

# Check device availability
check_device_available "$CARD"

# Show current settings
echo "Current ALSA mixer state:"
amixer -c "$CARD" 2>/dev/null
echo ""

# Save initial volume
INITIAL_VOLUME=$(get_capture_volume "$CARD")
echo "Initial capture volume: ${INITIAL_VOLUME}%"
echo ""

# ============================================================================
# Test 1: Baseline at current volume
# ============================================================================
echo "▶ TEST 1: Baseline Recording (current settings)"
echo "────────────────────────────────────────────────────────────────"
current_vol=$(get_capture_volume "$CARD")
record_test "$CARD" "$current_vol" "$OUTPUT_DIR/01_baseline_vol${current_vol}.wav"

# ============================================================================
# Test 2: Volume sweep
# ============================================================================
echo "▶ TEST 2: Volume Sweep"
echo "────────────────────────────────────────────────────────────────"
echo "Testing at different capture volumes to identify noise threshold"
echo ""

for vol in 0 25 50 75 100; do
    echo "── Test 2.$vol: Volume at ${vol}% ──"
    set_capture_volume "$CARD" "$vol"
    sleep 1  # Let it settle
    record_test "$CARD" "$vol" "$OUTPUT_DIR/02_sweep_vol${vol}.wav"
done

# ============================================================================
# Test 3: Before/After Mopidy test
# ============================================================================
echo "▶ TEST 3: Mopidy Interaction Test"
echo "────────────────────────────────────────────────────────────────"
echo "This test requires manual Mopidy control"
echo ""

# Restore a reasonable volume
set_capture_volume "$CARD" 50
sleep 1

echo "Step 1: Recording with Mopidy running..."
echo "  (Make sure Mopidy is running and PLAYING)"
read -p "Press Enter when Mopidy is PLAYING..."
record_test "$CARD" 50 "$OUTPUT_DIR/03_with_mopidy_playing.wav"

echo "Step 2: Recording with Mopidy stopped..."
echo "  (Make sure you STOP Mopidy playback - not pause)"
read -p "Press Enter when Mopidy is STOPPED..."
record_test "$CARD" 50 "$OUTPUT_DIR/04_with_mopidy_stopped.wav"

echo "Step 3: Recording after Mopidy service killed..."
echo "  (Optional: completely kill Mopidy service)"
read -p "Press Enter after killing Mopidy (or skip with Ctrl+C)..."
record_test "$CARD" 50 "$OUTPUT_DIR/05_after_mopidy_killed.wav"

# ============================================================================
# Test 4: Device reclaim test
# ============================================================================
echo "▶ TEST 4: Device State Test"
echo "────────────────────────────────────────────────────────────────"
echo "Recording while monitoring device state..."
echo ""

set_capture_volume "$CARD" 50

# Show what's using the device before
echo "Before recording:"
fuser -v /dev/snd/pcmC${CARD}D0c 2>&1
echo ""

# Start monitoring in background
(
    while true; do
        echo "[$(date +%H:%M:%S)] Device users:"
        fuser /dev/snd/pcmC${CARD}D0c 2>&1
        sleep 1
    done
) > "$OUTPUT_DIR/device_state_log.txt" 2>&1 &
MONITOR_PID=$!

# Record
record_test "$CARD" 50 "$OUTPUT_DIR/06_with_monitoring.wav"

# Stop monitoring
kill $MONITOR_PID 2>/dev/null
wait $MONITOR_PID 2>/dev/null

echo "Device state log saved to: $OUTPUT_DIR/device_state_log.txt"
echo ""

# ============================================================================
# Restore and Summary
# ============================================================================
echo "▶ CLEANUP"
echo "────────────────────────────────────────────────────────────────"

# Restore initial volume
if [ -n "$INITIAL_VOLUME" ] && [ "$INITIAL_VOLUME" != "?" ]; then
    echo "Restoring initial capture volume: ${INITIAL_VOLUME}%"
    set_capture_volume "$CARD" "$INITIAL_VOLUME"
fi

echo ""
echo "════════════════════════════════════════════════════════════════"
echo "  TESTS COMPLETE"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "Results saved to: $OUTPUT_DIR"
echo ""
echo "Files created:"
ls -lh "$OUTPUT_DIR"
echo ""
echo "Next steps:"
echo "  1. Listen to the recordings to identify when noise occurs"
echo "  2. Compare before/after Mopidy stop"
echo "  3. Check if volume=0 eliminates the noise"
echo "  4. Review device_state_log.txt for ownership changes"
echo ""
echo "To listen to a recording:"
echo "  aplay $OUTPUT_DIR/<filename>.wav"
echo ""
