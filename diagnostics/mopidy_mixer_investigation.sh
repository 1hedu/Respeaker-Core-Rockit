#!/bin/bash
# Mopidy Alsamixer Investigation Script
# Focused investigation of Headphone control and microphone interaction

echo "════════════════════════════════════════════════════════════════"
echo "  Mopidy Alsamixer Investigation"
echo "  Investigating: Headphone control vs Microphone noise"
echo "  $(date)"
echo "════════════════════════════════════════════════════════════════"
echo ""

CARD=0

# ============================================================================
# 1. CURRENT MIXER STATE - ALL CONTROLS
# ============================================================================
echo "▶ 1. ALL MIXER CONTROLS (Card $CARD)"
echo "────────────────────────────────────────────────────────────────"
echo ""

echo "[1.1] Simple Controls List:"
amixer -c "$CARD" scontrols
echo ""

echo "[1.2] Detailed Control Values:"
amixer -c "$CARD" contents
echo ""

echo "[1.3] Human-Readable Controls:"
amixer -c "$CARD"
echo ""

# ============================================================================
# 2. HEADPHONE CONTROL (What Mopidy uses)
# ============================================================================
echo "▶ 2. HEADPHONE CONTROL (Mopidy's mixer)"
echo "────────────────────────────────────────────────────────────────"
echo ""

amixer -c "$CARD" sget Headphone 2>/dev/null
if [ $? -ne 0 ]; then
    echo "ERROR: No 'Headphone' control found!"
    echo "Searching for similar controls..."
    amixer -c "$CARD" scontrols | grep -i headphone
fi
echo ""

# ============================================================================
# 3. CAPTURE CONTROLS
# ============================================================================
echo "▶ 3. CAPTURE/MICROPHONE CONTROLS"
echo "────────────────────────────────────────────────────────────────"
echo ""

echo "[3.1] Capture Control:"
amixer -c "$CARD" sget Capture 2>/dev/null || echo "No 'Capture' control"
echo ""

echo "[3.2] Mic Control:"
amixer -c "$CARD" sget Mic 2>/dev/null || echo "No 'Mic' control"
echo ""

echo "[3.3] All Capture-related controls:"
amixer -c "$CARD" scontrols | grep -i -E '(capture|mic|input|boost|adc)'
echo ""

# ============================================================================
# 4. LOOPBACK/MONITORING PATHS
# ============================================================================
echo "▶ 4. LOOPBACK/MONITORING CONTROLS"
echo "────────────────────────────────────────────────────────────────"
echo ""
echo "Looking for controls that might route microphone to output..."
echo ""

# Common loopback control names
for ctrl in "Loopback" "Playback" "Sidetone" "Monitor" "Bypass" "Passthrough"; do
    amixer -c "$CARD" scontrols | grep -i "$ctrl" || true
done

echo ""
echo "Checking specific routing controls..."
amixer -c "$CARD" contents | grep -A 5 -i -E '(route|path|mux|switch)' || echo "No routing controls found in simple search"
echo ""

# ============================================================================
# 5. WM8960 CODEC SPECIFIC
# ============================================================================
echo "▶ 5. WM8960 CODEC INFORMATION"
echo "────────────────────────────────────────────────────────────────"
echo ""

echo "[5.1] Codec registers (if accessible):"
if [ -f /sys/kernel/debug/asoc/soc-audio/wm8960* ]; then
    cat /sys/kernel/debug/asoc/soc-audio/wm8960* 2>/dev/null || echo "Cannot access codec registers"
else
    echo "Codec debug interface not available"
fi
echo ""

echo "[5.2] ALSA info for card:"
cat /proc/asound/card0/codec* 2>/dev/null || echo "No codec info in /proc"
echo ""

# ============================================================================
# 6. PROCESS CHECK (BusyBox compatible)
# ============================================================================
echo "▶ 6. PROCESSES USING AUDIO"
echo "────────────────────────────────────────────────────────────────"
echo ""

echo "[6.1] Mopidy process:"
ps | grep mopidy | grep -v grep || echo "Mopidy not running"
echo ""

echo "[6.2] All processes (wide output):"
ps w | grep -E '(mopidy|python|alsa|snd)' | grep -v grep || echo "No audio-related processes"
echo ""

