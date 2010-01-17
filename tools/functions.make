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

asmdefs2file = $(shell $(CC) $(PPCFLAGS) $(3) -S -x c -o - -include config.h $(1) | \
	perl -ne 'if(/^_?AD_(\w+):$$/){$$var=$$1}else{/^\W\.(?:word|long)\W(.*)$$/ && $$var && print "\#define $$var $$1\n";$$var=0}' > $2)

c2obj = $(addsuffix .o,$(basename $(subst $(ROOTDIR),$(BUILDDIR),$(1))))

# calculate dependencies for a list of source files $(2) and output them
# to a file $(1)_, to be later renamed to $(1).
mkdepfile = $(shell \
	perl $(TOOLSDIR)/multigcc.pl $(CC) $(PPCFLAGS) $(OTHER_INC) -MG -MM -include config.h -- $(2) | \
	sed -e "s: lang.h: lang/lang.h:" \
	-e 's:_asmdefs.o:_asmdefs.h:' \
	-e "s: max_language_size.h: lang/max_language_size.h:" | \
	$(TOOLSDIR)/addtargetdir.pl $(ROOTDIR) $(BUILDDIR) \
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

