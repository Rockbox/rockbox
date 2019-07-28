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
# The weird grep -v thing in here is due to Apple's stupidities and is needed
# to make this do right when used on Mac OS X.
#
# The sed line is to prepend the directory to all source files

preprocess = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c -include config.h $(1) | \
		grep -v '^\#' | grep -v "^ *$$" | \
		sed -e 's:^..*:$(dir $(1))&:')

preprocess2file = $(SILENT)$(CC) $(PPCFLAGS) $(3) -E -P -x c -include config.h $(1) | \
		grep -v '^\#' | grep -v "^$$" > $(2)

asmdefs2file = $(SILENT)$(CC) $(PPCFLAGS) $(3) -S -x c -o - -include config.h $(1) | \
	perl -ne 'if(/^_?AD_(\w+):$$/){$$var=$$1}else{/^\W\.(?:word|long)\W(.*)$$/ && $$var && print "\#define $$var $$1\n";$$var=0}' > $(2)

c2obj = $(addsuffix .o,$(basename $(call full_path_subst,$(ROOTDIR)/%,$(BUILDDIR)/%,$(1))))

a2lnk = $(patsubst lib%.a,-l%,$(notdir $(1)))

# objcopy wrapper that keeps debug symbols in DEBUG builds
# handles the $(1) == $(2) case too
ifndef APP_TYPE
objcopy = $(OC) $(if $(filter yes, $(USE_ELF)), -S -x, -O binary) $(1) $(2)	# objcopy native
else ifneq (,$(findstring sdl-sim,$(APP_TYPE)))
objcopy = cp $(1) $(1).tmp;mv -f $(1).tmp $(2)		# objcopy simulator
else
  ifdef DEBUG
    objcopy = cp $(1) $(1).tmp;mv -f $(1).tmp $(2)	# objcopy hosted (DEBUG)
  else
    objcopy = $(OC) -S -x $(1) $(2)					# objcopy hosted (!DEBUG)
   endif
endif

# calculate dependencies for a list of source files $(2) and output them to $(1)
mkdepfile = $(SILENT)perl $(TOOLSDIR)/multigcc.pl $(CC) $(PPCFLAGS) $(OTHER_INC) -MG -MM -include config.h -- $(2) | \
	sed -e "s: lang.h: lang/lang.h:" \
	-e 's:_asmdefs.o:_asmdefs.h:' \
	-e "s: max_language_size.h: lang/max_language_size.h:" | \
	$(TOOLSDIR)/addtargetdir.pl $(ROOTDIR) $(BUILDDIR) \
	>> $(1)

# function to create .bmp dependencies
bmpdepfile = $(SILENT) \
	for each in $(2); do \
	    obj=`echo $$each | sed -e 's/\.bmp/.o/' -e 's:$(ROOTDIR):$(BUILDDIR):'`; \
	    src=`echo $$each | sed -e 's/\.bmp/.c/' -e 's:$(ROOTDIR):$(BUILDDIR):'`; \
	    hdr=`echo $$each | sed -e 's/.*\/\(.*\)\..*\.bmp/bitmaps\/\1\.h/'`; \
	    echo $$obj: $$src; \
	    echo $$src: $$each; \
	    echo $(BUILDDIR)/$$hdr: $$src; \
	done \
	>> $(1)

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

# path substituion can be surprisingly tricky, indeed the following almost
# always work but not quite:
#   BMPOBJ = $(BMP:$(ROOTDIR)/%.bmp=$(BUILDDIR)/%.o)
# indeed if
#   ROOTDIR=/rockbox
#   BUILDDIR=/rockbox/build-bla
# then:
#   /rockbox/apps/bitmaps/rockboxlogo-bla.bmp -> /rockbox/apps/bitmaps/rockbox/build-blalogo.o
# this function ensure that this type of problems does not occur
#
# usage: $(call full_path_subst,patterm,replacement,text)
#
# example: $(call full_path_subst,$(ROOTDIR)/%.bmp,$(BUILDDIR)/%.o,$(BMP))
NO_MATCH=this_string_will_never_match_anything
full_path_subst=$(patsubst $(NO_MATCH)/%,%, $(patsubst $(NO_MATCH)/$(1), $(2), $(addprefix $(NO_MATCH)/, $(3))))
