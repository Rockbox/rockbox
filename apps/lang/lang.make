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
LANG_O = $(BUILDDIR)/lang.o

CLEANOBJS += $(BUILDDIR)/max_language_size.h $(BUILDDIR)/lang.*

# $(BUILDDIR)/apps/lang must exist before we create dependencies on it,
# otherwise make will simply ignore those dependencies.
# Therefore we create it here.
#DUMMY := $(shell mkdir -p $(BUILDDIR)/apps/lang)

$(BUILDDIR)/max_language_size.h: $(LANGOBJ)
	$(call PRINTS,Create $(notdir $@))
	$(SILENT)echo "#define MAX_LANGUAGE_SIZE `ls -ln $(BUILDDIR)/apps/lang/* | awk '{print $$5}' | sort -n | tail -1`" > $@

$(BUILDDIR)/lang.o: $(APPSDIR)/lang/$(LANGUAGE).lang $(BUILDDIR)/apps/features
	$(SILENT)for f in `cat $(BUILDDIR)/apps/features`; do feat="$$feat:$$f" ; done; \
		perl -s $(TOOLSDIR)/genlang -p=$(BUILDDIR)/lang -t=$(MODELNAME)$$feat $<
	$(call PRINTS,CC lang.c)$(CC) $(CFLAGS) -c $(BUILDDIR)/lang.c -o $@

$(BUILDDIR)/%.lng : $(ROOTDIR)/%.lang $(BUILDDIR)/apps/genlang-features
	$(call PRINTS,GENLANG $(subst $(ROOTDIR)/,,$<))
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(TOOLSDIR)/genlang -e=$(APPSDIR)/lang/english.lang -t=$(MODELNAME)`cat $(BUILDDIR)/apps/genlang-features` -i=$(TARGET_ID) -b=$@ $<
