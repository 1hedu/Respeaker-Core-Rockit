# ðŸŽ¹ Rockit Paraphonic v2.0 - NEW FEATURES!

## âœ¨ What's New in v2.0

### **MONO/PARA TOGGLE!**
Now you can switch between classic monophonic Rockit and paraphonic modes!

### **4 TOTAL MODES:**
1. **Monophonic** - Classic Rockit (detuned unison)
2. **Low Note Priority** - 3 lowest notes
3. **Last Note Priority** - 3 most recent notes
4. **Round Robin** - Cycles through voices

### **HARDWARE BUTTON CONTROL!**
Press **SELECT + SYNC** buttons together to cycle through all modes!

### **7-SEGMENT DISPLAY FEEDBACK!**
When you change modes, the display briefly shows:
- **0** = Monophonic
- **1** = Low Note
- **2** = Last Note
- **3** = Round Robin

Then returns to showing patch number.

---

## ðŸŽ›ï¸ How to Control

### Method 1: MIDI CC (No Button Mod Needed)

**CC 103 - Mono/Para Toggle:**
```
Send CC 103, value 0-63   = Monophonic mode
Send CC 103, value 64-127 = Paraphonic mode (keeps current para mode)
```

**CC 102 - Para Mode Select** (when in paraphonic):
```
Send CC 102, value 0-42   = Low Note Priority
Send CC 102, value 43-84  = Last Note Priority
Send CC 102, value 85-127 = Round Robin
```

### Method 2: Hardware Buttons (Optional)

**Press SELECT + SYNC together:**
- Cycles through: Mono â†’ Low â†’ Last â†’ RR â†’ Mono...
- LED changes to show current mode
- Instant tactile feedback!

See **BUTTON_INTEGRATION.md** for code to add button control.

---

## ðŸŽµ Mode Comparison

### **Monophonic Mode (LED 1)**
```
Play C â†’ Both DCOs play C (detuned unison)
Play C+E â†’ Only E sounds (last note wins)
```
**Use for:** Classic mono leads, bass, standard synth playing

### **Low Note Priority (LED 2)**
```
Play C-E-G â†’ DCO1=C, DCO2=E, EF-101D=G
Add B above â†’ Still C-E-G (lowest 3 locked)
```
**Use for:** Bass lines with chord accompaniment, stable voicings

### **Last Note Priority (LED 3)**
```
Play C...E...G â†’ DCO1=E, DCO2=G (2 most recent)
```
**Use for:** Lead melodies, legato playing

### **Round Robin (LED 4)**
```
Note 1 â†’ DCO1
Note 2 â†’ DCO2
Note 3 â†’ EF-101D
Note 4 â†’ DCO1 (cycles)
```
**Use for:** Arpeggios, rhythmic patterns, movement

---

## ðŸŽ¹ Usage Examples

### Live Performance Switching

**Verse: Mono Bass**
```
Send CC 103 = 0 (mono mode)
Play tight bass line with classic Rockit sound
```

**Chorus: Paraphonic Pad**
```
Send CC 103 = 127 (para mode)
Send CC 102 = 100 (round robin)
Play lush chord progressions
```

### Quick Mode Changes

**During a song:**
- Press SELECT + SYNC to cycle modes
- Watch LED to confirm mode
- Keep playing - notes reallocate instantly!

---

## ðŸ’¾ Code Size

**v2.0 adds approximately:**
- Flash: +400 bytes (mono mode + button support)
- RAM: +10 bytes (mode state)
- **Total estimate: ~2.2 KB flash, ~110 bytes RAM**

Should still fit comfortably in ATMEGA164P!

---

## ðŸ”§ Build Options

### Option A: Full Features (Recommended)
- All 4 modes
- Both MIDI CC and button control
- LED feedback
- ~2.2 KB

### Option B: MIDI Only (No Button Code)
- All 4 modes
- CC 103 and CC 102 only
- LED feedback
- ~2.0 KB
- **Don't add BUTTON_INTEGRATION.md code**

### Option C: If Too Big
Remove Last Note mode (least useful):
1. Comment out `MODE_LAST_NOTE` case in code
2. Saves ~300 bytes
3. Still have Mono + Low + RR

---

## ðŸ“Š Mode Selection Matrix

| Control Method | CC 103 Value | CC 102 Value | Buttons | Result Mode |
|----------------|--------------|--------------|---------|-------------|
| MIDI | 0 | - | - | Monophonic |
| MIDI | 100 | 20 | - | Low Note |
| MIDI | 100 | 60 | - | Last Note |
| MIDI | 100 | 100 | - | Round Robin |
| Hardware | - | - | SELECT+SYNC | Cycles all 4 |

---

## ðŸŽ¨ Creative Ideas

### **Mode Per Section**
- Intro: Mono lead
- Verse: Low Note (bass + chords)
- Chorus: Round Robin (movement)
- Bridge: Last Note (expressive)

### **Live Jamming**
- Start in Round Robin
- Hit SELECT+SYNC to explore different textures
- LED shows where you are

### **Recording**
- Record multiple takes in different modes
- Layer them for mega-thick sound
- Mono for focus, para for width

---

## âš¡ Quick Start v2.0

```bash
# 1. Compile
make clean && make

# 2. Check size (should see ~2.2KB para code)
# Look for: Program: XXXXX bytes

# 3. Upload
make program

# 4. Test!
# Power on (starts in Round Robin, LED 4)
# Press SELECT+SYNC â†’ cycles to Mono (LED 1)
# Press again â†’ Low Note (LED 2)
# Press again â†’ Last Note (LED 3)
# Press again â†’ Round Robin (LED 4)
```

---

## ðŸ› Troubleshooting v2.0

**LEDs not changing?**
- Make sure button integration code is added
- Check that `update_mode_leds()` is being called

**Buttons don't work?**
- See BUTTON_INTEGRATION.md for code
- Check button combo detection logic
- Try MIDI CC control instead

**Won't fit in flash?**
- Remove Last Note mode (comment out case)
- Use `-Os` optimization in Makefile
- Or skip button integration code

---

## ðŸŽ¯ Upgrade from v1.0

If you have v1.0 installed:

**What Changed:**
- Added monophonic mode
- Added CC 103 for mono/para toggle
- Added button cycling support
- Added mode LED feedback

**Migration:**
- Just replace the 3 files: rockit_paraphonic.c/.h and midi.c
- Recompile and upload
- Existing CC 102 behavior unchanged
- v1.0 patches still work!

---

## ðŸ“ Default Behavior

**Power On:**
- Starts in Round Robin mode (LED 4 lit)
- Paraphonic enabled by default
- Ready to play chords immediately!

**To Start in Mono:**
- Edit `paraphonic_init()` line 73
- Change to `MODE_MONOPHONIC`
- Or send CC 103 = 0 on startup

---

## ðŸ’° Selling Points v2.0

**Now you can honestly say:**
- âœ… 4-mode voice allocation system
- âœ… Switchable mono/paraphonic
- âœ… Hardware button control
- âœ… Visual LED feedback
- âœ… MIDI automation support
- âœ… Best of both worlds!

---

**Version:** 2.0  
**Date:** November 2025  
**Status:** Ready to compile  
**New Features:** Mono mode, CC 103, button cycling, LED feedback

**Build it and enjoy the flexibility!** ðŸŽ¹âœ¨ðŸš€
