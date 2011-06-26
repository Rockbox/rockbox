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

# disable strict aliasing optimizations for now, it gives warnings due to
# some horrid typecasting
_SPEEXFLAGS = $(filter-out -fstrict-aliasing, $(CODECFLAGS)) \
		-fno-strict-aliasing -DHAVE_CONFIG_H -DSPEEX_DISABLE_ENCODER \
		-I$(RBCODECLIB_DIR)/codecs/libspeex

# build voice codec with core -O switch
VOICESPEEXFLAGS = $(filter-out -ffunction-sections, $(filter-out -DCODEC,$(_SPEEXFLAGS))) -DROCKBOX_VOICE_CODEC

# libspeex is faster on ARM-targets with -O1 instead of -O2
SPEEXFLAGS = $(filter-out -O%,$(_SPEEXFLAGS))

ifeq ($(ARCH),arch_arm)
   SPEEXFLAGS += -O1
else
   SPEEXFLAGS += -O2
endif

# libspeex
SPEEXLIB := $(CODECDIR)/libspeex.a
SPEEXLIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libspeex/SOURCES)
SPEEXLIB_OBJ := $(call c2obj, $(SPEEXLIB_SRC))
OTHER_SRC += $(SPEEXLIB_SRC)

$(SPEEXLIB): $(SPEEXLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

# libspeex-voice
VOICESPEEXLIB := $(CODECDIR)/libspeex-voice.a
VOICESPEEXLIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libspeex/SOURCES)
VOICESPEEXLIB_OBJ := $(addsuffix .o,$(basename $(subst $(RBCODECLIB_DIR)/codecs/libspeex,$(RBCODEC_BLD)/codecs/libspeex-voice,$(VOICESPEEXLIB_SRC))))

$(VOICESPEEXLIB): $(VOICESPEEXLIB_OBJ)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

# pattern rules

$(CODECDIR)/libspeex-voice/%.o : $(RBCODECLIB_DIR)/codecs/libspeex/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(VOICESPEEXFLAGS) -c $< -o $@

$(CODECDIR)/libspeex-voice/%.o : $(RBCODECLIB_DIR)/codecs/libspeex/%.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(VOICESPEEXFLAGS) $(ASMFLAGS) -c $< -o $@

$(CODECDIR)/libspeex/%.o: $(RBCODECLIB_DIR)/codecs/libspeex/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(SPEEXFLAGS) -c $< -o $@

$(CODECDIR)/libspeex/%.o: $(RBCODECLIB_DIR)/codecs/libspeex/%.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(SPEEXFLAGS) $(ASMFLAGS) -c $< -o $@
