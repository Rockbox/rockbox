CC=gcc
LD=ld
OC=objcopy
CROSS_PREFIX=arm-elf-eabi-
REGS_PATH=../../../firmware/target/arm/imx233/regs
CFLAGS=-mcpu=arm926ej-s -std=gnu99 -I. -I$(REGS_PATH) -nostdlib -ffreestanding -fomit-frame-pointer -O
LDFLAGS=
# Edit the following variables when adding a new target.
# mkimxboot.c also needs to be edited to refer to these
# To add a new target x you need to:
# 1) add x to the list in TARGETS
# 2) create a variable named OPT_x of the form:
#    OPT_x=target specific defines
TARGETS=zenmozaic zenxfi zen zenv
OPT_zenmozaic=-DCREATIVE_ZENMOZAIC -DIMX233_SUBTARGET=3700
OPT_zenxfi=-DCREATIVE_ZENXFI -DIMX233_SUBTARGET=3700
OPT_zen=-DCREATIVE_ZEN -DIMX233_SUBTARGET=3700
OPT_zenv=-DCREATIVE_ZENV -DIMX233_SUBTARGET=3600

BOOTLDS=$(patsubst %, dualboot_%.lds, $(TARGETS))
BOOTOBJS=$(patsubst %, dualboot_%.o, $(TARGETS))
BOOTBINS=$(patsubst %, dualboot_%.arm-bin, $(TARGETS))
BOOTELFS=$(patsubst %, dualboot_%.arm-elf, $(TARGETS))

all: ../dualboot.h ../dualboot.c $(BOOTELFS)

# Dualboot bootloaders

dualboot_%.o: dualboot.c
	$(CROSS_PREFIX)$(CC) $(CFLAGS) $(OPT_$(@:dualboot_%.o=%)) -c -o $@ $^

dualboot_%.lds: dualboot.lds
	$(CROSS_PREFIX)$(CC) $(CFLAGS) $(OPT_$(@:dualboot_%.lds=%)) -E -x c - < $< | sed '/#/d' > $@

dualboot_%.arm-elf: dualboot_%.o dualboot_%.lds
	$(CROSS_PREFIX)$(LD) $(LDFLAGS) -T$(@:dualboot_%.arm-elf=dualboot_%.lds) -o $@ $<

# Rules for the ARM code embedded in mkamsboot - assemble, link, then extract
# the binary code and finally convert to .h for building in mkamsboot

%.arm-bin: %.arm-elf
	$(CROSS_PREFIX)$(OC) -O binary $< $@

../dualboot.c ../dualboot.h: $(BOOTBINS) bin2c
	./bin2c ../dualboot $(BOOTBINS)

bin2c: bin2c.c
	$(CC) -o bin2c bin2c.c

clean:
	rm -f *~ bin2c $(BOOTBINS) $(BOOTOBJS) $(BOOTELFS) $(BOOTLDS)
