$(OBJDIR)/%.o: %.c
	@mkdir -p `dirname $@`
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.S
	@mkdir -p `dirname $@`
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(DEPFILE): $(SOURCES)
	@rm -f $(DEPFILE)
	@(for each in $(SOURCES); do \
	  obj=`echo $$each | sed -e 's/\.c/.o/'`; \
	  $(CC) -MM -MT "$(OBJDIR)/$$obj.o" $(CFLAGS) $$each >> $(DEPFILE) 2>/dev/null; \
	done )

tags:
	@(for d in $(DIRS); do \
	    etags -o $(OBJDIR)/TAGS -a $$d/*.[ch]; \
	done)
