#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

FIXEDPOINTLIB := $(BUILDDIR)/lib/libfixedpoint.a
FIXEDPOINTLIB_DIR := $(ROOTDIR)/lib/fixedpoint
FIXEDPOINTLIB_SRC := $(FIXEDPOINTLIB_DIR)/fixedpoint.c
FIXEDPOINTLIB_OBJ := $(call c2obj, $(FIXEDPOINTLIB_SRC))

INCLUDES += -I$(FIXEDPOINTLIB_DIR)
OTHER_SRC += $(FIXEDPOINTLIB_SRC)

CORE_LIBS += $(FIXEDPOINTLIB)
CORE_GCSECTIONS := yes

FIXEDPOINTLIB_FLAGS := $(CFLAGS) $(SHARED_CFLAGS)

# Do not use '-ffunction-sections' and '-fdata-sections' when compiling sdl-sim
ifneq ($(findstring sdl-sim, $(APP_TYPE)), sdl-sim)
    FIXEDPOINTLIB_FLAGS += -ffunction-sections -fdata-sections
endif

$(FIXEDPOINTLIB_OBJ): $(FIXEDPOINTLIB_SRC)
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) \
		$(FIXEDPOINTLIB_FLAGS) -c $< -o $@

$(FIXEDPOINTLIB): $(FIXEDPOINTLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
