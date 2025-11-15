#!/bin/sh
# Rockit Synth + Web UI Launcher (Minimal - No Dependencies)

echo "🎹 Starting Rockit Synth with Web UI..."

# Kill any existing instances
pkill -f respeaker_rockit
pkill -f synth_webui_minimal.py
sleep 1

# Start the synth with TCP MIDI
echo "🎵 Starting synth engine..."
./respeaker_rockit --tcp-midi &
sleep 2

# Start the web UI (uses only Python stdlib - no Flask!)
echo "🌐 Starting web interface on port 8080..."
python synth_webui_minimal.py &

echo ""
echo "✅ Rockit Synth is running!"
echo "📡 Open browser to: http://$(ip addr show br-lan | grep 'inet ' | awk '{print $2}' | cut -d/ -f1):8080"
echo ""
echo "Press Ctrl+C to stop both..."

# Wait
trap "pkill -f respeaker_rockit; pkill -f synth_webui_minimal.py; exit" INT TERM
wait
