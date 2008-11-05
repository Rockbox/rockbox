#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES = -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I$(FIRMDIR)/common \
 -I$(FIRMDIR)/drivers -I$(APPSDIR) -Ilib -I$(BUILDDIR)
CFLAGS = $(INCLUDES) $(GCCOPTS) $(TARGET_INC) $(TARGET) $(EXTRA_DEFINES) \
 -DTARGET_ID=$(TARGET_ID) -DMEM=${MEMORYSIZE} -DCODEC

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

ifdef SOFTWARECODECS
   CODECLIBS = -lspc -lmad -la52 -lffmpegFLAC -ltremor -lwavpack -lmusepack -lalac -lfaad -lm4a -lspeex -ldemac -lwma -lasap
   OUTPUT = libspc libmad liba52 libffmpegFLAC libwma libtremor libwavpack libmusepack libalac libfaad libm4a libspeex libdemac libasap
endif

# we "borrow" the plugin LDS file
LDS := $(APPSDIR)/plugins/plugin.lds

LINKCODEC := $(OBJDIR)/codeclink.lds
DEPFILE = $(OBJDIR)/dep-codecs

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

ROCKS := $(SRC:%.c=$(OBJDIR)/%.codec)
SOURCES = $(SRC)
ELFS := $(SRC:%.c=$(OBJDIR)/%.elf)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
# as created by the cross-compiler for win32:
DEFS := $(SRC:%.c=$(OBJDIR)/%.def)
DIRS = .

CODECLIB := $(BUILDDIR)/libcodec.a
CODECDEPS = $(LINKCODEC) $(CODECLIB)

.PHONY: libspc libmad liba52 libffmpegFLAC libtremor libspeex libwavpack libmusepack libalac libfaad libm4a libdemac libwma libasap

all: $(LINKCODEC) $(ROCKS)

ifndef SIMVER
$(BUILDDIR)/%.a : % $(CODECDEPS)

# special dependencies
$(OBJDIR)/spc.elf : $(BUILDDIR)/libspc.a
$(OBJDIR)/mpa.elf : $(BUILDDIR)/libmad.a
$(OBJDIR)/a52.elf : $(BUILDDIR)/liba52.a
$(OBJDIR)/flac.elf : $(BUILDDIR)/libffmpegFLAC.a
$(OBJDIR)/vorbis.elf : $(BUILDDIR)/libtremor.a
$(OBJDIR)/speex.elf : $(BUILDDIR)/libspeex.a
$(OBJDIR)/mpc.elf : $(BUILDDIR)/libmusepack.a
$(OBJDIR)/wavpack.elf : $(BUILDDIR)/libwavpack.a
$(OBJDIR)/alac.elf : $(BUILDDIR)/libalac.a $(BUILDDIR)/libm4a.a
$(OBJDIR)/aac.elf : $(BUILDDIR)/libfaad.a $(BUILDDIR)/libm4a.a
$(OBJDIR)/shorten.elf : $(BUILDDIR)/libffmpegFLAC.a
$(OBJDIR)/ape.elf : $(BUILDDIR)/libdemac.a
$(OBJDIR)/wma.elf : $(BUILDDIR)/libwma.a
$(OBJDIR)/wavpack_enc.elf: $(BUILDDIR)/libwavpack.a
$(OBJDIR)/asap.elf : $(BUILDDIR)/libasap.a

# standard dependencies
$(OBJDIR)/%.elf: $(OBJDIR)/%.o $(OBJDIR)/codec_crt0.o $(CODECLIB)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) -o $@ $^ \
		-L$(BUILDDIR) -lcodec -lgcc -T$(LINKCODEC) \
		-Wl,--gc-sections,-Map,$(OBJDIR)/$*.map 

$(OBJDIR)/%.codec : $(OBJDIR)/%.elf
	$(call PRINTS,OBJCOPY $(@F))$(OC) -O binary $< $@
else

ifeq ($(SIMVER), x11)
###################################################
# This is the X11 simulator version

$(OBJDIR)/%.codec : $(OBJDIR)/%.o $(OBJDIR)/codec_crt0.o $(BUILDDIR)/libcodec.a $(OUTPUT)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) $(SHARED_FLAG) $< $(OBJDIR)/codec_crt0.o -L$(BUILDDIR) $(CODECLIBS) -lcodec -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	$(SILENT)chmod -x $@
endif

else	# end of x11-simulator
ifeq ($(SIMVER), sdl)
###################################################
# This is the SDL simulator version

$(OBJDIR)/%.codec : $(OBJDIR)/%.o $(OBJDIR)/codec_crt0.o $(BUILDDIR)/libcodec.a $(OUTPUT)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) $(SHARED_FLAG) $< $(OBJDIR)/codec_crt0.o -L$(BUILDDIR) $(CODECLIBS) -lcodec -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	$(SILENT)chmod -x $@
endif

