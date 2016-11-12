TOOLS_DIR=../../../../tools/
all: rockbox_icon.h tools_icon.h

%.h: %.bmp
	$(TOOLS_DIR)/bmp2rb -f 4 $^ > $@

clean:
	rm -rf *.h
