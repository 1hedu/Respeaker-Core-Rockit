#!/bin/bash
# ALSA State Investigation Script
# Comprehensive check of ALSA audio system state

echo "════════════════════════════════════════════════════════════════"
echo "  ALSA State Investigation - $(date)"
echo "════════════════════════════════════════════════════════════════"
echo ""

# ============================================================================
# 1. ALSA DEVICES OVERVIEW
# ============================================================================
echo "▶ 1. ALSA DEVICES OVERVIEW"
echo "────────────────────────────────────────────────────────────────"
echo ""
echo "[1.1] Available Sound Cards:"
cat /proc/asound/cards
echo ""

echo "[1.2] PCM Devices:"
aplay -l 2>/dev/null
echo ""
arecord -l 2>/dev/null
echo ""

# ============================================================================
# 2. DEVICE STATE & OWNERSHIP
# ============================================================================
echo "▶ 2. DEVICE STATE & OWNERSHIP"
echo "────────────────────────────────────────────────────────────────"
echo ""
echo "[2.1] Processes Using Audio Devices:"
fuser -v /dev/snd/* 2>&1 | grep -v "Cannot stat" || echo "No processes currently holding audio devices"
echo ""

echo "[2.2] Detailed Process List (lsof):"
if command -v lsof >/dev/null 2>&1; then
    lsof /dev/snd/* 2>/dev/null || echo "lsof: No audio devices in use"
else
    echo "lsof not available"
fi
echo ""

echo "[2.3] Process Tree of Audio Users:"
ps aux | grep -E '(alsa|pulse|mopidy|python|respeaker)' | grep -v grep
echo ""

# ============================================================================
# 3. ALSA PCM STATUS (via /proc)
# ============================================================================
echo "▶ 3. ALSA PCM STATUS (/proc/asound)"
echo "────────────────────────────────────────────────────────────────"
echo ""
for card in /proc/asound/card*/pcm*/sub0/status; do
    if [ -f "$card" ]; then
        echo "[Status] $card:"
        cat "$card"
        echo ""
    fi
done

for card in /proc/asound/card*/pcm*/sub0/hw_params; do
    if [ -f "$card" ]; then
        echo "[HW Params] $card:"
        cat "$card"
        echo ""
    fi
done

# ============================================================================
# 4. MIXER CONTROLS & LEVELS
# ============================================================================
echo "▶ 4. MIXER CONTROLS & LEVELS"
echo "────────────────────────────────────────────────────────────────"
echo ""

# Get all cards
for card_path in /proc/asound/card*; do
    if [ -d "$card_path" ]; then
        card_num=$(basename "$card_path" | sed 's/card//')
        card_name=$(cat "$card_path/id" 2>/dev/null || echo "Unknown")

        echo "[4.$card_num] Card $card_num: $card_name"
        echo "────────────────────────────────────────────────────────────────"

        # All mixer controls
        echo "All Controls:"
        amixer -c "$card_num" 2>/dev/null || echo "Cannot read mixer for card $card_num"
        echo ""

        # Specifically check capture controls
        echo "Capture-specific controls:"
        amixer -c "$card_num" scontrols 2>/dev/null | grep -i capture || echo "No capture controls found"
        echo ""

        # Get current capture volume/state
        echo "Current Capture Settings:"
        amixer -c "$card_num" sget Capture 2>/dev/null || echo "No 'Capture' control"
        amixer -c "$card_num" sget Mic 2>/dev/null || echo "No 'Mic' control"
        echo ""
        echo ""
    fi
done

# ============================================================================
# 5. DMIX / ASOUNDRC CONFIGURATION
# ============================================================================
echo "▶ 5. ALSA CONFIGURATION FILES"
echo "────────────────────────────────────────────────────────────────"
echo ""
echo "[5.1] /etc/asound.conf:"
if [ -f /etc/asound.conf ]; then
    cat /etc/asound.conf
else
    echo "(file does not exist)"
fi
echo ""

echo "[5.2] ~/.asoundrc:"
if [ -f ~/.asoundrc ]; then
    cat ~/.asoundrc
else
    echo "(file does not exist)"
fi
echo ""

echo "[5.3] /root/.asoundrc (if running as root):"
if [ -f /root/.asoundrc ]; then
    cat /root/.asoundrc
else
    echo "(file does not exist)"
fi
echo ""

# ============================================================================
# 6. MOPIDY STATUS & CONFIGURATION
# ============================================================================
echo "▶ 6. MOPIDY STATUS"
echo "────────────────────────────────────────────────────────────────"
echo ""
echo "[6.1] Mopidy Process:"
ps aux | grep mopidy | grep -v grep || echo "Mopidy not running"
echo ""

echo "[6.2] Mopidy Audio Config (if available):"
if [ -f /etc/mopidy/mopidy.conf ]; then
    grep -A 10 '\[audio\]' /etc/mopidy/mopidy.conf 2>/dev/null || echo "No [audio] section found"
elif [ -f ~/.config/mopidy/mopidy.conf ]; then
    grep -A 10 '\[audio\]' ~/.config/mopidy/mopidy.conf 2>/dev/null || echo "No [audio] section found"
else
    echo "Mopidy config not found in standard locations"
fi
echo ""

# ============================================================================
# 7. KERNEL MODULES
# ============================================================================
echo "▶ 7. AUDIO KERNEL MODULES"
echo "────────────────────────────────────────────────────────────────"
echo ""
lsmod | grep -E '^snd' | sort
echo ""

# ============================================================================
# 8. DEVICE FILES & PERMISSIONS
# ============================================================================
echo "▶ 8. DEVICE FILES & PERMISSIONS"
echo "────────────────────────────────────────────────────────────────"
echo ""
ls -la /dev/snd/
echo ""

# ============================================================================
# 9. RECENT AUDIO-RELATED DMESG
# ============================================================================
echo "▶ 9. RECENT AUDIO-RELATED KERNEL MESSAGES"
echo "────────────────────────────────────────────────────────────────"
echo ""
dmesg | grep -i -E '(alsa|sound|audio|snd|pcm)' | tail -30
echo ""

# ============================================================================
# SUMMARY
# ============================================================================
echo "════════════════════════════════════════════════════════════════"
echo "  INVESTIGATION COMPLETE"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "Key things to check:"
echo "  • Are any processes holding /dev/snd devices? (Section 2)"
echo "  • What is the PCM device status? (Section 3)"
echo "  • What are the current capture levels? (Section 4)"
echo "  • Is Mopidy running and what device is it using? (Section 6)"
echo ""
echo "Run this script with: ./alsa_state_check.sh | tee alsa_state_$(date +%Y%m%d_%H%M%S).log"
echo ""
