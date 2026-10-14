SHELL := /bin/bash

DEVKITXENON ?= $(PREFIX)
ifeq ($(strip $(DEVKITXENON)),)
$(error DEVKITXENON is not set)
endif

CC := $(DEVKITXENON)/bin/xenon-gcc
OBJCOPY := $(DEVKITXENON)/bin/xenon-objcopy
STRIP := $(DEVKITXENON)/bin/xenon-strip
TARGET := dreamkai360_cloud_probe
CFLAGS := -O2 -Wall -Wextra -std=c99 -DXENON -m32 -maltivec -fno-pic -mpowerpc64 -mhard-float -I$(DEVKITXENON)/usr/include -I../src
LDFLAGS := -m32 -maltivec -fno-pic -mpowerpc64 -mhard-float -L$(DEVKITXENON)/xenon/lib/32 -L$(DEVKITXENON)/usr/lib -Wl,--gc-sections -Wl,-Map,$(TARGET).map -n -T $(DEVKITXENON)/app.lds
LIBS := -lxenon -lm

.PHONY: all clean
all: $(TARGET).elf32

$(TARGET)_probe.o: xenon_probe.c ../src/dreamkai360.h
	$(CC) $(CFLAGS) -c $< -o $@

dreamkai360_core.o: ../src/dreamkai360.c ../src/dreamkai360.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(TARGET)_probe.o dreamkai360_core.o
	$(CC) $^ $(LDFLAGS) $(LIBS) -o $@

$(TARGET).elf32: $(TARGET).elf
	$(OBJCOPY) -O elf32-powerpc --adjust-vma 0x80000000 $< $@
	$(STRIP) $@

clean:
	rm -f $(TARGET)_probe.o dreamkai360_core.o $(TARGET).elf $(TARGET).elf32 $(TARGET).map
