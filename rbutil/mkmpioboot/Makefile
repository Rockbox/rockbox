
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
OUTPUT=mkmpioboot.exe
CFLAGS+=-mno-cygwin
else
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
OUTPUT=mkmpioboot.exe
else
ifeq ($(findstring mingw,$(CC)),mingw)
OUTPUT=mkmpioboot.exe
else
OUTPUT=mkmpioboot
endif
endif
endif

ifdef RBARCH
CFLAGS += -arch $(RBARCH)
OBJDIR = $(RBARCH)/
endif


all: $(OUTPUT)

# inputs
LIBSOURCES := mkmpioboot.c
SOURCES := $(LIBSOURCES) main.c
OBJS := $(patsubst %.c,%.o,$(addprefix $(OBJDIR),$(SOURCES)))
LIBOBJS := $(patsubst %.c,%.o,$(addprefix $(OBJDIR),$(LIBSOURCES)))
EXTRADEPS := $(LIBUCL)

# explicit dependencies
$(OBJDIR)mkmpioboot.o: mkmpioboot.c mkmpioboot.h
$(OBJDIR)main.o: main.c

$(OBJDIR)%.o: %.c
	@echo CC $< $
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CC) $(CFLAGS) -c -o $@ $<

libmkmpioboot$(RBARCH).a: $(LIBOBJS)
	@echo AR $@
	$(SILENT)$(AR) ruc $(TARGET_DIR)$@ $^

# building the standalone executable
$(OUTPUT): $(OBJS) $(EXTRADEPS)
	@echo LD $@
	$(SILENT)$(CC) $(CFLAGS) -o $(OUTPUT) $(OBJS) $(EXTRADEPS)

# some trickery to build ppc and i386 from a single call
ifeq ($(RBARCH),)
$(TARGET_DIR)libmkmpiobooti386.a:
	make RBARCH=i386 TARGET_DIR=$(TARGET_DIR) libmkmpiobooti386.a

$(TARGET_DIR)libmkmpiobootppc.a:
	make RBARCH=ppc TARGET_DIR=$(TARGET_DIR) libmkmpiobootppc.a
endif    

libmkmpioboot-universal: $(TARGET_DIR)libmkmpiobooti386.a $(TARGET_DIR)libmkmpiobootppc.a
	@echo lipo $(TARGET_DIR)libmkmpioboot.a
	$(SILENT) rm -f $(TARGET_DIR)libmkmpioboot.a
	$(SILENT)lipo -create $(TARGET_DIR)libmkmpiobootppc.a $(TARGET_DIR)libmkmpiobooti386.a -output $(TARGET_DIR)libmkmpioboot.a

clean:
	rm -f $(OBJS) $(OUTPUT) libmkmpioboot.o $(TARGET_DIR)libmkmpioboot*.a mkmpioboot.dmg
	rm -rf mkmpioboot-* i386 ppc $(OBJDIR)

mkmpioboot-i386:
	$(MAKE) RBARCH=i386
	mv mkmpioboot mkmpioboot-i386

mkmpioboot-ppc:
	$(MAKE) RBARCH=ppc
	mv mkmpioboot mkmpioboot-ppc

mkmpioboot-mac: mkmpioboot-i386 mkmpioboot-ppc
	$(SILENT)lipo -create mkmpioboot-ppc mkmpioboot-i386 -output mkmpioboot-mac

mkmpioboot.dmg: mkmpioboot-mac
	mkdir -p mkmpioboot-dmg
	cp -p mkmpioboot-mac mkmpioboot-dmg
	hdiutil create -srcfolder mkmpioboot-dmg mkmpioboot.dmg
