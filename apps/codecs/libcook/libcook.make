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
COOKLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libcook/SOURCES)
COOKLIB_OBJ := $(call c2obj, $(COOKLIB_SRC))
OTHER_SRC += $(COOKLIB_SRC)

$(COOKLIB): $(COOKLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

COOKFLAGS = -I$(APPSDIR)/codecs/libcook $(filter-out -O%,$(CODECFLAGS))

ifeq ($(CPU),coldfire)
	COOKFLAGS += -O2
else
	COOKFLAGS += -O1
endif

$(CODECDIR)/libcook/%.o: $(ROOTDIR)/apps/codecs/libcook/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(COOKFLAGS) -c $< -o $@

