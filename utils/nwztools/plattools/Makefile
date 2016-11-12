PREFIX?=arm-sony-linux-gnueabi-
CC=$(PREFIX)gcc
LD=$(PREFIX)gcc
CFLAGS=-std=gnu99 -Wall -O2
NWZ_DB_DIR=../database
INCLUDES=-I. -I$(NWZ_DB_DIR)

LIB_FILES=nwz_lib.c $(NWZ_DB_DIR)/nwz_db.c
TOOL_FILES=dest_tool.c test_adc.c test_adc.c test_bl.c test_display.c \
	test_keys.c test_power.c test_ts.c test_fb.c
ALL_ELF=$(patsubst %.c,%.elf,$(TOOL_FILES)) all_tools.elf dualboot.elf

all: $(ALL_ELF)

# image dependency
data/rockbox_icon.h data/tools_icon.h:
	make -C data

dualboot.elf: data/rockbox_icon.h data/tools_icon.h

%.elf: %.c $(LIB_FILES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

all_tools.elf: all_tools.c $(TOOL_FILES) $(LIB_FILES)
	$(CC) $(CFLAGS) -DNWZ_EMBED_TOOLS $(INCLUDES) -o $@ $^

dualboot.elf: dualboot.c all_tools.c $(TOOL_FILES) $(LIB_FILES)
	$(CC) $(CFLAGS) -DNWZ_DUALBOOT -DNWZ_EMBED_TOOLS $(INCLUDES) -o $@ $^

clean:
	rm -rf $(ALL_ELF)
