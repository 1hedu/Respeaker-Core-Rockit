#!/bin/sh
# Rockit Synth + MIDI Bridge Launcher (for lighttpd setup)

echo "🎹 Starting Rockit Synth with MIDI Bridge..."

# Kill any existing instances
killall respeaker_rockit 2>/dev/null
killall midi_bridge.py 2>/dev/null
sleep 1

# Start the synth with TCP MIDI
echo "🎵 Starting synth engine..."
./respeaker_rockit --tcp-midi &
SYNTH_PID=$!
sleep 2

# Start the MIDI bridge
echo "🌉 Starting MIDI bridge on port 8090..."
python midi_bridge.py &
BRIDGE_PID=$!
sleep 1

# Get IP address
IP=$(ip addr show br-lan | grep 'inet ' | awk '{print $2}' | cut -d/ -f1)
if [ -z "$IP" ]; then
    IP=$(ip addr show eth0 | grep 'inet ' | awk '{print $2}' | cut -d/ -f1)
fi
if [ -z "$IP" ]; then
    IP="<your-ip>"
fi

echo ""
echo "✅ Rockit Synth is running!"
echo "   Synth PID: $SYNTH_PID"
echo "   Bridge PID: $BRIDGE_PID"
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
