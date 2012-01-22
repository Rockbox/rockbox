#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

ARMSUPPORT_DIR = $(ROOTDIR)/lib/arm_support
ARMSUPPORT_SRC = $(ARMSUPPORT_DIR)/support-arm.S
ARMSUPPORT_OBJ := $(call c2obj, $(ARMSUPPORT_SRC))

OTHER_SRC += $(ARMSUPPORT_SRC)

LIBARMSUPPORT := $(BUILDDIR)/lib/libarm_support.a

$(LIBARMSUPPORT): $(ARMSUPPORT_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
