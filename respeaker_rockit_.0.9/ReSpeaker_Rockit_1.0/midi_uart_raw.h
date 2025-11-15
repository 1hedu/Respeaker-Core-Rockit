#pragma once
#include <stdint.h>

// Function to start monitoring a UART device file for raw MIDI data
int midi_uart_raw_start(const char* device_path,
                        void (*on_note_on)(uint8_t), 
                        void (*on_note_off)(uint8_t),
                        void (*on_cc)(uint8_t,uint8_t));
                        
void midi_uart_raw_stop(void);