#!/usr/bin/env python
"""
PyAudio Microphone Test
Simulates what the ReSpeaker Python library does when it claims the microphone
"""

import sys
import time
import struct

try:
    import pyaudio
except ImportError:
    print("ERROR: PyAudio not installed")
    print("Install with: pip install pyaudio")
    sys.exit(1)

# Configuration
CHUNK = 1024
FORMAT = pyaudio.paInt16
CHANNELS = 2
RATE = 16000
RECORD_SECONDS = 10

def list_devices(p):
    """List all audio devices"""
    print("=" * 70)
    print("  Available Audio Devices")
    print("=" * 70)
    print()

    info = p.get_host_api_info_by_index(0)
    numdevices = info.get('deviceCount')

    for i in range(0, numdevices):
        device_info = p.get_device_info_by_host_api_device_index(0, i)
        device_name = device_info.get('name')
        max_input = device_info.get('maxInputChannels')
        max_output = device_info.get('maxOutputChannels')

        if max_input > 0:
            print(f"[{i}] INPUT:  {device_name}")
            print(f"     Channels: {max_input}, Rate: {device_info.get('defaultSampleRate')}")

        if max_output > 0:
            print(f"[{i}] OUTPUT: {device_name}")
            print(f"     Channels: {max_output}, Rate: {device_info.get('defaultSampleRate')}")

        # Highlight ReSpeaker devices
        if 'respeaker' in device_name.lower():
            print(f"     *** ReSpeaker device detected ***")

        print()

def find_respeaker_device(p):
    """Find ReSpeaker device (same logic as respeaker_python_library)"""
    info = p.get_host_api_info_by_index(0)
    numdevices = info.get('deviceCount')

    for i in range(0, numdevices):
        device_info = p.get_device_info_by_host_api_device_index(0, i)
        device_name = device_info.get('name')

        if 'respeaker' in device_name.lower() and device_info.get('maxInputChannels') > 0:
            return i, device_name

    return None, None

def test_capture(p, device_index=None, duration=10):
    """
    Test microphone capture (simulates what ReSpeaker library does)
    This will claim the microphone device
    """
    print("=" * 70)
    print("  Microphone Capture Test")
    print("=" * 70)
    print()

    if device_index is None:
        print("Using default input device")
        device_name = "default"
    else:
        device_info = p.get_device_info_by_index(device_index)
        device_name = device_info.get('name')
        print(f"Using device [{device_index}]: {device_name}")

    print(f"Format: {CHANNELS} channels, {RATE}Hz, {duration}s")
    print()

    try:
        print("Opening audio stream...")
        stream = p.open(
            format=FORMAT,
            channels=CHANNELS,
            rate=RATE,
            input=True,
            input_device_index=device_index,
            frames_per_buffer=CHUNK
        )

        print("✓ Stream opened successfully")
        print()
        print(f"Recording for {duration} seconds...")
        print("(This CLAIMS the microphone - check if noise stops)")
        print()

        max_amplitude = 0
        frames_read = 0

        for i in range(0, int(RATE / CHUNK * duration)):
            try:
                data = stream.read(CHUNK, exception_on_overflow=False)
                frames_read += 1

                # Calculate RMS to show we're getting data
                if i % 10 == 0:  # Every ~0.3 seconds
                    shorts = struct.unpack('h' * (len(data) // 2), data)
                    rms = sum(s * s for s in shorts) / len(shorts)
                    rms = rms ** 0.5
                    max_amplitude = max(max_amplitude, rms)

                    # Progress indicator
                    elapsed = i * CHUNK / RATE
                    bars = int(elapsed / duration * 40)
                    print(f"\r[{'=' * bars}{' ' * (40-bars)}] {elapsed:.1f}s RMS: {rms:8.1f}", end='')
                    sys.stdout.flush()

            except IOError as e:
                print(f"\nWarning: Buffer overflow - {e}")
                continue

        print()
        print()
        print(f"✓ Recording complete: {frames_read} frames read")
        print(f"  Max RMS amplitude: {max_amplitude:.1f}")

        stream.stop_stream()
        stream.close()

        print("✓ Stream closed")
        print()

    except Exception as e:
        print(f"✗ Error during capture: {e}")
        print()
        return False

    return True

def main():
    print()
    print("=" * 70)
    print("  PyAudio Microphone Test")
    print("  Simulates ReSpeaker library behavior")
    print("=" * 70)
    print()

    p = pyaudio.PyAudio()

    # List all devices
    list_devices(p)

    # Find ReSpeaker device
    respeaker_idx, respeaker_name = find_respeaker_device(p)

    if respeaker_idx is not None:
        print("=" * 70)
        print(f"✓ Found ReSpeaker device: [{respeaker_idx}] {respeaker_name}")
        print("=" * 70)
        print()
    else:
        print("=" * 70)
        print("⚠ No ReSpeaker device found")
        print("  Will use default device")
        print("=" * 70)
        print()

    # Interactive menu
    while True:
        print()
        print("Options:")
        print("  1. Test capture (default/ReSpeaker device)")
        print("  2. Test capture (specify device)")
        print("  3. Test capture (extended duration)")
        print("  4. List devices again")
        print("  q. Quit")
        print()

        choice = input("Select option: ").strip().lower()
        print()

        if choice == '1':
            # Use ReSpeaker if found, otherwise default
            test_capture(p, device_index=respeaker_idx, duration=10)

        elif choice == '2':
            device = input("Enter device index: ").strip()
            try:
                device_idx = int(device)
                test_capture(p, device_index=device_idx, duration=10)
            except ValueError:
                print("Invalid device index")

        elif choice == '3':
            duration = input("Enter duration in seconds (default 30): ").strip()
            try:
                dur = int(duration) if duration else 30
                test_capture(p, device_index=respeaker_idx, duration=dur)
            except ValueError:
                print("Invalid duration")

        elif choice == '4':
            list_devices(p)

        elif choice == 'q':
            break

        else:
            print("Invalid option")

    p.terminate()
    print()
    print("=" * 70)
    print("  Test Complete")
    print("=" * 70)
    print()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print()
        print()
        print("Interrupted by user")
        sys.exit(0)
