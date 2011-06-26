#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libpcm
PCMSLIB := $(CODECDIR)/libpcm.a
PCMSLIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libpcm/SOURCES)
PCMSLIB_OBJ := $(call c2obj, $(PCMSLIB_SRC))
OTHER_SRC += $(PCMSLIB_SRC)

$(PCMSLIB): $(PCMSLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
