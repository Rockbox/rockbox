#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
SKINP_DIR = $(ROOTDIR)/lib/skin_parser
SKINP_SRC = $(call preprocess, $(SKINP_DIR)/SOURCES)
SKINP_OBJ := $(call c2obj, $(SKINP_SRC))

OTHER_SRC += $(SKINP_SRC)

SKINLIB = $(BUILDDIR)/lib/libskin_parser.a

INCLUDES += -I$(SKINP_DIR)

$(SKINLIB): $(SKINP_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
