.PHONY: all clean dirs bmp2rb
DEFINES += -DBSDIFF_EXECUTABLE
CFLAGS += -O1 -g -std=c99 -W -Wall $(DEFINES)
PROGS = bmp2rb fwcrypt fwdecrypt bsdiff/bsdiff bin2c
CROSS_COMPILE = arm-ypr0-linux-gnueabi-
R1_SAFEMODE_DIR = files/r1/etc/safemode
R0_SAFEMODE_DIR = files/r0/etc/safemode
DIRS = $(R1_SAFEMODE_DIR) $(R0_SAFEMODE_DIR)
R1_IMAGES = $(subst .bmp,.raw,$(addprefix $(R1_SAFEMODE_DIR)/,$(notdir $(wildcard files/images/r1/*.bmp))))
R0_IMAGES = $(subst .bmp,.raw,$(addprefix $(R0_SAFEMODE_DIR)/,$(notdir $(wildcard files/images/r0/*.bmp))))
BMP2RB = ../../tools/bmp2rb

all: $(R1_SAFEMODE_DIR)/cable_detect $(PROGS) $(R1_IMAGES) $(R0_IMAGES)
	$(MAKE) -C cramfs-1.1

bin2c: bin2c.o

fwdecrypt: fwdecrypt.o common.o ../../tools/fwpatcher/md5.o

fwcrypt: fwcrypt.o common.o ../../tools/fwpatcher/md5.o

bsdiff/bsdiff: bsdiff/bsdiff.o

$(BMP2RB):
	$(MAKE) -C ../../tools/ bmp2rb

$(R1_SAFEMODE_DIR)/cable_detect: cable_detect.c $(R1_SAFEMODE_DIR)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $< -o $@

$(R1_SAFEMODE_DIR)/%.raw: files/images/r1/%.bmp $(BMP2RB) $(R1_SAFEMODE_DIR)
	$(BMP2RB) -r -f 9 $< > $@

$(R0_SAFEMODE_DIR)/%.raw: files/images/r0/%.bmp $(BMP2RB) $(R0_SAFEMODE_DIR)
	$(BMP2RB) -r -f 9 $< > $@	

$(DIRS):
	$(SILENT)mkdir -p $@

dirs: $(DIRS)

bmp2rb: $(BMP2RB)

clean:
	$(MAKE) -C cramfs-1.1 clean
	rm -f *.o fwdecrypt fwcrypt fwpatch
	rm -f bsdiff/*.o bsdiff/bsdiff bsdiff/bspatch
	rm -rf $(R1_SAFEMODE_DIR)/../../etc $(R0_SAFEMODE_DIR)/../../etc
