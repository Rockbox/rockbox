GDB         := gdb
OPENOCD     := openocd
OPENOCD_CFG := $(ROOTDIR)/tools/echoplayer/openocd.cfg

# GDB boot protocol 'registers' used for communicating with the bootloader
GDBBOOT_REG_BASE    := 0x38000000
GDBBOOT_REG_MAGIC1  := ($(GDBBOOT_REG_BASE) + 0x00)
GDBBOOT_REG_MAGIC2  := ($(GDBBOOT_REG_BASE) + 0x04)
GDBBOOT_REG_MAGIC3  := ($(GDBBOOT_REG_BASE) + 0x08)
GDBBOOT_REG_MAGIC4  := ($(GDBBOOT_REG_BASE) + 0x0c)
GDBBOOT_REG_ELFADDR := ($(GDBBOOT_REG_BASE) + 0x10)
GDBBOOT_REG_ELFSIZE := ($(GDBBOOT_REG_BASE) + 0x14)

# Address where the RB binary will be loaded
GDBBOOT_LOAD_ADDR := 0x71e00000
GDBBOOT_LOAD_SIZE := 0x200000

ifneq (,$(findstring bootloader,$(APPSDIR)))
  TARGET_ELF := $(BUILDDIR)/bootloader.elf
else
  TARGET_ELF := $(BUILDDIR)/rockbox.elf
  TARGET_BIN := $(BUILDDIR)/rockbox.echo
endif

# Attach to running process
debug:
	$(GDB) $(TARGET_ELF) \
		-ex "target extended-remote | openocd -c \"gdb_port pipe\" -f $(OPENOCD_CFG)"
.PHONY: debug

ifneq (,$(findstring bootloader,$(APPSDIR)))

# Flash bootloader
flash: $(TARGET_ELF)
	$(OPENOCD) -f $(OPENOCD_CFG) -c "program $< verify reset exit"
.PHONY: flash

else

# Start Rockbox via debugger
start: $(TARGET_BIN)
	$(GDB) $(TARGET_ELF) \
		-ex "target extended-remote | openocd -c \"gdb_port pipe\" -f $(OPENOCD_CFG)" \
		-ex "monitor reset halt" \
		-ex "set *$(GDBBOOT_REG_MAGIC1) = 0x726f636b" \
		-ex "set *$(GDBBOOT_REG_MAGIC2) = 0x424f4f54" \
		-ex "set *$(GDBBOOT_REG_MAGIC3) = 0x6764626c" \
		-ex "set *$(GDBBOOT_REG_MAGIC4) = 0x6f616455" \
		-ex "continue" \
		-ex "restore $(TARGET_BIN) binary $(GDBBOOT_LOAD_ADDR)" \
		-ex "set *$(GDBBOOT_REG_ELFADDR) = $(GDBBOOT_LOAD_ADDR)" \
		-ex "set *$(GDBBOOT_REG_ELFSIZE) = $(GDBBOOT_LOAD_SIZE)" \
		-ex "continue"
.PHONY: start

endif

