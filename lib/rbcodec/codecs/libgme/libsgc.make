
# libsgc
SGCLIB := $(CODECDIR)/libsgc.a
SGCLIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libgme/SGCSOURCES)
SGCLIB_OBJ := $(call c2obj, $(SGCLIB_SRC))
OTHER_SRC += $(SGCLIB_SRC)

$(SGCLIB): $(SGCLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
