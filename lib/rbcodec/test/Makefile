default: all

.PHONY: default all clean dep

ROOTDIR = $(shell readlink -e ..)
BUILDDIR = $(shell pwd)/build
RBCODEC_DIR = $(ROOTDIR)
RBCODEC_BLD = $(BUILDDIR)
DEPFILE = $(BUILDDIR)/make.dep
APP_TYPE = sdl-sim
SOFTWARECODECS = yes

INCLUDES = -I$(shell pwd)
INCLUDES += -I$(ROOTDIR)/../../apps -I$(ROOTDIR)/../../apps/gui
INCLUDES += -I$(ROOTDIR)/../../firmware/export -I$(ROOTDIR)/../../firmware/include \
			-I$(ROOTDIR)/../../firmware/target/hosted \
			-I$(ROOTDIR)/../../firmware/target/hosted/sdl

CFLAGS = $(INCLUDES)
CFLAGS += -O0 -ggdb -DDEBUG -DSIMULATOR -DLOGF_ENABLE -Wall -Wno-pointer-sign
CFLAGS += -Wstrict-prototypes -pipe -std=gnu99
PPCFLAGS = $(CFLAGS)

SHARED_CFLAGS = -fPIC -fvisibility=hidden
SHARED_LDFLAG = -shared

WARBLE_OBJS = $(BUILDDIR)/test/warble.o
OTHER_SRC += $(RBCODEC_DIR)/test/warble.c
WARBLE_CFLAGS = '-DCODECDIR="$(CODECDIR)"' $(shell sdl-config --cflags)
WARBLE_LDFLAGS = -lm -ldl $(shell sdl-config --libs)

ifndef V
SILENT:=@
else
VERBOSEOPT:=-v
endif
PRINTS=$(SILENT)$(call info,$(1))
# old 'make' versions don't have the built-in 'info' function
info=old$(shell echo >&2 "Consider upgrading to GNU make 3.81+ for optimum build performance.")
ifeq ($(call info),old)
export info=echo "$$(1)";
endif

mkdepfile = $(SILENT)$(CC) $(PPCFLAGS) $(OTHER_INC) -MG -MM -include rbcodecconfig.h $(2) | \
			sed -e 's:^\S.*:$(RBCODEC_BLD)/&:' >> $(1)

RBCODEC_CFLAGS += -D_FILE_H_ -DLOGF_H -DDEBUG_H -D_KERNEL_H_ # will be removed later
WARBLE_CFLAGS += $(RBCODEC_CFLAGS)
include ../rbcodec.make

all: warble $(CODECS)

dep $(DEPFILE):
	$(SILENT)mkdir -p $(dir $(DEPFILE))
	$(call PRINTS,Generating dependencies)
	@rm -f $(DEPFILE)_
	$(call mkdepfile,$(DEPFILE)_,$(OTHER_SRC))
	@mv $(DEPFILE)_ $(DEPFILE)

-include $(DEPFILE)

warble: $(WARBLE_OBJS) $(RBCODEC_LIB)
	$(call PRINTS,LD $@)$(CC) $(LDFLAGS) $^ -o $@ $(WARBLE_LDFLAGS)

$(BUILDDIR)/%.o: $(ROOTDIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $<)$(CC) $(CFLAGS) $(WARBLE_CFLAGS) -c $< -o $@

clean:
	$(SILENT)echo Cleaning build directory
	$(SILENT)rm -rf warble $(BUILDDIR)/*
