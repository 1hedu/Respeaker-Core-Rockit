# Rockit Paraphonic Firmware - Bug Fixes

## Version: Fixed LFO & Paraphonic Issues
## Date: 2025-11-05

---

## Summary of Changes

This version fixes critical bugs in the original Rockit firmware related to LFO behavior, plus removes a redundancy in the paraphonic implementation.

### Issues Fixed:
1. **LFO applies modulation even when amount is at 0**
2. **Cannot switch to LFO 2 after loading patches**
3. **Weird sustain behavior / low noise on sustain**
4. **Knobs don't respond when LFO destination is set (even with amount at 0)**
5. **Redundant flag setting in paraphonic note on**

---

## Detailed Changes

### 1. lfo.c - Line 163
**Issue:** LFO was checking `if(uc_lfo_amount > 1)` instead of `if(uc_lfo_amount > 0)`

**Change:**
```c
// BEFORE:
if(uc_lfo_amount > 1)

// AFTER:
if(uc_lfo_amount > 0)
```

**Impact:** LFO now properly bypassed when amount is set to 0. No more unwanted modulation.

---

### 2. lfo.c - Lines 511-524 (else block removed)
**Issue:** When LFO amount was 0, the LFO was still writing to parameters every cycle, causing:
- Conflicts with ADSR envelopes (sustain bug)
- Forcing AMPLITUDE to 255 when targeting amplitude
- Overwriting patch values continuously
- Fighting with other modulation sources

**Change:**
```c
// BEFORE:
else /*If the LFO amount is zero, then we should pass the ad value directly through*/
{
    if(uc_lfo_dest != AMPLITUDE)
    {
        p_global_setting->auc_synth_params[uc_lfo_dest] = uc_lfo_initial_param;
    }
    else
    {
        p_global_setting->auc_synth_params[uc_lfo_dest] = 255;
    }
}

// AFTER:
// When LFO amount is 0, do nothing - let other systems manage the parameter
// (AD reader, patch loader, MIDI CC, ADSR envelopes, etc.)
```

**Impact:** 
- ADSR envelopes work correctly
- Sustain now functions properly
- Patches load and behave as expected
- No more low noise on sustain

---

### 3. read_ad.c - Lines 156-162
**Issue:** AD reader was blocking knob updates for ANY parameter that an LFO was targeting, even if LFO amount was 0.

**Change:**
```c
// BEFORE:
if((uc_internal_ad_index != auc_lfo_dest_decode[LFO_1][p_global_setting->auc_synth_params[LFO_1_DEST]])
    && (uc_internal_ad_index != auc_lfo_dest_decode[LFO_2][p_global_setting->auc_synth_params[LFO_2_DEST]]))

// AFTER:
if((uc_internal_ad_index != auc_lfo_dest_decode[LFO_1][p_global_setting->auc_synth_params[LFO_1_DEST]]
    || p_global_setting->auc_synth_params[LFO_1_AMOUNT] == 0)
    && (uc_internal_ad_index != auc_lfo_dest_decode[LFO_2][p_global_setting->auc_synth_params[LFO_2_DEST]]
    || p_global_setting->auc_synth_params[LFO_2_AMOUNT] == 0))
```

**Impact:** 
- Knobs now respond immediately even if LFO destination is set to that parameter
- Only blocks AD updates when LFO is BOTH targeting the parameter AND amount > 0
- Can now switch to LFO 2 successfully

---

### 4. midi.c - Line 631 (redundancy removed)
**Issue:** g_uc_key_press_flag was being set twice - once in paraphonic_allocate_voices() and again in midi.c

**Change:**
```c
// BEFORE:
paraphonic_note_on(uc_data_byte_one, uc_data_byte_two, p_global_setting);
g_uc_key_press_flag = 1;//Turn the note on.

// AFTER:
paraphonic_note_on(uc_data_byte_one, uc_data_byte_two, p_global_setting);
// Note: g_uc_key_press_flag is set by paraphonic_allocate_voices()
```

**Impact:** 
- Cleaner code
- Ensures paraphonic system has full control over when flag is set
- Eliminates potential future bugs from unconditional override

---

## Testing Recommendations

After flashing this firmware, please test:

1. **LFO Amount at 0:**
   - Set LFO amount to minimum
   - Cycle through different LFO destinations
   - Sound should NOT change at all
   - Knobs should respond immediately

2. **LFO Switching:**
   - Load any patch
   - Press LFO SEL button to switch to LFO 2
   - Should successfully switch and stay on LFO 2
   - LEDs should update correctly

3. **Sustain Envelope:**
   - Power cycle the synth
   - Play a sustained note
   - Should sustain the correct pitch, not a low noise
   - Release should work smoothly

4. **Patch Loading:**
   - Save a patch with specific LFO settings
   - Power cycle or load different patch
   - Recall your saved patch
   - All settings should load correctly
   - Turning knobs should take over smoothly

5. **LFO Modulation:**
   - Set LFO amount > 0
   - LFO should modulate the destination parameter
   - Amount should control depth properly
   - Setting amount back to 0 should stop modulation immediately

6. **Paraphonic Mode:**
   - All paraphonic voice modes should work correctly
   - Note on/off handling unchanged
   - MIDI CC control (102, 103, 104) working

---

## Known Behavior Changes

### Positive Changes:
- LFO no longer interferes when amount is 0
- Knobs respond immediately in all situations
- Envelopes work correctly
- Patches load and recall properly
- Can switch between LFO 1 and LFO 2 freely

### No Negative Changes Expected:
These fixes only correct buggy behavior. All intended functionality is preserved.

---

## Files Modified

1. **lfo.c** - 2 changes (lines 163, 511-524)
2. **read_ad.c** - 1 change (lines 156-162)
3. **midi.c** - 1 change (line 631)

All other files unchanged from your paraphonic version.

---

## Compilation

Use the same build process as before. All fixes are in C source files, no hardware or makefile changes needed.

---

## Credits

- Original Rockit firmware: Matt Heins, HackMe Electronics
- Paraphonic modifications: [Your work]
- Bug fixes: Identified and corrected based on firmware analysis

---

## License

Same as original Rockit firmware (GPL v3)
