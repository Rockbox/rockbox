CC=gcc
LD=ld
OC=objcopy
PREFIX?=arm-elf-eabi-
IMX233_PATH=../../../firmware/target/arm/imx233
CFLAGS=-mcpu=arm926ej-s -std=gnu99 -I. -I$(IMX233_PATH) -nostdlib -ffreestanding -fomit-frame-pointer -O
# Edit the following variables when adding a new target.
# mkimxboot.c also needs to be edited to refer to these
# To add a new target x you need to:
# 1) add x to the list in TARGETS
# 2) create a variable named OPT_x of the form:
#    OPT_x=target specific defines
TARGETS=fuzeplus zenxfi2 zenxfi3 nwze370 nwze360 zenxfistyle
OPT_fuzeplus=-DSANSA_FUZEPLUS -DIMX233_SUBTARGET=3780
OPT_zenxfi2=-DCREATIVE_ZENXFI2 -DIMX233_SUBTARGET=3780
OPT_zenxfi3=-DCREATIVE_ZENXFI3 -DIMX233_SUBTARGET=3780
OPT_nwze370=-DSONY_NWZE370 -DIMX233_SUBTARGET=3780
OPT_nwze360=-DSONY_NWZE360 -DIMX233_SUBTARGET=3780
OPT_zenxfistyle=-DCREATIVE_ZENXFISTYLE -DIMX233_SUBTARGET=3780

BOOTOBJS=$(patsubst %, dualboot_%.o, $(TARGETS))
BOOTBINS=$(patsubst %, dualboot_%.arm-bin, $(TARGETS))
BOOTELFS=$(patsubst %, dualboot_%.arm-elf, $(TARGETS))

all: ../dualboot.h ../dualboot.c $(BOOTELFS)

# Dualboot bootloaders

dualboot_%.o: dualboot.c
	$(PREFIX)$(CC) $(CFLAGS) $(OPT_$(@:dualboot_%.o=%)) -c -o $@ $^

dualboot_%.arm-elf: dualboot_%.o
	$(PREFIX)$(LD) $(LDFLAGS) -Tdualboot.lds -o $@ $<

# Rules for the ARM code embedded in mkamsboot - assemble, link, then extract
# the binary code and finally convert to .h for building in mkamsboot

%.arm-bin: %.arm-elf
	$(PREFIX)$(OC) -O binary $< $@

../dualboot.c ../dualboot.h: $(BOOTBINS) bin2c
	./bin2c ../dualboot $(BOOTBINS)

bin2c: bin2c.c
	$(CC) -o bin2c bin2c.c

clean:
	rm -f *~ bin2c $(BOOTBINS) $(BOOTOBJS) $(BOOTELFS)