else	# end of sdl-simulator
###################################################
# This is the win32 simulator version
DLLTOOLFLAGS = --export-all
DLLWRAPFLAGS = -s --entry _DllMain@12 --target=i386-mingw32 -mno-cygwin

$(OBJDIR)/%.codec : $(OBJDIR)/%.o $(OBJDIR)/codec_crt0.o $(BUILDDIR)/libcodec.a $(OUTPUT)
	$(call PRINTS,DLL $(@F))$(DLLTOOL) $(DLLTOOLFLAGS) -z $(OBJDIR)/$*.def $< $(OBJDIR)/codec_crt0.o
	$(SILENT)$(DLLWRAP) $(DLLWRAPFLAGS) --def $(OBJDIR)/$*.def $< $(OBJDIR)/codec_crt0.o\
		$(BUILDDIR)/libcodec.a $(patsubst -l%,$(BUILDDIR)/lib%.a,$(CODECLIBS)) \
		-o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	$(SILENT)chmod -x $@
endif
endif # end of win32-simulator
endif

endif # end of simulator section

include $(TOOLSDIR)/make.inc

$(BUILDDIR)/libcodec.a:
	$(SILENT)mkdir -p $(OBJDIR)/lib
	$(call PRINTS,MAKE in codecs/lib)$(MAKE) -C lib OBJDIR=$(OBJDIR)/lib

$(LINKCODEC): $(LDS)
	$(call PRINTS,build $(@F))cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) -DCODEC $(INCLUDES) $(TARGET) $(DEFINES) -E -P - >$@

$(BUILDDIR)/libspc.a: libspc

libspc:
	$(SILENT)mkdir -p $(OBJDIR)/libspc
	$(call PRINTS,MAKE in libspc)$(MAKE) -C libspc OBJDIR=$(OBJDIR)/libspc OUTPUT=$(BUILDDIR)/libspc.a

$(BUILDDIR)/libmad.a: libmad

libmad:
	$(SILENT)mkdir -p $(OBJDIR)/libmad
	$(call PRINTS,MAKE in libmad)$(MAKE) -C libmad OBJDIR=$(OBJDIR)/libmad OUTPUT=$(BUILDDIR)/libmad.a

$(BUILDDIR)/liba52.a: liba52

liba52:
	$(SILENT)mkdir -p $(OBJDIR)/liba52
	$(call PRINTS,MAKE in liba52)$(MAKE) -C liba52 OBJDIR=$(OBJDIR)/liba52 OUTPUT=$(BUILDDIR)/liba52.a

$(BUILDDIR)/libwma.a: libwma

libwma:
	$(SILENT)mkdir -p $(OBJDIR)/libwma
	$(call PRINTS,MAKE in libwma)$(MAKE) -C libwma OBJDIR=$(OBJDIR)/libwma OUTPUT=$(BUILDDIR)/libwma.a

$(BUILDDIR)/libffmpegFLAC.a: libffmpegFLAC

libffmpegFLAC:
	$(SILENT)mkdir -p $(OBJDIR)/libffmpegFLAC
	$(call PRINTS,MAKE in libffmpegFLAC)$(MAKE) -C libffmpegFLAC OBJDIR=$(OBJDIR)/libffmpegFLAC OUTPUT=$(BUILDDIR)/libffmpegFLAC.a

$(BUILDDIR)/libtremor.a: libtremor

libtremor:
	$(SILENT)mkdir -p $(OBJDIR)/libtremor
	$(call PRINTS,MAKE in libtremor)$(MAKE) -C libtremor OBJDIR=$(OBJDIR)/libtremor OUTPUT=$(BUILDDIR)/libtremor.a

$(BUILDDIR)/libspeex.a: libspeex

libspeex:
	$(SILENT)mkdir -p $(OBJDIR)/libspeex
	$(call PRINTS,MAKE in libspeex)$(MAKE) -C libspeex OBJDIR=$(OBJDIR)/libspeex OUTPUT=$(BUILDDIR)/libspeex.a

$(BUILDDIR)/libwavpack.a: libwavpack

libwavpack:
	$(SILENT)mkdir -p $(OBJDIR)/libwavpack
	$(call PRINTS,MAKE in libwavpack)$(MAKE) -C libwavpack OBJDIR=$(OBJDIR)/libwavpack OUTPUT=$(BUILDDIR)/libwavpack.a

$(BUILDDIR)/libmusepack.a: libmusepack

libmusepack:
	$(SILENT)mkdir -p $(OBJDIR)/libmusepack
	$(call PRINTS,MAKE in libmusepack)$(MAKE) -C libmusepack OBJDIR=$(OBJDIR)/libmusepack OUTPUT=$(BUILDDIR)/libmusepack.a

