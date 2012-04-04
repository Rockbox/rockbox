#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

LUA_SRCDIR := $(APPSDIR)/plugins/lua
LUA_BUILDDIR := $(BUILDDIR)/apps/plugins/lua

LUA_SRC := $(call preprocess, $(LUA_SRCDIR)/SOURCES)
LUA_OBJ := $(call c2obj, $(LUA_SRC))

OTHER_SRC += $(LUA_SRC)

ifndef APP_TYPE
ifneq (,$(strip $(foreach tgt,RECORDER ONDIO,$(findstring $(tgt),$(TARGET)))))
    ### lowmem targets
    ROCKS += $(LUA_BUILDDIR)/lua.ovl
    LUA_OUTLDS = $(LUA_BUILDDIR)/lua.link
    LUA_OVLFLAGS = -T$(LUA_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
else
    ### all other targets
    ROCKS += $(LUA_BUILDDIR)/lua.rock
endif
else
    ### simulator
    ROCKS += $(LUA_BUILDDIR)/lua.rock
endif

$(LUA_BUILDDIR)/lua.rock: $(LUA_OBJ) $(LUA_BUILDDIR)/actions.lua $(LUA_BUILDDIR)/buttons.lua $(LUA_BUILDDIR)/rocklib_aux.o

$(LUA_BUILDDIR)/actions.lua: $(LUA_OBJ) $(LUA_SRCDIR)/action_helper.pl
	$(call PRINTS,GEN $(@F))$(CC) $(PLUGINFLAGS) $(INCLUDES) -E $(APPSDIR)/plugins/lib/pluginlib_actions.h | $(LUA_SRCDIR)/action_helper.pl > $(LUA_BUILDDIR)/actions.lua

$(LUA_BUILDDIR)/buttons.lua: $(LUA_OBJ) $(LUA_SRCDIR)/button_helper.pl
	$(SILENT)$(CC) $(INCLUDES) -dM -E -include button-target.h - < /dev/null | $(LUA_SRCDIR)/button_helper.pl | $(HOSTCC) -fno-builtin $(INCLUDES) -x c -o $(LUA_BUILDDIR)/button_helper -
	$(call PRINTS,GEN $(@F))$(LUA_BUILDDIR)/button_helper > $(LUA_BUILDDIR)/buttons.lua

$(LUA_BUILDDIR)/rocklib_aux.c: $(APPSDIR)/plugin.h $(LUA_OBJ) $(LUA_SRCDIR)/rocklib_aux.pl
	$(call PRINTS,GEN $(@F))$(CC) $(PLUGINFLAGS) $(INCLUDES) -E -include plugin.h - < /dev/null | $(LUA_SRCDIR)/rocklib_aux.pl $(LUA_SRCDIR) > $(LUA_BUILDDIR)/rocklib_aux.c

$(LUA_BUILDDIR)/rocklib_aux.o: $(LUA_BUILDDIR)/rocklib_aux.c
	$(call PRINTS,CC $(<F))$(CC) $(INCLUDES) $(PLUGINFLAGS) -I $(LUA_SRCDIR) -c $< -o $@

$(LUA_BUILDDIR)/lua.refmap: $(LUA_OBJ)

$(LUA_OUTLDS): $(PLUGIN_LDS) $(LUA_BUILDDIR)/lua.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(LUA_BUILDDIR)/lua.refmap))

$(LUA_BUILDDIR)/lua.ovl: $(LUA_OBJ) $(LUA_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(LUA_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(call objcopy,$(basename $@).elf,$@)
