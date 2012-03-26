#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

ARMSUPPORTLIB_DIR := $(ROOTDIR)/lib/arm_support
ARMSUPPORTLIB_SRC := $(ARMSUPPORTLIB_DIR)/support-arm.S
ARMSUPPORTLIB_OBJ := $(call c2obj, $(ARMSUPPORTLIB_SRC))
ARMSUPPORTLIB := $(BUILDDIR)/lib/libarm_support.a

OTHER_SRC += $(ARMSUPPORTLIB_SRC)
# both core and plugins link this
CORE_LIBS += $(ARMSUPPORTLIB)
PLUGIN_LIBS += $(ARMSUPPORTLIB)

$(ARMSUPPORTLIB): $(ARMSUPPORTLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
