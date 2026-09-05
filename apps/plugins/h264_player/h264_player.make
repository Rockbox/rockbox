#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

H264PLAYERSRCDIR := $(APPSDIR)/plugins/h264_player
H264PLAYERBUILDDIR := $(BUILDDIR)/apps/plugins/h264_player

ROCKS += $(H264PLAYERBUILDDIR)/h264_player.rock

H264PLAYER_SRC := $(call preprocess, $(H264PLAYERSRCDIR)/SOURCES)
H264PLAYER_OBJ := $(call c2obj, $(H264PLAYER_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(H264PLAYER_SRC)

$(H264PLAYERBUILDDIR)/h264_player.rock: $(H264PLAYER_OBJ)

$(H264PLAYERBUILDDIR)/%.o: $(H264PLAYERSRCDIR)/%.c \
                          $(H264PLAYERSRCDIR)/h264_player.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) \
		$(PLUGINFLAGS) -c $< -o $@
