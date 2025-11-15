#!/bin/bash
# Audio Device/Stream Monitoring Script
# Real-time monitoring of ALSA device state changes

echo "════════════════════════════════════════════════════════════════"
echo "  ALSA Device Monitor - Real-time Tracking"
echo "  Started: $(date)"
echo "  Press Ctrl+C to stop"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Interval in seconds
INTERVAL=2

# Color codes (if terminal supports it)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to get timestamp
timestamp() {
    date '+%Y-%m-%d %H:%M:%S'
}

# Function to check PCM status
check_pcm_status() {
    local timestamp=$(timestamp)
    echo -e "${BLUE}[$(timestamp)]${NC} ──────────────────────────────────────────"

    # Check which processes are using audio
    echo -e "${YELLOW}▶ Processes using /dev/snd:${NC}"
    fuser -v /dev/snd/* 2>&1 | grep -v "Cannot stat" | head -20

    if [ $? -ne 0 ]; then
        echo -e "${GREEN}  → No processes holding audio devices${NC}"
    fi
    echo ""

    # Check PCM state for all cards
    echo -e "${YELLOW}▶ PCM Device States:${NC}"
    for status_file in /proc/asound/card*/pcm*/sub0/status; do
        if [ -f "$status_file" ]; then
            device=$(echo "$status_file" | sed 's|/proc/asound/||' | sed 's|/sub0/status||')
            state=$(grep "state:" "$status_file" | awk '{print $2}')

            if [ "$state" = "RUNNING" ]; then
                echo -e "  ${RED}[$device]${NC} State: ${RED}$state${NC}"
                cat "$status_file" | sed 's/^/    /'
            elif [ "$state" = "PREPARED" ]; then
                echo -e "  ${YELLOW}[$device]${NC} State: ${YELLOW}$state${NC}"
            else
                echo -e "  ${GREEN}[$device]${NC} State: ${GREEN}$state${NC}"
            fi
        fi
    done
    echo ""

    # Check capture levels
    echo -e "${YELLOW}▶ Capture Levels:${NC}"
    for card_path in /proc/asound/card*; do
        if [ -d "$card_path" ]; then
            card_num=$(basename "$card_path" | sed 's/card//')

            # Try to get capture volume
            capture=$(amixer -c "$card_num" sget Capture 2>/dev/null | grep -E '(Mono|Front Left|Playback)' | head -1)
            if [ -n "$capture" ]; then
                echo "  Card $card_num: $capture"
            fi
        fi
    done
    echo ""

    # List processes that might interfere
    echo -e "${YELLOW}▶ Relevant Processes:${NC}"
    ps aux | grep -E '(mopidy|python|respeaker|arecord|aplay|pulseaudio)' | grep -v grep | while read line; do
        echo "  $line"
    done
    echo ""
}

# Function to monitor continuously
monitor_loop() {
    while true; do
        clear
        echo "════════════════════════════════════════════════════════════════"
        echo "  ALSA Device Monitor - Real-time (updates every ${INTERVAL}s)"
        echo "  Current time: $(timestamp)"
        echo "  Press Ctrl+C to stop"
        echo "════════════════════════════════════════════════════════════════"
        echo ""

        check_pcm_status

        echo -e "${BLUE}Waiting ${INTERVAL} seconds...${NC}"
        sleep "$INTERVAL"
    done
}

# Function to log mode (append to file instead of clearing screen)
log_mode() {
    local logfile="audio_monitor_$(date +%Y%m%d_%H%M%S).log"
    echo "Logging to: $logfile"
    echo ""

    while true; do
        check_pcm_status >> "$logfile"
        echo "──────────────────────────────────────────" >> "$logfile"
        echo "" >> "$logfile"

        # Also show on screen
        echo "[$(timestamp)] Logged snapshot to $logfile"

        sleep "$INTERVAL"
    done
}

# Parse command line arguments
case "${1}" in
    --log)
        log_mode
        ;;
    --once)
        check_pcm_status
        ;;
    --help)
        echo "Usage: $0 [OPTION]"
        echo ""
        echo "Options:"
        echo "  (no args)   Interactive monitor mode (clears screen)"
        echo "  --log       Log mode (append to file, no clear)"
        echo "  --once      Run once and exit"
        echo "  --help      Show this help"
        echo ""
        exit 0
        ;;
    *)
        monitor_loop
        ;;
esac
