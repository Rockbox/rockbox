.PHONY: all buildmanual clean

all: manual-pdf 

manual-prep: rockbox.tex
	@if [ "$(OBJDIR)" = "" ]; then echo Run make in you build directory!; false; fi
	@mkdir -p $(OBJDIR)
	@cp -R * $(OBJDIR)
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
