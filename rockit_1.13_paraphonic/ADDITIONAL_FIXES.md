# Additional Fixes for Pitch/Detune Stuck Issue

## The Problem You Reported

When cycling through LFO destinations with amount at 0, the sound changes (shouldn't happen), and when landing on Pitch Shift or Detune, those parameters get **stuck** and won't respond to knobs anymore.

## Root Causes Found

### Issue 1: Parameters Get Stuck When Cycling Destinations
When you press the LFO DEST button to cycle through destinations:
1. Destination changes (e.g., to PITCH_SHIFT)
2. But the parameter value doesn't immediately sync to the knob position
3. Parameter source might still be SOURCE_EXTERNAL (from loaded patch)
4. Result: Parameter is stuck at patch value until you wiggle the knob enough

### Issue 2: PITCH_SHIFT Set to Wrong Value
Original code (line 171): `p_global_setting->auc_synth_params[PITCH_SHIFT] = 127;`
- **WRONG!** ZERO_PITCH_BEND is defined as 64, not 127
- This caused pitch issues when leaving pitch shift as destination

## Fixes Applied

### Fix 1: led_switch_handler.c - Force AD Source When Cycling Destinations

**When you change LFO destination with amount=0:**
```c
// NEW CODE:
if(p_global_setting->auc_synth_params[LFO_1_AMOUNT] == 0)
{
    unsigned char new_dest = auc_lfo_dest_decode[LFO_1][uc_led_lfo_dest_state];
    // Force parameter source to AD so knob takes control immediately
    p_global_setting->auc_parameter_source[new_dest] = SOURCE_AD;
}
```

**What this does:**
- When LFO amount is 0 and you cycle destinations
- Immediately sets the new destination parameter to use AD (knob) as source
- Prevents parameters getting stuck at patch/external values
- Knob can control the parameter right away

### Fix 2: led_switch_handler.c - Correct PITCH_SHIFT Center Value

**Changed line 171:**
```c
// BEFORE:
p_global_setting->auc_synth_params[PITCH_SHIFT] = 127;

// AFTER:
p_global_setting->auc_synth_params[PITCH_SHIFT] = ZERO_PITCH_BEND;  // = 64
```

### Fix 3: eight_bit_synth_main.c - Initialize External Params

**Added initialization for external_params:**
```c
global_setting.auc_external_params[LFO_1_AMOUNT] = 0;
global_setting.auc_external_params[LFO_2_AMOUNT] = 0;
global_setting.auc_external_params[PITCH_SHIFT] = ZERO_PITCH_BEND;
```

**Why:** Prevents issues when loading empty patches

### Fix 4: save_recall.c - Handle Empty Patches

**Added check after loading patch:**
```c
if(p_global_setting->auc_synth_params[PITCH_SHIFT] == 0)
{
    p_global_setting->auc_synth_params[PITCH_SHIFT] = ZERO_PITCH_BEND;
    p_global_setting->auc_external_params[PITCH_SHIFT] = ZERO_PITCH_BEND;
}
```

**Why:** Empty patches have PITCH_SHIFT=0, which causes extreme pitch down

## Expected Behavior After Fix

1. **Cycling LFO destinations with amount=0:**
   - Sound should NOT change as you cycle
   - When you land on any destination, knob responds immediately
   - No more stuck parameters

2. **Pitch Shift:**
   - Empty patches now have correct center value (64)
   - Cycling through destinations sets correct value
   - No more pitch weirdness

3. **Detune:**
   - Should work normally (detune=0 is probably correct neutral)
   - Knob responds immediately when selected as destination

## Why The Original Code Had This Issue

The original designer tried to handle this with the "passthrough" code in lfo.c (the buggy code we removed). That code was supposed to keep parameters updated even when LFO amount=0, but it created more problems than it solved (fighting with envelopes, patches, etc.).

Our solution is cleaner: 
- When amount=0, LFO does nothing ✓
- When cycling destinations with amount=0, force parameter to use AD source ✓
- AD reader can update the parameter immediately ✓

## Testing

After flashing:
1. Set LFO amount to 0
2. Press LFO DEST button repeatedly to cycle through all 6 destinations
3. Sound should remain stable, not change
4. At each destination, turn the corresponding knob - should respond immediately
5. Pay special attention to Pitch Shift and Detune - should work smoothly

## Files Changed

1. led_switch_handler.c - Main fix for stuck parameters
2. eight_bit_synth_main.c - Better initialization
3. save_recall.c - Handle empty patches
4. (Previous fixes in lfo.c and read_ad.c still apply)
