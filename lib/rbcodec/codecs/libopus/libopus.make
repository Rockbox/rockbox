#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libopus
OPUSLIB := $(CODECDIR)/libopus.a
OPUSLIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libopus/SOURCES)
OPUSLIB_OBJ := $(call c2obj, $(OPUSLIB_SRC))

# codec specific compilation flags
# prepend paths to avoid name clash issues
$(OPUSLIB) : CODECFLAGS := -DHAVE_CONFIG_H \
   -I$(RBCODECLIB_DIR)/codecs/libopus \
   -I$(RBCODECLIB_DIR)/codecs/libopus/celt \
   -I$(RBCODECLIB_DIR)/codecs/libopus/silk \
   $(CODECFLAGS)

$(OPUSLIB): $(OPUSLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
