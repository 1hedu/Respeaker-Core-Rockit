#!/usr/bin/env python
-- coding: utf-8 --

""" Rockit MIDI Bridge - Minimal TCP-to-MIDI forwarder Listens on port 8090 for HTTP requests, forwards to TCP MIDI on port 50000 """

import socket import struct from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler from urlparse import urlparse, parse_qs

TCP_HOST = "127.0.0.1" TCP_PORT = 50000

MIDI_NOTE_ON = 0x90 MIDI_NOTE_OFF = 0x80 MIDI_CC = 0xB0

def send_raw_midi(status, data1, data2): """Send 3-byte MIDI message via TCP""" try: msg = struct.pack("BBB", status, data1, data2) s = socket.socket(socket.AF_INET, socket.SOCK_STREAM) s.settimeout(0.5) s.connect((TCP_HOST, TCP_PORT)) s.sendall(msg) s.close() return True except: return False

class MIDIBridgeHandler(BaseHTTPRequestHandler): def do_OPTIONS(self): """Handle CORS preflight""" self.send_response(200) self.send_header('Access-Control-Allow-Origin', '*') self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS') self.send_header('Access-Control-Allow-Headers', 'Content-Type') self.end_headers()

def do_GET(self):
    """Handle GET requests"""
    if self.path.startswith('/status'):
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write(b'OK')
    else:
        self.send_response(404)
        self.end_headers()

def do_POST(self):
    """Handle POST requests"""
    parsed = urlparse(self.path)
    params = parse_qs(parsed.query)

    self.send_response(200)
    self.send_header('Access-Control-Allow-Origin', '*')
    self.send_header('Content-type', 'text/plain')
    self.end_headers()

    if parsed.path == '/note':
        note = int(params.get('note', ['60'])[0])
        action = params.get('action', ['on'])[0]
        velocity = int(params.get('velocity', ['100'])[0])

        if action == 'on':
            send_raw_midi(MIDI_NOTE_ON, note, velocity)
        else:
            send_raw_midi(MIDI_NOTE_OFF, note, 0)

        self.wfile.write(b'OK')

    elif parsed.path == '/cc':
        cc = int(params.get('cc', ['0'])[0])
        value = int(params.get('value', ['0'])[0])
        value = max(0, min(127, value))

        send_raw_midi(MIDI_CC, cc, value)
        self.wfile.write(b'OK')

    elif parsed.path == '/panic':
        for note in range(128):
            send_raw_midi(MIDI_NOTE_OFF, note, 0)
        self.wfile.write(b'OK')
    else:
        self.wfile.write(b'ERROR')

def log_message(self, format, *args):
    # Suppress default logging
    pass

if name == 'main': print("MIDI Bridge starting on port 8090...") print("Forwarding to TCP MIDI on %s:%d" % (TCP_HOST, TCP_PORT))

server = HTTPServer(('0.0.0.0', 8090), MIDIBridgeHandler)
try:
    server.serve_forever()
except KeyboardInterrupt:
    print("\nStopping...")
    server.shutdown()
