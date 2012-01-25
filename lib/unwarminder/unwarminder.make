#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

UNWARM_DIR = $(ROOTDIR)/lib/unwarminder
UNWARM_SRC = $(call preprocess, $(UNWARM_DIR)/SOURCES)
UNWARM_OBJ := $(call c2obj, $(UNWARM_SRC))

OTHER_SRC += $(UNWARM_SRC)

UNWARMINDER = $(BUILDDIR)/lib/libunwarminder.a

INCLUDES += -I$(UNWARM_DIR)

$(UNWARMINDER): $(UNWARM_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
