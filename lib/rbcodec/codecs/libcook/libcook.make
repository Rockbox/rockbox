#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id:$
#

# libcook
COOKLIB := $(CODECDIR)/libcook.a
COOKLIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libcook/SOURCES)
COOKLIB_OBJ := $(call c2obj, $(COOKLIB_SRC))
OTHER_SRC += $(COOKLIB_SRC)

$(COOKLIB): $(COOKLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
