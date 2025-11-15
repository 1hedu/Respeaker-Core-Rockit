#!/bin/bash

echo "Compiling audio generation test..."

cd /mnt/c/Users/darek/Downloads/rockit_respeaker_final/rockit_respeaker_v2

gcc -o test_audio_gen \
    test_audio_generation.c \
    rockit_engine.c \
    params.c \
    filter_svf.c \
    wavetables.c \
    paraphonic.c \
    -lm \
    -I.

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    echo ""
    echo "Running test..."
    ./test_audio_gen
else
    echo "Compilation failed!"
    exit 1
fi