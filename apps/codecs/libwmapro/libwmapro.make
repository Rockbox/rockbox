#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libwmapro
WMAPROLIB := $(CODECDIR)/libwmapro.a
WMAPROLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libwmapro/SOURCES)
WMAPROLIB_OBJ := $(call c2obj, $(WMAPROLIB_SRC))
OTHER_SRC += $(WMAPROLIB_SRC)

$(WMAPROLIB): $(WMAPROLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

WMAPROFLAGS = -I$(APPSDIR)/codecs/libwmapro $(filter-out -O%,$(CODECFLAGS))

ifeq ($(CPU),coldfire)
	WMAPROFLAGS += -O3
else
	WMAPROFLAGS += -O1
endif

$(CODECDIR)/libwmapro/%.o: $(ROOTDIR)/apps/codecs/libwmapro/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(WMAPROFLAGS) -c $< -o $@
