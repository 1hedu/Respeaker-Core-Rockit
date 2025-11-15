#!/bin/bash
# Quick Setup Script for Rockit Web UI
# Run this on your ReSpeaker to set everything up

echo "🎹 Rockit Synth Web UI - Quick Setup"
echo "===================================="
echo ""

# Install Flask if needed
echo "📦 Checking for Flask..."
if ! python3 -c "import flask" 2>/dev/null; then
    echo "Installing Flask..."
    pip3 install flask
else
    echo "✅ Flask already installed"
fi

echo ""
echo "✅ Setup complete!"
echo ""
echo "To start the synth with web UI:"
echo "  ./start_rockit_webui.sh"
echo ""
echo "Or manually:"
echo "  1. Start synth: ./respeaker_rockit --tcp-midi &"
echo "  2. Start web UI: python3 synth_webui.py &"
echo ""
echo "Then open browser to: http://<your-respeaker-ip>:8080"
echo ""
