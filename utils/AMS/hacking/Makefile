
# Change INFILE to point to your original firmware file
INFILE=$(HOME)/FW/AMS/CLIP/m300a-1.1.17A.bin

# OUTFILE is the file you copy to your device's root and rename to
# (e.g.) m300a.bin
OUTFILE=patched.bin


all: amsinfo $(OUTFILE)

amsinfo: amsinfo.c
	gcc -o amsinfo -W -Wall amsinfo.c

mkamsboot: mkamsboot.c
	gcc -o mkamsboot -W -Wall mkamsboot.c

# Rules for our test ARM application - assemble, link, then extract
# the binary code

test.o: test.S
	arm-elf-as -o test.o test.S

test.elf: test.o
	arm-elf-ld -e 0 -o test.elf test.o

test.bin: test.elf
	arm-elf-objcopy -O binary test.elf test.bin

$(OUTFILE): mkamsboot test.bin $(INFILE)
	./mkamsboot $(INFILE) test.bin $(OUTFILE)

clean:
	rm -fr amsinfo mkamsboot test.bin test.o test.elf $(OUTFILE) *~
