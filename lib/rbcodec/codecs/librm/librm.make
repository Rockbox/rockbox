#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: librm.make 20151 2009-03-01 09:04:15Z amiconn $
#

# librm
RMLIB := $(CODECDIR)/librm.a
RMLIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/librm/SOURCES)
RMLIB_OBJ := $(call c2obj, $(RMLIB_SRC))
OTHER_SRC += $(RMLIB_SRC)

$(RMLIB): $(RMLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
