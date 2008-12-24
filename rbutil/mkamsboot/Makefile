CC=gcc

# We use the UCL code available in the Rockbox tools/ directory
CFLAGS=-I../../tools/ucl/include
LIBUCL=../../tools/ucl/src/libucl.a

# Edit the following variables (plus copy/paste another set of rules) when
# adding a new target.  mkamsboot.c also needs to be edited to refer to these
# new images.

BOOTIMAGES = dualboot_clip.o dualboot_e200v2.o dualboot_c200v2.o dualboot_m200v4.o dualboot_fuze.o
BOOTHEADERS = dualboot_clip.h dualboot_e200v2.h dualboot_c200v2.h dualboot_m200v4.h dualboot_fuze.h

CLIPFILES = dualboot_clip.arm-o dualboot_clip.o dualboot_clip.c dualboot_clip.h

E200V2FILES = dualboot_e200v2.arm-o dualboot_e200v2.o dualboot_e200v2.c \
              dualboot_e200v2.h

M200V4FILES = dualboot_m200v4.arm-o dualboot_m200v4.o dualboot_m200v4.arm-bin \
              dualboot_m200v4.c dualboot_m200v4.h

C200V2FILES = dualboot_c200v2.arm-o dualboot_c200v2.o dualboot_c200v2.c \
              dualboot_c200v2.h

FUZEFILES = dualboot_fuze.arm-o dualboot_fuze.o dualboot_fuze.c dualboot_fuze.h

all: mkamsboot

# Dualboot bootloaders

dualboot_clip.arm-o: dualboot.S
	arm-elf-gcc -DSANSA_CLIP -c -o dualboot_clip.arm-o dualboot.S

dualboot_fuze.arm-o: dualboot.S
	arm-elf-gcc -DSANSA_FUZE -c -o dualboot_fuze.arm-o dualboot.S

dualboot_e200v2.arm-o: dualboot.S
	arm-elf-gcc -DSANSA_E200V2 -c -o dualboot_e200v2.arm-o dualboot.S

dualboot_m200v4.arm-o: dualboot.S
	arm-elf-gcc -DSANSA_M200V4 -c -o dualboot_m200v4.arm-o dualboot.S

dualboot_c200v2.arm-o: dualboot.S
	arm-elf-gcc -DSANSA_C200V2 -c -o dualboot_c200v2.arm-o dualboot.S

# Rules for the ucl unpack function
nrv2e_d8.arm-o: nrv2e_d8.S
	arm-elf-gcc -DPURE_THUMB -c -o nrv2e_d8.arm-o nrv2e_d8.S



$(LIBUCL):
	make -C ../../tools/ucl/src libucl.a

md5.o: md5.c md5.h
	$(CC) $(CFLAGS) -c -o md5.o -W -Wall md5.c

mkamsboot.o: mkamsboot.c $(BOOTHEADERS) nrv2e_d8.h md5.h
	$(CC) $(CFLAGS) -c -o mkamsboot.o -W -Wall mkamsboot.c

mkamsboot: mkamsboot.o $(BOOTIMAGES) nrv2e_d8.o md5.o $(LIBUCL)
	$(CC) -o mkamsboot mkamsboot.o $(BOOTIMAGES) nrv2e_d8.o md5.o $(LIBUCL)

# Rules for the ARM code embedded in mkamsboot - assemble, link, then extract
# the binary code and finally convert to .c/.h for linking with mkamsboot

%.arm-elf: %.arm-o
	arm-elf-ld -e 0 -Ttext=0 -o $@ $<

%.arm-bin: %.arm-elf
	arm-elf-objcopy -O binary $< $@

%.c %.h: %.arm-bin bin2c
	./bin2c $< $*

# Generic host rule.
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Cancel the implicit .S -> .o rule
%.o: %.S

bin2c: bin2c.c
	$(CC) -o bin2c bin2c.c

clean:
	rm -f mkamsboot mkamsboot.o nrv2e_d8.arm-o nrv2e_d8.arm-elf \
	nrv2e_d8.arm-bin *~ bin2c nrv2e_d8.c nrv2e_d8.h nrv2e_d8.o md5.o \
	$(BOOTIMAGES) $(CLIPFILES) $(E200V2FILES) $(M200V4FILES) $(FUZEFILES) \
	$(C200V2FILES)
