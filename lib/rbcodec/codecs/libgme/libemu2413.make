
# libemu2413
EMU2413LIB := $(CODECDIR)/libemu2413.a
EMU2413LIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libgme/EMU2413SOURCES)
EMU2413LIB_OBJ := $(call c2obj, $(EMU2413LIB_SRC))
OTHER_SRC += $(EMU2413LIB_SRC)

$(EMU2413LIB): $(EMU2413LIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
