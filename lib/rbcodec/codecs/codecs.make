#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

CODECDIR = $(RBCODEC_BLD)/codecs
CODECS_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/SOURCES)
OTHER_SRC += $(CODECS_SRC)

CODECS := $(CODECS_SRC:.c=.codec)
CODECS := $(subst $(RBCODECLIB_DIR),$(RBCODEC_BLD),$(CODECS))

# the codec helper library
include $(RBCODECLIB_DIR)/codecs/lib/libcodec.make
OTHER_INC += -I$(RBCODECLIB_DIR)/codecs/lib

# extra libraries
CODEC_LIBS := $(CODECLIB) $(FIXEDPOINTLIB)

# compile flags for codecs
CODECFLAGS := $(CFLAGS) $(RBCODEC_CFLAGS) -fstrict-aliasing \
			 -I$(RBCODECLIB_DIR)/codecs -I$(RBCODECLIB_DIR)/codecs/lib -DCODEC

ifdef APP_TYPE
 CODECLDFLAGS = $(SHARED_LDFLAG) -Wl,--gc-sections -Wl,-Map,$(CODECDIR)/$*.map
 CODECFLAGS += $(SHARED_CFLAGS) # <-- from Makefile
else
 CODECLDFLAGS = -T$(CODECLINK_LDS) -Wl,--gc-sections -Wl,-Map,$(CODECDIR)/$*.map
endif
CODECLDFLAGS += $(GLOBAL_LDOPTS)

# the codec libraries
include $(RBCODECLIB_DIR)/codecs/demac/libdemac.make
include $(RBCODECLIB_DIR)/codecs/liba52/liba52.make
include $(RBCODECLIB_DIR)/codecs/libalac/libalac.make
include $(RBCODECLIB_DIR)/codecs/libasap/libasap.make
include $(RBCODECLIB_DIR)/codecs/libasf/libasf.make
include $(RBCODECLIB_DIR)/codecs/libfaad/libfaad.make
include $(RBCODECLIB_DIR)/codecs/libffmpegFLAC/libffmpegFLAC.make
include $(RBCODECLIB_DIR)/codecs/libm4a/libm4a.make
include $(RBCODECLIB_DIR)/codecs/libmad/libmad.make
include $(RBCODECLIB_DIR)/codecs/libmusepack/libmusepack.make
include $(RBCODECLIB_DIR)/codecs/libspc/libspc.make
include $(RBCODECLIB_DIR)/codecs/libspeex/libspeex.make
include $(RBCODECLIB_DIR)/codecs/libtremor/libtremor.make
include $(RBCODECLIB_DIR)/codecs/libwavpack/libwavpack.make
include $(RBCODECLIB_DIR)/codecs/libwma/libwma.make
include $(RBCODECLIB_DIR)/codecs/libwmapro/libwmapro.make
include $(RBCODECLIB_DIR)/codecs/libcook/libcook.make
include $(RBCODECLIB_DIR)/codecs/librm/librm.make
include $(RBCODECLIB_DIR)/codecs/libatrac/libatrac.make
include $(RBCODECLIB_DIR)/codecs/libpcm/libpcm.make
include $(RBCODECLIB_DIR)/codecs/libtta/libtta.make
include $(RBCODECLIB_DIR)/codecs/libgme/libay.make
include $(RBCODECLIB_DIR)/codecs/libgme/libgbs.make
include $(RBCODECLIB_DIR)/codecs/libgme/libhes.make
include $(RBCODECLIB_DIR)/codecs/libgme/libnsf.make
include $(RBCODECLIB_DIR)/codecs/libgme/libsgc.make
include $(RBCODECLIB_DIR)/codecs/libgme/libvgm.make
include $(RBCODECLIB_DIR)/codecs/libgme/libkss.make
include $(RBCODECLIB_DIR)/codecs/libgme/libemu2413.make
include $(RBCODECLIB_DIR)/codecs/libopus/libopus.make

