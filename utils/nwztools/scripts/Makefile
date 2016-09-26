upgtool:="../upgtools/upgtool"
scsitool:="../scsitools/scsitool"

.SUFFIXES: # disable old suffix rules

all:
	@echo "Please select an action:"
	@echo "- dump_rootfs: dumps the root filesystem to rootfs.tgz"
	@echo "- do_fw_upgrade: put the device in NWZ_DEV in firmware upgrade mode"
	@echo "- copy_fw_upgrade: copy firmware to a device with the right name"
	@echo "- exec_file: craft an upgrade that executes a script/executable"
	@echo "- list_targets: produce of list of available targets"

ifndef UPG
want_upg:
	$(info Please set UPG to the upg filename. For example:)
	$(info make exec_file UPG=hello_world.upg ...)
	$(error "")
else
want_upg: ;
endif

ifndef EXEC
want_exec:
	$(info Please set EXEC to the executable filename. For example:)
	$(info make exec_file EXEC=hello_world.sh ...)
	$(error "")
else
want_exec: ;
endif

ifndef NWZ_TARGET
want_target:
	$(info Please set NWZ_TARGET to your target. For example:)
	$(info make dump_rootfs NWZ_TARGET=nwz-e463)
	$(info Run 'make list_targets' to get a list of all targets)
	$(error "")
else
want_target: ;
endif

ifndef NWZ_DEV
want_dev:
	$(info Please set NWZ_DEV to your dev. For example:)
	$(info make do_fw_upgrade NWZ_DEV=/dev/sdx)
else
want_dev: ;
endif

ifndef NWZ_MOUNT
want_mount:
	$(info Please set NWZ_MOUNT to your dev mount point. For example:)
	$(info make copy_fw_upgrade NWZ_MOUNT=/media/WALKMAN ...)
else
want_mount: ;
endif

UPGPACK=$(upgtool) -c -m $(NWZ_TARGET) $(UPG) $(1)

exec_file: want_target want_exec want_upg
	$(call UPGPACK, exec_file.sh $(EXEC))

dump_rootfs: want_target want_upg
	$(call UPGPACK, dump_rootfs.sh)

clean:
	rm -rf *.upg

list_targets:
	$(upgtool) -m ?; true # upgtool returns an error in this case, ignore it

copy_fw_upgrade: want_upg want_mount
	cp $(UPG) "$(NWZ_MOUNT)/NW_WM_FW.UPG"

do_fw_upgrade: want_dev
	@echo "Device: $(NWZ_DEV)"
	$(scsitool) $(NWZ_DEV) do_fw_upgrade
