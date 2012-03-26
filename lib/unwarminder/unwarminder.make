#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

UNWARMLIB_DIR = $(ROOTDIR)/lib/unwarminder
UNWARMLIB_SRC = $(call preprocess, $(UNWARMLIB_DIR)/SOURCES)
UNWARMLIB_OBJ := $(call c2obj, $(UNWARMLIB_SRC))

OTHER_SRC += $(UNWARMLIB_SRC)

UNWARMLIB = $(BUILDDIR)/lib/libunwarminder.a
CORE_LIBS += $(UNWARMLIB)

INCLUDES += -I$(UNWARMLIB_DIR)

$(UNWARMLIB): $(UNWARMLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
