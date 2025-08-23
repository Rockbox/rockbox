RG_NANO_DIR=$(ROOTDIR)/packaging/rgnano
MKSQUASHFS=$(FUNKEY_SDK_PATH)/bin/mksquashfs
INSTALL_DIR=$(OPK_BUILD_DIR)/install
OPK_BUILD_DIR=opkdir

opkdir:
	mkdir $(OPK_BUILD_DIR)

opkclean:
	rm -rf $(OPK_BUILD_DIR)

opk: opkclean opkdir $(MKSQUASHFS) build
	make PREFIX=$(OPK_BUILD_DIR)/rockbox install

	# Install opk files
	cp $(RG_NANO_DIR)/icon.png $(OPK_BUILD_DIR)
	cp $(RG_NANO_DIR)/mapping.key $(OPK_BUILD_DIR)
	cp $(RG_NANO_DIR)/rockbox.funkey-s.desktop $(OPK_BUILD_DIR)
	cp $(RG_NANO_DIR)/config.cfg $(OPK_BUILD_DIR)
	cp $(RG_NANO_DIR)/run.sh $(OPK_BUILD_DIR)

	# Organize files
	mkdir $(INSTALL_DIR)
	mv $(OPK_BUILD_DIR)/rockbox/bin/rockbox $(INSTALL_DIR)
	mv $(OPK_BUILD_DIR)/rockbox/lib/rockbox/* $(INSTALL_DIR)
	mv $(OPK_BUILD_DIR)/rockbox/share/rockbox/* $(INSTALL_DIR)

	rm -rf $(OPK_BUILD_DIR)/rockbox
	mv $(INSTALL_DIR)/rockbox $(OPK_BUILD_DIR)

	# Plugin workarounds
	mkdir $(INSTALL_DIR)/rocks.data
	mv $(INSTALL_DIR)/rocks/games/.picross $(INSTALL_DIR)/rocks.data/.picross

	# Permissions
	chmod +x $(OPK_BUILD_DIR)/rockbox
	chmod +x $(OPK_BUILD_DIR)/run.sh

	# Make opk
	$(MKSQUASHFS) $(OPK_BUILD_DIR) rockbox_funkey-s.opk -all-root -noappend -no-exports -no-xattrs

opk-zip: opk
	zip -9 -q rockbox-opk.zip rockbox_funkey-s.opk rockbox-info.txt
