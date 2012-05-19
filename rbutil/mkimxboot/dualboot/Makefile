CC=gcc
CROSS_PREFIX=arm-elf-eabi
# Edit the following variables (plus copy/paste another set of rules) when
# adding a new target.  mkimxboot.c also needs to be edited to refer to these
# new images.

BOOTOBJS = dualboot_fuzeplus.o dualboot_zenxfi2.o dualboot_zenxfi3.o
BOOTBINS = dualboot_fuzeplus.arm-bin dualboot_zenxfi2.arm-bin dualboot_zenxfi3.arm-bin

all: ../dualboot.h ../dualboot.c

# Dualboot bootloaders

dualboot_fuzeplus.o: dualboot.S
	$(CROSS_PREFIX)-$(CC) -mcpu=arm926ej-s -DSANSA_FUZEPLUS -c -o dualboot_fuzeplus.o dualboot.S

dualboot_zenxfi2.o: dualboot.S
	$(CROSS_PREFIX)-$(CC) -mcpu=arm926ej-s -DCREATIVE_ZENXFI2 -c -o dualboot_zenxfi2.o dualboot.S

dualboot_zenxfi3.o: dualboot.S
	$(CROSS_PREFIX)-$(CC) -mcpu=arm926ej-s -DCREATIVE_ZENXFI3 -c -o dualboot_zenxfi3.o dualboot.S

# Rules for the ARM code embedded in mkamsboot - assemble, link, then extract
# the binary code and finally convert to .h for building in mkamsboot

%.arm-elf: %.o
	$(CROSS_PREFIX)-ld -Tdualboot.lds -o $@ $<

%.arm-bin: %.arm-elf
	$(CROSS_PREFIX)-objcopy -O binary $< $@

../dualboot.c ../dualboot.h: $(BOOTBINS) bin2c
	./bin2c ../dualboot $(BOOTBINS)

bin2c: bin2c.c
	$(CC) -o bin2c bin2c.c

clean:
	rm -f *~ bin2c $(BOOTBINS) $(BOOTOBJS)
