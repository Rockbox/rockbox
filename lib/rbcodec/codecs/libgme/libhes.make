
# libhes
HESLIB := $(CODECDIR)/libhes.a
HESLIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libgme/HESSOURCES)
HESLIB_OBJ := $(call c2obj, $(HESLIB_SRC))
OTHER_SRC += $(HESLIB_SRC)

$(HESLIB): $(HESLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
