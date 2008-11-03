# -*- Makefile -*-

ifndef V
SILENT=@
endif
PRINTS=$(SILENT)$(call info,$(1))

$(OBJDIR)/%.o: %.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $<)$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $<)$(CC) $(CFLAGS) -c $< -o $@

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
# The trailing sed hack for a range of files is present because how gcc works
# for generating dependencies when the include file doesn't exist. Then it
# makes the dependency without any path, and that breaks how things work for
# us. We thus add the correct path for a few generated files!
#
$(DEPFILE): $(SOURCES)
	$(SILENT)mkdir -p $(dir $(DEPFILE))
	$(SILENT)rm -f $(DEPFILE)
	$(SILENT)(for each in $(SOURCES) x; do \
	    if test "$$each" = "credits.c"; then \
	        del="$$del$(BUILDDIR)/credits.raw"; \
	        touch $(BUILDDIR)/credits.raw; \
	    fi; \
	    if test "x" != "$$each"; then \
	        obj=`echo $$each | sed -e 's/\.[cS]/.o/'`; \
	        $(CC) -MG -MM -MT "$(OBJDIR)/$$obj" $(CFLAGS) $$each 2>/dev/null; \
	    fi; \
	    if test -n "$$del"; then \
	        rm $$del; \
	        del=""; \
	    fi \
	done | sed -e "s:[^[:space:]]*lang.h:$(OBJDIR)/lang.o:" \
	-e "s:[^[:space:]]*sysfont.h:$(BUILDDIR)/sysfont.h:" \
	-e "s:[^[:space:]]*max_language_size.h:$(BUILDDIR)/max_language_size.h:" \
	-e "s: bitmaps/: $(BUILDDIR)/bitmaps/:g" \
	-e "s: pluginbitmaps/: $(BUILDDIR)/pluginbitmaps/:g" \
	> $(DEPFILE); \
	echo "oo" > /dev/null )

tags:
	$(SILENT)(for d in $(DIRS); do \
	    etags -o $(OBJDIR)/TAGS -a $$d/*.[ch]; \
	done)
