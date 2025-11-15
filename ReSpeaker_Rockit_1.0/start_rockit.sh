#!/bin/sh
# ReSpeaker Rockit Startup Script
# Launches MIDI bridge and synth engine

# Kill any existing processes
killall midi_bridge 2>/dev/null
killall respeaker_rockit 2>/dev/null
sleep 1

# Start synth engine first (it listens on port 50000)
echo "Starting Rockit synth engine..."
./respeaker_rockit --tcp-midi &
SYNTH_PID=$!
sleep 2

# Start C MIDI bridge (HTTP port 8090 -> TCP MIDI port 50000)
echo "Starting MIDI bridge..."
./midi_bridge &
BRIDGE_PID=$!

echo ""
echo "Rockit is running!"
echo "  Synth PID: $SYNTH_PID"
echo "  Bridge PID: $BRIDGE_PID"
echo ""
echo "Web UI: http://$(hostname -I | awk '{print $1}'):8000"
echo "MIDI Bridge: http://$(hostname -I | awk '{print $1}'):8090"
echo ""
echo "Press Ctrl+C to stop..."

# Wait for Ctrl+C
trap "kill $SYNTH_PID $BRIDGE_PID 2>/dev/null; exit" INT TERM
wait
