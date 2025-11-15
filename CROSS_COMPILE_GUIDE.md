# Cross-Compilation Guide for ReSpeaker Core v1.0

Complete guide for cross-compiling the Rockit Synth firmware for ReSpeaker Core v1.0 (MIPS platform).

## Target Hardware Specifications

- **Device**: Seeed ReSpeaker Core v1.0
- **CPU**: MediaTek MT7688AN (MIPS 24KEc @ 580 MHz)
- **Architecture**: MIPS32 release 2 with DSP extensions
- **OS**: OpenWrt Chaos Calmer (Linux 3.18.23)
- **C Library**: uClibc 0.9.33.2 (NOT musl!)

## Prerequisites

### Required Tools
- Linux host system (Ubuntu/Debian recommended)
- ~2GB free disk space
- Internet connection for downloads

### Verify Target Device Info
```bash
# On ReSpeaker device:
uname -a
# Linux ReSpeaker 3.18.23 #3 Thu Mar 24 04:48:47 UTC 2016 mips GNU/Linux

cat /proc/cpuinfo | grep "cpu model"
# cpu model: MIPS 24KEc V5.0

ldd --version
# uClibc 0.9.33.2
```

## Step 1: Download OpenWrt SDK

**CRITICAL**: Must use Chaos Calmer 15.05.1 (uClibc-based), NOT newer versions (musl-based).

```bash
cd ~
mkdir respeaker-cross
cd respeaker-cross

# Download the correct SDK
wget https://downloads.openwrt.org/chaos_calmer/15.05.1/ramips/mt7688/OpenWrt-SDK-15.05.1-ramips-mt7688_gcc-4.8-linaro_uClibc-0.9.33.2.Linux-x86_64.tar.bz2

# Extract
tar xjf OpenWrt-SDK-15.05.1-ramips-mt7688_gcc-4.8-linaro_uClibc-0.9.33.2.Linux-x86_64.tar.bz2

# Set up environment (add to ~/.bashrc for persistence)
export SDK_PATH=~/respeaker-cross/OpenWrt-SDK-15.05.1-ramips-mt7688_gcc-4.8-linaro_uClibc-0.9.33.2.Linux-x86_64
export STAGING_DIR=$SDK_PATH/staging_dir
export PATH=$STAGING_DIR/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin:$PATH
```

### Verify Toolchain
```bash
mipsel-openwrt-linux-gcc --version
# Should show: gcc version 4.8.3 (OpenWrt/Linaro GCC 4.8-2014.04)

mipsel-openwrt-linux-gcc -dumpmachine
# Should show: mipsel-openwrt-linux-uclibc
```

## Step 2: Cross-Compile ALSA Libraries

The ReSpeaker device has ALSA support but the SDK doesn't include the development libraries.

```bash
cd ~/respeaker-cross
mkdir alsa-build
cd alsa-build

# Create staging directory for installed libraries
mkdir -p ~/respeaker-staging/usr

# Download ALSA library (version matching or close to device)
wget ftp://ftp.alsa-project.org/pub/lib/alsa-lib-1.1.9.tar.bz2
tar xjf alsa-lib-1.1.9.tar.bz2
cd alsa-lib-1.1.9

# Set cross-compilation environment variables
export CC=mipsel-openwrt-linux-gcc
export CXX=mipsel-openwrt-linux-g++
export AR=mipsel-openwrt-linux-ar
export RANLIB=mipsel-openwrt-linux-ranlib
export LD=mipsel-openwrt-linux-ld
export CFLAGS="-Os -march=mips32r2 -mtune=24kec -mdsp"
export LDFLAGS="-L$STAGING_DIR/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/lib"

# Configure for cross-compilation
./configure \
    --host=mipsel-openwrt-linux \
    --build=x86_64-linux-gnu \
    --prefix=/usr \
    --disable-python \
    --disable-mixer \
    --disable-rawmidi \
    --disable-hwdep \
    --disable-seq \
    --disable-alisp \
    --disable-old-symbols \
    --with-configdir=/usr/share/alsa \
    --with-plugindir=/usr/lib/alsa-lib

# Compile
make -j$(nproc)

# Install to staging directory
make install DESTDIR=~/respeaker-staging
```

### Verify ALSA Libraries
```bash
file ~/respeaker-staging/usr/lib/libasound.so.2.0.0
# Should show: ELF 32-bit LSB shared object, MIPS, MIPS32 rel2 version 1
```

## Step 3: Compile Rockit Synth

```bash
cd ~/Respeaker-Core-Rockit/ReSpeaker_Rockit_1.0

# Set environment
export STAGING=~/respeaker-staging
export CC=mipsel-openwrt-linux-gcc
```

### Makefile Setup

Create or update `Makefile`:

