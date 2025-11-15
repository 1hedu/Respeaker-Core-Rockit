#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Rockit Synth Web UI - Minimal version
Uses only Python stdlib - no Flask or external dependencies
"""

import socket
import struct
import json
from http.server import HTTPServer, BaseHTTPRequestHandler
import os

TCP_HOST = "127.0.0.1"
TCP_PORT = 50000

MIDI_NOTE_ON = 0x90
MIDI_NOTE_OFF = 0x80
MIDI_CC = 0xB0

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
        print("MIDI error: %s" % str(e))
        return False


class SynthHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/' or self.path == '/index.html':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(HTML_CONTENT.encode())
        else:
            self.send_response(404)
            self.end_headers()
    
    def do_POST(self):
        if self.path == '/api/note':
            length = int(self.headers['Content-Length'])
            data = json.loads(self.rfile.read(length).decode())
            note = int(data.get('note', 60))
            velocity = int(data.get('velocity', 100))
            action = data.get('action', 'on')
            
            if action == 'on':
                send_raw_midi(MIDI_NOTE_ON, note, velocity)
            else:
                send_raw_midi(MIDI_NOTE_OFF, note, 0)
            
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(b'{"status":"ok"}')
            
        elif self.path == '/api/cc':
            length = int(self.headers['Content-Length'])
            data = json.loads(self.rfile.read(length).decode())
            cc = int(data.get('cc'))
            value = int(data.get('value'))
            value = max(0, min(127, value))
            
            send_raw_midi(MIDI_CC, cc, value)
            
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(('{"status":"ok","cc":%d,"value":%d}' % (cc, value)).encode())
            
        elif self.path == '/api/panic':
            # Send all notes off
            for note in range(128):
                send_raw_midi(MIDI_NOTE_OFF, note, 0)
            
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(b'{"status":"ok"}')
        else:
            self.send_response(404)
            self.end_headers()
    
    def log_message(self, format, *args):
        # Suppress default logging
        pass


# Embedded HTML - single file, no external dependencies
HTML_CONTENT = '''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Rockit Synth</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial, sans-serif;
            background: #1a1a2e;
            color: #e0e0e0;
            padding: 10px;
        }
        .container { max-width: 900px; margin: 0 auto; }
        h1 { text-align: center; color: #00d4ff; margin-bottom: 20px; font-size: 2em; }
        .panel {
            background: rgba(255,255,255,0.05);
            border-radius: 10px;
            padding: 15px;
            margin-bottom: 15px;
            border: 1px solid rgba(255,255,255,0.1);
        }
        .panel-title { color: #00d4ff; margin-bottom: 10px; font-size: 1.1em; }
        .controls { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 15px; }
        .control-group { display: flex; flex-direction: column; }
        .control-label {
            font-size: 0.85em;
            color: #b0b0b0;
            margin-bottom: 5px;
            display: flex;
            justify-content: space-between;
        }
        .control-value { color: #00d4ff; font-weight: bold; }
        input[type="range"] {
            width: 100%;
            height: 4px;
            border-radius: 2px;
            background: rgba(255,255,255,0.1);
            outline: none;
        }
        .keyboard {
            background: rgba(0,0,0,0.3);
            border-radius: 8px;
            padding: 15px;
            overflow-x: auto;
        }
        .keys-container {
            display: flex;
            gap: 2px;
            min-width: 700px;
            position: relative;
            height: 150px;
        }
        .key {
            border: 1px solid #333;
            border-radius: 0 0 4px 4px;
            cursor: pointer;
            user-select: none;
            position: relative;
            min-width: 30px;
        }
        .key.white {
            background: linear-gradient(to bottom, #fff 0%, #e0e0e0 100%);
            height: 150px;
            flex: 1;
        }
        .key.white:active, .key.white.active { background: linear-gradient(to bottom, #00d4ff 0%, #0099cc 100%); }
        .key.black {
            background: linear-gradient(to bottom, #2a2a2a 0%, #000 100%);
            height: 90px;
            width: 20px;
            position: absolute;
            z-index: 2;
            margin-left: -10px;
        }
        .key.black:active, .key.black.active { background: linear-gradient(to bottom, #00d4ff 0%, #0099cc 100%); }
        .key-label {
            position: absolute;
            bottom: 8px;
            left: 0;
            right: 0;
            text-align: center;
            font-size: 0.7em;
            color: #666;
            font-weight: bold;
        }
        .key.black .key-label { color: #888; bottom: 4px; font-size: 0.6em; }
        button {
            padding: 10px 20px;
            background: linear-gradient(135deg, #00d4ff 0%, #0099cc 100%);
            color: white;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            font-size: 0.9em;
            font-weight: bold;
            margin: 5px;
        }
        button:active { transform: translateY(1px); }
        button.panic { background: linear-gradient(135deg, #ff4444 0%, #cc0000 100%); }
        button.toggle { background: rgba(255,255,255,0.1); border: 2px solid rgba(255,255,255,0.3); }
        button.toggle.active { background: linear-gradient(135deg, #00d4ff 0%, #0099cc 100%); }
        .status { text-align: center; padding: 8px; background: rgba(0,212,255,0.1); border-radius: 6px; margin-top: 15px; font-size: 0.85em; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎹 Rockit Synth</h1>
        
        <div class="panel">
            <div class="panel-title">Keyboard</div>
            <div class="keyboard">
                <div class="keys-container" id="keyboard"></div>
            </div>
        </div>
        
        <div class="panel">
            <div class="panel-title">Filter</div>
            <div class="controls">
                <div class="control-group">
                    <div class="control-label"><span>Cutoff</span><span class="control-value" id="cutoff-val">100</span></div>
                    <input type="range" id="cutoff" min="0" max="127" value="100" data-cc="74">
                </div>
                <div class="control-group">
                    <div class="control-label"><span>Resonance</span><span class="control-value" id="resonance-val">20</span></div>
                    <input type="range" id="resonance" min="0" max="127" value="20" data-cc="71">
                </div>
            </div>
        </div>
        
        <div class="panel">
            <div class="panel-title">Envelope</div>
            <div class="controls">
                <div class="control-group">
                    <div class="control-label"><span>Attack</span><span class="control-value" id="attack-val">5</span></div>
                    <input type="range" id="attack" min="0" max="127" value="5" data-cc="73">
                </div>
                <div class="control-group">
                    <div class="control-label"><span>Decay</span><span class="control-value" id="decay-val">40</span></div>
                    <input type="range" id="decay" min="0" max="127" value="40" data-cc="75">
                </div>
                <div class="control-group">
                    <div class="control-label"><span>Release</span><span class="control-value" id="release-val">30</span></div>
                    <input type="range" id="release" min="0" max="127" value="30" data-cc="70">
                </div>
            </div>
        </div>
        
        <div class="panel">
            <div class="panel-title">Oscillator</div>
            <div class="controls">
                <div class="control-group">
                    <div class="control-label"><span>Mix</span><span class="control-value" id="oscmix-val">64</span></div>
                    <input type="range" id="oscmix" min="0" max="127" value="64" data-cc="72">
                </div>
                <div class="control-group">
                    <div class="control-label"><span>Sub Osc</span></div>
                    <button class="toggle" id="subosc" data-cc="76" data-state="0">OFF</button>
                </div>
            </div>
        </div>
        
        <div class="panel">
            <div class="panel-title">LFO</div>
            <div class="controls">
                <div class="control-group">
                    <div class="control-label"><span>Depth</span><span class="control-value" id="lfodepth-val">0</span></div>
                    <input type="range" id="lfodepth" min="0" max="127" value="0" data-cc="1">
                </div>
            </div>
        </div>
        
        <div class="panel">
            <div class="panel-title">Voice Mode</div>
            <button class="toggle" id="monopara" data-cc="102" data-state="0">Mono</button>
            <button class="toggle" id="voicecount" data-cc="103" data-state="0">2-Voice</button>
        </div>
        
        <div class="panel">
            <div class="panel-title">Master</div>
            <div class="controls">
                <div class="control-group">
                    <div class="control-label"><span>Volume</span><span class="control-value" id="volume-val">100</span></div>
                    <input type="range" id="volume" min="0" max="127" value="100" data-cc="7">
                </div>
            </div>
            <button class="panic" id="panic">🚨 PANIC</button>
        </div>
        
        <div class="status">Connected • TCP MIDI Active</div>
    </div>

    <script>
        function sendCC(cc, value) {
            fetch('/api/cc', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ cc: cc, value: value })
            }).catch(function(e) { console.error(e); });
        }
        
        function sendNote(note, action, velocity) {
            velocity = velocity || 100;
            fetch('/api/note', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ note: note, action: action, velocity: velocity })
            }).catch(function(e) { console.error(e); });
        }
        
        // Setup sliders
        var sliders = document.querySelectorAll('input[type="range"]');
        for (var i = 0; i < sliders.length; i++) {
            var slider = sliders[i];
            var cc = parseInt(slider.getAttribute('data-cc'));
            var valueDisplay = document.getElementById(slider.id + '-val');
            
            slider.addEventListener('input', function(e) {
                var value = parseInt(e.target.value);
                var display = document.getElementById(e.target.id + '-val');
                display.textContent = value;
                sendCC(parseInt(e.target.getAttribute('data-cc')), value);
            });
        }
        
        // Setup toggles
        var toggles = document.querySelectorAll('button.toggle');
        for (var i = 0; i < toggles.length; i++) {
            var btn = toggles[i];
            btn.addEventListener('click', function(e) {
                var cc = parseInt(this.getAttribute('data-cc'));
                var state = parseInt(this.getAttribute('data-state'));
                state = state === 0 ? 1 : 0;
                this.setAttribute('data-state', state);
                
                if (this.id === 'subosc') {
                    this.textContent = state === 1 ? 'ON' : 'OFF';
                    this.classList.toggle('active', state === 1);
                    sendCC(cc, state === 1 ? 127 : 0);
                } else if (this.id === 'monopara') {
                    this.textContent = state === 1 ? 'Para' : 'Mono';
                    this.classList.toggle('active', state === 1);
                    sendCC(cc, state === 1 ? 127 : 0);
                } else if (this.id === 'voicecount') {
                    this.textContent = state === 1 ? '3-Voice' : '2-Voice';
                    this.classList.toggle('active', state === 1);
                    sendCC(cc, state === 1 ? 127 : 0);
                }
            });
        }
        
        // Panic button
        document.getElementById('panic').addEventListener('click', function() {
            fetch('/api/panic', { method: 'POST' });
            var keys = document.querySelectorAll('.key.active');
            for (var i = 0; i < keys.length; i++) {
                keys[i].classList.remove('active');
            }
        });
        
        // Build keyboard
        var keyboard = document.getElementById('keyboard');
        var notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        var startNote = 48;
        var octaves = 2;
        var whiteKeyIndex = 0;
        
        for (var octave = 0; octave <= octaves; octave++) {
            for (var i = 0; i < 12; i++) {
                if (octave === octaves && i > 0) break;
                
                var noteName = notes[i];
                var midiNote = startNote + (octave * 12) + i;
                var isBlack = noteName.indexOf('#') !== -1;
                
                var key = document.createElement('div');
                key.className = 'key ' + (isBlack ? 'black' : 'white');
                key.setAttribute('data-note', midiNote);
                
                var label = document.createElement('div');
                label.className = 'key-label';
                label.textContent = noteName + (3 + octave);
                key.appendChild(label);
                
                if (isBlack) {
                    key.style.left = (whiteKeyIndex * 32 - 10) + 'px';
                } else {
                    key.style.left = (whiteKeyIndex * 32) + 'px';
                    whiteKeyIndex++;
                }
                
                keyboard.appendChild(key);
                
                key.addEventListener('mousedown', function(e) {
                    e.preventDefault();
                    this.classList.add('active');
                    sendNote(parseInt(this.getAttribute('data-note')), 'on');
                });
                
                key.addEventListener('mouseup', function() {
                    this.classList.remove('active');
                    sendNote(parseInt(this.getAttribute('data-note')), 'off');
                });
                
                key.addEventListener('mouseleave', function() {
                    if (this.classList.contains('active')) {
                        this.classList.remove('active');
                        sendNote(parseInt(this.getAttribute('data-note')), 'off');
                    }
                });
                
                key.addEventListener('touchstart', function(e) {
                    e.preventDefault();
                    this.classList.add('active');
                    sendNote(parseInt(this.getAttribute('data-note')), 'on');
                });
                
                key.addEventListener('touchend', function(e) {
                    e.preventDefault();
                    this.classList.remove('active');
                    sendNote(parseInt(this.getAttribute('data-note')), 'off');
                });
            }
        }
    </script>
</body>
</html>
'''


if __name__ == '__main__':
    print("Rockit Synth Web UI starting...")
    print("Open browser to: http://<respeaker-ip>:8080")
    
    server = HTTPServer(('0.0.0.0', 8080), SynthHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping server...")
        server.shutdown()
