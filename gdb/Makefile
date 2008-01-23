#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

ifeq ($(MODELNAME),ifp7xx)

INCLUDES= -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I. -I$(OBJDIR)	\
 -I$(BUILDDIR)

DEPFILE = $(OBJDIR)/dep-stub
LDS := linker.cfg

SRC := $(shell cat SOURCES | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P -include "config.h" - )
DIRS = .

ifdef APPEXTRA
   DIRS += $(subst :, ,$(APPEXTRA))
   INCLUDES += $(patsubst %,-I%,$(subst :, ,$(APPEXTRA)))
endif

ifndef VERSION
VERSION=$(shell date +%y%m%d-%H%M)
endif

CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(DEFINES)			\
 -DAPPSVERSION=\"$(VERSION)\" $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE}

OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
SOURCES = $(SRC)
LINKFILE = $(OBJDIR)/linkage.lds

LIBROCKBOX = $(BUILDDIR)/librockbox.a

all: $(BUILDDIR)/$(BINARY) $(FLASHFILE)

dep: $(DEPFILE)

$(LINKFILE): $(LDS)
	$(call PRINTS,Build $(@F))cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(OBJDIR)/stub.elf : $(OBJS) $(LINKFILE) $(DEPFILE) $(LIBROCKBOX)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJS) -L$(BUILDDIR) -L$(BUILDDIR)/firmware -lrockbox -lgcc -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/stub.map

$(OBJDIR)/stub.bin : $(OBJDIR)/stub.elf
	$(call PRINTS,OBJCOPY $(@F))$(OC) -O binary $< $@

$(BUILDDIR)/$(BINARY) : $(OBJDIR)/stub.bin
	$(call PRINTS,Build stub file)$(MKFIRMWARE) $< $@

include $(TOOLSDIR)/make.inc

clean:
	$(call PRINTS,cleaning stub)-rm -f $(OBJS) $(BUILDDIR)/$(BINARY) \
	$(OBJDIR)/stub.bin $(OBJDIR)/stub.elf $(OBJDIR)/*.map \
	$(LINKFILE) $(DEPFILE)

-include $(DEPFILE)

else # not ifp7xx

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

start.o: start.s
sh-stub.o: sh-stub.c
setjmp.o: setjmp.S

endif
