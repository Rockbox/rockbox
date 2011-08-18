
# libsgc
SGCLIB := $(CODECDIR)/libsgc.a
SGCLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libgme/sgc/SGCSOURCES)
SGCLIB_OBJ := $(call c2obj, $(SGCLIB_SRC))
OTHER_SRC += $(SGCLIB_SRC)

$(SGCLIB): $(SGCLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

SGCFLAGS = $(filter-out -O%,$(CODECFLAGS)) -fno-strict-aliasing -DGME_SGC_TYPE
ifeq ($(CPU),arm)
   SGCFLAGS += -O1
else
   SGCFLAGS += -O2
endif

$(CODECDIR)/libgme/sgc/%.o: $(ROOTDIR)/apps/codecs/libgme/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(SGCFLAGS) -c $< -o $@
