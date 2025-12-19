GDB         := gdb
OPENOCD     := openocd
OPENOCD_CFG := $(ROOTDIR)/tools/echoplayer/openocd.cfg

ifneq (,$(findstring bootloader,$(APPSDIR)))
  TARGET_ELF := $(BUILDDIR)/bootloader.elf
else
  TARGET_ELF := $(BUILDDIR)/rockbox.elf
endif

debug:
	$(GDB) $(TARGET_ELF) -ex "target extended-remote | openocd -c \"gdb_port pipe\" -f $(OPENOCD_CFG)"

flash: $(TARGET_ELF)
	$(OPENOCD) -f $(OPENOCD_CFG) -c "program $(TARGET_ELF) verify reset exit"

.PHONY: debug flash
