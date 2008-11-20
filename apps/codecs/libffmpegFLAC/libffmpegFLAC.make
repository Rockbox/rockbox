#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

# libffmpegFLAC
FFMPEGFLACLIB := $(CODECDIR)/libffmpegFLAC.a
FFMPEGFLACLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libffmpegFLAC/SOURCES)
FFMPEGFLACLIB_OBJ := $(call c2obj, $(FFMPEGFLACLIB_SRC))
OTHER_SRC += $(FFMPEGFLACLIB_SRC)

$(FFMPEGFLACLIB): $(FFMPEGFLACLIB_OBJ)
	$(call PRINTS,AR $(@F))$(AR) rs $@ $^ >/dev/null 2>&1
