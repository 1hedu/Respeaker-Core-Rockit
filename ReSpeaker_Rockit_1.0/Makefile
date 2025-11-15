# Cross-compiler setup
CC = mipsel-openwrt-linux-gcc
TARGET = respeaker_rockit

STAGING = $(HOME)/respeaker-staging
# -mdsp enables MIPS DSP ASE Rev 1 (provides ~3x speedup for audio processing)
CFLAGS = -std=gnu99 -Os -march=mips32r2 -mtune=24kec -mdsp -Wall -I. -I$(STAGING)/usr/include
LDFLAGS = -Wl,--no-as-needed -L$(STAGING)/usr/lib -Wl,-rpath-link,$(STAGING)/usr/lib

# SRCS: midi_alsa_seq.c removed, socket_midi_raw.c added, patch_storage.c added
SRCS = main.c rockit_engine.c params.c wavetables.c filter_svf.c socket_midi_raw.c patch_storage.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
    # The linking stage, using the correct library paths for the uClibc toolchain.
	$(CC) -o $@ $(OBJS) $(LDFLAGS) -lasound -lpthread -lm

%.o: %.c
    # THIS LINE MUST START WITH A TAB
	$(CC) $(CFLAGS) -c $< -o $@

clean:
    # THIS LINE MUST START WITH A TAB
	rm -f $(TARGET) $(OBJS)

deploy:
    # THIS LINE MUST START WITH A TAB
	scp $(TARGET) root@192.168.1.25:/tmp/

.PHONY: all clean deploy