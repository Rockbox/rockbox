#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

# liba52
A52LIB := $(CODECDIR)/liba52.a
A52LIB_SRC := $(call preprocess, $(APPSDIR)/codecs/liba52/SOURCES)
A52LIB_OBJ := $(call c2obj, $(A52LIB_SRC))
OTHER_SRC += $(A52LIB_SRC)

$(A52LIB): $(A52LIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rs $@ $^ >/dev/null 2>&1
