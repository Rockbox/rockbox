SILENT = @
INCLUDE = -I../../firmware/export \
	-I../../apps -I../../uisimulator/sdl -I/usr/include/SDL
FIRMINC = -I../../firmware/include -fno-builtin
DEFINES = -D__PCTOOL__ -DHAVE_TAGCACHE -DSIMULATOR -DCONFIG_CODEC=1 \
	-DROCKBOX_LITTLE_ENDIAN -DROCKBOX_DIR=\".rockbox\" -DROCKBOX_HAS_LOGF \
	-DCONFIG_CODEC=1
CFLAGS = -g $(INCLUDE) $(DEFINES) -Wno-pointer-sign 

SRC = database.o tagcache.o replaygain.o \
	metadata.o metadata_common.o mp3data.o \
	a52.o mp3.o adx.o mp4.o aiff.o mpc.o ape.o ogg.o \
	asap.o sid.o asf.o spc.o flac.o vorbis.o wave.o \
	mod.o wavpack.o monkeys.o \
	logf.o unicode.o ctype.o structec.o crc32.o io.o

OBJ = $(SRC:.c=.o)

# source code search path
VPATH = ../../apps ../../apps/metadata ../../firmware/common ../../firmware/ \
	../../uisimulator/common

all: database

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
	rm $(OBJ)
