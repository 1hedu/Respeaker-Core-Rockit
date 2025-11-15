# Makefile for Rockit Paraphonic v1.0
# Based on HackMe Rockit 1.12
# Modified to include paraphonic voice allocation

MCU = atmega644p
F_CPU = 8000000UL
FORMAT = ihex
TARGET = rockit_paraphonic

# Source files
CSRCS = eight_bit_synth_main.c \
		interrupt_routines.c \
		sys_init.c \
		spi.c \
		uart.c \
		oscillator.c \
		amp_adsr.c \
		filter.c \
		read_ad.c \
		led_switch_handler.c \
		lfo.c \
		midi.c \
		calculate_pitch.c \
		drone_loop.c \
		arpeggiator.c \
		save_recall.c \
		wavetables.c \
		rockit_paraphonic.c

# Compiler and linker flags
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size

CFLAGS =  -mmcu=$(MCU) -Os -Wall -Wstrict-prototypes -DF_CPU=$(F_CPU) -std=gnu99  -ffunction-sections -fdata-sections -I.
LDFLAGS = -Wl,-Map,$(TARGET).map -mmcu=$(MCU) -Wl,--gc-sections

# Objects
OBJS = $(CSRCS:.c=.o)

# Default target
all: $(TARGET).hex size

# Link
$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Convert to hex
$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Show size
size: $(TARGET).elf
	$(SIZE) --format=avr --mcu=$(MCU) $(TARGET).elf

# Clean
clean:
	rm -f $(TARGET).hex $(TARGET).elf $(TARGET).map $(OBJS)

# Program the device using avrdude (adjust programmer as needed)
program: $(TARGET).hex
	avrdude -p $(MCU) -c usbasp -U flash:w:$(TARGET).hex:i

# Alternative programming with Arduino as ISP
program-arduino: $(TARGET).hex
	avrdude -p $(MCU) -c stk500v1 -P /dev/ttyUSB0 -b 19200 -U flash:w:$(TARGET).hex:i

.PHONY: all clean size program program-arduino
