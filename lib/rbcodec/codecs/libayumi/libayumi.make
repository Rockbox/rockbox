#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libayumi
AYUMILIB := $(CODECDIR)/libayumi.a
AYUMILIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libayumi/SOURCES)
AYUMILIB_OBJ := $(call c2obj, $(AYUMILIB_SRC))
OTHER_SRC += $(AYUMILIB_SRC)

$(AYUMILIB): $(AYUMILIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
