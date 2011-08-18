
# libgbs
GBSLIB := $(CODECDIR)/libgbs.a
GBSLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libgme/GBSSOURCES)
GBSLIB_OBJ := $(call c2obj, $(GBSLIB_SRC))
OTHER_SRC += $(GBSLIB_SRC)

$(GBSLIB): $(GBSLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
