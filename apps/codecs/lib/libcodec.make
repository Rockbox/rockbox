#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

CODECLIB := $(CODECDIR)/libcodec.a
CODECLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/lib/SOURCES)
CODECLIB_OBJ := $(call c2obj, $(CODECLIB_SRC))
OTHER_SRC += $(CODECLIB_SRC)

$(CODECLIB): $(CODECLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rs $@ $^ >/dev/null 2>&1

CODECLIBFLAGS = $(CODECFLAGS) -ffunction-sections

$(CODECDIR)/lib/%.o: $(ROOTDIR)/apps/codecs/lib/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) \
		-I$(dir $<) $(CODECLIBFLAGS) -c $< -o $@
