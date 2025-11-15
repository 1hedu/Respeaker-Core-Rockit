# Button Integration for Paraphonic Mode Switching

## How to Add SELECT + SYNC Button Combo

### Location: led_switch_handler.c

Find the switch button handling section (around line 140-250).

### Add This Code:

After the existing button handlers (around line 260), add a new section to check for button combinations:

```c
// PARAPHONIC: Check for SELECT + SYNC combo to cycle modes
// Add this BEFORE the individual button case statements

// Read current button state from I/O expander
unsigned char button_state = uc_received_byte_one;

// Check if BOTH SELECT and SYNC are pressed (both bits LOW)
if ((button_state & TACT_SELECT) == 0 && (button_state & TACT_LFO_SYNC) == 0) {
    // Both buttons pressed - cycle paraphonic mode
    static unsigned char last_combo_state = 0xFF;
    
    // Only trigger once per button press (debounce)
    if (last_combo_state == 0xFF) {
        paraphonic_cycle_mode(p_global_setting);
        last_combo_state = 0;
        uc_state = READ;
        clear_button_press_flag();
        return;  // Don't process individual buttons
    }
} else {
    // Buttons released - reset combo state
    last_combo_state = 0xFF;
}

// Continue with normal button handling...
```

### Alternative Simple Integration:

If you want even simpler, just add this to the existing TACT_SELECT case:

```c
case TACT_SELECT:
    // PARAPHONIC: Check if SYNC is also pressed
    if ((uc_received_byte_one & TACT_LFO_SYNC) == 0) {
        // Both SELECT and SYNC pressed - cycle mode
        paraphonic_cycle_mode(p_global_setting);
        uc_state = READ;
        clear_button_press_flag();
        break;
    }
    
    // Normal SELECT button handling (existing code)
    if(PIND & BUTTON_PRESS_MASK)
    {
        // ... rest of existing SELECT code ...
    }
    break;
```

## LED Feedback

The `update_mode_leds()` function automatically uses the LFO Dest LEDs to show mode:

- **LED 1** = Monophonic
- **LED 2** = Low Note Priority
- **LED 3** = Last Note Priority
- **LED 4** = Round Robin

## Testing

1. Press SELECT + SYNC together
2. LED should change showing new mode
3. Press again to cycle: Mono â†’ Low â†’ Last â†’ RR â†’ Mono...
4. Play notes to hear the mode change

## If You Don't Want Button Control

Just don't add this code! You can still use:
- CC 103 to switch between mono/para
- CC 102 to select para mode
- Buttons are optional

