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

LUA_INCLUDEDIR := $(LUA_SRCDIR)/include_lua
LUA_INCLUDELIST := $(addprefix $(LUA_BUILDDIR)/,audio.lua blit.lua color.lua \
						draw.lua draw_floodfill.lua draw_poly.lua draw_num.lua \
						draw_text.lua files.lua image.lua image_save.lua lcd.lua \
						math_ex.lua print.lua timer.lua playlist.lua pcm.lua \
						sound.lua rbcompat.lua rbsettings.lua poly_points.lua \
						printtable.lua printmenus.lua printsubmenu.lua \
						menubuttons.lua menucoresettings.lua create_kbd_layout.lua \
                        temploader.lua)

ifndef APP_TYPE
    ROCKS += $(LUA_BUILDDIR)/lua.rock
else
    ### simulator
    ROCKS += $(LUA_BUILDDIR)/lua.rock
endif

$(LUA_BUILDDIR)/lua.rock: $(LUA_OBJ) $(TLSFLIB) $(LUA_BUILDDIR)/actions.lua $(LUA_BUILDDIR)/buttons.lua $(LUA_BUILDDIR)/settings.lua \
                                                $(LUA_BUILDDIR)/rocklib_aux.o $(LUA_BUILDDIR)/rb_defines.lua $(LUA_BUILDDIR)/sound_defines.lua \
                                                $(LUA_INCLUDELIST)

$(LUA_BUILDDIR)/actions.lua: $(LUA_OBJ) $(LUA_SRCDIR)/action_helper.pl
	$(call PRINTS,GEN $(@F))$(CC) $(PLUGINFLAGS) $(INCLUDES) -E -P $(APPSDIR)/plugins/lib/pluginlib_actions.h | $(LUA_SRCDIR)/action_helper.pl > $(LUA_BUILDDIR)/actions.lua

$(LUA_BUILDDIR)/settings.lua: $(LUA_OBJ) $(LUA_SRCDIR)/settings_helper.pl
	$(SILENT)$(CC) $(INCLUDES) -E -P $(TARGET) $(CFLAGS) -include plugin.h -include cuesheet.h - < /dev/null | $(LUA_SRCDIR)/settings_helper.pl | \
		$(CC) $(INCLUDES) $(TARGET) $(CFLAGS) -S -x c -include config.h -include plugin.h -o $(LUA_BUILDDIR)/settings_helper.s -
	$(call PRINTS,GEN $(@F))$(LUA_SRCDIR)/settings_helper.pl < $(LUA_BUILDDIR)/settings_helper.s > $(LUA_BUILDDIR)/settings.lua

HOST_INCLUDES := $(filter-out %/libc/include,$(INCLUDES))
$(LUA_BUILDDIR)/buttons.lua: $(LUA_OBJ) $(LUA_SRCDIR)/button_helper.pl
	$(SILENT)$(CC) $(INCLUDES) $(TARGET) $(CFLAGS) -dM -E -P -include button-target.h - < /dev/null | $(LUA_SRCDIR)/button_helper.pl | $(HOSTCC) $(TARGET) -fno-builtin $(HOST_INCLUDES) -x c -o $(LUA_BUILDDIR)/button_helper -
	$(call PRINTS,GEN $(@F))$(LUA_BUILDDIR)/button_helper > $(LUA_BUILDDIR)/buttons.lua

$(LUA_BUILDDIR)/rb_defines.lua: $(LUA_OBJ) $(LUA_SRCDIR)/rbdefines_helper.pl
	$(SILENT)$(CC) $(INCLUDES) -dD -E -P $(TARGET) $(CFLAGS) -include plugin.h - < /dev/null | $(LUA_SRCDIR)/rbdefines_helper.pl "rb_defines" | \
	$(HOSTCC) -fno-builtin $(HOST_INCLUDES) -x c -o $(LUA_BUILDDIR)/rbdefines_helper -
	$(call PRINTS,GEN $(@F))$(LUA_BUILDDIR)/rbdefines_helper > $(LUA_BUILDDIR)/rb_defines.lua

$(LUA_BUILDDIR)/sound_defines.lua: $(LUA_OBJ) $(LUA_SRCDIR)/rbdefines_helper.pl
	$(SILENT)$(CC) $(INCLUDES) -dD -E -P $(TARGET) $(CFLAGS) -include config.h -include audiohw_settings.h - < /dev/null | $(LUA_SRCDIR)/rbdefines_helper.pl "sound_defines" | \
	$(HOSTCC) -fno-builtin $(HOST_INCLUDES) -x c -o $(LUA_BUILDDIR)/sounddefines_helper -
	$(call PRINTS,GEN $(@F))$(LUA_BUILDDIR)/sounddefines_helper > $(LUA_BUILDDIR)/sound_defines.lua

$(LUA_BUILDDIR)/rocklib_aux.c: $(APPSDIR)/plugin.h $(LUA_OBJ) $(LUA_SRCDIR)/rocklib_aux.pl
	$(call PRINTS,GEN $(@F))$(CC) $(PLUGINFLAGS) $(INCLUDES) -E -P -include plugin.h - < /dev/null | $(LUA_SRCDIR)/rocklib_aux.pl $(LUA_SRCDIR) > $(LUA_BUILDDIR)/rocklib_aux.c

$(LUA_BUILDDIR)/rocklib_aux.o: $(LUA_BUILDDIR)/rocklib_aux.c
	$(call PRINTS,CC $(<F))$(CC) $(INCLUDES) $(PLUGINFLAGS) -I $(LUA_SRCDIR) -c $< -o $@

$(LUA_BUILDDIR)/%.lua: $(LUA_INCLUDEDIR)/%.lua | $(LUA_BUILDDIR)
	$(call PRINTS,CP $(subst $(LUA_INCLUDEDIR)/,,$<))cp $< $@

$(LUA_BUILDDIR)/lua.refmap: $(LUA_OBJ) $(TLSFLIB)

$(LUA_OUTLDS): $(PLUGIN_LDS) $(LUA_BUILDDIR)/lua.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(LUA_BUILDDIR)/lua.refmap))

$(LUA_BUILDDIR)/lua.ovl: $(LUA_OBJ) $(TLSFLIB) $(LUA_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(LUA_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(call objcopy,$(basename $@).elf,$@)

$(LUA_BUILDDIR):
	$(call PRINTS,MKDIR $@)mkdir -p $(LUA_BUILDDIR)/
