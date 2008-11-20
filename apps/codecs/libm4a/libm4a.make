#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

# libm4a
M4ALIB := $(CODECDIR)/libm4a.a
M4ALIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libm4a/SOURCES)
M4ALIB_OBJ := $(call c2obj, $(M4ALIB_SRC))
OTHER_SRC += $(M4ALIB_SRC)

$(M4ALIB): $(M4ALIB_OBJ)
	$(call PRINTS,AR $(@F))$(AR) rs $@ $^ >/dev/null 2>&1

M4AFLAGS = $(filter-out -O%,$(CODECFLAGS))
M4AFLAGS += -O3

$(CODECDIR)/libm4a/%.o: $(ROOTDIR)/apps/codecs/libm4a/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(M4AFLAGS) -c $< -o $@
