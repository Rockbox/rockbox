PANDORA_DIR=$(ROOTDIR)/packaging/pandora
PND_MAKE=/usr/local/angstrom/arm/scripts/pnd_make
PND_BUILD_DIR=pnddir

pnddir:
	mkdir $(PND_BUILD_DIR)

pnd: pnddir $(PND_MAKE) $(DEPFILE) build
	# Creating PND file
	make PREFIX=$(PND_BUILD_DIR)/rockbox fullinstall

	# Install Pandora build files
	cp $(PANDORA_DIR)/PXML.xml $(PND_BUILD_DIR)
	cp $(PANDORA_DIR)/rockbox.png $(PND_BUILD_DIR)
	cp $(PANDORA_DIR)/rockbox_preview.jpg $(PND_BUILD_DIR)
	cp $(PANDORA_DIR)/run_rockbox.sh $(PND_BUILD_DIR)

	# Remove stuff that's broken because of missing keymapping.
	# Otherwise the user will have a hard time to shut down rockbox
	rm -f $(PND_BUILD_DIR)/rockbox/lib/rockbox/rocks/apps/*
	rm -f $(PND_BUILD_DIR)/rockbox/lib/rockbox/rocks/demos/*
	rm -f $(PND_BUILD_DIR)/rockbox/lib/rockbox/rocks/games/*

	# Add docs folder
	cp -rf $(ROOTDIR)/docs $(PND_BUILD_DIR)

	# Fix up permissions
	chmod -R a+r $(PND_BUILD_DIR)

	# Invoke pndmake
	$(PND_MAKE) -p rockbox.pnd -d $(PND_BUILD_DIR) -x $(PND_BUILD_DIR)/PXML.xml -i $(PND_BUILD_DIR)/rockbox.png -c
