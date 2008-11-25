#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libdemac
DEMACLIB := $(CODECDIR)/libdemac.a
DEMACLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/demac/libdemac/SOURCES)
DEMACLIB_OBJ := $(call c2obj, $(DEMACLIB_SRC))
OTHER_SRC += $(DEMACLIB_SRC)

$(DEMACLIB): $(DEMACLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rs $@ $^ >/dev/null 2>&1

DEMACFLAGS = $(filter-out -O%,$(CODECFLAGS))
DEMACFLAGS += -O3

$(CODECDIR)/demac/%.o: $(ROOTDIR)/apps/codecs/demac/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(DEMACFLAGS) -c $< -o $@
