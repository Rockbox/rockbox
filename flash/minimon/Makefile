#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

CC    = sh-elf-gcc 
LD    = sh-elf-ld
AR    = sh-elf-ar
AS    = sh-elf-as
OC    = sh-elf-objcopy

FIRMWARE := ../../firmware
TOOLSDIR=../../tools

TARGET = minimon
LDS := $(TARGET).lds

INCLUDES= -I$(FIRMWARE)/export -I. -I$(OBJDIR)
OBJDIR := .

CFLAGS = -fpic -O -W -Wall -m1 -nostdlib -ffreestanding -Wstrict-prototypes -fomit-frame-pointer -fschedule-insns $(INCLUDES) $(DEFINES)
AFLAGS += -small -relax


ifdef DEBUG
    DEFINES := -DDEBUG
    CFLAGS += -g
endif

SRC := $(wildcard *.c)

OBJS := $(SRC:%.c=$(OBJDIR)/%.o)

LINKFILE = $(OBJDIR)/$(TARGET).lds


$(OBJDIR)/$(TARGET).bin : $(OBJDIR)/$(TARGET).elf
	$(OC) -O binary $(OBJDIR)/$(TARGET).elf $(OBJDIR)/$(TARGET).bin
	$(TOOLSDIR)/sh2d $(OBJDIR)/$(TARGET).bin -o 0x0ffff000 > $(OBJDIR)/$(TARGET).asm

$(OBJDIR)/$(TARGET).elf : $(OBJS) 
	$(CC) -Os -nostdlib -o $(OBJDIR)/$(TARGET).elf -L$(OBJDIR) -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/$(TARGET).map


clean:
	-rm -f $(OBJS) $(OBJDIR)/$(TARGET).asm \
	$(OBJDIR)/$(TARGET).bin \
	$(OBJDIR)/$(TARGET).elf \
	$(OBJDIR)/$(TARGET).map
