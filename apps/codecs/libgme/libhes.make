
# libhes
HESLIB := $(CODECDIR)/libhes.a
HESLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libgme/HESSOURCES)
HESLIB_OBJ := $(call c2obj, $(HESLIB_SRC))
OTHER_SRC += $(HESLIB_SRC)

$(HESLIB): $(HESLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

HESFLAGS = $(filter-out -O%,$(CODECFLAGS)) -fno-strict-aliasing -DGME_HES_TYPE
ifeq ($(CPU),arm)
   HESFLAGS += -O3
else
   HESFLAGS += -O2
endif

$(CODECDIR)/libgme/%.o: $(ROOTDIR)/apps/codecs/libgme/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(HESFLAGS) -c $< -o $@
