#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

ifndef V
SILENT=@
endif

# Needed to build the features list.
INCLUDES= $(TARGET_INC) -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I. -I$(BUILDDIR)	\
 -I$(BUILDDIR)/bitmaps -I$(OBJDIR)

.PHONY: all buildmanual clean objdir-check
MANFILE := rockbox-build

VERSION ?= $(shell $(TOOLSDIR)/version.sh $(ROOTDIR))

.PHONY: $(OBJDIR)/version.tex $(OBJDIR)/$(MANFILE).tex

all: manual-pdf 

features: $(OBJDIR)/features.tex

objdir-check:
	@if [ "$(OBJDIR)" = "" ]; then echo Run make in you build directory!; false; fi


manual-prep: objdir-check rockbox.tex $(OBJDIR)/CREDITS.tex $(OBJDIR)/version.tex $(OBJDIR)/$(MANFILE).tex
	$(SILENT)mkdir -p $(OBJDIR)
	$(SILENT)find * -type d \! -regex '.*\.svn.*' -exec mkdir -p $(OBJDIR)/{} \;
	$(SILENT)find -L * -type f \! -regex '.*\.svn.*' -exec cp {} $(OBJDIR)/{} \;
	$(SILENT)mv $(OBJDIR)/Makefile.pdflatex $(OBJDIR)/Makefile

$(OBJDIR)/$(MANFILE).tex:
	@echo "creating $(notdir $@)"
	$(SILENT)printf "\\\\newcommand{\\\\platform}{${MANUALDEV}}\n" > $@
	$(SILENT)printf "\\\\newcommand{\\\\buildversion}{$(VERSION)}\n" >> $@
	$(SILENT)printf "\\\\input{rockbox.tex}\n" >> $@

$(OBJDIR)/CREDITS.tex: $(DOCSDIR)/CREDITS
	@echo "processing $(notdir $^)"
	$(SILENT)mkdir -p $(OBJDIR)
	$(SILENT)perl credits.pl < $(DOCSDIR)/CREDITS  > $@

$(OBJDIR)/version.tex:
	@echo "creating $(notdir $@)"
	$(SILENT)mkdir -p $(OBJDIR)
	$(SILENT)printf "%s%%" $(VERSION) > $@

$(OBJDIR)/features.tex: $(ROOTDIR)/apps/features.txt
	@echo "processing $(notdir $^)"
	$(SILENT)mkdir -p $(OBJDIR)
	$(SILENT)mkdir -p `dirname $@`
	$(SILENT) for f in \
	    $$(cat $< | $(HOSTCC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) \
	       $(TARGET) $(DEFINES) $(EXTRA_DEFINES) -E -P -imacros "config.h" \
	       -imacros "button.h" - | grep -v "^\#" | grep -v "^$$"); \
	    do feat="$$feat\\n\\\\edef\\\\UseOption{\\\\UseOption,$$f}"; done ; \
	printf "$$feat" > $@

manual-pdf: objdir-check features manual-prep rockbox.tex
	$(SILENT)$(MAKE) -C $(OBJDIR) buildmanual

manual-html: objdir-check features manual-prep rockbox.tex
	$(SILENT)$(MAKE) -C $(OBJDIR) buildhtml

manual-txt: manual-html
	$(SILENT)$(MAKE) -C $(OBJDIR) buildtxt

manual-txt-zip: manual-txt
	$(SILENT)$(MAKE) -C $(OBJDIR) txtzip

manual-zip: manual-html
	$(SILENT)$(MAKE) -C $(OBJDIR) htmlzip

clean: objdir-check
	$(SILENT)rm -rf $(OBJDIR)/manual $(OBJDIR)/*.pdf
	$(SILENT)rm -rf $(OBJDIR)/html $(OBJDIR)/txt