echo "[6.3] Process using capture device:"
if [ -e /dev/snd/pcmC0D0c ]; then
    # Try lsof if available
    if command -v lsof >/dev/null 2>&1; then
        lsof /dev/snd/pcmC0D0c 2>/dev/null || echo "No process using capture device"
    else
        # Check procfs
        for pid in /proc/[0-9]*; do
            if [ -d "$pid/fd" ]; then
                for fd in "$pid/fd"/*; do
                    if [ -L "$fd" ]; then
                        target=$(readlink "$fd" 2>/dev/null)
                        if [ "$target" = "/dev/snd/pcmC0D0c" ]; then
                            pid_num=$(basename "$pid")
                            echo "PID $pid_num is using capture device"
                            ps | grep "^ *$pid_num " || true
                        fi
                    fi
                done
            fi
        done
    fi
fi
echo ""

# ============================================================================
# 7. INTERACTIVE TESTS
# ============================================================================
echo "▶ 7. INTERACTIVE MOPIDY MIXER TEST"
echo "────────────────────────────────────────────────────────────────"
echo ""
echo "This test will help identify what changes when Mopidy stops."
echo ""

read -p "Is Mopidy currently RUNNING? (y/n) " -n 1 -r
echo ""
if [ "$REPLY" = "y" ] || [ "$REPLY" = "Y" ]; then
    echo ""
    echo "──── Snapshot 1: WITH Mopidy Running ────"
    echo ""
    echo "Headphone control:"
    amixer -c "$CARD" sget Headphone 2>/dev/null || echo "Not found"
    echo ""
    echo "All controls summary:"
    amixer -c "$CARD" | grep -E '(Playback|Capture).*\[' || true
    echo ""

    read -p "Now STOP Mopidy (not pause) and press Enter..." -r
    echo ""

    echo "──── Snapshot 2: AFTER Mopidy Stopped ────"
    echo ""
    echo "Headphone control:"
    amixer -c "$CARD" sget Headphone 2>/dev/null || echo "Not found"
    echo ""
    echo "All controls summary:"
    amixer -c "$CARD" | grep -E '(Playback|Capture).*\[' || true
    echo ""

    echo "──── Comparison ────"
    echo "Check if any values changed above!"
    echo "Especially look for:"
    echo "  - Headphone volume/mute state"
    echo "  - Any 'Capture' or 'Mic' controls changing"
    echo "  - Any 'Loopback' or 'Monitor' controls appearing/changing"
    echo ""

else
    echo "Test skipped - start Mopidy first and run this again"
fi

# ============================================================================
# 8. RECOMMENDATIONS
# ============================================================================
echo "▶ 8. INVESTIGATION RECOMMENDATIONS"
echo "────────────────────────────────────────────────────────────────"
echo ""
echo "Based on your Mopidy config:"
echo "  [alsamixer]"
echo "  card = 0"
echo "  control = Headphone"
echo ""
echo "When Mopidy stops, it releases the 'Headphone' mixer control."
echo ""
echo "Possible causes of noise:"
echo ""
echo "  1. LOOPBACK ENABLED"
echo "     When Headphone is released, codec defaults to routing"
echo "     microphone input directly to headphone output (monitoring)"
echo ""
echo "  2. CAPTURE LEVEL CHANGE"
echo "     Stopping Mopidy might reset capture volume to max"
echo ""
echo "  3. CODEC STATE"
echo "     WM8960 codec might have a default state that enables"
echo "     various signal paths when not actively controlled"
echo ""
echo "  4. SHARED CLOCK/SAMPLE RATE"
echo "     Playback stopping might affect capture clock, causing"
echo "     issues with anything else trying to capture"
echo ""
echo "Next steps:"
echo "  A. Compare 'before' and 'after' snapshots above"
echo "  B. Look for any 'Loopback', 'Monitor', or 'Sidetone' controls"
echo "  C. Check if Capture volume changes when Mopidy stops"
echo "  D. Test: manually set Headphone to same state as when"
echo "     Mopidy is running, then stop Mopidy and see if noise occurs"
echo ""

echo "════════════════════════════════════════════════════════════════"
echo "  Investigation Complete"
echo "════════════════════════════════════════════════════════════════"
echo ""
