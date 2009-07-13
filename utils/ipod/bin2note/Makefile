#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

all: bin2note

bin2note: bin2note.c
	gcc -W -Wall -o bin2note bin2note.c

test.htm: bin2note test.bin
	./bin2note test.bin test.htm

test.o: test.S
	arm-elf-as -o $@ $<

test.elf: test.o
	arm-elf-ld -e 0 -Ttext=0 -o $@ $<

test.bin: test.elf
	arm-elf-objcopy -O binary $< $@

clean:
	rm -f bin2note test.o test.elf test.bin test.htm
