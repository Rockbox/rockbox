# -*- Makefile -*-

ifndef V
SILENT=@
endif

$(OBJDIR)/%.o: %.c
	$(SILENT)mkdir -p `dirname $@`
	@echo "CC $<"
	$(SILENT)$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.S
	$(SILENT)mkdir -p `dirname $@`
	@echo "CC $<"
	$(SILENT)$(CC) $(CFLAGS) -c $< -o $@

# The echo stuff last in the dep update shell magic is to prevent any compiler
# errors/warnings to cause an error code to get returned and thus stop the
# build
#
# This script checks for the presence of $(BUILDDIR)/lang.h and if it doesn't
# alreay exist, it is created. This is because lots of files include this
# *generated* file and if it doesn't exist when we generate the deps it is no
# good. There's a -MG compiler option to cover for this case, but it doesn't
# play nicely into this as it then adds a dependency to the lang.h file
# without the proper path.
#
$(DEPFILE): $(SOURCES)
	$(SILENT)mkdir -p `dirname $(DEPFILE)`
	$(SILENT)rm -f $(DEPFILE)
	$(SILENT)(if test ! -f $(BUILDDIR)/lang.h; then \
	  touch $(BUILDDIR)/lang.h; \
	fi; \
	for each in $(SOURCES) x; do \
          if test "x" != "$$each"; then \
	    obj=`echo $$each | sed -e 's/\.[cS]/.o/'`; \
	    $(CC) -MM -MT "$(OBJDIR)/$$obj" $(CFLAGS) $$each >> $(DEPFILE) 2>/dev/null; \
          fi; \
	done; \
	echo "oo" >/dev/null )

tags:
	$(SILENT)(for d in $(DIRS); do \
	    etags -o $(OBJDIR)/TAGS -a $$d/*.[ch]; \
	done)
