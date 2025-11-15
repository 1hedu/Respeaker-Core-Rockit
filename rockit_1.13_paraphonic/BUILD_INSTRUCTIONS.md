# Build Instructions - Rockit Paraphonic Fixed

## Requirements

- avr-gcc toolchain
- avrdude for programming
- USB programmer (usbasp or Arduino as ISP)

## Quick Build

```bash
# Extract the zip
unzip rockit_paraphonic_fixed.zip
cd fixed_rockit

# Compile
make clean
make

# Check the size (should fit in atmega644p)
# Flash size should be under 64KB
# RAM usage should be under 4KB

# Program (choose one method):

# Method 1: Using USBasp
make program

# Method 2: Using Arduino as ISP (adjust port as needed)
make program-arduino

# Method 3: Manual avrdude
avrdude -p atmega644p -c usbasp -U flash:w:rockit_paraphonic.hex:i
```

## Build Output

After successful compilation you'll have:
- `rockit_paraphonic.hex` - The firmware to flash
- `rockit_paraphonic.elf` - ELF file with debug symbols
- `rockit_paraphonic.map` - Memory map

## Makefile Targets

- `make` or `make all` - Compile everything
- `make clean` - Remove all build files
- `make size` - Show memory usage
- `make program` - Flash using USBasp
- `make program-arduino` - Flash using Arduino as ISP

## Troubleshooting

### Compilation Errors

If you get "command not found" errors:
```bash
# Install avr-gcc on Ubuntu/Debian:
sudo apt-get install gcc-avr avr-libc avrdude

# Install on macOS:
brew tap osx-cross/avr
brew install avr-gcc avrdude
```

### Programming Errors

If avrdude can't find the device:
- Check USB connections
- Verify programmer type (`-c usbasp` or `-c stk500v1`)
- Check device permissions (may need sudo)
- Verify correct port for Arduino ISP (`/dev/ttyUSB0` on Linux, `/dev/cu.usbserial*` on macOS)

### Size Issues

If the hex file is too large:
```bash
# Check current size
make size

# The atmega644p has:
# - 64KB flash (program memory)
# - 4KB SRAM
# - 2KB EEPROM
```

The paraphonic firmware should compile to approximately:
- Flash: ~50-55KB (well under the 64KB limit)
- SRAM: ~2-3KB (should be safe)

## What Changed From Original

This firmware includes:
1. All paraphonic voice allocation features
2. Bug fixes for LFO behavior
3. Fixed MIDI note handling
4. Improved AD reader logic

See BUGFIXES.md for detailed changes.

## Fuse Settings

If programming a fresh chip, use these fuse settings for atmega644p:

```bash
# 8MHz internal oscillator, no clock divide
avrdude -p atmega644p -c usbasp -U lfuse:w:0xE2:m -U hfuse:w:0xD9:m -U efuse:w:0xFF:m
```

**WARNING:** Double-check fuse settings for your specific hardware! Incorrect fuses can brick the chip.

## Verification

After flashing:
1. Power cycle the Rockit
2. Check that it boots (LED display shows patch number)
3. Test basic functionality (play a note)
4. Test LFO with amount at 0 (should not affect sound)
5. Try loading/saving patches
6. Test paraphonic modes (CC 102 to switch modes)

## Support Files

- `BUGFIXES.md` - Detailed explanation of all fixes
- `FIXES_SUMMARY.txt` - Quick overview
- `BUILD_INSTRUCTIONS.md` - This file
- All original Rockit documentation
- Paraphonic-specific documentation (INTEGRATION_GUIDE.md, etc.)
