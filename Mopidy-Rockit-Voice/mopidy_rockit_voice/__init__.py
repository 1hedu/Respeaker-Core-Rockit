"""
Mopidy-Rockit-Voice: Voice Control for Rockit Synth via MIDI

Based on mopidy-hallo by ReSpeaker
Modified to send MIDI commands to Rockit synth instead of playing music

This keeps the microphone claimed by Mopidy continuously, avoiding the
release/reclaim cycle that causes audio noise on ReSpeaker.
"""
from __future__ import unicode_literals

import logging
import os
import socket
import time
from threading import Thread, Event

import pykka
from mopidy import config, ext, core
from mopidy.audio import PlaybackState

try:
    from respeaker import Microphone, pixel_ring
    RESPEAKER_AVAILABLE = True
except ImportError:
    RESPEAKER_AVAILABLE = False
    print("Warning: ReSpeaker library not available. Voice control will not work.")

__version__ = '0.1.0'

logger = logging.getLogger(__name__)


class RockitVoiceFrontend(pykka.ThreadingActor, core.CoreListener):
    """
    Mopidy frontend that listens for voice commands and sends MIDI to Rockit synth.

    Keeps the microphone continuously claimed to avoid ALSA device release issues.
    Uses keyword spotting for simple, reliable voice control.
    """

    def __init__(self, config, core):
        super(RockitVoiceFrontend, self).__init__()
        self.core = core
        self.config = config['rockit_voice']
        self.quit_event = Event()

        # MIDI configuration
        self.midi_host = self.config['midi_host']
        self.midi_port = self.config['midi_port']

        # MIDI CC mappings for Rockit parameters
        self.cc_map = {
            'volume': 7,
            'release': 70,
            'resonance': 71,
            'mix': 72,
            'attack': 73,
            'cutoff': 74,
            'decay': 75,
            'envelope': 85,
            'sustain': 86,
            'glide': 90,
        }

        # Modifier values
        self.modifiers = {
            'up': 20,      # Increment by ~15%
            'down': -20,   # Decrement by ~15%
            'max': 127,
            'min': 0,
            'zero': 0,
            'half': 64,
            'on': 100,
            'off': 0,
        }

        # Note mappings
        self.notes = {
            'c': 60,
            'd': 62,
            'e': 64,
            'f': 65,
            'g': 67,
            'a': 69,
            'b': 71,
        }

        # Current parameter values (for relative adjustments)
        self.current_values = {}
        for param in self.cc_map.keys():
            self.current_values[param] = 64  # Start at middle

    def on_start(self):
        """Start the voice recognition thread"""
        if not RESPEAKER_AVAILABLE:
            logger.error("ReSpeaker library not available. Voice control disabled.")
            return

        thread = Thread(target=self._run)
        thread.daemon = True
        thread.start()

    def on_stop(self):
        """Stop the voice recognition"""
        self.quit_event.set()

    def _run(self):
        """Main loop: listen for wake word, then command keywords"""
        if not RESPEAKER_AVAILABLE:
            return

        try:
            logger.info('Initializing microphone...')
            mic = Microphone()
            logger.info('Rockit Voice Control started. Listening for wake word...')
            logger.info('MIDI target: {0}:{1}'.format(self.midi_host, self.midi_port))
        except Exception as e:
            logger.error('Failed to initialize microphone: {0}'.format(e))
            logger.error('Voice control disabled. Check if another process is using the microphone.')
            return

        while not self.quit_event.is_set():
            try:
                # Wait for wake word
                if mic.wakeup('respeaker'):
                    logger.info('Wake word detected!')
                    pixel_ring.listen()  # Show listening mode (green LEDs)

                    # Pause Mopidy playback while listening
                    if self.core.playback.get_state().get() == PlaybackState.PLAYING:
                        self.core.playback.pause()

                    # Listen for command (use recognize which works with keywords)
                    logger.info('Listening for command...')
                    data = mic.listen()
                    text = mic.recognize(data)

                    if text:
                        logger.info('Recognized: "{0}"'.format(text))
                        pixel_ring.wait()  # Show processing (spinning green LEDs)

                        # Process command
                        self._process_keywords(text.lower())
                    else:
                        logger.info('No keywords recognized')
                        pixel_ring.off()

                    time.sleep(1)
                    pixel_ring.off()

            except Exception as e:
                logger.error('Error in voice recognition: {0}'.format(e))
                pixel_ring.off()
                time.sleep(1)

    def _process_keywords(self, text):
        """Process recognized keywords and execute MIDI commands"""
        logger.info('Processing keywords: {0}'.format(text))

        words = text.split()
        if not words:
            logger.info('No words to process')
            return

        # Check for note playing: "play [note]" or just "[note]"
        if 'play' in words or 'note' in words:
            for word in words:
                if word in self.notes:
                    self._play_note(self.notes[word])
                    return
            # Default to middle C if no note specified
            self._play_note(60)
            return

        # Check for stop command
        if 'stop' in words or 'mute' in words:
            self._send_cc(123, 0)  # All notes off
            logger.info('All notes off')
            return

        # Check for parameter control: "[parameter] [modifier]"
        param = None
        modifier = None

        for word in words:
            if word in self.cc_map:
                param = word
            if word in self.modifiers:
                modifier = word

        if param and modifier:
            self._adjust_parameter(param, modifier)
            return

        # Check for parameter only (default to 'up')
        if param:
            self._adjust_parameter(param, 'up')
            return

        logger.info('No recognized command pattern in: {0}'.format(text))

    def _adjust_parameter(self, param, modifier):
        """Adjust a Rockit parameter via MIDI CC"""
        cc_num = self.cc_map[param]
        current = self.current_values[param]

        # Calculate new value
        if modifier in ['up', 'down']:
            delta = self.modifiers[modifier]
            new_value = current + delta
        else:
            new_value = self.modifiers[modifier]

        # Clamp to MIDI range
        new_value = max(0, min(127, new_value))

        # Send MIDI
        self._send_cc(cc_num, new_value)

        # Update current value
        self.current_values[param] = new_value

        logger.info('{0} {1}: {2} -> {3} (CC{4})'.format(
            param, modifier, current, new_value, cc_num))

    def _play_note(self, note):
        """Play a note - holds until stop command"""
        logger.info('Playing note {0} (held)'.format(note))
        self._send_midi([0x90, note, 100])  # Note on - stays on!

    def _send_cc(self, cc_num, value):
        """Send MIDI Control Change"""
        self._send_midi([0xB0, cc_num, value])

    def _send_midi(self, midi_bytes):
        """Send raw MIDI bytes to Rockit synth via TCP"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(1.0)
            sock.connect((self.midi_host, self.midi_port))
            sock.sendall(bytes(midi_bytes))
            sock.close()
            logger.debug('Sent MIDI: {0}'.format(midi_bytes))
            return True
        except Exception as e:
            logger.error('Failed to send MIDI: {0}'.format(e))
            return False

    # ========================================================================
    # Mopidy Integration
    # ========================================================================

    def volume_changed(self, volume):
        """Sync Mopidy volume to pixel ring display"""
        if RESPEAKER_AVAILABLE:
            pixel_ring.set_volume(volume)

    def mute_changed(self, mute):
        """Handle mute state changes"""
        pass


class Extension(ext.Extension):
    """Mopidy extension registration"""

    dist_name = 'Mopidy-Rockit-Voice'
    ext_name = 'rockit_voice'
    version = __version__

    def get_default_config(self):
        return config.read(os.path.join(os.path.dirname(__file__), 'ext.conf'))

    def get_config_schema(self):
        schema = super(Extension, self).get_config_schema()
        schema['midi_host'] = config.String()
        schema['midi_port'] = config.Integer()
        return schema

    def setup(self, registry):
        registry.add('frontend', RockitVoiceFrontend)
