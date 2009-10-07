#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: checkwps.make 22680 2009-09-11 17:58:17Z gevaerts $
#

FIRMINC = -I../../firmware/include -fno-builtin

DBDEFINES=-g -DDEBUG -D__PCTOOL__ -DSIMULATOR
CFLAGS+=$(DBDEFINES)

SRC= $(call preprocess, $(TOOLSDIR)/database/SOURCES)

FIRMINC = -I$(ROOTDIR)/firmware/include -fno-builtin

INCLUDES = -I$(ROOTDIR)/apps/gui \
           -I$(ROOTDIR)/firmware/export \
           -I$(ROOTDIR)/apps \
           -I$(APPSDIR) \
           -I$(BUILDDIR) \

SIMINCLUDES += -I$(ROOTDIR)/uisimulator/sdl -I$(ROOTDIR)/uisimulator/common \
	-I$(FIRMDIR)/export $(TARGET_INC) -I$(BUILDDIR) -I$(APPSDIR) -I/usr/include/SDL

# Makes mkdepfile happy
OLDGCCOPTS:=$(GCCOPTS)
GCCOPTS+=-D__PCTOOL__  $(FIRMINC) $(SIMINCLUDES)

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

$(BUILDDIR)/$(BINARY): $$(OBJ)
	@echo LD $(BINARY)
	$(SILENT)$(HOSTCC) $(INCLUDE) $(FLAGS) -ldl -o $@ $+

SIMFLAGS += $(SIMINCLUDES) $(DBDEFINES) -DHAVE_CONFIG_H $(OLDGCCOPTS) $(INCLUDES)

$(BUILDDIR)/tools/database/../../uisimulator/%.o: $(ROOTDIR)/uisimulator/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(SIMFLAGS) -c $< -o $@

$(BUILDDIR)/tools/database/database.o: $(APPSDIR)/database.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(SIMFLAGS) -c $< -o $@
