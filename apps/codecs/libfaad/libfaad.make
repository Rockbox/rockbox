#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libfaad
FAADLIB := $(CODECDIR)/libfaad.a
FAADLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libfaad/SOURCES)
FAADLIB_OBJ := $(call c2obj, $(FAADLIB_SRC))
OTHER_SRC += $(FAADLIB_SRC)
OTHER_INC += -I$(APPSDIR)/codecs/libfaad

$(FAADLIB): $(FAADLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

# libfaad is faster on ARM with -O2, use -O1 for other CPUs
FAADFLAGS = -I$(APPSDIR)/codecs/libfaad $(filter-out -O%,$(CODECFLAGS)) 
FAADFLAGS += -O2

$(CODECDIR)/libfaad/%.o: $(ROOTDIR)/apps/codecs/libfaad/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(FAADFLAGS) -c $< -o $@
