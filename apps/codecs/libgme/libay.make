
# libay
AYLIB := $(CODECDIR)/libay.a
AYLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libgme/AYSOURCES)
AYLIB_OBJ := $(call c2obj, $(AYLIB_SRC))
OTHER_SRC += $(AYLIB_SRC)

$(AYLIB): $(AYLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

AYFLAGS = $(filter-out -O%,$(CODECFLAGS)) -fno-strict-aliasing -DGME_AY_TYPE
ifeq ($(CPU),arm)
   AYFLAGS += -O3
else
   AYFLAGS += -O2
endif

$(CODECDIR)/libgme/%.o: $(ROOTDIR)/apps/codecs/libgme/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(AYFLAGS) -c $< -o $@
