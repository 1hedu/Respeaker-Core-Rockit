#pragma once
#include <stdint.h>
#include <stddef.h>
#include "params.h"

typedef struct { 
    uint32_t dummy; 
} rockit_engine_t;

void rockit_engine_init(rockit_engine_t *e);
void rockit_engine_render(rockit_engine_t *e, int16_t *out, size_t frames, int sample_rate);
void rockit_note_on(uint8_t midi_note);
void rockit_note_off(uint8_t midi_note);
void rockit_handle_cc(uint8_t cc, uint8_t value);
