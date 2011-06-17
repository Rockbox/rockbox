default: all

.PHONY: default all clean dep

ROOTDIR = $(shell readlink -e ../../..)
BUILDDIR = $(shell pwd)/build
APPSDIR = $(ROOTDIR)/apps
TOOLSDIR = $(ROOTDIR)/tools
DEPFILE = $(BUILDDIR)/make.dep
APP_TYPE = sdl-sim

INCLUDES = -I$(shell pwd)
INCLUDES += -I$(APPSDIR) -I$(APPSDIR)/codecs -I$(APPSDIR)/codecs/lib \
			-I$(APPSDIR)/gui -I$(APPSDIR)/metadata
INCLUDES += -I$(ROOTDIR)/firmware/export -I$(ROOTDIR)/firmware/include \
			-I$(ROOTDIR)/firmware/target/hosted \
			-I$(ROOTDIR)/firmware/target/hosted/sdl

CFLAGS = $(INCLUDES) -DROCKBOX -DSIMULATOR=1
CFLAGS += -O0 -ggdb -DDEBUG -DLOGF_ENABLE -Wall -Wno-pointer-sign
CFLAGS += -Wstrict-prototypes -pipe -std=gnu99
PPCFLAGS = $(CFLAGS)

SHARED_CFLAGS = -fPIC -fvisibility=hidden
SHARED_LDFLAG = -shared

WARBLE_OBJS = $(BUILDDIR)/warble.o
WARBLE_CFLAGS = '-DCODECDIR="$(CODECDIR)"' $(shell sdl-config --cflags)
WARBLE_LDFLAGS = -lm -ldl $(shell sdl-config --libs)

include $(ROOTDIR)/tools/functions.make
include $(APPSDIR)/codecs/codecs.make

SRC = $(ROOTDIR)/apps/metadata.c $(ROOTDIR)/apps/replaygain.c \
	  $(ROOTDIR)/firmware/buflib.c \
	  $(ROOTDIR)/firmware/core_alloc.c \
	  $(ROOTDIR)/firmware/common/strlcpy.c \
	  $(ROOTDIR)/firmware/common/unicode.c \
	  $(ROOTDIR)/firmware/common/structec.c $(ROOTDIR)/apps/mp3data.c \
	  $(ROOTDIR)/apps/fixedpoint.c $(ROOTDIR)/uisimulator/common/io.c
SRC += $(APPSDIR)/compressor.c $(APPSDIR)/dsp.c $(APPSDIR)/eq.c $(APPSDIR)/tdspeed.c
SRC += $(wildcard $(ROOTDIR)/apps/metadata/*.c)

OBJ := $(SRC:.c=.o)
OBJ := $(OBJ:.S=.o)
OBJ := $(subst $(ROOTDIR),$(BUILDDIR),$(OBJ))

all: warble $(CODECS)

dep $(DEPFILE):
	$(SILENT)mkdir -p $(dir $(DEPFILE))
	$(call PRINTS,Generating dependencies)
	@rm -f $(DEPFILE)_
	$(call mkdepfile,$(DEPFILE)_,$(SRC))
	$(call mkdepfile,$(DEPFILE)_,$(OTHER_SRC))
	$(call mkdepfile,$(DEPFILE)_,$(ASMDEFS_SRC))
	@mv $(DEPFILE)_ $(DEPFILE)

-include $(DEPFILE)

warble: $(WARBLE_OBJS) $(OBJ)
	$(call PRINTS,LD $@)$(CC) $(LDFLAGS) $^ -o $@ $(WARBLE_LDFLAGS)

$(BUILDDIR)/%.o: %.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $<)$(CC) $(CFLAGS) -c $< -o $@ $(WARBLE_CFLAGS)

$(BUILDDIR)/%.o: $(ROOTDIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $<)$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(SILENT)echo Cleaning build directory
	$(SILENT)rm -rf warble $(BUILDDIR)/*