$(BUILDDIR)/libalac.a: libalac

libalac:
	$(SILENT)mkdir -p $(OBJDIR)/libalac
	$(call PRINTS,MAKE in libalac)$(MAKE) -C libalac OBJDIR=$(OBJDIR)/libalac OUTPUT=$(BUILDDIR)/libalac.a

$(BUILDDIR)/libm4a.a: libm4a

libm4a:
	$(SILENT)mkdir -p $(OBJDIR)/libm4a
	$(call PRINTS,MAKE in libm4a)$(MAKE) -C libm4a OBJDIR=$(OBJDIR)/libm4a OUTPUT=$(BUILDDIR)/libm4a.a

$(BUILDDIR)/libfaad.a: libfaad

libfaad:
	$(SILENT)mkdir -p $(OBJDIR)/libfaad
	$(call PRINTS,MAKE in libfaad)$(MAKE) -C libfaad OBJDIR=$(OBJDIR)/libfaad OUTPUT=$(BUILDDIR)/libfaad.a

$(BUILDDIR)/libdemac.a: libdemac

libdemac:
	$(SILENT)mkdir -p $(OBJDIR)/libdemac
	$(call PRINTS,MAKE in libdemac)$(MAKE) -C demac/libdemac OBJDIR=$(OBJDIR)/libdemac OUTPUT=$(BUILDDIR)/libdemac.a

$(BUILDDIR)/libasap.a: libasap

libasap:
	$(SILENT)mkdir -p $(OBJDIR)/libasap
	$(call PRINTS,MAKE in libasap)$(MAKE) -C libasap OBJDIR=$(OBJDIR)/libasap OUTPUT=$(BUILDDIR)/libasap.a
clean:
	$(call PRINTS,cleaning codecs)rm -fr \
		$(OBJDIR)/libspc $(BUILDDIR)/libspc.a \
		$(OBJDIR)/libmad $(BUILDDIR)/libmad.a \
		$(OBJDIR)/liba52 $(BUILDDIR)/liba52.a \
		$(OBJDIR)/libffmpegFLAC $(BUILDDIR)/libffmpegFLAC.a \
		$(OBJDIR)/libtremor $(BUILDDIR)/libtremor.a \
		$(OBJDIR)/libspeex $(BUILDDIR)/libSpeex.a \
		$(OBJDIR)/libwavpack $(BUILDDIR)/libwavpack.a \
		$(OBJDIR)/libmusepack $(BUILDDIR)/libmusepack.a \
		$(OBJDIR)/libalac $(BUILDDIR)/libalac.a \
		$(OBJDIR)/libfaad $(BUILDDIR)/libfaad.a \
		$(OBJDIR)/libm4a $(BUILDDIR)/libm4a.a \
		$(OBJDIR)/libdemac $(BUILDDIR)/libdemac.a \
		$(OBJDIR)/libwma $(BUILDDIR)/libwma.a
		$(OBJDIR)/libasap $(BUILDDIR)/libasap.a
	$(SILENT)$(MAKE) -C libspc clean OBJDIR=$(OBJDIR)/libspc
	$(SILENT)$(MAKE) -C libmad clean OBJDIR=$(OBJDIR)/libmad
	$(SILENT)$(MAKE) -C liba52 clean OBJDIR=$(OBJDIR)/liba52
	$(SILENT)$(MAKE) -C libffmpegFLAC clean OBJDIR=$(OBJDIR)/libffmpegFLAC
	$(SILENT)$(MAKE) -C libtremor clean OBJDIR=$(OBJDIR)/libtremor
	$(SILENT)$(MAKE) -C libspeex clean OBJDIR=$(OBJDIR)/libspeex
	$(SILENT)$(MAKE) -C libwavpack clean OBJDIR=$(OBJDIR)/libwavpack
	$(SILENT)$(MAKE) -C libmusepack clean OBJDIR=$(OBJDIR)/libmusepack
	$(SILENT)$(MAKE) -C libalac clean OBJDIR=$(OBJDIR)/libalac
	$(SILENT)$(MAKE) -C libfaad clean OBJDIR=$(OBJDIR)/libfaad
	$(SILENT)$(MAKE) -C libm4a clean OBJDIR=$(OBJDIR)/libm4a
	$(SILENT)$(MAKE) -C demac/libdemac clean OBJDIR=$(OBJDIR)/libdemac
	$(SILENT)$(MAKE) -C libwma clean OBJDIR=$(OBJDIR)/libwma
	$(SILENT)$(MAKE) -C libasap clean OBJDIR=$(OBJDIR)/libasap
	$(SILENT)$(MAKE) -C lib clean OBJDIR=$(OBJDIR)/lib

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPFILE)
endif