# set CODECFLAGS per codec lib, since gcc takes the last -Ox and the last
# in a -ffoo -fno-foo pair, there is no need to filter them out
$(A52LIB) : CODECFLAGS += -O1
$(ALACLIB) : CODECFLAGS += -O1
$(ASAPLIB) : CODECFLAGS += -O1
$(ASFLIB) : CODECFLAGS += -O2
$(ATRACLIB) : CODECFLAGS += -O1
$(AYLIB) : CODECFLAGS += -O2
$(COOKLIB): CODECFLAGS += -O1
$(DEMACLIB) : CODECFLAGS += -O3
$(FAADLIB) : CODECFLAGS += -O2
$(FFMPEGFLACLIB) : CODECFLAGS += -O2
$(GBSLIB) : CODECFLAGS +=  -O2
$(HESLIB) : CODECFLAGS +=  -O2
$(KSSLIB) : CODECFLAGS +=  -O2
$(M4ALIB) : CODECFLAGS += -O3
$(MUSEPACKLIB) : CODECFLAGS += -O1
$(NSFLIB) : CODECFLAGS +=  -O2
$(OPUSLIB) : CODECFLAGS +=  -O2
$(PCMSLIB) : CODECFLAGS += -O1
$(RMLIB) : CODECFLAGS += -O3
$(SGCLIB) : CODECFLAGS +=  -O2
$(SPCLIB) : CODECFLAGS +=  -O1
$(TREMORLIB) : CODECFLAGS += -O2
$(TTALIB) : CODECFLAGS += -O2
$(VGMLIB) : CODECFLAGS +=  -O2
$(EMU2413LIB) : CODECFLAGS +=  -O3
$(WAVPACKLIB) : CODECFLAGS += -O1
$(WMALIB) : CODECFLAGS += -O2
$(WMAPROLIB) : CODECFLAGS += -O1
$(WMAVOICELIB) : CODECFLAGS += -O1

# fine-tuning of CODECFLAGS per cpu arch
ifeq ($(ARCH),arch_arm)
  # redo per arm generation
  $(ALACLIB) : CODECFLAGS += -O2
  $(AYLIB) : CODECFLAGS +=  -O1
  $(GBSLIB) : CODECFLAGS +=  -O1
  $(HESLIB) : CODECFLAGS +=  -O1
  $(KSSLIB) : CODECFLAGS +=  -O1
  $(NSFLIB) : CODECFLAGS +=  -O1
  $(SGCLIB) : CODECFLAGS +=  -O1
  $(VGMLIB) : CODECFLAGS +=  -O1
  $(EMU2413LIB) : CODECFLAGS +=  -O3
  $(WAVPACKLIB) : CODECFLAGS += -O3
else ifeq ($(ARCH),arch_m68k)
  $(A52LIB) : CODECFLAGS += -O2
  $(ASFLIB) : CODECFLAGS += -O3
  $(ATRACLIB) : CODECFLAGS += -O2
  $(COOKLIB): CODECFLAGS += -O2
  $(DEMACLIB) : CODECFLAGS += -O2
  $(SPCLIB) : CODECFLAGS +=  -O3
  $(WMAPROLIB) : CODECFLAGS += -O3
  $(WMAVOICELIB) : CODECFLAGS += -O2
else ifeq ($(ARCH),arch_mips)
  $(OPUSLIB) : CODECFLAGS += -fno-strict-aliasing
endif

ifeq ($(MEMORYSIZE),2)
  $(ASFLIB) : CODECFLAGS += -Os
  $(WMALIB) : CODECFLAGS += -Os
endif

ifndef APP_TYPE
  CONFIGFILE := $(FIRMDIR)/export/config/$(MODELNAME).h
  CODEC_LDS := $(APPSDIR)/plugins/plugin.lds # codecs and plugins use same file
  CODECLINK_LDS := $(CODECDIR)/codec.link
endif

CODEC_CRT0 := $(CODECDIR)/codec_crt0.o

$(CODECS): $(CODEC_CRT0) $(CODECLINK_LDS)

$(CODECLINK_LDS): $(CODEC_LDS) $(CONFIGFILE)
	$(call PRINTS,PP $(@F))
	$(shell mkdir -p $(dir $@))
	$(call preprocess2file, $<, $@, -DCODEC)

