#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

CODECDIR = $(BUILDDIR)/apps/codecs
CODECS_SRC := $(call preprocess, $(APPSDIR)/codecs/SOURCES)
OTHER_SRC += $(CODECS_SRC)

CODECS := $(CODECS_SRC:.c=.codec)
CODECS := $(subst $(ROOTDIR),$(BUILDDIR),$(CODECS))

# the codec helper library
include $(APPSDIR)/codecs/lib/libcodec.make

# the codec libraries
include $(APPSDIR)/codecs/demac/libdemac.make
include $(APPSDIR)/codecs/liba52/liba52.make
include $(APPSDIR)/codecs/libalac/libalac.make
include $(APPSDIR)/codecs/libasap/libasap.make
include $(APPSDIR)/codecs/libfaad/libfaad.make
include $(APPSDIR)/codecs/libffmpegFLAC/libffmpegFLAC.make
include $(APPSDIR)/codecs/libm4a/libm4a.make
include $(APPSDIR)/codecs/libmad/libmad.make
include $(APPSDIR)/codecs/libmusepack/libmusepack.make
include $(APPSDIR)/codecs/libspc/libspc.make
include $(APPSDIR)/codecs/libspeex/libspeex.make
include $(APPSDIR)/codecs/libtremor/libtremor.make
include $(APPSDIR)/codecs/libwavpack/libwavpack.make
include $(APPSDIR)/codecs/libwma/libwma.make

# compile flags for codecs
CODECFLAGS = $(CFLAGS) -I$(APPSDIR)/codecs -I$(APPSDIR)/codecs/lib \
	-DCODEC

CODEC_LDS := $(APPSDIR)/plugins/plugin.lds # codecs and plugins use same file
CODECLINK_LDS := $(CODECDIR)/codec.link
CODEC_CRT0 := $(CODECDIR)/codec_crt0.o

CODECLIBS := $(DEMACLIB) $(A52LIB) $(ALACLIB) $(ASAPLIB) \
	$(FAADLIB) $(FFMPEGFLACLIB) $(M4ALIB) $(MADLIB) $(MUSEPACKLIB) \
	$(SPCLIB) $(SPEEXLIB) $(TREMORLIB) $(WAVPACKLIB) $(WMALIB) \
	$(CODECLIB)

$(CODECS): $(CODEC_CRT0) $(CODECLINK_LDS) 

$(CODECLINK_LDS): $(CODEC_LDS)
	$(call PRINTS,PP $(@F))
	$(shell mkdir -p $(dir $@))
	$(call preprocess2file, $<, $@, -DCODEC)

# codec/library dependencies
$(CODECDIR)/spc.codec : $(CODECDIR)/libspc.a
$(CODECDIR)/mpa.codec : $(CODECDIR)/libmad.a
$(CODECDIR)/a52.codec : $(CODECDIR)/liba52.a
$(CODECDIR)/flac.codec : $(CODECDIR)/libffmpegFLAC.a
$(CODECDIR)/vorbis.codec : $(CODECDIR)/libtremor.a
$(CODECDIR)/speex.codec : $(CODECDIR)/libspeex.a
$(CODECDIR)/mpc.codec : $(CODECDIR)/libmusepack.a
$(CODECDIR)/wavpack.codec : $(CODECDIR)/libwavpack.a
$(CODECDIR)/alac.codec : $(CODECDIR)/libalac.a $(CODECDIR)/libm4a.a 
$(CODECDIR)/aac.codec : $(CODECDIR)/libfaad.a $(CODECDIR)/libm4a.a
$(CODECDIR)/shorten.codec : $(CODECDIR)/libffmpegFLAC.a
$(CODECDIR)/ape.codec : $(CODECDIR)/libdemac.a
$(CODECDIR)/wma.codec : $(CODECDIR)/libwma.a
$(CODECDIR)/wavpack_enc.codec: $(CODECDIR)/libwavpack.a
$(CODECDIR)/asap.codec : $(CODECDIR)/libasap.a

$(CODECS): $(CODECLIB) # this must be last in codec dependency list

# libfaad and libmusepack both contain a huffman.h file, with different
# content. So we compile them with special command lines:

# pattern rule for compiling codecs
$(CODECDIR)/%.o: $(ROOTDIR)/apps/codecs/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) \
		-I$(dir $<) $(CODECFLAGS) -c $< -o $@

# pattern rule for compiling codecs
$(CODECDIR)/%.o: $(ROOTDIR)/apps/codecs/%.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) \
		-I$(dir $<) $(CODECFLAGS) -c $< -o $@

ifdef SIMVER
 CODECLDFLAGS = $(SHARED_FLAG) # <-- from Makefile
else
 CODECLDFLAGS = -T$(CODECLINK_LDS) -Wl,--gc-sections -Wl,-Map,$(CODECDIR)/$*.map
 CODECFLAGS += -UDEBUG -DNDEBUG
endif

$(CODECDIR)/%.codec: $(CODECDIR)/%.o
	$(call PRINTS,LD $(@F))$(CC) $(CODECFLAGS) -o $(CODECDIR)/$*.elf \
		$(filter %.o, $^) \
		$(filter %.a, $^) \
		-lgcc $(CODECLDFLAGS)
ifdef SIMVER
	$(SILENT)cp $(CODECDIR)/$*.elf $@
else
	$(SILENT)$(OC) -O binary $(CODECDIR)/$*.elf $@
endif
