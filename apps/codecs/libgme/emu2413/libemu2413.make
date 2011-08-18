
# libemu
EMULIB := $(CODECDIR)/libemu2413.a
EMULIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libgme/emu2413/EMU2413SOURCES)
EMULIB_OBJ := $(call c2obj, $(EMULIB_SRC))
OTHER_SRC += $(EMULIB_SRC)

$(EMULIB): $(EMULIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

EMUFLAGS = $(filter-out -O%,$(CODECFLAGS)) -fno-strict-aliasing -DGME_EMU_TYPE
ifeq ($(CPU),arm)
   EMUFLAGS += -O3
else
   EMUFLAGS += -O2
endif

$(CODECDIR)/libgme/emu2413/%.o: $(ROOTDIR)/apps/codecs/libgme/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(EMUFLAGS) -c $< -o $@
