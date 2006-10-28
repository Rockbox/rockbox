all: $(OUTPUT)

dep: $(DEPFILE)

ifndef V
SILENT=@
endif
PRINTS=$(SILENT)$(call info,$(1))

$(OBJDIR)/%.c: %.bmp
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)mkdir -p $(BMPINCDIR)
	$(call PRINTS,BMP2RB $<)$(BMP2RB) -h $(BMPINCDIR) $< > $@

$(OBJDIR)/%.o: $(OBJDIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(<F))$(CC) $(CFLAGS) -c $< -o $@

$(DEPFILE): SOURCES
	$(SILENT)mkdir -p $(dir $(DEPFILE))
	$(SILENT)rm -f $(DEPFILE)
	$(SILENT)(for each in $(SOURCES) x; do \
          if test "x" != "$$each"; then \
            src=`echo $$each | sed -e 's/\.bmp/.c/'`; \
            echo $(OBJDIR)/$$src: $$each >> $(DEPFILE) ; \
            obj=`echo $$each | sed -e 's/\.bmp/.o/'`; \
            echo $(OBJDIR)/$$obj: $(OBJDIR)/$$src >> $(DEPFILE) ; \
          fi; \
        done)

$(OUTPUT): $(OBJS)
	$(call PRINTS,AR+RANLIB $(@F))$(AR) ruv $@ $+ >/dev/null 2>&1
	$(SILENT)$(RANLIB) $@
