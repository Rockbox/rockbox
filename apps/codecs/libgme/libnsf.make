
# libnsf
NSFLIB := $(CODECDIR)/libnsf.a
NSFLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libgme/NSFSOURCES)
NSFLIB_OBJ := $(call c2obj, $(NSFLIB_SRC))
OTHER_SRC += $(NSFLIB_SRC)

$(NSFLIB): $(NSFLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
