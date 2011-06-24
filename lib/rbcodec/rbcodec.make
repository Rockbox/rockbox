#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

RBCODEC_LIB = $(RBCODEC_BLD)/librbcodec.a
RBCODEC_SRC := $(call preprocess, $(RBCODEC_DIR)/SOURCES)
RBCODEC_OBJ := $(call c2obj, $(RBCODEC_SRC))
INCLUDES += -I$(RBCODEC_DIR) -I$(RBCODEC_DIR)/dsp -I$(RBCODEC_DIR)/metadata
OTHER_SRC += $(RBCODEC_SRC)

$(RBCODEC_BLD)/%.o: $(RBCODEC_DIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $<)$(CC) $(CFLAGS) $(RBCODEC_CFLAGS) -c $< -o $@

$(RBCODEC_LIB): $(RBCODEC_OBJ)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
