#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

# cRSID
CRSID := $(CODECDIR)/cRSID.a
CRSID_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/cRSID/SOURCES)
CRSID_OBJ := $(call c2obj, $(CRSID_SRC))
OTHER_SRC += $(CRSID_SRC)

$(CRSID): $(CRSID_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
