#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libasf
ASFLIB := $(CODECDIR)/libasf.a
ASFLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libasf/SOURCES)
ASFLIB_OBJ := $(call c2obj, $(ASFLIB_SRC))
OTHER_SRC += $(ASFLIB_SRC)

$(ASFLIB): $(ASFLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

ASFFLAGS = $(filter-out -O%,$(CODECFLAGS))
ASFFLAGS += -O3

$(CODECDIR)/libasf/%.o: $(ROOTDIR)/apps/codecs/libasf/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(ASFFLAGS) -c $< -o $@
