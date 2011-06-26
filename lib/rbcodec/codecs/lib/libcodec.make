#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

CODECLIB := $(CODECDIR)/libcodec.a
CODECLIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/lib/SOURCES)
CODECLIB_OBJ := $(call c2obj, $(CODECLIB_SRC))
OTHER_SRC += $(CODECLIB_SRC)

$(CODECLIB): $(CODECLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

CODECLIBFLAGS = $(filter-out -O%,$(CODECFLAGS))

ifeq ($(MEMORYSIZE),2)
    CODECLIBFLAGS += -Os
else ifeq ($(ARCH),arch_m68k)
    CODECLIBFLAGS += -O2
else
    CODECLIBFLAGS += -O1
endif

# Do not use '-ffunction-sections' when compiling sdl-sim
ifneq ($(findstring sdl-sim, $(APP_TYPE)), sdl-sim)
    CODECLIBFLAGS += -ffunction-sections
endif

$(CODECDIR)/lib/%.o: $(RBCODECLIB_DIR)/codecs/lib/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) \
		-I$(dir $<) $(CODECLIBFLAGS) -c $< -o $@
