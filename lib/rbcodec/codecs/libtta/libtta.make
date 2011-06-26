#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libtta
TTALIB := $(CODECDIR)/libtta.a
TTALIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libtta/SOURCES)
TTALIB_OBJ := $(call c2obj, $(TTALIB_SRC))
OTHER_SRC += $(TTALIB_SRC)

$(TTALIB): $(TTALIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
