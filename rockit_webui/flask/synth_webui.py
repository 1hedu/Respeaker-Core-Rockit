#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Rockit Synth Web UI
Runs a web server on the ReSpeaker to control the synth via TCP MIDI
"""

from flask import Flask, render_template, request, jsonify
import socket
import struct
import threading
import time

app = Flask(__name__)

# TCP MIDI configuration
TCP_HOST = "127.0.0.1"
TCP_PORT = 50000

# MIDI constants
MIDI_NOTE_ON = 0x90
MIDI_NOTE_OFF = 0x80
MIDI_CC = 0xB0

# Track held notes for keyboard
held_notes = set()
notes_lock = threading.Lock()


def send_raw_midi(status, data1, data2):
    """Send a 3-byte MIDI message via TCP"""
    try:
        msg = struct.pack("BBB", status, data1, data2)
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(1.0)
        s.connect((TCP_HOST, TCP_PORT))
        s.sendall(msg)
        s.close()
        return True
    except Exception as e:
        print(f"MIDI send error: {e}")
        return False


@app.route('/')
def index():
    """Serve the main synth UI"""
    return render_template('index.html')


@app.route('/api/note', methods=['POST'])
def note_control():
    """Handle note on/off"""
    data = request.json
    note = int(data.get('note', 60))
    velocity = int(data.get('velocity', 100))
    action = data.get('action', 'on')
    
    if action == 'on':
        with notes_lock:
            held_notes.add(note)
        send_raw_midi(MIDI_NOTE_ON, note, velocity)
    else:
        with notes_lock:
            held_notes.discard(note)
        send_raw_midi(MIDI_NOTE_OFF, note, 0)
    
    return jsonify({'status': 'ok'})


@app.route('/api/cc', methods=['POST'])
def cc_control():
    """Handle CC messages"""
    data = request.json
    cc = int(data.get('cc'))
    value = int(data.get('value'))
    
    # Clamp value to MIDI range
    value = max(0, min(127, value))
    
    send_raw_midi(MIDI_CC, cc, value)
    return jsonify({'status': 'ok', 'cc': cc, 'value': value})


@app.route('/api/panic', methods=['POST'])
def panic():
    """Turn off all notes"""
    with notes_lock:
        for note in list(held_notes):
            send_raw_midi(MIDI_NOTE_OFF, note, 0)
        held_notes.clear()
    return jsonify({'status': 'ok'})


if __name__ == '__main__':
    print("🎹 Rockit Synth Web UI starting...")
    print("📡 Open browser to: http://<respeaker-ip>:8080")
    app.run(host='0.0.0.0', port=8080, debug=False)
