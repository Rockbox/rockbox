#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

TOOLSCFLAGS := -O -g -W -Wall -Wshadow -pedantic -I$(ROOTDIR)/tools

$(TOOLSDIR)/scramble: $(TOOLSDIR)/scramble.c $(TOOLSDIR)/iriver.c \
		$(TOOLSDIR)/mi4.c $(TOOLSDIR)/gigabeat.c \
		$(TOOLSDIR)/gigabeats.c $(TOOLSDIR)/telechips.c \
		$(TOOLSDIR)/iaudio_bl_flash.c \
		$(TOOLSDIR)/creative.c $(TOOLSDIR)/hmac-sha1.c
$(TOOLSDIR)/rdf2binary:	$(TOOLSDIR)/rdf2binary.c
$(TOOLSDIR)/convbdf: $(TOOLSDIR)/convbdf.c
$(TOOLSDIR)/codepages: $(TOOLSDIR)/codepages.c $(TOOLSDIR)/codepage_tables.c
$(TOOLSDIR)/mkboot: $(TOOLSDIR)/mkboot.c
$(TOOLSDIR)/mktccboot: $(TOOLSDIR)/mktccboot.c $(TOOLSDIR)/telechips.c
$(TOOLSDIR)/wavtrim: $(TOOLSDIR)/wavtrim.c
$(TOOLSDIR)/voicefont: $(TOOLSDIR)/voicefont.c

$(TOOLSDIR)/iaudio_bl_flash.c $(TOOLSDIR)/iaudio_bl_flash.h: $(TOOLSDIR)/iaudio_bl_flash.bmp $(TOOLSDIR)/bmp2rb
	$(call PRINTS,BMP2RB $(@F))
	$(SILENT)$(TOOLSDIR)/bmp2rb -f 7 -h $(TOOLSDIR) $< >$(TOOLSDIR)/iaudio_bl_flash.c

$(TOOLSDIR)/bmp2rb: $(TOOLSDIR)/bmp2rb.c
	$(call PRINTS,CC $(@F))
	$(SILENT)$(HOSTCC) -DAPPLICATION_NAME=\"$@\" $(TOOLSCFLAGS) $+ -o $@

$(TOOLSDIR)/uclpack: $(TOOLSDIR)/ucl/uclpack.c $(wildcard $(TOOLSDIR)/ucl/src/*.c)
	$(call PRINTS,CC $(@F))$(HOSTCC) $(TOOLSCFLAGS) -I$(TOOLSDIR)/ucl \
		-I$(TOOLSDIR)/ucl/include -o $@ $^

# implicit rule for simple tools
$(TOOLSDIR)/%: $(TOOLSDIR)/%.c
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$@))
	$(SILENT)$(HOSTCC) $(TOOLSCFLAGS) -o $@ $^
