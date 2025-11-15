#pragma once
#include <stdint.h>

// Starts a thread listening on the loopback address for 3-byte MIDI messages
int socket_midi_raw_start(uint16_t port,
                          void (*on_note_on)(uint8_t), 
                          void (*on_note_off)(uint8_t),
                          void (*on_cc)(uint8_t, uint8_t));

void socket_midi_raw_stop(void);