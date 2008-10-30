
# We use the UCL code available in the Rockbox tools/ directory

CFLAGS=-I../../tools/ucl/include
LIBUCL=../../tools/ucl/src/libucl.a

# Edit the following variables (plus copy/paste another set of rules) when
# adding a new target.  mkamsboot.c also needs to be edited to refer to these
# new images.
#
# If anyone reading this wants to improve this makefile, please do!

BOOTIMAGES = bootimg_clip.o bootimg_e200v2.o bootimg_m200v2.o
BOOTHEADERS = bootimg_clip.h bootimg_e200v2.h bootimg_m200v2.h

CLIPFILES = dualboot-clip.o dualboot-clip.elf dualboot-clip.o \
            dualboot-clip.bin bootimg_clip.c bootimg_clip.h

E200V2FILES = dualboot-e200v2.o dualboot-e200v2.elf dualboot-e200v2.o \
              dualboot-e200v2.bin bootimg_e200v2.c bootimg_e200v2.h

M200V2FILES = dualboot-m200v2.o dualboot-m200v2.elf dualboot-m200v2.o \
              dualboot-m200v2.bin bootimg_m200v2.c bootimg_m200v2.h

all: mkamsboot

$(LIBUCL):
	make -C ../../tools/ucl/src libucl.a

md5.o: md5.c md5.h
	gcc $(CFLAGS) -c -o md5.o -W -Wall md5.c

mkamsboot.o: mkamsboot.c $(BOOTHEADERS) uclimg.h md5.h
	gcc $(CFLAGS) -c -o mkamsboot.o -W -Wall mkamsboot.c

mkamsboot: mkamsboot.o $(BOOTIMAGES) uclimg.o md5.o $(LIBUCL)
	gcc -o mkamsboot mkamsboot.o $(BOOTIMAGES) uclimg.o md5.o $(LIBUCL)

# Rules for our test ARM application - assemble, link, then extract
# the binary code

# CLIP

dualboot-clip.o: dualboot.S
	arm-elf-gcc -DSANSA_CLIP -c -o dualboot-clip.o dualboot.S

dualboot-clip.elf: dualboot-clip.o
	arm-elf-ld -e 0 -Ttext=0 -o dualboot-clip.elf dualboot-clip.o

dualboot-clip.bin: dualboot-clip.elf
	arm-elf-objcopy -O binary dualboot-clip.elf dualboot-clip.bin

bootimg_clip.c bootimg_clip.h: dualboot-clip.bin bin2c
	./bin2c dualboot-clip.bin bootimg_clip

bootimg_clip.o: bootimg_clip.c
	gcc -c -o bootimg_clip.o bootimg_clip.c

# E200V2

dualboot-e200v2.o: dualboot.S
	arm-elf-gcc -DSANSA_E200V2 -c -o dualboot-e200v2.o dualboot.S

dualboot-e200v2.elf: dualboot-e200v2.o
	arm-elf-ld -e 0 -Ttext=0 -o dualboot-e200v2.elf dualboot-e200v2.o

dualboot-e200v2.bin: dualboot-e200v2.elf
	arm-elf-objcopy -O binary dualboot-e200v2.elf dualboot-e200v2.bin

bootimg_e200v2.c bootimg_e200v2.h: dualboot-e200v2.bin bin2c
	./bin2c dualboot-e200v2.bin bootimg_e200v2

bootimg_e200v2.o: bootimg_e200v2.c
	gcc -c -o bootimg_e200v2.o bootimg_e200v2.c

# M200V2    
    
dualboot-m200v2.o: dualboot.S
	arm-elf-gcc -DSANSA_M200V2 -c -o dualboot-m200v2.o dualboot.S

dualboot-m200v2.elf: dualboot-m200v2.o
	arm-elf-ld -e 0 -Ttext=0 -o dualboot-m200v2.elf dualboot-m200v2.o

dualboot-m200v2.bin: dualboot-m200v2.elf
	arm-elf-objcopy -O binary dualboot-m200v2.elf dualboot-m200v2.bin

bootimg_m200v2.c bootimg_m200v2.h: dualboot-m200v2.bin bin2c
	./bin2c dualboot-m200v2.bin bootimg_m200v2

bootimg_m200v2.o: bootimg_m200v2.c
	gcc -c -o bootimg_m200v2.o bootimg_m200v2.c
        
# Rules for the ucl unpack function
nrv2e_d8.o: nrv2e_d8.S
	arm-elf-gcc -DPURE_THUMB -c -o nrv2e_d8.o nrv2e_d8.S

# NOTE: this function has no absolute references, so the link address (-e)
# is irrelevant.  We just link at address 0, but it can run from anywhere.
nrv2e_d8.elf: nrv2e_d8.o
	arm-elf-ld -e 0 -Ttext=0 -o nrv2e_d8.elf nrv2e_d8.o

nrv2e_d8.bin: nrv2e_d8.elf
	arm-elf-objcopy -O binary nrv2e_d8.elf nrv2e_d8.bin

uclimg.c uclimg.h: nrv2e_d8.bin bin2c
	./bin2c nrv2e_d8.bin uclimg

uclimg.o: uclimg.c
	gcc -c -o uclimg.o uclimg.c

bin2c: bin2c.c
	gcc -o bin2c bin2c.c

clean:
	rm -f mkamsboot mkamsboot.o nrv2e_d8.o nrv2e_d8.elf nrv2e_d8.bin *~ \
	bin2c uclimg.c uclimg.h uclimg.o md5.o \
	$(BOOTIMAGES) $(CLIPFILES) $(E200V2FILES) $(M200V2FILES)
