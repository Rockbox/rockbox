#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

ifdef RECORDER
EXTRA = -DRECORDER
EXT = ajz
else
EXT = mod
endif

TARGET = stub
OBJS = start.o sh-stub.o setjmp.o
LIBS = -lgcc

.s.o:
	sh-elf-as -o $@ $<

.c.o:
	sh-elf-gcc -O $(EXTRA) -I../firmware/export -I../firmware/include -m1 -Wall -Wstrict-prototypes -c -o $@ $<

.S.o:
	sh-elf-gcc -O -I../firmware/export -I../firmware/include -m1 -Wall -Wstrict-prototypes -c -o $@ $<

$(TARGET).$(EXT): $(TARGET).elf
	sh-elf-objcopy -O binary $(TARGET).elf $(TARGET).out
	../tools/scramble $(TARGET).out $(TARGET).$(EXT)
	../tools/sh2d $(TARGET).out -o 09000000 > $(TARGET).asm

$(TARGET).elf: $(OBJS)
	sh-elf-gcc -nostartfiles $(OBJS) -nostdlib -Wl,-Map,$(TARGET).map -o $(TARGET).elf -Tlinker.cfg

clean:
	rm $(OBJS) $(TARGET).map $(TARGET).elf $(TARGET).out $(TARGET).mod $(TARGET).ajz

start.o: start.S
sh-stub.o: sh-stub.c
setjmp.o: setjmp.S
