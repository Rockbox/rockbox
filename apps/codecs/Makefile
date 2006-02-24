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
CFLAGS = $(INCLUDES) $(GCCOPTS) $(TARGET) $(EXTRA_DEFINES) \
 -DTARGET_ID=$(TARGET_ID) -DMEM=${MEMORYSIZE} -DCODEC

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

ifdef SOFTWARECODECS
   CODECLIBS = -lmad -la52 -lffmpegFLAC -lTremor -lwavpack -lmusepack -lalac -lfaad -lm4a
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

CODECDEPS = $(LINKCODEC) $(BUILDDIR)/libcodec.a

.PHONY: libmad liba52 libffmpegFLAC libTremor libwavpack dumb libmusepack libalac libfaad libm4a

OUTPUT = $(SOFTWARECODECS)

all: $(ROCKS)

ifndef SIMVER
$(OBJDIR)/wav.elf : $(OBJDIR)/wav.o
$(OBJDIR)/aiff.elf : $(OBJDIR)/aiff.o
$(OBJDIR)/mpa.elf : $(OBJDIR)/mpa.o $(BUILDDIR)/libmad.a
$(OBJDIR)/a52.elf : $(OBJDIR)/a52.o $(BUILDDIR)/liba52.a
$(OBJDIR)/flac.elf : $(OBJDIR)/flac.o $(BUILDDIR)/libffmpegFLAC.a
$(OBJDIR)/vorbis.elf : $(OBJDIR)/vorbis.o $(BUILDDIR)/libTremor.a
$(OBJDIR)/mpc.elf : $(OBJDIR)/mpc.o $(BUILDDIR)/libmusepack.a
$(OBJDIR)/wavpack.elf : $(OBJDIR)/wavpack.o $(BUILDDIR)/libwavpack.a
$(OBJDIR)/alac.elf : $(OBJDIR)/alac.o $(BUILDDIR)/libalac.a $(BUILDDIR)/libm4a.a
$(OBJDIR)/aac.elf : $(OBJDIR)/aac.o $(BUILDDIR)/libfaad.a $(BUILDDIR)/libm4a.a
$(OBJDIR)/shorten.elf : $(OBJDIR)/shorten.o $(BUILDDIR)/libffmpegFLAC.a

$(OBJDIR)/%.elf :
	@echo "LD $(notdir $@)"
	$(SILENT)$(CC) $(GCCOPTS) -O -nostdlib -o $@ $^ -L$(BUILDDIR) -lcodec \
		-lgcc -T$(LINKCODEC) -Wl,-Map,$(OBJDIR)/$*.map

$(BUILDDIR)/%.a : % $(CODECDEPS)
	

$(OBJDIR)/%.codec : $(OBJDIR)/%.elf
	@echo "OBJCOPY $(notdir $@)"
	$(SILENT)$(OC) -O binary $< $@
else

ifeq ($(SIMVER), x11)
###################################################
# This is the X11 simulator version

$(OBJDIR)/%.codec : $(OBJDIR)/%.o  $(BUILDDIR)/libcodec.a $(OUTPUT)
	@echo "LD $(notdir $@)"
	$(SILENT)$(CC) $(CFLAGS) -shared $< -L$(BUILDDIR) $(CODECLIBS) -lcodec -o $@
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

$(OBJDIR)/%.codec : $(OBJDIR)/%.o  $(BUILDDIR)/libcodec.a $(OUTPUT)
	@echo "LD $(notdir $@)"
	$(SILENT)$(CC) $(CFLAGS) -shared $< -L$(BUILDDIR) $(CODECLIBS) -lcodec -o $@
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

$(OBJDIR)/%.codec : $(OBJDIR)/%.o  $(BUILDDIR)/libcodec.a $(OUTPUT)
	@echo "DLL $(notdir $@)"
	$(SILENT)$(DLLTOOL) $(DLLTOOLFLAGS) -z $(OBJDIR)/$*.def $<
	$(SILENT)$(DLLWRAP) $(DLLWRAPFLAGS) --def $(OBJDIR)/$*.def $< \
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
	@echo "MAKE in codecs/lib"
	$(SILENT)mkdir -p $(OBJDIR)/lib
	$(SILENT)$(MAKE) -C lib OBJDIR=$(OBJDIR)/lib

$(LINKCODEC): $(LDS)
	@echo "build $@"
	$(SILENT)cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) -DCODEC $(INCLUDES) $(TARGET) $(DEFINES) -E -P - >$@

libmad:
	@echo "MAKE in libmad"
	$(SILENT)mkdir -p $(OBJDIR)/libmad
	$(SILENT)$(MAKE) -C libmad OBJDIR=$(OBJDIR)/libmad OUTPUT=$(BUILDDIR)/libmad.a

