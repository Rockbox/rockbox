PREFIX = /usr/
EXT = pd_linux

# pd specific

VPATH = ../obj:./
OBJ_DIR = ../obj
BIN_DIR = ../bin

BROOT=/usr
X11LIB = $(BROOT)/X11R6/lib

DEFINES = -DPD -DUNIX

pd-gui_INCLUDES = -I$(BROOT)/include/tcl8.4 -I$(BROOT)/X11R6/include
pd-gui_LIBS = -ltk8.4 -ltcl8.4  -lX11 -ldl
pd-gui_LDFLAGS = -L$(X11LIB)
pd-gui_DEFINES = $(DEFINES)

pd_LIBS = -lm -lpthread -ldl
pd_LDFLAGS = -Wl,-export-dynamic
pd_DEFINES = $(DEFINES) -DINSTALL_PREFIX=\"$(PREFIX)\" \
             -DFIXEDPOINT -DUSEAPI_OSS -DDL_OPEN

extra_DEFINES = $(DEFINES) -DFIXEDPOINT
extra_INCLUDES = -I../src
extra_LDFLAGS = -shared

# IPOD toolchain  

ifeq ($(CC), arm-elf-gcc)
pd_LDFLAGS += -elf2flt
pd_LIBS = -lm -lpthread
pd_DEFINES = $(DEFINES) -DINSTALL_PREFIX=\"$(PREFIX)\" \
             -DFIXEDPOINT -DUSEAPI_OSS -D__linux__ -Dfork=getpid
extra_DEFINES += -D__linux__ -Dfork=getpid
endif

# BLACKFIN toolchain

ifeq ($(CC), bfin-uclinux-gcc)
pd_LIBS = -lm -lpthread
pd_DEFINES = $(DEFINES) -DINSTALL_PREFIX=\"$(PREFIX)\" \
             -DFIXEDPOINT -DUSEAPI_OSS -D__linux__ 
extra_DEFINES += -D__linux__
endif

# the sources

pd_SRC = g_canvas.c g_graph.c g_text.c g_rtext.c g_array.c g_template.c g_io.c \
    g_scalar.c g_traversal.c g_guiconnect.c g_readwrite.c g_editor.c \
    g_all_guis.c g_bang.c g_hdial.c g_hslider.c g_mycanvas.c g_numbox.c \
    g_toggle.c g_vdial.c g_vslider.c g_vumeter.c \
    m_pd.c m_class.c m_obj.c m_atom.c m_memory.c m_binbuf.c \
    m_conf.c m_glob.c m_sched.c \
    s_main.c s_inter.c s_file.c s_print.c \
    s_loader.c s_path.c s_entry.c s_audio.c s_midi.c \
    d_ugen.c d_arithmetic.c d_dac.c d_misc.c \
    d_fft.c d_mayer_fft.c d_fftroutine.c d_global.c \
    d_resample.c d_ctl.c d_soundfile.c \
    x_arithmetic.c x_connective.c x_interface.c x_midi.c x_misc.c \
    x_time.c x_acoustics.c x_net.c x_qlist.c x_gui.c \
    s_midi_oss.c s_audio_oss.c 

pd_SRC += d_imayer_fft.c m_fixed.c

pd_OBJ = $(pd_SRC:.c=.o) 

pd-gui_SRC =  t_main.c t_tkcmd.c
pd-gui_OBJ = $(pd-gui_SRC:.c=.o)

