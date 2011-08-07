
# libkss
KSSLIB := $(CODECDIR)/libkss.a
KSSLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libgme/KSSSOURCES)
KSSLIB_OBJ := $(call c2obj, $(KSSLIB_SRC))
OTHER_SRC += $(KSSLIB_SRC)

$(KSSLIB): $(KSSLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

KSSFLAGS = $(filter-out -O%,$(CODECFLAGS)) -fno-strict-aliasing -DGME_KSS_TYPE
ifeq ($(CPU),arm)
   KSSFLAGS += -O3
else
   KSSFLAGS += -O2
endif

$(CODECDIR)/libgme/%.o: $(ROOTDIR)/apps/codecs/libgme/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(KSSFLAGS) -c $< -o $@
