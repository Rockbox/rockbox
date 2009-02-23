#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libspc
SPCLIB := $(CODECDIR)/libspc.a
SPCLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libspc/SOURCES)
SPCLIB_OBJ := $(call c2obj, $(SPCLIB_SRC))
OTHER_SRC += $(SPCLIB_SRC)

$(SPCLIB): $(SPCLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rs $@ $^ >/dev/null

SPCFLAGS = $(filter-out -O%,$(CODECFLAGS))
SPCFLAGS += -O1

$(CODECDIR)/libspc/%.o: $(ROOTDIR)/apps/codecs/libspc/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(SPCFLAGS) -c $< -o $@
