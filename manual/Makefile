DOCUMENT := rockbox-build
LATEX := pdflatex
include LaTeX.Rules
GRAPHIC_FILES:=`find . -name \*.jpg \*.jpeg`

.PHONY: all cleaner buildmanual output

buildmanual: rockbox-build.tex
	@cp $(OBJDIR)/rockbox-build.pdf $(OBJDIR)/../rockbox-$(ARCHOS)-$(VERSION).pdf

