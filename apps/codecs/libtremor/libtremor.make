#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

# libtremor
TREMORLIB := $(CODECDIR)/libtremor.a
TREMORLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libtremor/SOURCES)
TREMORLIB_OBJ := $(call c2obj, $(TREMORLIB_SRC))
OTHER_SRC += $(TREMORLIB_SRC)

$(TREMORLIB): $(TREMORLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rs $@ $^ >/dev/null 2>&1

$(CODECDIR)/libtremor/%.o: $(ROOTDIR)/apps/codecs/libtremor/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) \
		-I$(APPSDIR)/codecs/libtremor \
		$(CODECFLAGS) $(CFLAGS) -c $< -o $@

TREMORFLAGS = -I$(APPSDIR)/codecs/libtremor $(filter-out -O%,$(CODECFLAGS)) 

# Tremor is slightly faster on coldfire with -O3
ifeq ($(CPU),coldfire)
    TREMORFLAGS += -O3
else
    TREMORFLAGS += -O2
endif

$(CODECDIR)/libtremor/%.o: $(ROOTDIR)/apps/codecs/libtremor/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(TREMORFLAGS) -c $< -o $@
