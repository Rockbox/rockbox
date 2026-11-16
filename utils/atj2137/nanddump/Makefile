CROSS ?= mipsel-elf-
TCC = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
TCFLAGS = -Os -march=mips32r2 -msoft-float -G0 -mno-abicalls -fno-pic \
          -ffreestanding -fno-builtin -nostdlib -W -Wall

HWSTUB_DIR = ../../hwstub
CXXFLAGS = -Wall -O2 -std=c++11 -g -I$(HWSTUB_DIR)/include `pkg-config --cflags libusb-1.0`
LDFLAGS = -L$(HWSTUB_DIR)/lib -lhwstub `pkg-config --libs libusb-1.0` -pthread

all: helper.bin atjnanddump

helper.elf: start.S helper.c helper.h helper.lds
	$(TCC) $(TCFLAGS) -T helper.lds -o $@ start.S helper.c

# zero filled .bss is part of the image so it is cleared on upload
helper.bin: helper.elf
	$(OBJCOPY) -O binary -j .params -j .text $< $@

$(HWSTUB_DIR)/lib/libhwstub.a:
	make -C $(HWSTUB_DIR)/lib

atjnanddump: atjnanddump.cpp helper.h $(HWSTUB_DIR)/lib/libhwstub.a
	g++ $(CXXFLAGS) -o $@ atjnanddump.cpp $(LDFLAGS)

clean:
	rm -f helper.elf helper.bin atjnanddump