extra_SRC = $(shell ls ../intern/*.c) $(shell ls ../extra/*.c)
extra_OBJ = $(extra_SRC:.c=.o)
extra_EXT =  $(extra_SRC:.c=.pd_linux)

#
#  ------------------ targets ------------------------------------
#

all: $(BIN_DIR)/pd \
     $(BIN_DIR)/pd-gui \
     $(BIN_DIR)/pd-watchdog \
     $(BIN_DIR)/pdsend \
     $(BIN_DIR)/pdreceive \
     $(BIN_DIR)/pd.tk \
     extra

pd: $(BIN_DIR)/pd
pd-gui: $(BIN_DIR)/pd-gui
pd-watchdog: $(BIN_DIR)/pd-watchdog

static: 
	make pd_SRC="$(pd_SRC) $(extra_SRC)" DEFINES="-DPD -DUNIX -DSTATIC" \
	     pd pd-gui pd-watchdog $(BIN_DIR)/pdsend \
	     $(BIN_DIR)/pdreceive  $(BIN_DIR)/pd.tk

extra: $(extra_EXT)

ipod:
	make CC=arm-elf-gcc static

blackfin:
	make CC=bfin-uclinux-gcc static

$(pd_OBJ) : %.o : %.c
	$(CC) $(CFLAGS) $(pd_DEFINES) $(pd_INCLUDES) -c -o $(OBJ_DIR)/$@  $+ 

$(pd-gui_OBJ) : %.o : %.c
	$(CC) $(CFLAGS) $(pd-gui_DEFINES) $(pd-gui_INCLUDES) -c -o $(OBJ_DIR)/$@ $+

$(extra_OBJ) : %.o : %.c
	$(CC) $(CFLAGS) $(extra_DEFINES) $(extra_INCLUDES) -c -o $@ $*.c

$(extra_EXT) : %.$(EXT) : %.o
	$(CC) -o $@ $(extra_LDFLAGS)  $+

$(BIN_DIR)/pd-watchdog: s_watchdog.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ s_watchdog.c

$(BIN_DIR)/pdsend: u_pdsend.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ u_pdsend.c

$(BIN_DIR)/pdreceive: u_pdreceive.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ u_pdreceive.c

$(BIN_DIR)/pd: $(pd_OBJ) 
	cd ../obj;  $(CC) $(pd_LDFLAGS) -o $@ $(pd_OBJ) $(pd_LIBS)

$(BIN_DIR)/pd-gui: $(pd-gui_OBJ) 
	cd ../obj; $(CC) -o $@ $(pd-gui_OBJ) $(pd-gui_LDFLAGS) $(pd-gui_LIBS)

$(BIN_DIR)/pd.tk: u_main.tk
	echo set pd_nt 0 > $(BIN_DIR)/pd.tk
	grep -v "set pd_nt" < u_main.tk >> $@

INSTDIR = $(DESTDIR)/$(PREFIX)

install:  all
# Create the directory structure

	install -d $(INSTDIR)/lib/pd/bin
	install -d $(INSTDIR)/lib/pd/extra
	install -d $(INSTDIR)/lib/pd/intern
	install -d $(INSTDIR)/lib/pd/doc
	install -d $(INSTDIR)/bin
	install -d $(INSTDIR)/include

	install $(BIN_DIR)/pd-gui $(INSTDIR)/lib/pd/bin/
	install $(BIN_DIR)/pd-watchdog $(INSTDIR)/lib/pd/bin/
	install -m 644 $(BIN_DIR)/pd.tk $(INSTDIR)/lib/pd/bin/
	install -m 755 $(BIN_DIR)/pd $(INSTDIR)/bin/
	install -m 755 $(BIN_DIR)/pdsend $(INSTDIR)/bin/pdsend
	install -m 755 $(BIN_DIR)/pdreceive $(INSTDIR)/bin/pdreceive 
	cp -r ../doc/7.stuff $(INSTDIR)/lib/pd/doc
	cp -r ../doc/5.reference $(INSTDIR)/lib/pd/doc
	install -m 644 m_pd.h $(INSTDIR)/include/m_pd.h

# Install the externals if possible
	cp ../extra/*.pd_linux  $(INSTDIR)/lib/pd/extra
	cp ../intern/*.pd_linux $(INSTDIR)/lib/pd/intern

# Install the ICON and desktop file
	install -d $(INSTDIR)/share/pixmaps
	install -d $(INSTDIR)/share/applications
	cp ../ipkg/pd-icon.png $(INSTDIR)/share/pixmaps
	cp ../ipkg/pd.desktop $(INSTDIR)/share/applications/



clean: 
	-rm -f `find ../extra/ -name "*.pd_*"`
	-rm -f tags
	-rm -f ../obj/* $(BIN_DIR)/* ../extra/*.{o,$(EXT)} ../intern/*.{o,$(EXT)}
	-rm -f *~
	-rm -f $(BIN_DIR)/pdsend $(BIN_DIR)/pdreceive


PREFIX = /usr/
EXT = pd_linux

# pd specific

VPATH = ../obj:./
OBJ_DIR = ../obj
BIN_DIR = ../bin

BROOT=/usr
X11LIB = $(BROOT)/X11R6/lib

DEFINES = -DPD -DUNIX

pd-gui_INCLUDES = -I$(BROOT)/include/tcl8.4 -I$(BROOT)/X11R6/include
pd-gui_LIBS = -ltk8.4 -ltcl8.4  -lX11 -ldl
pd-gui_LDFLAGS = -L$(X11LIB)
pd-gui_DEFINES = $(DEFINES)

pd_LIBS = -lm -lpthread -ldl
pd_LDFLAGS = -Wl,-export-dynamic
pd_DEFINES = $(DEFINES) -DINSTALL_PREFIX=\"$(PREFIX)\" \
             -DFIXEDPOINT -DUSEAPI_OSS -DDL_OPEN

extra_DEFINES = $(DEFINES) -DFIXEDPOINT
extra_INCLUDES = -I../src
extra_LDFLAGS = -shared

# IPOD toolchain  

ifeq ($(CC), arm-elf-gcc)
pd_LDFLAGS += -elf2flt
pd_LIBS = -lm -lpthread
pd_DEFINES = $(DEFINES) -DINSTALL_PREFIX=\"$(PREFIX)\" \
             -DFIXEDPOINT -DUSEAPI_OSS -D__linux__ -Dfork=getpid
extra_DEFINES += -D__linux__ -Dfork=getpid
endif

# BLACKFIN toolchain

ifeq ($(CC), bfin-uclinux-gcc)
pd_LIBS = -lm -lpthread
pd_DEFINES = $(DEFINES) -DINSTALL_PREFIX=\"$(PREFIX)\" \
             -DFIXEDPOINT -DUSEAPI_OSS -D__linux__ 
extra_DEFINES += -D__linux__
endif

# the sources

pd_SRC = g_canvas.c g_graph.c g_text.c g_rtext.c g_array.c g_template.c g_io.c \
    g_scalar.c g_traversal.c g_guiconnect.c g_readwrite.c g_editor.c \
    g_all_guis.c g_bang.c g_hdial.c g_hslider.c g_mycanvas.c g_numbox.c \
    g_toggle.c g_vdial.c g_vslider.c g_vumeter.c \
    m_pd.c m_class.c m_obj.c m_atom.c m_memory.c m_binbuf.c \
    m_conf.c m_glob.c m_sched.c \
    s_main.c s_inter.c s_file.c s_print.c \
    s_loader.c s_path.c s_entry.c s_audio.c s_midi.c \
    d_ugen.c d_arithmetic.c d_dac.c d_misc.c \
    d_fft.c d_mayer_fft.c d_fftroutine.c d_global.c \
    d_resample.c d_ctl.c d_soundfile.c \
    x_arithmetic.c x_connective.c x_interface.c x_midi.c x_misc.c \
    x_time.c x_acoustics.c x_net.c x_qlist.c x_gui.c \
    s_midi_oss.c s_audio_oss.c 

pd_SRC += d_imayer_fft.c m_fixed.c

pd_OBJ = $(pd_SRC:.c=.o) 

pd-gui_SRC =  t_main.c t_tkcmd.c
pd-gui_OBJ = $(pd-gui_SRC:.c=.o)

extra_SRC = $(shell ls ../intern/*.c) $(shell ls ../extra/*.c)
extra_OBJ = $(extra_SRC:.c=.o)
extra_EXT =  $(extra_SRC:.c=.pd_linux)

#
#  ------------------ targets ------------------------------------
#

all: $(BIN_DIR)/pd \
     $(BIN_DIR)/pd-gui \
     $(BIN_DIR)/pd-watchdog \
     $(BIN_DIR)/pdsend \
     $(BIN_DIR)/pdreceive \
     $(BIN_DIR)/pd.tk \
     extra

pd: $(BIN_DIR)/pd
pd-gui: $(BIN_DIR)/pd-gui
pd-watchdog: $(BIN_DIR)/pd-watchdog

static: 
	make pd_SRC="$(pd_SRC) $(extra_SRC)" DEFINES="-DPD -DUNIX -DSTATIC" \
	     pd pd-gui pd-watchdog $(BIN_DIR)/pdsend \
	     $(BIN_DIR)/pdreceive  $(BIN_DIR)/pd.tk

extra: $(extra_EXT)

ipod:
	make CC=arm-elf-gcc static

blackfin:
	make CC=bfin-uclinux-gcc static

$(pd_OBJ) : %.o : %.c
	$(CC) $(CFLAGS) $(pd_DEFINES) $(pd_INCLUDES) -c -o $(OBJ_DIR)/$@  $+ 

$(pd-gui_OBJ) : %.o : %.c
	$(CC) $(CFLAGS) $(pd-gui_DEFINES) $(pd-gui_INCLUDES) -c -o $(OBJ_DIR)/$@ $+

$(extra_OBJ) : %.o : %.c
	$(CC) $(CFLAGS) $(extra_DEFINES) $(extra_INCLUDES) -c -o $@ $*.c

$(extra_EXT) : %.$(EXT) : %.o
	$(CC) -o $@ $(extra_LDFLAGS)  $+

$(BIN_DIR)/pd-watchdog: s_watchdog.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ s_watchdog.c

$(BIN_DIR)/pdsend: u_pdsend.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ u_pdsend.c

$(BIN_DIR)/pdreceive: u_pdreceive.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ u_pdreceive.c

$(BIN_DIR)/pd: $(pd_OBJ) 
	cd ../obj;  $(CC) $(pd_LDFLAGS) -o $@ $(pd_OBJ) $(pd_LIBS)

$(BIN_DIR)/pd-gui: $(pd-gui_OBJ) 
	cd ../obj; $(CC) -o $@ $(pd-gui_OBJ) $(pd-gui_LDFLAGS) $(pd-gui_LIBS)

$(BIN_DIR)/pd.tk: u_main.tk
	echo set pd_nt 0 > $(BIN_DIR)/pd.tk
	grep -v "set pd_nt" < u_main.tk >> $@

INSTDIR = $(DESTDIR)/$(PREFIX)

install:  all
# Create the directory structure

	install -d $(INSTDIR)/lib/pd/bin
	install -d $(INSTDIR)/lib/pd/extra
	install -d $(INSTDIR)/lib/pd/intern
	install -d $(INSTDIR)/lib/pd/doc
	install -d $(INSTDIR)/bin
	install -d $(INSTDIR)/include

	install $(BIN_DIR)/pd-gui $(INSTDIR)/lib/pd/bin/
	install $(BIN_DIR)/pd-watchdog $(INSTDIR)/lib/pd/bin/
	install -m 644 $(BIN_DIR)/pd.tk $(INSTDIR)/lib/pd/bin/
	install -m 755 $(BIN_DIR)/pd $(INSTDIR)/bin/
	install -m 755 $(BIN_DIR)/pdsend $(INSTDIR)/bin/pdsend
	install -m 755 $(BIN_DIR)/pdreceive $(INSTDIR)/bin/pdreceive 
	cp -r ../doc/7.stuff $(INSTDIR)/lib/pd/doc
	cp -r ../doc/5.reference $(INSTDIR)/lib/pd/doc
	install -m 644 m_pd.h $(INSTDIR)/include/m_pd.h

# Install the externals if possible
	cp ../extra/*.pd_linux  $(INSTDIR)/lib/pd/extra
	cp ../intern/*.pd_linux $(INSTDIR)/lib/pd/intern

# Install the ICON and desktop file
	install -d $(INSTDIR)/share/pixmaps
	install -d $(INSTDIR)/share/applications
	cp ../ipkg/pd-icon.png $(INSTDIR)/share/pixmaps
	cp ../ipkg/pd.desktop $(INSTDIR)/share/applications/



clean: 
	-rm -f `find ../extra/ -name "*.pd_*"`
	-rm -f tags
	-rm -f ../obj/* $(BIN_DIR)/* ../extra/*.{o,$(EXT)} ../intern/*.{o,$(EXT)}
	-rm -f *~
	-rm -f $(BIN_DIR)/pdsend $(BIN_DIR)/pdreceive


