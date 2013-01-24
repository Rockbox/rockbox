#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

FICLSRCDIR := $(APPSDIR)/plugins/ficl
FICLBUILDDIR := $(BUILDDIR)/apps/plugins/ficl

ROCKS += $(FICLBUILDDIR)/ficl.rock

FICL_SRC := $(call preprocess, $(FICLSRCDIR)/SOURCES)
FICL_OBJ := $(call c2obj, $(FICL_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(FICL_SRC)

# -Wno-clobbered is to silence warning in vm.c about possibly clobbered vars by setjmp
FICLCFLAGS = $(PLUGINFLAGS) -DFICL_WANT_PLATFORM -DFICL_ANSI -Wno-clobbered -Wno-strict-prototypes -Os -fno-strict-aliasing

$(FICLBUILDDIR)/ficl.rock: $(FICL_OBJ) $(TLSFLIB)

# vm.c needs to built optimizations turned off otherwise stack underrun error pops up
$(FICLBUILDDIR)/vm.o: $(FICLSRCDIR)/vm.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(FICLCFLAGS) -O0 -c $< -o $@

# new rule needed to use extra compile flags
$(FICLBUILDDIR)/%.o: $(FICLSRCDIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(FICLCFLAGS) -c $< -o $@
