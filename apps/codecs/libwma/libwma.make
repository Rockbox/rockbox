#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libwma
WMALIB := $(CODECDIR)/libwma.a
WMALIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libwma/SOURCES)
WMALIB_OBJ := $(call c2obj, $(WMALIB_SRC))
OTHER_SRC += $(WMALIB_SRC)

$(WMALIB): $(WMALIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

WMAFLAGS = -I$(APPSDIR)/codecs/libwma $(filter-out -O%,$(CODECFLAGS))

ifeq ($(MEMORYSIZE),2)
    WMAFLAGS += -Os
else
    WMAFLAGS += -O2
endif

$(CODECDIR)/libwma/%.o: $(ROOTDIR)/apps/codecs/libwma/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(WMAFLAGS) -c $< -o $@

