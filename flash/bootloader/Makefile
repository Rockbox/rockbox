#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

CC = sh-elf-gcc 
LD = sh-elf-ld
AR = sh-elf-ar
AS = sh-elf-as
OC = sh-elf-objcopy

FIRMWARE := ../../firmware
TOOLSDIR=../../tools

TARGET = bootloader
LDS := $(TARGET).lds


ifndef PLATFORM 
not_configured:
	@echo "No platform given."
	@echo "Use make PLATFORM=PLAYER|RECORDER|FM|ONDIO {NO_ROM=1}"
##else
##configured:
##	@echo "Building bootloader for platform "$(PLATFORM)
endif


INCLUDES= -I$(FIRMWARE)/export -I. -I$(OBJDIR)
DEFINES= -DPLATFORM_$(PLATFORM)

OBJDIR := .

CFLAGS = -O -W -Wall -m1 -nostdlib -ffreestanding -Wstrict-prototypes -fomit-frame-pointer -fschedule-insns $(INCLUDES) $(DEFINES)
AFLAGS += -small -relax


ifdef DEBUG
	DEFINES := -DDEBUG
	CFLAGS += -g
endif

SRC := $(wildcard *.c)

OBJS := $(SRC:%.c=$(OBJDIR)/%.o)

ifdef NO_ROM
LINKFILE = $(OBJDIR)/no_rom.lds
ORIGIN = 0
DEFINES += -DNO_ROM
else
LINKFILE = $(OBJDIR)/$(TARGET).lds
ORIGIN = FFFF500
endif

$(OBJDIR)/$(TARGET).bin : $(OBJDIR)/$(TARGET).elf
	$(OC) -O binary $(OBJDIR)/$(TARGET).elf $(OBJDIR)/$(TARGET).bin
	$(TOOLSDIR)/sh2d $(OBJDIR)/$(TARGET).bin -o $(ORIGIN) > $(OBJDIR)/$(TARGET).asm
ifndef NO_ROM
	$(TOOLSDIR)/scramble $(OBJDIR)/$(TARGET).bin $(OBJDIR)/$(TARGET).ajz
endif

$(OBJDIR)/$(TARGET).elf : $(OBJS) 
	$(CC) -Os -nostdlib -o $(OBJDIR)/$(TARGET).elf -L$(OBJDIR) -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/$(TARGET).map


clean:
	-rm -f \
	$(OBJS) \
	$(OBJDIR)/$(TARGET).asm \
	$(OBJDIR)/$(TARGET).bin \
	$(OBJDIR)/$(TARGET).ajz \
	$(OBJDIR)/$(TARGET).elf \
	$(OBJDIR)/$(TARGET).map
