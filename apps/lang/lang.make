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
ENGLISH := english

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

$(BUILDDIR)/lang/lang_core.o: $(BUILDDIR)/lang/lang.h $(BUILDDIR)/lang/lang_core.c
	$(call PRINTS,CC lang_core.c)$(CC) $(CFLAGS) -c $(BUILDDIR)/lang/lang_core.c -o $@

# genlang creates *both* lang.c and lang.h but in Make there is no wat to express this rule
# (multiple target rules DO NOT express that, they are a simple shortcut for multiple rules)
# instead we pretend that genlang create lang_core.c and that lang.c depends from lang.h
# it will work fine as long as one never manually removes lang.c and not lang.h, and it will avoid
# race conditions such as running genlang twice or worse in parallel with other things!
$(BUILDDIR)/lang/lang.h: $(APPSDIR)/lang/$(ENGLISH).lang $(BUILDDIR)/apps/features $(TOOLSDIR)/genlang
	$(call PRINTS,GEN lang.h)
	$(SILENT)for f in `cat $(BUILDDIR)/apps/features`; do feat="$$feat:$$f" ; done; \
		perl -s $(TOOLSDIR)/genlang -p=$(BUILDDIR)/lang -t=$(MODELNAME)$$feat $<
$(BUILDDIR)/lang/lang_core.c: $(BUILDDIR)/lang/lang.h $(TOOLSDIR)/genlang

$(BUILDDIR)/lang_enum.h: $(BUILDDIR)/lang/lang.h $(TOOLSDIR)/genlang

# NOTE: for some weird reasons in GNU make, multi targets rules WITH patterns actually express
# the fact that the two files are created as the result of one invocation of the rule
$(BUILDDIR)/%.lng $(BUILDDIR)/%.vstrings: $(ROOTDIR)/%.lang $(APPSDIR)/lang/$(ENGLISH).lang $(BUILDDIR)/apps/genlang-features $(TOOLSDIR)/genlang $(TOOLSDIR)/updatelang
	$(call PRINTS,GENLANG $(subst $(ROOTDIR)/,,$<))
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(TOOLSDIR)/updatelang $(APPSDIR)/lang/$(ENGLISH).lang $< $@.tmp
	$(SILENT)$(TOOLSDIR)/genlang -e=$(APPSDIR)/lang/$(ENGLISH).lang -t=$(MODELNAME):`cat $(BUILDDIR)/apps/genlang-features` -i=$(TARGET_ID) -b=$*.lng -c=$*.vstrings $@.tmp
	$(SILENT)rm -f $@.tmp

$(BUILDDIR)/apps/lang/voice-corrections.txt: $(ROOTDIR)/tools/voice-corrections.txt
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CP $(subst $(ROOTDIR)/,,$<))cp $< $@

$(BUILDDIR)/apps/lang/voicestrings.zip: $(VOICEOBJ) $(wildcard $(BUILDDIR)/apps/lang/*.talk) $(BUILDDIR)/apps/lang/voice-corrections.txt
	$(call PRINTS,ZIP $(subst $(BUILDDIR)/,,$@))
	$(SILENT)zip -9 -q $@ $(subst $(BUILDDIR)/,,$^)

#copy any included talk files to the /lang directory
$(BUILDDIR)/apps/lang/%.talk: $(ROOTDIR)/apps/lang/%.talk
	$(call PRINTS,CP $(subst $(ROOTDIR)/,,$<))cp $< $(BUILDDIR)/apps/lang
