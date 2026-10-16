#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

include $(TOOLSDIR)/functions.make

DEFINES = $(EXTRA_DEFINES)
INCLUDES = -I$(BUILDDIR) $(TARGET_INC)

CFLAGS = $(INCLUDES) $(DEFINES) $(GCCOPTS)

# list suffixes to be understood by $*
.SUFFIXES: .map .elf .c .o

.PHONY: all clean build

all: $(DEPFILE) build

# todo: move these to configure script
STRIP    = psp-strip
MKSFO    = mksfoex
PACK_PBP = pack-pbp
FIXUP    = psp-fixup-imports

PSPSDK := $$(shell psp-config --pspsdk-path)

INCLUDES += -I$(ROOTDIR)/bootloader
SRC = $(ROOTDIR)/bootloader/psp.c

CONFIGFILE := $(FIRMDIR)/export/config/$(MODELNAME).h

CLEANOBJS += $(BUILDDIR)/bootloader.* $(BUILDDIR)/PARAM.SFO $(BUILDDIR)/EBOOT.PBP

EXPORT_OBJ = $(PSPSDK)/lib/prxexports.o

SFOFLAGS := -d MEMSIZE=0 $(SFOFLAGS)

BINARY = EBOOT.PBP

# EBOOT parameters
PSP_EBOOT_TITLE = rockbox
PSP_EBOOT_SFO 	= $(BUILDDIR)/PARAM.SFO
PSP_EBOOT_ICON 	= $(ROOTDIR)/packaging/psp/ICON0.png
PSP_EBOOT_ICON1 = NULL
PSP_EBOOT_PIC0 	= NULL
PSP_EBOOT_PIC1 	= $(ROOTDIR)/packaging/psp/PIC1.png
PSP_EBOOT_SND0 	= $(ROOTDIR)/packaging/psp/SND0.at3
PSP_EBOOT_PSAR 	= NULL

OBJ := $(SRC:.c=.o)
OBJ := $(OBJ:.S=.o)
OBJ += $(BMP:.bmp=.o)
OBJ := $(call full_path_subst,$(ROOTDIR)/%,$(BUILDDIR)/%,$(OBJ))

build: $(BUILDDIR)/$(BINARY)

$(DEPFILE) dep:
	$(call PRINTS,Generating dependencies)
	$(call mkdepfile,$(DEPFILE)_,$(SRC))
	$(call mkdepfile,$(DEPFILE)_,$(OTHER_SRC:%.lua=))
	@mv $(DEPFILE)_ $(DEPFILE)

-include $(DEPFILE)

clean::
	$(SILENT)echo Cleaning build directory
	$(SILENT)rm -rf $(BINARY)

.SECONDEXPANSION:

$(PSP_EBOOT_SFO):
	$(MKSFO) $(SFOFLAGS) '$(PSP_EBOOT_TITLE)' $@

$(BUILDDIR)/bootloader.elf: $$(OBJ) $(EXPORT_OBJ)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -o $@ $(OBJ) $(LDOPTS)\
		-Wl,-Map,$(BUILDDIR)/bootloader.map
	$(FIXUP) $@

$(BUILDDIR)/$(BINARY): $(BUILDDIR)/bootloader.elf $(PSP_EBOOT_SFO)
	$(STRIP) $(BUILDDIR)/bootloader.elf -o $(BUILDDIR)/bootloader_strip.elf
	$(PACK_PBP) $(BUILDDIR)/$(BINARY) $(PSP_EBOOT_SFO) $(PSP_EBOOT_ICON)  \
		$(PSP_EBOOT_ICON1) $(PSP_EBOOT_PIC0) $(PSP_EBOOT_PIC1)  \
		$(PSP_EBOOT_SND0) $(BUILDDIR)/bootloader_strip.elf $(PSP_EBOOT_PSAR)
	-rm -f $(BUILDDIR)/bootloader_strip.elf

### general compile rules:

# when source and object are in different locations (normal):
$(BUILDDIR)/%.o: $(ROOTDIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(ROOTDIR)/%.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(CFLAGS) -c $< -o $@

# generated definitions for use in .S files
$(BUILDDIR)/%_asmdefs.h: $(ROOTDIR)/%_asmdefs.c
	$(call PRINTS,ASMDEFS $(@F))
	$(SILENT)mkdir -p $(dir $@)
	$(call asmdefs2file,$<,$@)

# when source and object are both in BUILDDIR (generated code):
%.o: %.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(CFLAGS) -c $< -o $@
