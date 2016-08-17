upgtool:="../upgtools/upgtool"
scsitool:="../scsitools/scsitool"

.SUFFIXES: # disable old suffix rules

all:
	@echo "Please select an action:"
	@echo "- update: uses script update.sh"
	@echo "- dump_rootfs: dumps the root filesystem to rootfs.tgz"
	@echo "- my_update: craft an arbitrary upgrade script found in my_update.sh"
	@echo "- do_fw_upgrade: send a firmware upgrade to the device in NWZ_DEV"
	@echo "- list_targets: produce of list of available targets"

my_update: my_update.upg
dump_rootfs: dump_rootfs.upg

%.upg: %.sh
ifndef NWZ_TARGET
	@echo "Please set NWZ_TARGET to your target. For example:"
	@echo "make $@ NWZ_TARGET=nwz-e463"
	@echo "Run 'make list_targets' to get a list of all targets"
else
	@echo "Target: $(NWZ_TARGET)"
	$(upgtool) -c -m $(NWZ_TARGET) $@ $^
endif

clean:
	rm -rf *.UPG

list_targets:
	$(upgtool) -m ?; true # upgtool returns an error in this case, ignore it

do_fw_upgrade:
ifdef NWZ_DEV
	@echo "Device: $(NWZ_DEV)"
	$(scsitool) $(NWZ_DEV) do_fw_upgrade
else
	@echo "Please set NWZ_DEV to your dev. For example:"
	@echo "make do_fw_upgrade NWZ_DEV=/dev/sdx"
endif
