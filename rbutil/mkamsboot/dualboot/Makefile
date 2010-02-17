CC=gcc

# Edit the following variables (plus copy/paste another set of rules) when
# adding a new target.  mkamsboot.c also needs to be edited to refer to these
# new images.

BOOTOBJS = nrv2e_d8.o dualboot_clip.o dualboot_e200v2.o dualboot_c200v2.o dualboot_m200v4.o dualboot_fuze.o dualboot_clipv2.o dualboot_clipplus.o
BOOTBINS = nrv2e_d8.arm-bin dualboot_clip.arm-bin dualboot_e200v2.arm-bin dualboot_c200v2.arm-bin dualboot_m200v4.arm-bin dualboot_fuze.arm-bin dualboot_clipv2.arm-bin dualboot_clipplus.arm-bin

all: ../dualboot.h ../dualboot.c

# Dualboot bootloaders

dualboot_clip.o: dualboot.S
	arm-elf-gcc -DSANSA_CLIP -c -o dualboot_clip.o dualboot.S

dualboot_fuze.o: dualboot.S
	arm-elf-gcc -DSANSA_FUZE -c -o dualboot_fuze.o dualboot.S

dualboot_e200v2.o: dualboot.S
	arm-elf-gcc -DSANSA_E200V2 -c -o dualboot_e200v2.o dualboot.S

dualboot_m200v4.o: dualboot.S
	arm-elf-gcc -DSANSA_M200V4 -c -o dualboot_m200v4.o dualboot.S

dualboot_c200v2.o: dualboot.S
	arm-elf-gcc -DSANSA_C200V2 -c -o dualboot_c200v2.o dualboot.S

dualboot_clipv2.o: dualboot.S
	arm-elf-gcc -DSANSA_CLIPV2 -c -o dualboot_clipv2.o dualboot.S

dualboot_clipplus.o: dualboot.S
	arm-elf-gcc -DSANSA_CLIPPLUS -c -o dualboot_clipplus.o dualboot.S

# Rules for the ucl unpack function
nrv2e_d8.o: nrv2e_d8.S
	arm-elf-gcc -DPURE_THUMB -c -o nrv2e_d8.o nrv2e_d8.S

# Rules for the ARM code embedded in mkamsboot - assemble, link, then extract
# the binary code and finally convert to .h for building in mkamsboot

%.arm-elf: %.o
	arm-elf-ld -e 0 -Ttext=0 -o $@ $<

%.arm-bin: %.arm-elf
	arm-elf-objcopy -O binary $< $@

../dualboot.c ../dualboot.h: $(BOOTBINS) bin2c
	./bin2c ../dualboot $(BOOTBINS)

bin2c: bin2c.c
	$(CC) -o bin2c bin2c.c

clean:
	rm -f *~ bin2c $(BOOTBINS) $(BOOTOBJS)