# codec/library dependencies
$(CODECDIR)/spc.codec : $(CODECDIR)/libspc.a
$(CODECDIR)/mpa.codec : $(CODECDIR)/libmad.a $(CODECDIR)/libasf.a
$(CODECDIR)/a52.codec : $(CODECDIR)/liba52.a
$(CODECDIR)/flac.codec : $(CODECDIR)/libffmpegFLAC.a
$(CODECDIR)/vorbis.codec : $(CODECDIR)/libtremor.a $(TLSFLIB) $(SETJMPLIB)
$(CODECDIR)/speex.codec : $(CODECDIR)/libspeex.a
$(CODECDIR)/mpc.codec : $(CODECDIR)/libmusepack.a
$(CODECDIR)/wavpack.codec : $(CODECDIR)/libwavpack.a
$(CODECDIR)/alac.codec : $(CODECDIR)/libalac.a $(CODECDIR)/libm4a.a 
$(CODECDIR)/aac.codec : $(CODECDIR)/libfaad.a $(CODECDIR)/libm4a.a
$(CODECDIR)/shorten.codec : $(CODECDIR)/libffmpegFLAC.a
$(CODECDIR)/ape-pre.map : $(CODECDIR)/libdemac-pre.a
$(CODECDIR)/ape.codec : $(CODECDIR)/libdemac.a
$(CODECDIR)/wma.codec : $(CODECDIR)/libwma.a $(CODECDIR)/libasf.a
$(CODECDIR)/wmapro.codec : $(CODECDIR)/libwmapro.a $(CODECDIR)/libasf.a
$(CODECDIR)/wavpack_enc.codec: $(CODECDIR)/libwavpack.a
$(CODECDIR)/asap.codec : $(CODECDIR)/libasap.a
$(CODECDIR)/cook.codec : $(CODECDIR)/libcook.a $(CODECDIR)/librm.a
$(CODECDIR)/raac.codec : $(CODECDIR)/libfaad.a $(CODECDIR)/librm.a
$(CODECDIR)/a52_rm.codec : $(CODECDIR)/liba52.a $(CODECDIR)/librm.a
$(CODECDIR)/atrac3_rm.codec : $(CODECDIR)/libatrac.a $(CODECDIR)/librm.a
$(CODECDIR)/atrac3_oma.codec : $(CODECDIR)/libatrac.a
$(CODECDIR)/aiff.codec : $(CODECDIR)/libpcm.a
$(CODECDIR)/wav.codec : $(CODECDIR)/libpcm.a
$(CODECDIR)/smaf.codec : $(CODECDIR)/libpcm.a
$(CODECDIR)/au.codec : $(CODECDIR)/libpcm.a
$(CODECDIR)/vox.codec : $(CODECDIR)/libpcm.a
$(CODECDIR)/wav64.codec : $(CODECDIR)/libpcm.a
$(CODECDIR)/tta.codec : $(CODECDIR)/libtta.a
$(CODECDIR)/ay.codec : $(CODECDIR)/libay.a
$(CODECDIR)/gbs.codec : $(CODECDIR)/libgbs.a
$(CODECDIR)/hes.codec : $(CODECDIR)/libhes.a
$(CODECDIR)/nsf.codec : $(CODECDIR)/libnsf.a $(CODECDIR)/libemu2413.a
$(CODECDIR)/sgc.codec : $(CODECDIR)/libsgc.a $(CODECDIR)/libemu2413.a
$(CODECDIR)/vgm.codec : $(CODECDIR)/libvgm.a $(CODECDIR)/libemu2413.a
$(CODECDIR)/kss.codec : $(CODECDIR)/libkss.a $(CODECDIR)/libemu2413.a
$(CODECDIR)/opus.codec : $(CODECDIR)/libopus.a $(TLSFLIB)
$(CODECDIR)/aac_bsf.codec : $(CODECDIR)/libfaad.a

$(CODECS): $(CODEC_LIBS) # this must be last in codec dependency list

# pattern rule for compiling codecs
$(CODECDIR)/%.o: $(RBCODECLIB_DIR)/codecs/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) \
		-I$(dir $<) $(CODECFLAGS) -c $< -o $@

# pattern rule for compiling codecs
$(CODECDIR)/%.o: $(RBCODECLIB_DIR)/codecs/%.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) \
		-I$(dir $<) $(CODECFLAGS) $(ASMFLAGS) -c $< -o $@

$(CODECDIR)/%-pre.map: $(CODEC_CRT0) $(CODECLINK_LDS) $(CODECDIR)/%.o $(CODECS_LIBS)
	$(call PRINTS,LD $(@F))$(CC) $(CODECFLAGS) -o $(CODECDIR)/$*-pre.elf \
		$(filter %.o, $^) \
		$(filter-out $(CODECLIB),$(filter %.a, $+)) $(CODECLIB) \
		-lgcc $(subst .map,-pre.map,$(CODECLDFLAGS))

$(CODECDIR)/%.codec: $(CODECDIR)/%.o
	$(call PRINTS,LD $(@F))$(CC) $(CODECFLAGS) -o $(CODECDIR)/$*.elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(CODECLDFLAGS)
	$(SILENT)$(call objcopy,$(CODECDIR)/$*.elf,$@)
