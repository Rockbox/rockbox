#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

# RBCODEC_BLD is defined in the calling Makefile
RBCODECLIB_DIR := $(ROOTDIR)/lib/rbcodec
RBCODECLIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/SOURCES, \
	$(RBCODEC_CFLAGS) -imacros rbcodecconfig.h)
RBCODECLIB_OBJ := $(call c2obj, $(RBCODECLIB_SRC))
RBCODECLIB := $(BUILDDIR)/lib/librbcodec.a

INCLUDES += -I$(RBCODECLIB_DIR) -I$(RBCODECLIB_DIR)/codecs \
			-I$(RBCODECLIB_DIR)/dsp -I$(RBCODECLIB_DIR)/metadata
OTHER_SRC += $(RBCODECLIB_SRC)
CORE_LIBS += $(RBCODECLIB)

$(RBCODECLIB): $(RBCODECLIB_OBJ)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

ifdef SOFTWARECODECS
  include $(RBCODECLIB_DIR)/codecs/codecs.make
endif
