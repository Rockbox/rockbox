
# libvgm
VGMLIB := $(CODECDIR)/libvgm.a
VGMLIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libgme/VGMSOURCES)
VGMLIB_OBJ := $(call c2obj, $(VGMLIB_SRC))
OTHER_SRC += $(VGMLIB_SRC)

$(VGMLIB): $(VGMLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
