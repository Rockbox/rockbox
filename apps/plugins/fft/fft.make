#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

FFTSRCDIR := $(APPSDIR)/plugins/fft
FFTBUILDDIR := $(BUILDDIR)/apps/plugins/fft

ROCKS += $(FFTBUILDDIR)/fft.rock

FFT_SRC := $(call preprocess, $(FFTSRCDIR)/SOURCES)
FFT_OBJ := $(call c2obj, $(FFT_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(FFT_SRC)

FFTFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O3 -DFIXED_POINT=16

$(FFTBUILDDIR)/fft.rock: $(FFT_OBJ)

$(FFTBUILDDIR)/%.o: $(FFTSRCDIR)/%.c $(FFTSRCDIR)/fft.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(FFTFLAGS) -c $< -o $@
