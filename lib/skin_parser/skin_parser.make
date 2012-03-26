#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
SKINPARSLIB_DIR = $(ROOTDIR)/lib/skin_parser
SKINPARSLIB_SRC = $(call preprocess, $(SKINPARSLIB_DIR)/SOURCES)
SKINPARSLIB_OBJ := $(call c2obj, $(SKINPARSLIB_SRC))

SKINPARSLIB = $(BUILDDIR)/lib/libskin_parser.a

INCLUDES += -I$(SKINPARSLIB_DIR)
OTHER_SRC += $(SKINPARSLIB_SRC)
CORE_LIBS += $(SKINPARSLIB)

$(SKINPARSLIB): $(SKINPARSLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
