#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

MADFLAGS = $(CODECFLAGS) -I$(RBCODECLIB_DIR)/codecs/libmad
MADFLAGS += -UDEBUG -DNDEBUG -DHAVE_LIMITS_H -DHAVE_ASSERT_H

# libmad
MADLIB := $(CODECDIR)/libmad.a
MADLIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libmad/SOURCES)
MADLIB_OBJ := $(call c2obj, $(MADLIB_SRC))
OTHER_SRC += $(MADLIB_SRC)

$(MADLIB): $(MADLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

# pattern rules

$(CODECDIR)/libmad/%.o: $(RBCODECLIB_DIR)/codecs/libmad/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<)) \
		$(CC) $(MADFLAGS) -c $< -o $@

$(CODECDIR)/libmad/%.o: $(RBCODECLIB_DIR)/codecs/libmad/%.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<)) \
		$(CC) $(MADFLAGS) -c $< -o $@
