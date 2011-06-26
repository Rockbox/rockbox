#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libalac
ALACLIB := $(CODECDIR)/libalac.a
ALACLIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libalac/SOURCES)
ALACLIB_OBJ := $(call c2obj, $(ALACLIB_SRC))
OTHER_SRC += $(ALACLIB_SRC)

$(ALACLIB): $(ALACLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