liba52:
	@echo "MAKE in liba52"
	$(SILENT)mkdir -p $(OBJDIR)/liba52
	$(SILENT)$(MAKE) -C liba52 OBJDIR=$(OBJDIR)/liba52 OUTPUT=$(BUILDDIR)/liba52.a

libffmpegFLAC:
	@echo "MAKE in libffmpegFLAC"
	$(SILENT)mkdir -p $(OBJDIR)/libffmpegFLAC
	$(SILENT)$(MAKE) -C libffmpegFLAC OBJDIR=$(OBJDIR)/libffmpegFLAC OUTPUT=$(BUILDDIR)/libffmpegFLAC.a

libTremor:
	@echo "MAKE in Tremor"
	$(SILENT)mkdir -p $(OBJDIR)/Tremor
	$(SILENT)$(MAKE) -C Tremor OBJDIR=$(OBJDIR)/Tremor OUTPUT=$(BUILDDIR)/libTremor.a

libwavpack:
	@echo "MAKE in libwavpack"
	$(SILENT)mkdir -p $(OBJDIR)/libwavpack
	$(SILENT)$(MAKE) -C libwavpack OBJDIR=$(OBJDIR)/libwavpack OUTPUT=$(BUILDDIR)/libwavpack.a

dumb:
	@echo "MAKE in dumb"
	$(SILENT)mkdir -p $(OBJDIR)/dumb
	$(SILENT)$(MAKE) -C dumb OBJDIR=$(OBJDIR)/dumb LIBDIR=$(BUILDDIR) OFLAGS="$(CFLAGS)"

libmusepack:
	@echo "MAKE in libmusepack"
	$(SILENT)mkdir -p $(OBJDIR)/libmusepack
	$(SILENT)$(MAKE) -C libmusepack OBJDIR=$(OBJDIR)/libmusepack OUTPUT=$(BUILDDIR)/libmusepack.a

libalac:
	@echo "MAKE in libalac"
	$(SILENT)mkdir -p $(OBJDIR)/libalac
	$(SILENT)$(MAKE) -C libalac OBJDIR=$(OBJDIR)/libalac OUTPUT=$(BUILDDIR)/libalac.a

libm4a:
	@echo "MAKE in libm4a"
	$(SILENT)mkdir -p $(OBJDIR)/libm4a
	$(SILENT)$(MAKE) -C libm4a OBJDIR=$(OBJDIR)/libm4a OUTPUT=$(BUILDDIR)/libm4a.a

libfaad:
	@echo "MAKE in libfaad"
	$(SILENT)mkdir -p $(OBJDIR)/libfaad
	$(SILENT)$(MAKE) -C libfaad OBJDIR=$(OBJDIR)/libfaad OUTPUT=$(BUILDDIR)/libfaad.a

clean:
	@echo "cleaning codecs"
	$(SILENT)rm -fr $(OBJDIR)/libmad $(BUILDDIR)/libmad.a $(OBJDIR)/liba52 $(BUILDDIR)/liba52.a $(OBJDIR)/libffmpegFLAC $(BUILDDIR)/libffmpegFLAC.a $(OBJDIR)/Tremor $(BUILDDIR)/libTremor.a $(OBJDIR)/libwavpack $(BUILDDIR)/libwavpack.a $(OBJDIR)/dumb $(BUILDDIR)/libdumb.a $(BUILDDIR)/libdumbd.a $(OBJDIR)/libmusepack $(BUILDDIR)/libmusepack.a $(OBJDIR)/libalac $(BUILDDIR)/libalac.a $(OBJDIR)/libfaad $(BUILDDIR)/libfaad.a $(OBJDIR)/libm4a $(BUILDDIR)/libm4a.a
	$(SILENT)$(MAKE) -C libmad clean OBJDIR=$(OBJDIR)/libmad
	$(SILENT)$(MAKE) -C liba52 clean OBJDIR=$(OBJDIR)/liba52
	$(SILENT)$(MAKE) -C libffmpegFLAC clean OBJDIR=$(OBJDIR)/libffmpegFLAC
	$(SILENT)$(MAKE) -C Tremor clean OBJDIR=$(OBJDIR)/Tremor
	$(SILENT)$(MAKE) -C libwavpack clean OBJDIR=$(OBJDIR)/libwavpack
	$(SILENT)$(MAKE) -C libmusepack clean OBJDIR=$(OBJDIR)/libmusepack
	$(SILENT)$(MAKE) -C libalac clean OBJDIR=$(OBJDIR)/libalac
	$(SILENT)$(MAKE) -C libfaad clean OBJDIR=$(OBJDIR)/libfaad
	$(SILENT)$(MAKE) -C libm4a clean OBJDIR=$(OBJDIR)/libm4a
	$(SILENT)$(MAKE) -C dumb clean OBJDIR=$(OBJDIR)/dumb
	$(SILENT)$(MAKE) -C lib clean OBJDIR=$(OBJDIR)/lib

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPFILE)
endif
