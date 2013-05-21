#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

LIBQUEUELIB := $(BUILDDIR)/lib/libqueuelib.a
LIBQUEUELIB_DIR := $(ROOTDIR)/lib/queuelib
LIBQUEUELIB_SRC := $(LIBQUEUELIB_DIR)/general.c \
					$(LIBQUEUELIB_DIR)/queue.c \
					$(LIBQUEUELIB_DIR)/thread.c
LIBQUEUELIB_OBJ := $(call c2obj, $(LIBQUEUELIB_SRC))

INCLUDES += -I$(LIBQUEUELIB_DIR)
OTHER_SRC += $(LIBQUEUELIB_SRC)

CORE_LIBS += $(LIBQUEUELIB)
CORE_GCSECTIONS := yes

LIBQUEUELIB_FLAGS := $(CFLAGS) $(SHARED_CFLAGS)

# Do not use '-ffunction-sections' and '-fdata-sections' when compiling sdl-sim
ifneq ($(findstring sdl-sim, $(APP_TYPE)), sdl-sim)
    LIBQUEUELIB_FLAGS += -ffunction-sections -fdata-sections
endif

$(LIBQUEUELIB)/lib/queuelib/%.o: $(LIBQUEUELIB_DIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) \
		$(LIBQUEUELIB_FLAGS) -c $< -o $@

$(LIBQUEUELIB): $(LIBQUEUELIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
