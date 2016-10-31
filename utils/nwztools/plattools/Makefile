PREFIX?=arm-sony-linux-gnueabi-
CC=$(PREFIX)gcc
LD=$(PREFIX)gcc
CFLAGS=-std=gnu99 -Wall -O2
INCLUDES=-I.

LIB_FILES=nwz_lib.c nwz_lib_devlist.c
TOOL_FILES=dest_tool.c test_adc.c test_adc.c test_bl.c test_display.c \
	test_keys.c test_power.c test_ts.c
ALL_ELF=$(patsubst %.c,%.elf,$(TOOL_FILES)) all_tools.elf

all: $(ALL_ELF)

%.elf: %.c $(LIB_FILES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

all_tools.elf: all_tools.c $(TOOL_FILES) $(LIB_FILES)
	$(CC) $(CFLAGS) -DNWZ_EMBED_TOOLS $(INCLUDES) -o $@ $^

clean:
	rm -rf $(ALL_ELF)
