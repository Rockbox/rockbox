
# libgbs
GBSLIB := $(CODECDIR)/libgbs.a
GBSLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libgme/GBSSOURCES)
GBSLIB_OBJ := $(call c2obj, $(GBSLIB_SRC))
OTHER_SRC += $(GBSLIB_SRC)

$(GBSLIB): $(GBSLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

GBSFLAGS = $(filter-out -O%,$(CODECFLAGS)) -fno-strict-aliasing -DGME_GBS_TYPE
ifeq ($(CPU),arm)
   GBSFLAGS += -O3
else
   GBSFLAGS += -O2
endif

$(CODECDIR)/libgme/%.o: $(ROOTDIR)/apps/codecs/libgme/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(GBSFLAGS) -c $< -o $@
