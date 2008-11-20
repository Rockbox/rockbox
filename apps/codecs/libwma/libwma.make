#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

# libwma
WMALIB := $(CODECDIR)/libwma.a
WMALIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libwma/SOURCES)
WMALIB_OBJ := $(call c2obj, $(WMALIB_SRC))
OTHER_SRC += $(WMALIB_SRC)

$(WMALIB): $(WMALIB_OBJ)
	$(call PRINTS,AR $(@F))$(AR) rs $@ $^ >/dev/null 2>&1
