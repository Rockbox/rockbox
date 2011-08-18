
# libnsf
NSFLIB := $(CODECDIR)/libnsf.a
NSFLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libgme/NSFSOURCES)
NSFLIB_OBJ := $(call c2obj, $(NSFLIB_SRC))
OTHER_SRC += $(NSFLIB_SRC)

$(NSFLIB): $(NSFLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

NSFFLAGS = $(filter-out -O%,$(CODECFLAGS)) -fno-strict-aliasing -DGME_NSF_TYPE
ifeq ($(CPU),arm)
   NSFFLAGS += -O3
else
   NSFFLAGS += -O2
endif

$(CODECDIR)/libgme/%.o: $(ROOTDIR)/apps/codecs/libgme/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(NSFFLAGS) -c $< -o $@
