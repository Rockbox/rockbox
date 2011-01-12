#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: libatrac.make 20151 2009-03-01 09:04:15Z amiconn $
#

# libatrac
ATRACLIB := $(CODECDIR)/libatrac.a
ATRACLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libatrac/SOURCES)
ATRACLIB_OBJ := $(call c2obj, $(ATRACLIB_SRC))
OTHER_SRC += $(ATRACLIB_SRC)

$(ATRACLIB): $(ATRACLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

ATRACFLAGS = -I$(APPSDIR)/codecs/libatrac $(filter-out -O%,$(CODECFLAGS))
# not tuned for arm
ifeq ($(CPU),coldfire)
    ATRACFLAGS += -O2
else
    ATRACFLAGS += -O1
endif

$(CODECDIR)/libatrac/%.o: $(ROOTDIR)/apps/codecs/libatrac/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(ATRACFLAGS) -c $< -o $@

