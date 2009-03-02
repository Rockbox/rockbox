#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# preprocess - run preprocessor on a file and return the result as a string
#
# This uses the native 'gcc' compiler and not $(CC) since we use the -imacros
# option and older gcc compiler doesn't have that. We use one such older
# compiler for the win32 cross-compiles on Linux.
#
# The weird grep -v thing in here is due to Apple's stupidities and is needed
# to make this do right when used on Mac OS X.
#
# The sed line is to prepend the directory to all source files

preprocess = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c -include config.h $(1) | \
		grep -v '^\#' | \
		sed -e 's:^..*:$(dir $(1))&:')

preprocess2file = $(shell $(CC) $(PPCFLAGS) $(3) -E -P -x c -include config.h $(1) | \
		grep -v '^\#' | grep -v "^$$" > $(2))

c2obj = $(addsuffix .o,$(basename $(subst $(ROOTDIR),$(BUILDDIR),$(1))))

# calculate dependencies for a list of source files $(2) and output them
# to a file $(1)
mkdepfile = $(shell \
	$(CC) $(PPCFLAGS) $(OTHER_INC) -MG -MM -include config.h $(2) | \
	$(TOOLSDIR)/addtargetdir.pl $(ROOTDIR) $(BUILDDIR) | \
	sed -e "s: lang.h: $(BUILDDIR)/lang/lang_core.o:" \
	-e "s: sysfont.h: $(BUILDDIR)/sysfont.h:" \
	-e "s: max_language_size.h: $(BUILDDIR)/lang/max_language_size.h:" \
	-e "s: bitmaps/: $(BUILDDIR)/bitmaps/:g" \
	-e "s: pluginbitmaps/: $(BUILDDIR)/pluginbitmaps/:g" \
	-e "s: lib/: $(APPSDIR)/plugins/lib/:g" \
	-e "s: codeclib.h: $(APPSDIR)/codecs/lib/codeclib.h:g" \
	>> $(1)_)

# function to create .bmp dependencies
bmpdepfile = $(shell \
	for each in $(2); do \
	    obj=`echo $$each | sed -e 's/\.bmp/.o/' -e 's:$(ROOTDIR):$(BUILDDIR):'`; \
	    src=`echo $$each | sed -e 's/\.bmp/.c/' -e 's:$(ROOTDIR):$(BUILDDIR):'`; \
	    echo $$obj: $$src; \
	    echo $$src: $$each; \
	done \
	>> $(1); )

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

