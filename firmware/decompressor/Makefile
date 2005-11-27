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

TOOLSDIR=../../tools
OBJDIR := .
# FIXME: get proper value from build system
MEMORYSIZE = 2


LDS := link.lds
LINKFILE = $(OBJDIR)/linkage.lds
OBJS := $(OBJDIR)/decompressor.o $(OBJDIR)/rockboxucl.o

CFLAGS = -O -W -Wall -m1 -nostdlib -ffreestanding -Wstrict-prototypes -fomit-frame-pointer -fschedule-insns

$(OBJDIR)/compressed.bin : $(OBJDIR)/compressed.elf
	@echo "OBJCOPY "`basename $@`
	@$(OC) -O binary $< $@

$(OBJDIR)/compressed.elf : $(OBJS) $(LINKFILE)
	@echo "LD rockbox.elf"
	@$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJS) -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/compressed.map

$(LINKFILE): $(LDS)
	@echo "Build LDS file"
	@cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(OBJDIR)/rockboxucl.o: $(OBJDIR)/rockbox.ucl
	@echo "OBJCOPY rockbox.ucl"
	@$(OC) -I binary -O elf32-sh -B sh --rename-section .data=.image,alloc,load,data,contents $< $@
