# Change INFILE to point to your original firmware file
INFILE=$(HOME)/FW/AMS/CLIP/m300a-1.1.17A.bin

# OUTFILE is the file you copy to your device's root and rename to
# (e.g.) m300a.bin
OUTFILE=patched.bin

# The uclpack command
UCLPACK=../../tools/uclpack

all: $(OUTFILE)

mkamsboot: mkamsboot.c
	gcc -o mkamsboot -W -Wall mkamsboot.c

extract_fw: extract_fw.c
	gcc -o extract_fw -W -Wall extract_fw.c

# Rules for our test ARM application - assemble, link, then extract
# the binary code

test.o: test.S
	arm-elf-as -o test.o test.S

test.elf: test.o
	arm-elf-ld -e 0 -Ttext=0 -o test.elf test.o

test.bin: test.elf
	arm-elf-objcopy -O binary test.elf test.bin

# Rules for the ucl unpack function - this is inserted in the padding at 
# the end of the original firmware block
nrv2e_d8.o: nrv2e_d8.S
	arm-elf-gcc -DPURE_THUMB -c -o nrv2e_d8.o nrv2e_d8.S

# NOTE: this function has no absolute references, so the link address (-e)
# is irrelevant.  We just link at address 0.
nrv2e_d8.elf: nrv2e_d8.o
	arm-elf-ld -e 0 -Ttext=0 -o nrv2e_d8.elf nrv2e_d8.o

nrv2e_d8.bin: nrv2e_d8.elf
	arm-elf-objcopy -O binary nrv2e_d8.elf nrv2e_d8.bin

firmware_block.ucl: firmware_block.bin
	$(UCLPACK) --best --2e firmware_block.bin firmware_block.ucl

firmware_block.bin: $(INFILE) extract_fw
	./extract_fw $(INFILE) firmware_block.bin

$(OUTFILE): mkamsboot firmware_block.ucl test.bin nrv2e_d8.bin $(INFILE)
	./mkamsboot $(INFILE) firmware_block.ucl test.bin nrv2e_d8.bin $(OUTFILE)

clean:
	rm -fr amsinfo mkamsboot test.o test.elf test.bin extract_fw \
	nrv2e_d8.o nrv2e_d8.elf nrv2e_d8.bin firmware_block.bin \
	firmware_block.ucl $(OUTFILE) *~ 
