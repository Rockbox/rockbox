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

# the lame echo stuff down here is to prevent any compiler errors/warnings
# to cause an error code to get returned and thus stop the build
$(DEPFILE): $(SOURCES)
	$(SILENT)rm -f $(DEPFILE)
	$(SILENT)(for each in $(SOURCES) x; do \
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
