#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

ENGLISH := english

# Use global GCCOPTS
GCCOPTS += -D__PCTOOL__ -DCHECKWPS

CHECKWPS_SRC = $(call preprocess, $(TOOLSDIR)/checkwps/SOURCES)
CHECKWPS_OBJ = $(call c2obj,$(CHECKWPS_SRC)) $(BUILDDIR)/lang/lang_core.o

OTHER_SRC += $(CHECKWPS_SRC)

INCLUDES = -I$(ROOTDIR)/apps/gui \
           -I$(ROOTDIR)/apps/gui/skin_engine \
           -I$(FIRMDIR)/kernel/include \
           -I$(ROOTDIR)/firmware/export \
           -I$(ROOTDIR)/firmware/include \
           -I$(ROOTDIR)/firmware/target/hosted \
           -I$(ROOTDIR)/firmware/target/hosted/sdl \
           -I$(ROOTDIR)/apps \
           -I$(ROOTDIR)/apps/recorder \
           -I$(ROOTDIR)/apps/radio \
           -I$(ROOTDIR)/lib/rbcodec \
           -I$(ROOTDIR)/lib/rbcodec/metadata \
           -I$(ROOTDIR)/lib/rbcodec/dsp \
           -I$(APPSDIR) \
           -I$(BUILDDIR) \
           -I$(BUILDDIR)/lang \
           $(TARGET_INC)

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

$(BUILDDIR)/$(BINARY): $$(CHECKWPS_OBJ) $$(CORE_LIBS)
	@echo LD $(BINARY)
	$(SILENT)$(HOSTCC) -o $@ $+ $(INCLUDE) $(GCCOPTS)  \
	-L$(BUILDDIR)/lib $(call a2lnk,$(CORE_LIBS))

#### Everything below is hacked in from apps.make and lang.make

$(BUILDDIR)/apps/features: $(ROOTDIR)/apps/features.txt
	$(SILENT)mkdir -p $(BUILDDIR)/apps
	$(SILENT)mkdir -p $(BUILDDIR)/lang
	$(call PRINTS,PP $(<F))
	$(SILENT)$(CC) $(PPCFLAGS) \
		-E -P -imacros "config.h" -imacros "button.h" -x c $< | \
	grep -v "^#" | grep -v "^ *$$" > $(BUILDDIR)/apps/features; \

$(BUILDDIR)/apps/genlang-features:  $(BUILDDIR)/apps/features
	$(call PRINTS,GEN $(subst $(BUILDDIR)/,,$@))tr \\n : < $< > $@

$(BUILDDIR)/lang_enum.h: $(BUILDDIR)/lang/lang.h $(TOOLSDIR)/genlang

$(BUILDDIR)/lang/lang.h: $(ROOTDIR)/apps/lang/$(ENGLISH).lang $(BUILDDIR)/apps/features $(TOOLSDIR)/genlang $(BUILDDIR)/apps/genlang-features
	$(call PRINTS,GEN lang.h)
	$(SILENT)$(TOOLSDIR)/genlang -e=$(ROOTDIR)/apps/lang/$(ENGLISH).lang -p=$(BUILDDIR)/lang -t=$(MODELNAME):`cat $(BUILDDIR)/apps/genlang-features` $<

$(BUILDDIR)/lang/lang_core.c: $(BUILDDIR)/lang/lang.h $(TOOLSDIR)/genlang

$(BUILDDIR)/lang/lang_core.o: $(BUILDDIR)/lang/lang.h $(BUILDDIR)/lang/lang_core.c
	$(call PRINTS,CC lang_core.c)$(CC) $(CFLAGS) -c $(BUILDDIR)/lang/lang_core.c -o $@

$(BUILDDIR)/lang/max_language_size.h: $(BUILDDIR)/lang/lang.h
	$(call PRINTS,GEN $(subst $(BUILDDIR)/,,$@))
	$(SILENT)echo "#define MAX_LANGUAGE_SIZE 131072" > $@
