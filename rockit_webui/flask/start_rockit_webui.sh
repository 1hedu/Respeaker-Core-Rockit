#!/bin/bash
# Rockit Synth + Web UI Launcher
# This script starts both the synth engine and web interface

echo "🎹 Starting Rockit Synth with Web UI..."

# Check if synth binary exists
if [ ! -f "./respeaker_rockit" ]; then
    echo "❌ Error: respeaker_rockit binary not found"
    echo "Please compile the synth first"
    exit 1
fi

# Check if web UI exists
if [ ! -f "./synth_webui.py" ]; then
    echo "❌ Error: synth_webui.py not found"
    exit 1
fi

# Kill any existing instances
echo "🧹 Cleaning up existing processes..."
pkill -f respeaker_rockit
pkill -f synth_webui.py
sleep 1

# Start the synth in the background with TCP MIDI
echo "🎵 Starting Rockit synth engine..."
./respeaker_rockit --tcp-midi &
SYNTH_PID=$!
sleep 2

# Start the web UI
echo "🌐 Starting web interface on port 8080..."
python3 synth_webui.py &
WEBUI_PID=$!

echo ""
echo "✅ Rockit Synth is running!"
echo "   Synth PID: $SYNTH_PID"
echo "   Web UI PID: $WEBUI_PID"
echo ""
echo "📡 Open browser to: http://$(hostname -I | awk '{print $1}'):8080"
echo ""
echo "Press Ctrl+C to stop..."

# Handle Ctrl+C gracefully
trap "echo ''; echo '🛑 Stopping...'; kill $SYNTH_PID $WEBUI_PID 2>/dev/null; exit 0" INT

# Wait for both processes
wait