```makefile
# Cross-compilation toolchain
CC = mipsel-openwrt-linux-gcc
STAGING = $(HOME)/respeaker-staging

# Compiler flags
CFLAGS = -std=gnu99 -Os -march=mips32r2 -mtune=24kec -mdsp -Wall -I. -I$(STAGING)/usr/include

# Linker flags
LDFLAGS = -Wl,--no-as-needed -L$(STAGING)/usr/lib -Wl,-rpath-link,$(STAGING)/usr/lib

# Libraries (MUST come after object files)
LIBS = -lasound -lm -lpthread

# Source files
SOURCES = main.c rockit_engine.c params.c wavetables.c filter_svf.c socket_midi_raw.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = respeaker_rockit

# Build rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: all clean
```

### Compile
```bash
make clean
make

# Verify binary
file respeaker_rockit
# Should show: ELF 32-bit LSB executable, MIPS, MIPS32 rel2 version 1 (SYSV), dynamically linked, interpreter /lib/ld-uClibc.so.0

# Check dependencies
mipsel-openwrt-linux-readelf -d respeaker_rockit | grep NEEDED
# Should show libasound, libm, libpthread, etc.
```

## Step 4: Deploy to ReSpeaker

```bash
# Copy binary to device
scp respeaker_rockit root@respeaker.local:/root/

# Copy ALSA libraries (if needed)
scp ~/respeaker-staging/usr/lib/libasound.so* root@respeaker.local:/usr/lib/

# SSH into device
ssh root@respeaker.local

# Test
cd /root
./respeaker_rockit --tcp-midi

# Or specify audio device
./respeaker_rockit hw:0,0 --tcp-midi
```

## Common Issues and Solutions

### Issue: "No such file or directory" when running binary
```bash
# Check if interpreter exists
ls -l /lib/ld-uClibc.so.0

# If missing, wrong toolchain was used (likely musl-based SDK)
# Solution: Re-download Chaos Calmer 15.05.1 SDK
```

### Issue: "Incompatible library" errors
```bash
# Check library architecture
file /usr/lib/libasound.so.2

# If shows "musl" or wrong architecture:
# Solution: Re-cross-compile ALSA with correct toolchain
```

### Issue: Compilation fails with "error: 'for' loop initial declarations"
```bash
# GCC 4.8 requires explicit C99 mode
# Solution: Add -std=gnu99 to CFLAGS
```

### Issue: Undefined reference to pthread functions
```bash
# Library order matters
# WRONG: gcc -lpthread main.o
# RIGHT: gcc main.o -lpthread

# Solution: Put $(LIBS) AFTER $(OBJECTS) in Makefile
```

### Issue: ALSA configure fails to detect compiler
```bash
# Cross-compilation variables not set
# Solution: Export CC, AR, RANLIB before ./configure:
export CC=mipsel-openwrt-linux-gcc
export AR=mipsel-openwrt-linux-ar
export RANLIB=mipsel-openwrt-linux-ranlib
```

## Architecture Compatibility Matrix

| SDK Version | C Library | Compatible? | Notes |
|-------------|-----------|-------------|-------|
| Chaos Calmer 15.05 | uClibc 0.9.33.2 | ✅ YES | **USE THIS** |
| LEDE 17.01 | musl | ❌ NO | Wrong C library |
| OpenWrt 18.06+ | musl | ❌ NO | Wrong C library |
| Barrier Breaker 14.07 | uClibc | ⚠️ Maybe | GCC version may differ |

## Verification Commands

### On Host (During Compilation)
```bash
# Check binary architecture
file respeaker_rockit
mipsel-openwrt-linux-readelf -h respeaker_rockit

# Check library dependencies
mipsel-openwrt-linux-readelf -d respeaker_rockit | grep NEEDED

# Check symbols
mipsel-openwrt-linux-nm -D respeaker_rockit | grep snd_pcm
```

### On Target Device
```bash
# Check binary can load
ldd respeaker_rockit

# Verify ALSA devices
aplay -l

# Test audio output
speaker-test -t wav -c 2

# Monitor CPU usage
top
# Should stay under 30% CPU for synth
```

## Performance Tips

1. **Optimize for size**: Use `-Os` instead of `-O2` (limited flash space)
2. **Enable DSP extensions**: Use `-mdsp` flag for MIPS DSP instructions
3. **Static linking**: For production, consider `-static` to avoid library issues
4. **Strip symbols**: Use `mipsel-openwrt-linux-strip` to reduce binary size

## Additional Resources

- OpenWrt SDK Downloads: https://downloads.openwrt.org/
- ALSA Project: https://www.alsa-project.org/
- OpenWrt Build System: https://openwrt.org/docs/guide-developer/toolchain/use-buildsystem
- ReSpeaker Docs: https://wiki.seeedstudio.com/ReSpeaker/

## Success Criteria

✅ Binary file shows: `MIPS, MIPS32 rel2`
✅ ldd shows all dependencies found
✅ No "incompatible" errors when running
✅ Audio output works with aplay
✅ Synth produces sound on note events

---

**Last Updated**: 2025-01-15
**Tested On**: ReSpeaker Core v1.0 (MT7688)
**Toolchain**: OpenWrt SDK 15.05.1 / GCC 4.8.3 / uClibc 0.9.33.2
