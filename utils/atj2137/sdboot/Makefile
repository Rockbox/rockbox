CROSS ?= mipsel-elf-
CC = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
CFLAGS = -Os -march=mips32r2 -msoft-float -G0 -mno-abicalls -fno-pic \
         -ffreestanding -fno-builtin -nostdlib -W -Wall
LDFLAGS = -nostdlib -Wl,--gc-sections

BINS = stage1.bin stage15_sd.bin stage15_adfu.bin

all: $(BINS)

stage1.elf: stage1.S sdboot.h stage1.lds
	$(CC) $(CFLAGS) $(LDFLAGS) -T stage1.lds -o $@ stage1.S

# stage1.5 loaded by stage1 to IRAM from SD
stage15_sd.elf: start15.S stage15.c sdboot.h stage15.lds
	$(CC) $(CFLAGS) $(LDFLAGS) -T stage15.lds -Wl,--defsym=LOAD_ADDR=0xb4041400 \
	    -o $@ start15.S stage15.c

# stage1.5 uploaded with adfuload as stage2 to SDRAM, payload appended
stage15_adfu.elf: start15.S stage15.c sdboot.h stage15.lds
	$(CC) $(CFLAGS) $(LDFLAGS) -T stage15.lds -Wl,--defsym=LOAD_ADDR=0xa0000000 \
	    -o $@ start15.S stage15.c

%.bin: %.elf
	$(OBJCOPY) -O binary -j .text $< $@

clean:
	rm -f *.elf $(BINS)
