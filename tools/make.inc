# -*- Makefile -*-

$(OBJDIR)/%.o: %.c
	@mkdir -p `dirname $@`
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.S
	@mkdir -p `dirname $@`
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# the lame echo stuff down here is to prevent any compiler errors/warnings
# to cause an error code to get returned and thus stop the build
$(DEPFILE): $(SOURCES)
	@rm -f $(DEPFILE)
	@(for each in $(SOURCES) x; do \
          if test "x" != "$$each"; then \
	    obj=`echo $$each | sed -e 's/\.[cS]/.o/'`; \
	    $(CC) -MM -MT "$(OBJDIR)/$$obj" $(CFLAGS) $$each >> $(DEPFILE) 2>/dev/null; \
          fi; \
	done; \
	echo "oo" >/dev/null )

tags:
	@(for d in $(DIRS); do \
	    etags -o $(OBJDIR)/TAGS -a $$d/*.[ch]; \
	done)
