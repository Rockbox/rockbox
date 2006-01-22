all: $(OUTPUT)

dep: $(DEPFILE)

ifndef V
SILENT=@
endif

$(OBJDIR)/%.c: %.bmp
	$(SILENT)mkdir -p `dirname $@`
	@echo BMP2RB $<
	$(SILENT)$(BMP2RB) $< > $@

$(OBJDIR)/%.o: $(OBJDIR)/%.c
	$(SILENT)mkdir -p `dirname $@`
	@echo CC `basename $<`
	$(SILENT)$(CC) $(CFLAGS) -c $< -o $@

$(DEPFILE): SOURCES
	$(SILENT)mkdir -p `dirname $(DEPFILE)`
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
	@echo "AR+RANLIB $@"
	@$(AR) ruv $@ $+ >/dev/null 2>&1
	@$(RANLIB) $@
