
# libay
AYLIB := $(CODECDIR)/libay.a
AYLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libgme/AYSOURCES)
AYLIB_OBJ := $(call c2obj, $(AYLIB_SRC))
OTHER_SRC += $(AYLIB_SRC)

$(AYLIB): $(AYLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
