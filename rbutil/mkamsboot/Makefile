
#change for releases
ifndef APPVERSION
APPVERSION=`../../tools/version.sh ../../`
endif

# We use the UCL code available in the Rockbox tools/ directory
CFLAGS=-I../../tools/ucl/include -Wall -DVERSION=\"$(APPVERSION)\"

ifndef V
SILENT = @
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
OUTPUT=mkamsboot.exe
CFLAGS+=-mno-cygwin
else
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
OUTPUT=mkamsboot.exe
else
ifeq ($(findstring mingw,$(CC)),mingw)
OUTPUT=mkamsboot.exe
else
OUTPUT=mkamsboot
endif
endif
endif

ifdef RBARCH
CFLAGS += -arch $(RBARCH)
OBJDIR = $(RBARCH)/
endif


all: $(OUTPUT)

# additional link dependencies for the standalone executable
LIBUCL=../../tools/ucl/src/libucl$(RBARCH).a

$(LIBUCL):
	make -C ../../tools/ucl/src $(TARGET_DIR)libucl$(RBARCH).a

# inputs
LIBSOURCES := dualboot.c md5.c mkamsboot.c
SOURCES := $(LIBSOURCES) main.c
OBJS := $(patsubst %.c,%.o,$(addprefix $(OBJDIR),$(SOURCES)))
LIBOBJS := $(patsubst %.c,%.o,$(addprefix $(OBJDIR),$(LIBSOURCES)))
EXTRADEPS := $(LIBUCL)

$(OBJDIR)%.o: %.c
	@echo CC $< $
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CC) $(CFLAGS) -c -o $@ $<

libmkamsboot$(RBARCH).a: $(LIBOBJS)
	@echo AR $@
	$(SILENT)$(AR) ruc $(TARGET_DIR)$@ $^

# building the standalone executable
$(OUTPUT): $(OBJS) $(EXTRADEPS)
	@echo LD $@
	$(SILENT)$(CC) $(CFLAGS) -o $(OUTPUT) $(OBJS) $(EXTRADEPS)

# some trickery to build ppc and i386 from a single call
ifeq ($(RBARCH),)
$(TARGET_DIR)libmkamsbooti386.a:
	make RBARCH=i386 TARGET_DIR=$(TARGET_DIR) libmkamsbooti386.a

$(TARGET_DIR)libmkamsbootppc.a:
	make RBARCH=ppc TARGET_DIR=$(TARGET_DIR) libmkamsbootppc.a
endif    

libmkamsboot-universal: $(TARGET_DIR)libmkamsbooti386.a $(TARGET_DIR)libmkamsbootppc.a
	@echo lipo $(TARGET_DIR)libmkamsboot.a
	$(SILENT) rm -f $(TARGET_DIR)libmkamsboot.a
	$(SILENT)lipo -create $(TARGET_DIR)libmkamsbootppc.a $(TARGET_DIR)libmkamsbooti386.a -output $(TARGET_DIR)libmkamsboot.a

clean:
	rm -f $(OBJS) $(OUTPUT) libmkamsboot.o $(TARGET_DIR)libmkamsboot*.a mkamsboot.dmg
	rm -rf mkamsboot-* i386 ppc $(OBJDIR)

mkamsboot-i386:
	$(MAKE) RBARCH=i386
	mv mkamsboot mkamsboot-i386

mkamsboot-ppc:
	$(MAKE) RBARCH=ppc
	mv mkamsboot mkamsboot-ppc

mkamsboot-mac: mkamsboot-i386 mkamsboot-ppc
	$(SILENT)lipo -create mkamsboot-ppc mkamsboot-i386 -output mkamsboot-mac

mkamsboot.dmg: mkamsboot-mac
	mkdir -p mkamsboot-dmg
	cp -p mkamsboot-mac mkamsboot-dmg
	hdiutil create -srcfolder mkamsboot-dmg mkamsboot.dmg
