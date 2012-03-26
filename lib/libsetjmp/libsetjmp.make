#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

SETJMPLIB_DIR = $(ROOTDIR)/lib/libsetjmp
SETJMPLIB_SRC = $(call preprocess, $(SETJMPLIB_DIR)/SOURCES)
SETJMPLIB_OBJ := $(call c2obj, $(SETJMPLIB_SRC))

SETJMPLIB = $(BUILDDIR)/lib/libsetjmp.a

INCLUDES += -I$(SETJMPLIB_DIR)
OTHER_SRC += $(SETJMPLIB_SRC)
EXTRA_LIBS += $(SETJMPLIB)

$(SETJMPLIB): $(SETJMPLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
