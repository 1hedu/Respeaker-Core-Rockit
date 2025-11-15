#!/usr/bin/python
# -*- coding: utf-8 -*-

from respeaker import Microphone
import socket
import struct
import time

# --- TCP config (must match respeaker_rockit) ---
TCP_HOST = "127.0.0.1"
TCP_PORT = 50000

# --- MIDI message constants ---
MIDI_NOTE_ON  = 0x90
MIDI_NOTE_OFF = 0x80
MIDI_CC       = 0xB0

def send_raw_midi(status, data1, data2):
    msg = struct.pack("BBB", status, data1, data2)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((TCP_HOST, TCP_PORT))
        s.sendall(msg)
        print("-> MIDI: %02X %3d %3d" % (status, data1, data2))
    except IOError as e:
        print("socket error:", e)
    finally:
        s.close()

def send_midi_note(note, velocity=100, duration=0.5):
    send_raw_midi(MIDI_NOTE_ON, note, velocity)
    time.sleep(duration)
    send_raw_midi(MIDI_NOTE_OFF, note, 0)

def send_midi_cc(cc, value):
    send_raw_midi(MIDI_CC, cc, value)

def parse_and_execute(text):
    t = text.strip().lower()
    print("🧠 command:", t)

    if "play c four" in t or "play c4" in t:
        send_midi_note(60)
    elif "play d four" in t or "play d4" in t:
        send_midi_note(62)
    elif "play e four" in t or "play e4" in t:
        send_midi_note(64)
    elif "play g four" in t or "play g4" in t:
        send_midi_note(67)
    elif "volume one twenty seven" in t:
        send_midi_cc(7, 127)
    elif "cutoff zero" in t:
        send_midi_cc(74, 0)
    elif "cutoff one twenty seven" in t:
        send_midi_cc(74, 127)
    else:
        print("no match.")

def main():
    print("🎙️  Initializing ReSpeaker microphone...")
    mic = Microphone()
    print("✅ Ready. Say: 'respeaker play C four'")

    while True:
        print("⏳ Waiting for wake word: 'respeaker'")
        if mic.wakeup("respeaker"):
            print("👂 Wake word detected!")
            data = mic.listen()
            text = mic.recognize(data)
            if text:
                print("🔊 Recognized:", text)
                parse_and_execute(text)
            else:
                print("🤖 No recognizable speech.")

if __name__ == "__main__":
    main()
