# Final Fixes - Release and Stock Patch Issues

## Issues Fixed in This Update

### 1. Weird Low-Fi Noise on Release (Power-Up & User Patches)
**Root Cause:** In `clear_voice()`, when a note was released, the paraphonic code was setting the oscillator note to 0 (very low pitch). The ADSR release phase would then play this very low note, creating digital noise.

**Fix:** Don't change the oscillator note when clearing voices. Let the ADSR release play out naturally on the last note that was playing.

**File:** rockit_paraphonic.c, lines 532-547

### 2. Can't Select LFO2 After Loading Stock/Empty Patch
**Root Cause:** When loading a patch with all zeros, LFO destinations were set to 0 and all parameters had SOURCE_EXTERNAL. This locked out the AD reader from updating those parameters, preventing LFO selection from working properly.

**Fix:** After loading a patch, if LFO amounts are 0, immediately set the destination parameters to use SOURCE_AD so knobs can control them.

**File:** save_recall.c, added after line 162

### 3. Weird Beating/Modulation When Holding Notes (Stock Patch)
**Root Cause:** Related to #2 - LFOs were targeting parameters but those parameters were locked to external source values from the patch, creating interference.

**Fix:** Same as #2 - by setting parameters to SOURCE_AD when LFO amount=0, the beating stops.

### 4. Synth Starts in Paraphonic Mode Instead of Mono
**Root Cause:** `paraphonic_init()` was setting mode to MODE_ROUND_ROBIN by default.

**Fix:** Changed default mode to MODE_MONOPHONIC for classic Rockit behavior at startup.

**File:** rockit_paraphonic.c, line 74

### 5. Filter Not Engaged in Stock Patch
**Root Cause:** Stock/empty patches have all parameters at 0, which might put the filter in an off state.

**Fix:** The LFO parameter source fix (#2) should help, but you may still need to save a proper default patch with filter engaged.

## Summary of Changes

### rockit_paraphonic.c
1. **Line 74:** Changed startup mode from ROUND_ROBIN to MONOPHONIC
2. **Lines 532-547:** Removed code that was setting oscillator notes to 0 on voice clear

### save_recall.c  
1. **After line 162:** Added check to set LFO destination parameters to SOURCE_AD when LFO amounts are 0
2. **Added includes:** lfo.h for access to auc_lfo_dest_decode table

## Expected Behavior After These Fixes

**Power-Up (uninitialized RAM):**
- ✅ Starts in monophonic mode
- ✅ Release works correctly (no weird noise)
- ✅ Can select LFO2

**Stock/Empty Patch:**
- ✅ Release works correctly
- ✅ Can select LFO2
- ✅ No weird beating
- ⚠️ Filter may still need manual engagement (consider saving a proper default patch)

**User-Saved Patch:**
- ✅ Everything works as expected

## How The Release Bug Worked

**Original Code Flow:**
1. User releases key
2. `paraphonic_note_off()` called
3. `clear_voice()` sets `auc_midi_note_index[OSC_1] = 0` ← BUG!
4. ADSR envelope goes into release phase
5. Oscillator is now playing note 0 (very low) during release
6. Result: Low-fi digital noise during release tail

**Fixed Code Flow:**
1. User releases key
2. `paraphonic_note_off()` called
3. `clear_voice()` marks voice as inactive but doesn't touch oscillator note
4. ADSR envelope goes into release phase
5. Oscillator continues playing the SAME note (whatever was last playing)
6. Result: Clean, natural release of the note you were playing

## Classic Rockit Behavior Preserved

In the original Rockit (monophonic):
- When you release a key, the oscillator keeps playing the same pitch
- The ADSR envelope controls the amplitude during release
- The note naturally fades out

Our fix restores this behavior. The paraphonic code now only manages voice allocation and triggering, not the actual release behavior.

## Recommendation

After flashing, save a "default" patch with good settings:
- Filter engaged (type selected)
- LFO amounts at 0
- PITCH_SHIFT at center (64)
- Other parameters at sensible defaults

Then save this to all 16 patch slots. This ensures you never load a problematic all-zero patch.

## Testing

1. Power cycle the Rockit
2. Play a note and release - should have clean, natural release (no noise)
3. Load stock patch - should work correctly now
4. Try to select LFO2 - should work
5. Hold a note - should be steady, no beating
6. Try all paraphonic modes with CC 102
7. Verify monophonic mode is default on power-up
