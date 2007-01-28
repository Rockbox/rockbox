#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

.PHONY: all buildmanual clean

all: manual-pdf 

manual-prep: rockbox.tex
	@if [ "$(OBJDIR)" = "" ]; then echo Run make in you build directory!; false; fi
	@mkdir -p $(OBJDIR)
	@find * -type d \! -regex '.*\.svn.*' -exec mkdir -p $(OBJDIR)/{} \;
	@find * -type f \! -regex '.*\.svn.*' -exec cp {} $(OBJDIR)/{} \;
	@../tools/svnversion.sh | sed -e 's/\n*$$/%/' > version.tex
	@perl credits.pl < ../docs/CREDITS | iconv -f utf-8 -t iso-8859-1 > $(OBJDIR)/CREDITS.tex
	@echo "\newcommand{\platform}{${MANUALDEV}}" > $(OBJDIR)/rockbox-build.tex
	@echo "\newcommand{\buildversion}{$(VERSION)}" >> $(OBJDIR)/rockbox-build.tex
	@echo "\input{rockbox.tex}" >> $(OBJDIR)/rockbox-build.tex
	@mv $(OBJDIR)/Makefile.pdflatex $(OBJDIR)/Makefile

manual-pdf: manual-prep rockbox.tex
	$(MAKE) -C $(OBJDIR) buildmanual

manual-html: manual-prep rockbox.tex
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
