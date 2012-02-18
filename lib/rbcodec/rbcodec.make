#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

ifndef PRINTS
  PRINTS=$(SILENT)$(call info,$(1))
endif
ifndef preprocess
  preprocess = $(shell $(CC) $(PPCFLAGS) $(RBCODEC_CFLAGS) $(2) -E -P -x c -imacros rbcodecconfig.h $(1) | \
		grep -v '^\#' | grep -v "^ *$$" | \
		sed -e 's:^..*:$(dir $(1))&:')
endif
ifndef c2obj
  c2obj = $(addsuffix .o,$(basename $(subst $(RBCODEC_DIR),$(RBCODEC_BLD),$(1))))
endif
ifndef MEMORYSIZE
  MEMORYSIZE = 128
endif
ifndef CPU
  CPU =
endif

RBCODEC_CFLAGS += -I$(RBCODEC_DIR)/util
RBCODEC_LIB = $(RBCODEC_BLD)/librbcodec.a
RBCODEC_SRC := $(call preprocess, $(RBCODEC_DIR)/SOURCES, $(RBCODEC_CFLAGS) \
	-imacros rbcodecconfig.h)
RBCODEC_OBJ := $(call c2obj, $(RBCODEC_SRC))
INCLUDES += -I$(RBCODEC_DIR) -I$(RBCODEC_DIR)/codecs -I$(RBCODEC_DIR)/dsp \
            -I$(RBCODEC_DIR)/metadata
OTHER_SRC += $(RBCODEC_SRC)

$(RBCODEC_BLD)/%.o: $(RBCODEC_DIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $<)$(CC) $(CFLAGS) $(RBCODEC_CFLAGS) -c $< -o $@

$(RBCODEC_LIB): $(RBCODEC_OBJ)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

ifdef SOFTWARECODECS
  include $(RBCODEC_DIR)/codecs/codecs.make
endif
