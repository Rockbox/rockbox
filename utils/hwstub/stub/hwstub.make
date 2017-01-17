INCLUDES+=-I$(ROOT_DIR) -I$(ROOT_DIR)/../include/
LINKER_FILE=hwstub.lds
TMP_LDS=$(BUILD_DIR)/link.lds
TMP_MAP=$(BUILD_DIR)/hwstub.map
CFLAGS=$(GCCOPTS) $(DEFINES) -W -Wall -Wundef -O -nostdlib -ffreestanding -Wstrict-prototypes -pipe -std=gnu99 -fomit-frame-pointer -Wno-pointer-sign -Wno-override-init $(INCLUDES)
ASFLAGS=$(CFLAGS) -D__ASSEMBLER__
LDFLAGS=-lgcc -Os -nostdlib -T$(TMP_LDS) -Wl,-Map,$(TMP_MAP) $(INCLUDES) -L$(BUILD_DIR)

SRC:=$(shell cat $(ROOT_DIR)/SOURCES | $(CC) $(INCLUDES) \
    $(DEFINES) -E -P -include "config.h" - 2>/dev/null \
    | grep -v "^\#")
SRC:=$(foreach src,$(SRC),$(BUILD_DIR)/$(src))
OBJ=$(SRC:.c=.o)
OBJ:=$(OBJ:.S=.o)
OBJ_EXCEPT_CRT0=$(filter-out $(BUILD_DIR)/%/crt0.o,$(OBJ))
EXEC_ELF=$(BUILD_DIR)/hwstub.elf
EXEC_BIN=$(BUILD_DIR)/hwstub.bin
DEPS=$(foreach obj,$(OBJ),$(obj).d)

EXEC+=$(EXEC_ELF) $(EXEC_BIN)

ifndef V
SILENT=@
endif
PRINTS=$(SILENT)$(call info,$(1))

all: $(EXEC)

# pull in dependency info for *existing* .o files
-include $(DEPS)

$(BUILD_DIR)/%.o: $(ROOT_DIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(<F))
	$(SILENT)$(CC) $(CFLAGS) -c -o $@ $<
	$(SILENT)$(CC) -MM $(CFLAGS) $< -MT $@ > $@.d

$(BUILD_DIR)/%.o: $(ROOT_DIR)/%.S
	$(call PRINTS,AS $(<F))
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(AS) $(ASFLAGS) -c -o $@ $<

$(TMP_LDS): $(LINKER_FILE)
	$(call PRINTS,PP $(<F))
	$(SILENT)$(CC) $(CFLAGS) -E -x c - < $< | sed '/#/d' > $@

$(EXEC_ELF): $(OBJ) $(TMP_LDS)
	$(call PRINTS,LD $(@F))
	$(SILENT)$(LD) $(LDFLAGS) -o $@ $(OBJ_EXCEPT_CRT0)

$(EXEC_BIN): $(EXEC_ELF)
	$(call PRINTS,OC $(@F))
	$(SILENT)$(OC) -O binary $< $@

clean:
	$(SILENT)rm -rf $(OBJ) $(DEPS) $(EXEC) $(TMP_LDS) $(TMP_MAP)
