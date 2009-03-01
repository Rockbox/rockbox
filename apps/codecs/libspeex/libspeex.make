#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# we need to build two different speex libraries
# (one for codec, one for core voice)
# so a little trickery is necessary

SPEEXFLAGS = $(filter-out -O%,$(CODECFLAGS)) \
		 -DHAVE_CONFIG_H -DSPEEX_DISABLE_ENCODER \
		-I$(APPSDIR)/codecs/libspeex

# libspeex is faster on ARM-targets with -O1 instead of -O2
ifeq ($(CPU),arm)
   SPEEXFLAGS += -O1
else
   SPEEXFLAGS += -O2
endif

VOICESPEEXFLAGS = $(filter-out -ffunction-sections, $(filter-out -DCODEC,$(SPEEXFLAGS))) -DROCKBOX_VOICE_CODEC

# libspeex
SPEEXLIB := $(CODECDIR)/libspeex.a
SPEEXLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libspeex/SOURCES)
SPEEXLIB_OBJ := $(call c2obj, $(SPEEXLIB_SRC))
OTHER_SRC += $(SPEEXLIB_SRC)

$(SPEEXLIB): $(SPEEXLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

# libspeex-voice
VOICESPEEXLIB := $(CODECDIR)/libspeex-voice.a
VOICESPEEXLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libspeex/SOURCES)
VOICESPEEXLIB_OBJ := $(addsuffix .o,$(basename $(subst $(ROOTDIR)/apps/codecs/libspeex,$(BUILDDIR)/apps/codecs/libspeex-voice,$(VOICESPEEXLIB_SRC))))

$(VOICESPEEXLIB): $(VOICESPEEXLIB_OBJ)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

# pattern rules

$(CODECDIR)/libspeex-voice/%.o : $(ROOTDIR)/apps/codecs/libspeex/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(VOICESPEEXFLAGS) -c $< -o $@

$(CODECDIR)/libspeex-voice/%.o : $(ROOTDIR)/apps/codecs/libspeex/%.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(VOICESPEEXFLAGS) -c $< -o $@

$(CODECDIR)/libspeex/%.o: $(ROOTDIR)/apps/codecs/libspeex/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(SPEEXFLAGS) -c $< -o $@

$(CODECDIR)/libspeex/%.o: $(ROOTDIR)/apps/codecs/libspeex/%.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(SPEEXFLAGS) -c $< -o $@
