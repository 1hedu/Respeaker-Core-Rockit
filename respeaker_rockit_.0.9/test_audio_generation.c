#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "rockit_engine.h"

int main() {
    rockit_engine_t engine;
    rockit_engine_init(&engine);
    
    printf("Testing audio generation...\n");
    printf("Master Volume: %d\n", params_get(P_MASTER_VOL));
    printf("Filter Cutoff: %d\n", params_get(P_FILTER_CUTOFF));
    printf("Filter Resonance: %d\n", params_get(P_FILTER_RESONANCE));
    
    // Trigger note 60 (middle C)
    printf("\nTriggering note 60...\n");
    rockit_note_on(60);
    
    // Render 4800 samples (100ms at 48kHz)
    int16_t buffer[4800 * 2]; // stereo
    rockit_engine_render(&engine, buffer, 4800, 48000);
    
    // Check if any audio was generated
    int64_t sum = 0;
    int64_t abs_sum = 0;
    int16_t max_val = 0;
    int16_t min_val = 0;
    
    for(int i = 0; i < 4800 * 2; i++) {
        sum += buffer[i];
        abs_sum += abs(buffer[i]);
        if(buffer[i] > max_val) max_val = buffer[i];
        if(buffer[i] < min_val) min_val = buffer[i];
    }
    
    printf("\nAudio Statistics:\n");
    printf("  Average value: %.2f\n", sum / (4800.0 * 2.0));
    printf("  Average absolute: %.2f\n", abs_sum / (4800.0 * 2.0));
    printf("  Max value: %d\n", max_val);
    printf("  Min value: %d\n", min_val);
    printf("  Peak-to-peak: %d\n", max_val - min_val);
    
    // Print first 20 samples
    printf("\nFirst 20 samples (left channel):\n");
    for(int i = 0; i < 20; i++) {
        printf("  [%d] = %d\n", i, buffer[i * 2]);
    }
    
    if(abs_sum == 0) {
        printf("\n*** ERROR: No audio generated! All samples are zero. ***\n");
        return 1;
    } else if(abs_sum < 1000) {
        printf("\n*** WARNING: Audio is very quiet (avg abs: %.2f) ***\n", abs_sum / (4800.0 * 2.0));
    } else {
        printf("\n*** SUCCESS: Audio is being generated! ***\n");
    }
    
    return 0;
}