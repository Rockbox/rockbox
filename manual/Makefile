#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# Needed to build the features list.
INCLUDES= $(TARGET_INC) -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I. -I$(BUILDDIR)	\
 -I$(BUILDDIR)/bitmaps -I$(OBJDIR)

.PHONY: all buildmanual clean
MANFILE := rockbox-build

all: manual-pdf 

features: $(OBJDIR)/features

manual-prep: rockbox.tex
	@if [ "$(OBJDIR)" = "" ]; then echo Run make in you build directory!; false; fi
	@mkdir -p $(OBJDIR)
	@find * -type d \! -regex '.*\.svn.*' -exec mkdir -p $(OBJDIR)/{} \;
	@find * -type f \! -regex '.*\.svn.*' -exec cp -u {} $(OBJDIR)/{} \;
	@printf "%s%%" $(VERSION) > $(OBJDIR)/version.tex
	@perl credits.pl < $(DOCSDIR)/CREDITS  > $(OBJDIR)/CREDITS.tex
	@printf "\\\\newcommand{\\\\platform}{${MANUALDEV}}\n" > $(OBJDIR)/$(MANFILE).tex
	@printf "\\\\newcommand{\\\\buildversion}{$(VERSION)}\n" >> $(OBJDIR)/$(MANFILE).tex
	@printf "\\\\input{rockbox.tex}\n" >> $(OBJDIR)/$(MANFILE).tex
	@mv $(OBJDIR)/Makefile.pdflatex $(OBJDIR)/Makefile

$(OBJDIR)/features: $(ROOTDIR)/apps/features.txt
	$(SILENT)mkdir -p `dirname $@`
	$(SILENT)cat $< | $(HOSTCC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) \
	$(TARGET) $(DEFINES) $(EXTRA_DEFINES) -E -P -imacros "config.h" -imacros "button.h" - | \
	grep -v "^\#" | grep -v "^$$" > $@; \
	for f in `cat $(OBJDIR)/features`; do feat="$$feat\\n\\\\edef\\\\UseOption{\\\\UseOption,$$f}" ; done ; \
	printf "$$feat" >$(OBJDIR)/features.tex

manual-pdf: features manual-prep rockbox.tex
	$(MAKE) -C $(OBJDIR) buildmanual

manual-html: features manual-prep rockbox.tex
	$(MAKE) -C $(OBJDIR) buildhtml

manual-txt: manual-html
	$(MAKE) -C $(OBJDIR) buildtxt

manual-txt-zip: manual-txt
	$(MAKE) -C $(OBJDIR) txtzip

manual-zip: manual-html
	$(MAKE) -C $(OBJDIR) htmlzip

clean:
	@if [ "$(OBJDIR)" = "" ]; then echo Run make in you build directory!; false; fi
	@rm -rf $(OBJDIR)/manual $(OBJDIR)/*.pdf
	@rm -rf $(OBJJDIR)/html $(OBJDIR)/txt
