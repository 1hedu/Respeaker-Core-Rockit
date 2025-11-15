# Rockit Paraphonic Implementation Checklist

Use this checklist to track your progress integrating the paraphonic firmware.

## âœ… Pre-Integration

- [ ] Downloaded all firmware files
- [ ] Have working Rockit synth with programmer
- [ ] Have AVR development tools installed
- [ ] Have backup of original Rockit firmware (just in case!)
- [ ] Read README.md for overview

## âœ… Code Integration

- [ ] Copied `rockit_paraphonic.c` and `.h` to Rockit source directory
- [ ] Added to Makefile: `CSRCS += rockit_paraphonic.c`
- [ ] Added `#include "rockit_paraphonic.h"` to main file
- [ ] Implemented `set_dco1_note()` function
- [ ] Implemented `set_dco2_note()` function  
- [ ] Implemented `send_midi_note_out()` function
- [ ] Added `paraphonic_init()` to `main()`
- [ ] Modified MIDI handler to call `paraphonic_midi_handler()`
- [ ] Removed or commented out old monophonic note handling
- [ ] Code compiles without errors

## âœ… Hardware Setup

- [ ] Rockit MIDI OUT connected to Respeaker MIDI IN
- [ ] Respeaker I2C DAC configured and tested
- [ ] DAC CV output connected to EF-101D CV input
- [ ] EF-101D audio output routed back to Rockit input
- [ ] Pre-delay pedal inserted before filter (Rockit mod)
- [ ] Booster and tube pedals with order-swap rocker
- [ ] Stereo splitter after booster/tube
- [ ] Two chorus pedals with linked pots (inverted rate on slave)
- [ ] Monitor speakers connected to Respeaker amp

## âœ… Testing - Basic Functionality

- [ ] Rockit powers on normally after firmware upload
- [ ] Can play single notes
- [ ] DCO1 and DCO2 both working
- [ ] EF-101D receiving CV and tracking pitch
- [ ] All three oscillators audible in mix

## âœ… Testing - Paraphonic Modes

### Low Note Priority
- [ ] Send CC 102 value 0 (or 1-42)
- [ ] Play C-E-G-B chord
- [ ] Verify C-E-G are sounding (3 lowest)
- [ ] Release G, verify A starts playing if added
- [ ] Mode working correctly: **YES / NO**

### Last Note Priority  
- [ ] Send CC 102 value 50 (or 43-84)
- [ ] Play C-E-G-B slowly
- [ ] Verify E-G-B are sounding (3 most recent)
- [ ] Play another note, oldest should drop off
- [ ] Mode working correctly: **YES / NO**

### Round Robin
- [ ] Send CC 102 value 100 (or 85-127)
- [ ] Play repeated notes quickly
- [ ] Verify notes cycle through voices
- [ ] Should hear different voice for each new note
- [ ] Mode working correctly: **YES / NO**

## âœ… Testing - Effects Chain

- [ ] Pre-delay working and positioned before filter
- [ ] Delay creates desired texture with filter sweeps
- [ ] Booster and tube work in both orders
- [ ] Rocker switch toggles booster/tube order
- [ ] Stereo chorus creates wide image
- [ ] Inverted rate creates slow movement
- [ ] Monitor speakers working from Respeaker

## âœ… Final Testing

- [ ] No stuck notes when playing normally
- [ ] Notes release cleanly
- [ ] No audio pops or clicks on note changes
- [ ] Mode switches via CC without issues
- [ ] EF-101D stays in tune across octaves
- [ ] Pre-delay mix sweet spot found
- [ ] Chorus depth and rate adjusted
- [ ] Tube drive level set
- [ ] Overall output level good

## âœ… Sound Design

- [ ] Tested "Juno Pad" patch (Round Robin + chorus)
- [ ] Tested "Bass Stack" patch (Low Note + tube)
- [ ] Tested "Lead Synth" patch (Last Note + delay)
- [ ] Created at least 3 custom patches
- [ ] Documented best patches for demo

## âœ… Pre-Sale Prep

- [ ] Cleaned and tested all hardware
- [ ] Verified all knobs and switches work
- [ ] Prepared demo audio/video
- [ ] Wrote description highlighting paraphonic feature
- [ ] Priced synth appropriately  
- [ ] Ready to list!

## ðŸ“ Notes & Observations

**What worked well:**
_______________________________________________________

**Issues encountered:**
_______________________________________________________

**Solutions found:**
_______________________________________________________

**Best patches:**
1. _______________________________________________________
2. _______________________________________________________
3. _______________________________________________________

**Sale price target:** $_________

**Date completed:** ___________

---

## ðŸŽ¯ Success Criteria

You're ready to sell when:
âœ… All three modes work reliably
âœ… EF-101D tracks pitch accurately  
âœ… No stuck notes or audio glitches
âœ… Effects chain sounds great
âœ… Can demo impressive patches

## ðŸš¨ If Something's Not Working

1. Check INTEGRATION_GUIDE.md troubleshooting section
2. Review integration_example.c for code examples
3. Use MIDI monitor to debug note messages
4. Verify hardware connections  
5. Test each component individually

## ðŸ’° Selling Points to Emphasize

When listing this synth, highlight:
- 3-voice paraphonic (rare in this price range)
- Three intelligent voice allocation modes
- Juno-inspired chorus architecture
- Pre-filter delay for unique textures
- Tube overdrive with switchable routing
- Custom firmware development
- 8-bit + analog hybrid character
- Monitor speakers built-in
- Ready to perform/record

Good luck with the build and the sale! ðŸŽ¹ðŸš€
