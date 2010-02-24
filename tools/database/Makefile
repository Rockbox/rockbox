ifndef V
SILENT = @
endif
INCLUDE = -I../../firmware/export \
	-I../../apps -I../../apps/gui -I../../uisimulator/sdl -I/usr/include/SDL
FIRMINC = -I../../firmware/include -fno-builtin
DEFINES = -D__PCTOOL__ -DHAVE_TAGCACHE -DSIMULATOR -DCONFIG_CODEC=1 \
	-DROCKBOX_LITTLE_ENDIAN -DROCKBOX_DIR=\".rockbox\" -DROCKBOX_HAS_LOGF \
	-DCONFIG_CODEC=1
CFLAGS = -g $(INCLUDE) $(DEFINES) -Wno-pointer-sign 

SRC = database.c tagcache.c replaygain.c \
	metadata.c metadata_common.c mp3data.c \
	a52.c mp3.c adx.c mp4.c aiff.c mpc.c ape.c ogg.c \
	asap.c sid.c asf.c spc.c flac.c vorbis.c wave.c \
	mod.c wavpack.c monkeys.c rm.c nsf.c smaf.c \
	logf.c unicode.c ctype.c structec.c crc32.c io.c \
	misc.c strlcpy.c fixedpoint.c

OBJ = $(SRC:.c=.o)

# source code search path
VPATH = ../../apps ../../apps/metadata ../../firmware/common ../../firmware/ \
	../../uisimulator/common

TARGET= database

all: $(TARGET)

%.o : ../../uisimulator/common/%.c
	@echo CC $(<F)
	$(SILENT)$(CC) $(CFLAGS) -c -o $@ $<

%.o : %.c $<
	@echo CC $(<F)
	$(SILENT)$(CC) $(FIRMINC) $(CFLAGS) -c -o $@ $<

database: $(OBJ)
	@echo LD $@
	$(SILENT)$(CC) -g -ldl -o $@ $+

clean:
	rm -f $(OBJ) $(TARGET)
