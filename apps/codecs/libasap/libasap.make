#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libasap
ASAPLIB := $(CODECDIR)/libasap.a
ASAPLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libasap/SOURCES)
ASAPLIB_OBJ := $(call c2obj, $(ASAPLIB_SRC))
OTHER_SRC += $(ASAPLIB_SRC)

$(ASAPLIB): $(ASAPLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rs $@ $^ >/dev/null 2>&1

ASAPFLAGS = $(filter-out -O%,$(CODECFLAGS))
ASAPFLAGS += -O1

$(CODECDIR)/libasap/%.o: $(ROOTDIR)/apps/codecs/libasap/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(ASAPFLAGS) -c $< -o $@
