#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libmusepack
MUSEPACKLIB := $(CODECDIR)/libmusepack.a
MUSEPACKLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libmusepack/SOURCES)
MUSEPACKLIB_OBJ := $(call c2obj, $(MUSEPACKLIB_SRC))
OTHER_SRC += $(MUSEPACKLIB_SRC)

$(MUSEPACKLIB): $(MUSEPACKLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rs $@ $^ >/dev/null

# libmusepack is faster on ARM-targets with -O1 than -O2
MUSEPACKFLAGS = $(filter-out -O%,$(CODECFLAGS)) -I$(APPSDIR)/codecs/libmusepack
ifeq ($(CPU),arm)
   MUSEPACKFLAGS += -O1
else
   MUSEPACKFLAGS += -O2
endif

$(CODECDIR)/libmusepack/%.o: $(ROOTDIR)/apps/codecs/libmusepack/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(MUSEPACKFLAGS) -c $< -o $@
