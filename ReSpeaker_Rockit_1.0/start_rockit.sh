#!/bin/sh
# Rockit Synth + C MIDI Bridge Launcher (for lighttpd setup)

echo "🎹 Starting Rockit Synth with C MIDI Bridge..."

# Kill any existing instances (both old Python and new C versions)
killall respeaker_rockit 2>/dev/null
killall midi_bridge.py 2>/dev/null
killall midi_bridge 2>/dev/null
killall python 2>/dev/null
sleep 1

# Start the synth with TCP MIDI
echo "🎵 Starting synth engine..."
./respeaker_rockit --tcp-midi &
SYNTH_PID=$!
sleep 2

# Start the C MIDI bridge (replaces Python version for better performance)
echo "🌉 Starting C MIDI bridge on port 8090..."
./midi_bridge &
BRIDGE_PID=$!
sleep 1

# Get IP address
IP=$(ifconfig br-lan | grep 'inet ' | awk '{print $2}' | cut -d/ -f1)

if [ -z "$IP" ]; then
    IP=$(ifconfig eth0 | grep 'inet ' | awk '{print $2}' | cut -d/ -f1)
fi
if [ -z "$IP" ]; then
    IP="<your-ip>"
fi

echo ""
echo "✅ Rockit Synth is running!"
echo "   Synth PID: $SYNTH_PID"
echo "   Bridge PID: $BRIDGE_PID (C version - fast!)"
echo ""
echo "📡 Open browser to:"
echo "   http://$IP/rockit.html"
echo ""
echo "   (Make sure rockit.html is in lighttpd document root!)"
echo ""
echo "Press Ctrl+C to stop..."

# Handle Ctrl+C gracefully
trap "echo ''; echo '🛑 Stopping...'; kill $SYNTH_PID $BRIDGE_PID 2>/dev/null; exit 0" INT TERM

# Wait
wait
