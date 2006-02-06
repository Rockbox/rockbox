.PHONY: all buildmanual clean

all: rockbox-build.tex

rockbox-build.tex: rockbox.tex
	@if [ "$(OBJDIR)" = "" ]; then echo Run make in you build diriectory!; false; fi
	@mkdir -p $(OBJDIR)
	@cp -R * $(OBJDIR)
	@echo "\newcommand{\platform}{${ARCHOS}}" > $(OBJDIR)/rockbox-build.tex
	@echo "\newcommand{\buildversion}{$(VERSION)}" >> $(OBJDIR)/rockbox-build.tex
	@echo "\input{rockbox.tex}" >> $(OBJDIR)/rockbox-build.tex
	@mv $(OBJDIR)/Makefile.pdflatex $(OBJDIR)/Makefile
	make -C $(OBJDIR)

clean:
	@if [ "$(OBJDIR)" == "" ]; then echo Run make in you build diriectory!; false; fi
	@rm -rf $(OBJDIR)/manual $(OBJDIR)/*.pdf
