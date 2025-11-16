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
    """

    def __init__(self, config, core):
        super(RockitVoiceFrontend, self).__init__()
        self.core = core
        self.config = config['rockit_voice']
        self.quit_event = Event()

        # MIDI configuration
        self.midi_host = self.config['midi_host']
        self.midi_port = self.config['midi_port']

        # Voice commands mapping
        self.commands = {
            'play': self._cmd_play_note,
            'stop': self._cmd_stop_notes,
            'louder': self._cmd_volume_up,
            'quieter': self._cmd_volume_down,
            'filter up': self._cmd_filter_up,
            'filter down': self._cmd_filter_down,
        }

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
        """Main loop: listen for wake word, recognize speech, send MIDI"""
        if not RESPEAKER_AVAILABLE:
            return

        mic = Microphone()

        logger.info('Rockit Voice Control started. Listening for wake word...')
        logger.info('MIDI target: {0}:{1}'.format(self.midi_host, self.midi_port))

        while not self.quit_event.is_set():
            try:
                # Wait for wake word
                if mic.wakeup('respeaker'):
                    logger.info('Wake word detected!')
                    pixel_ring.think()  # Show thinking animation

                    # Pause Mopidy playback while listening
                    if self.core.playback.get_state().get() == PlaybackState.PLAYING:
                        self.core.playback.pause()

                    # Listen and recognize
                    data = mic.listen()
                    text = mic.recognize(data)

                    if text:
                        logger.info('Recognized: "{0}"'.format(text))
                        pixel_ring.speak()  # Show speaking animation

                        # Process command
                        self._process_command(text.lower())
                    else:
                        logger.info('No speech recognized')
                        pixel_ring.off()

                    time.sleep(1)
                    pixel_ring.off()

            except Exception as e:
                logger.error('Error in voice recognition: {0}'.format(e))
                pixel_ring.off()
                time.sleep(1)

    def _process_command(self, text):
        """Process recognized voice command"""
        logger.info('Processing command: {0}'.format(text))

        # Check for known commands
        for cmd_phrase, cmd_func in self.commands.items():
            if cmd_phrase in text:
                logger.info('Matched command: {0}'.format(cmd_phrase))
                cmd_func(text)
                return

        logger.info('Unknown command: {0}'.format(text))

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

    def _send_note_on(self, note, velocity=100):
        """Send MIDI Note On (0x90)"""
        return self._send_midi([0x90, note, velocity])

    def _send_note_off(self, note):
        """Send MIDI Note Off (0x80)"""
        return self._send_midi([0x80, note, 0])

    def _send_cc(self, cc_num, value):
        """Send MIDI Control Change (0xB0)"""
        return self._send_midi([0xB0, cc_num, value])

    # ========================================================================
    # Command Handlers
    # ========================================================================

    def _cmd_play_note(self, text):
        """Play a note - could be expanded to parse note names"""
        # Simple demo: play middle C (MIDI note 60)
        note = 60

        # Try to extract note from speech (simple matching)
        note_names = {
            'c': 60, 'see': 60,
            'd': 62, 'dee': 62,
            'e': 64,
            'f': 65,
            'g': 67,
            'a': 69,
            'b': 71,
        }

        for name, midi_note in note_names.items():
            if name in text:
                note = midi_note
                break

        logger.info('Playing note {0}'.format(note))
        self._send_note_on(note, 100)
        time.sleep(0.5)
        self._send_note_off(note)

    def _cmd_stop_notes(self, text):
        """Stop all notes (MIDI All Notes Off)"""
        logger.info('Stopping all notes')
        self._send_cc(123, 0)  # CC 123 = All Notes Off

    def _cmd_volume_up(self, text):
        """Increase master volume (CC 7)"""
        logger.info('Volume up')
        self._send_cc(7, 110)  # Set to ~85%

    def _cmd_volume_down(self, text):
        """Decrease master volume (CC 7)"""
        logger.info('Volume down')
        self._send_cc(7, 64)  # Set to 50%

    def _cmd_filter_up(self, text):
        """Increase filter cutoff (CC 74)"""
        logger.info('Filter up')
        self._send_cc(74, 100)

    def _cmd_filter_down(self, text):
        """Decrease filter cutoff (CC 74)"""
        logger.info('Filter down')
        self._send_cc(74, 30)

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
