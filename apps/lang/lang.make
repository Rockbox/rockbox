#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

LANGS := $(call preprocess, $(APPSDIR)/lang/SOURCES)
LANGOBJ := $(LANGS:$(ROOTDIR)/%.lang=$(BUILDDIR)/%.lng)
VOICEOBJ := $(LANGS:$(ROOTDIR)/%.lang=$(BUILDDIR)/%.vstrings)
LANG_O = $(BUILDDIR)/lang/lang_core.o

CLEANOBJS += $(BUILDDIR)/lang/max_language_size.h $(BUILDDIR)/lang/lang*

# $(BUILDDIR)/apps/lang must exist before we create dependencies on it,
# otherwise make will simply ignore those dependencies.
# Therefore we create it here.
#DUMMY := $(shell mkdir -p $(BUILDDIR)/apps/lang)

# Calculate the maximum language size. Currently based on the file size
# of the largest lng file. Subtract 10 due to HEADER_SIZE and 
# SUBHEADER_SIZE.
# TODO: In the future generate this file within genlang or another script
# in order to only calculate the maximum size based on the core strings.
$(BUILDDIR)/lang/max_language_size.h: $(LANGOBJ) $(BUILDDIR)/apps/lang/voicestrings.zip
	$(call PRINTS,GEN $(subst $(BUILDDIR)/,,$@))
	$(SILENT)echo "#define MAX_LANGUAGE_SIZE `ls -ln $(BUILDDIR)/apps/lang/*.lng | awk '{print $$5-10}' | sort -n | tail -1`" > $@

$(BUILDDIR)/lang/lang_core.o: $(APPSDIR)/lang/$(LANGUAGE).lang $(BUILDDIR)/apps/features
	$(SILENT)for f in `cat $(BUILDDIR)/apps/features`; do feat="$$feat:$$f" ; done; \
		perl -s $(TOOLSDIR)/genlang -p=$(BUILDDIR)/lang -t=$(MODELNAME)$$feat $<
	$(call PRINTS,CC lang_core.c)$(CC) $(CFLAGS) -c $(BUILDDIR)/lang/lang_core.c -o $@

$(BUILDDIR)/lang/lang.h: $(BUILDDIR)/lang/lang_core.o

$(BUILDDIR)/%.lng $(BUILDDIR)/%.vstrings: $(ROOTDIR)/%.lang $(BUILDDIR)/apps/genlang-features
	$(call PRINTS,GENLANG $(subst $(ROOTDIR)/,,$<))
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(TOOLSDIR)/genlang -e=$(APPSDIR)/lang/english.lang -t=$(MODELNAME):`cat $(BUILDDIR)/apps/genlang-features` -i=$(TARGET_ID) -b=$*.lng -c=$*.vstrings $<

$(BUILDDIR)/apps/lang/voicestrings.zip: $(VOICEOBJ)
	$(call PRINTS,ZIP $(subst $(BUILDDIR)/,,$@))
	$(SILENT)zip -9 -q $@ $(subst $(BUILDDIR)/,,$^)
