#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
ifndef V
SILENT = @
endif

CC = gcc
LD = ld
OC = objcopy
CROSS ?= arm-elf-eabi-

ROOTDIR  = ../../..
FIRMDIR  = $(ROOTDIR)/firmware
FWARM    = $(FIRMDIR)/target/arm
FW8702   = $(FWARM)/s5l8702
BUILDDIR = build/
LINKFILE = dualboot.lds

# Edit the following variables when adding a new target.
# mks5lboot.c also needs to be edited to refer to these
# To add a new target x you need to:
# 1) add x to the list in TARGETS
# 2) create a variable named OPT_x of the form:
#    OPT_x=target specific defines
TARGETS = ipod6g
OPT_ipod6g = -DIPOD_6G -DMEMORYSIZE=64

LOADERS = install uninstall
OPT_install =
OPT_uninstall = -DDUALBOOT_UNINSTALL

# target/loader specific options
$(foreach l, $(LOADERS),$(foreach t, $(TARGETS),\
	$(eval OPT_$(l)_$(t) = $(OPT_$(l)) $(OPT_$(t)))))

DEFINES = -DBOOTLOADER

SOURCES  = init.S dualboot.c
SOURCES += $(ROOTDIR)/lib/arm_support/support-arm.S
SOURCES += $(wildcard $(FIRMDIR)/asm/mem*.c $(FIRMDIR)/libc/mem*.c)
SOURCES += $(addprefix $(FWARM)/, mmu-arm.S)
SOURCES += $(addprefix $(FW8702)/, clocking-s5l8702.c spi-s5l8702.c nor-s5l8702.c crypto-s5l8702.c)
# target/loader specific sources
SRCTARGET = piezo-.c
$(foreach l, $(LOADERS), $(foreach t, $(TARGETS),\
	$(eval SRC_$(l)_$(t) = $(addprefix $(FW8702)/$(t)/, $(subst -.,-$(subst ipod,,$(t)).,$(SRCTARGET))))))

INCLUDES += -I. -I.. -I$(FIRMDIR) -I$(FWARM) -I$(FW8702)
INCLUDES += $(addprefix -I$(FIRMDIR)/, export include libc/include kernel/include)
# target/loader specific includes
$(foreach l,$(LOADERS),$(foreach t,$(TARGETS),$(eval INC_$(l)_$(t) = -I$(FW8702)/$(t))))

CFLAGS = $(INCLUDES) -mcpu=arm926ej-s -std=gnu99 -nostdlib -ffreestanding -Os -W -Wall\
		-Wundef -Wstrict-prototypes -ffunction-sections -fdata-sections -Wl,--gc-sections $(DEFINES)

# Build filenames prefix
PFX = dualboot_

BOOTBINS = $(foreach l, $(LOADERS),$(foreach t, $(TARGETS),$(PFX)$(l)_$(t).arm-bin))

OUTPUTDUALBOOT = ../dualboot.h ../dualboot.c
OUTPUTDEBUG = $(BOOTBINS:%.arm-bin=$(BUILDDIR)%.arm-elf) $(BOOTBINS:%.arm-bin=$(BUILDDIR)%.lds)


all: $(BUILDDIR) $(OUTPUTDUALBOOT)

$(BUILDDIR)$(PFX)%.lds: $(LINKFILE)
	@echo Creating $@
	$(SILENT)$(CROSS)$(CC) $(INC_$*) $(CFLAGS) $(OPT_$*) -E -x c - < $< | sed '/#/d' > $@

$(BUILDDIR)$(PFX)%.arm-elf: $(BUILDDIR)$(PFX)%.lds $(SOURCES)
	@echo CC -T $(notdir $^ $(SRC_$*))
	$(SILENT)$(CROSS)$(CC) $(INC_$*) $(CFLAGS) $(OPT_$*) -o $@ -T$^ $(SRC_$*)

$(PFX)%.arm-bin: $(BUILDDIR)$(PFX)%.arm-elf
	@echo OC $<
	$(SILENT)$(CROSS)$(OC) -O binary $< $@

bin2c: bin2c.c
	$(CC) -o $@ $<

$(OUTPUTDUALBOOT): bin2c $(BOOTBINS)
	./bin2c ../dualboot $(BOOTBINS)

$(BUILDDIR):
	mkdir -p $@

clean:
	rm -rf bin2c $(BOOTBINS) $(BUILDDIR)

.PRECIOUS: $(OUTPUTDEBUG)
