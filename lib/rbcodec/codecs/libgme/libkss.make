
# libkss
KSSLIB := $(CODECDIR)/libkss.a
KSSLIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libgme/KSSSOURCES)
KSSLIB_OBJ := $(call c2obj, $(KSSLIB_SRC))
OTHER_SRC += $(KSSLIB_SRC)

$(KSSLIB): $(KSSLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
