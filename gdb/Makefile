#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
TARGET = stub
OBJS = start.o sh-stub.o
LIBS = -lgcc

.s.o:
	sh-elf-as -o $@ $<

.c.o:
	sh-elf-gcc -O -I../firmware/drivers -m1 -Wall -Wstrict-prototypes -c -o $@ $<

archos.mod: $(TARGET).elf
	sh-elf-objcopy -O binary $(TARGET).elf $(TARGET).out
	../tools/scramble $(TARGET).out archos.mod

$(TARGET).elf: $(OBJS)
	sh-elf-gcc -nostartfiles $(OBJS) -lgcc -Wl,-Map,$(TARGET).map -o $(TARGET).elf -Tlinker.cfg

clean:
	rm $(OBJS) $(TARGET).map $(TARGET).elf $(TARGET).out archos.mod

start.o: start.s
sh-stub.o: sh-stub.c
