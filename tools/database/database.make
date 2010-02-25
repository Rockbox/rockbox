#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: checkwps.make 22680 2009-09-11 17:58:17Z gevaerts $
#

DBDEFINES=-g -DDEBUG -D__PCTOOL__ -DSIMULATOR
CFLAGS+=$(DBDEFINES)

createsrc = $(shell cat $(1) > $(3); echo "\#if CONFIG_CODEC == SWCODEC" >> $(3); \
                                     echo $(2) | sed 's/ /\n/g' >> $(3); \
                                     echo "\#endif" >> $(3); \
                                     echo $(3))

METADATAS := $(subst $(ROOTDIR), ../.., $(wildcard $(ROOTDIR)/apps/metadata/*.c))

SRCFILE := $(call createsrc, $(TOOLSDIR)/database/SOURCES, \
                             $(METADATAS), \
                             $(TOOLSDIR)/database/SOURCES.build)

SRC= $(call preprocess, $(SRCFILE))

FIRMINC = -I$(ROOTDIR)/firmware/include -fno-builtin

INCLUDES = -I$(ROOTDIR)/apps/gui \
           -I$(ROOTDIR)/firmware/export \
           -I$(ROOTDIR)/apps \
           -I$(ROOTDIR)/apps/recorder \
           -I$(APPSDIR) \
           -I$(BUILDDIR) \

SIMINCLUDES += -I$(ROOTDIR)/uisimulator/sdl -I$(ROOTDIR)/uisimulator/common \
	-I$(FIRMDIR)/export $(TARGET_INC) -I$(BUILDDIR) -I$(APPSDIR)

# Makes mkdepfile happy
GCCOPTS+=`$(SDLCONFIG) --cflags`
OLDGCCOPTS:=$(GCCOPTS)
GCCOPTS+=-D__PCTOOL__  $(FIRMINC) $(SIMINCLUDES)

LIBS=`$(SDLCONFIG) --libs`
ifneq ($(findstring MINGW,$(shell uname)),MINGW)
LIBS += -ldl
endif

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

SIMFLAGS += $(SIMINCLUDES) $(DBDEFINES) -DHAVE_CONFIG_H $(OLDGCCOPTS) $(INCLUDES)

$(BUILDDIR)/$(BINARY): $$(OBJ)
	@echo LD $(BINARY)
	$(SILENT)$(HOSTCC) $(SIMFLAGS) $(LIBS) -o $@ $+

$(BUILDDIR)/tools/database/../../uisimulator/%.o: $(ROOTDIR)/uisimulator/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(SIMFLAGS) -c $< -o $@

$(BUILDDIR)/tools/database/database.o: $(APPSDIR)/database.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(SIMFLAGS) -c $< -o $@
