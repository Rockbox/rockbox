#############################################################################
##             __________               __   ___.
##   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
##   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
##   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
##   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
##                     \/            \/     \/    \/            \/
## Copyright Alan Korr, 2002. All rights reserved.
##
## Permission to use, copy, modify, and distribute this software for any
## purpose is hereby granted without fee, provided that this copyright and
## permissions notice appear in all copies and derivatives, and that no
## charge may be made for the software and its documentation except to cover
## cost of distribution.
##
## This software is provided "as is" without express or implied warranty.
#############################################################################
ARCH	= test

CC      = gcc 
AS      = as
LD      = ld
AR      = ar
RL      = ranlib
OC      = objcopy
GZ      = gzip -f

PREFIX  = ~/rockbox/$(ARCH)
PACKAGE = memory
VERSION = 0.1
DEFINES = -DTEST

#####################################################"
# Compiler flags :

CFLAGS  = -g 
#CFLAGS += -save-temps
CFLAGS += -Wall \
	  -W \
	  -Wshadow \
	  -Wpointer-arith \
	  -Waggregate-return \
	  -Wstrict-prototypes \
	  -Wredundant-decls \
	  -Winline \
	  -Wmissing-prototypes \
	  -Werror \
	  -Wsign-compare \
	  -Wmissing-declarations \
	  -Wmissing-noreturns \
	  -Wnested-externs
CFLAGS += -pipe -O3
CFLAGS += -fomit-frame-pointer \
	  -fschedule-insns
CFLAGS += $(EXTRA_CFLAGS)
CFLAGS += $(DEFINES)

#######################################################################
## PLEASE CONSIDER THERE IS NOTHING TO CHANGE IN THE FOLLOWING LINES
##              SINCE THERE ARE COMMON FOR ALL LIBRARY             
##

.SUFFIXES : .o .c .s

INCLUDES = -I. \
	   -I$(PREFIX)/headers

STATIC_LIBRARY_PATH = $(PREFIX)/libraries

LIBRARY = lib$(PACKAGE).a

#######################################################################
## PLEASE CHANGE ONLY THE FOLLOWING LINES
##

LIBS = 

HEADERS = $(PACKAGE).h \
	  config.h \
	  defines.h \
	  types.h \
	  return_values.h \
	  inlines.h \
	  functions.h

SOURCES = $(PACKAGE)-page.c \
	  $(PACKAGE)-slab.c 

OBJECTS  = $(SOURCES:.c=.o)

DEPENDENCIES = $(SOURCES:.c=.d)

HEADER_PATH = $(PREFIX)/headers/$(PACKAGE)/.

#######################################################################
## PLEASE CONSIDER THERE IS NOTHING TO CHANGE IN THE FOLLOWING LINES
##              SINCE THERE ARE COMMON FOR ALL LIBRARY             
##

%.o: %.c
	@echo "Compiling" $<...
	@$(CC) -o $(@) $(CFLAGS) $(INCLUDES) -c $<
	@$(CC) -M $< $(CFLAGS) $(INCLUDES) > $(*F).d

%.o: %.s
	@echo "Assembling" $<...
	@$(CC) -o $(@) $(CFLAGS) $(INCLUDES) -c $<
	@$(CC) -M $< $(CFLAGS) $(INCLUDES) > $(*F).d

.PHONY: splash all clean backup restore dist install

all: splash $(LIBRARY) test

splash:
	@echo "<<< " $(PACKAGE) "-" $(VERSION) ">>>"

####################################################
# LIBRAY PART :

$(LIBRARY): $(OBJECTS)
	@echo "Creating library" $(LIBRARY)... 
	@$(AR) cru $(@) $(OBJECTS)
	@$(RL) $(@)


####################################################
# TEST PART :

test: test.tab.o test.lex.o $(LIBRARY)
	@echo "Creating executable" $@...
	@$(CC) $(INCLUDES) -g -o $(@) $(+) -lfl -lreadline

test.tab.o: test.tab.c
	@echo "Compiling" $<...
	@$(CC) -I. -g -o $(@) -O3 -fomit-frame-pointer -c test.tab.c

test.lex.o: test.lex.c
	@echo "Compiling" $<...
	@$(CC) -I. -g -o $(@) -O3 -fomit-frame-pointer -c test.lex.c

test.tab.h: test.tab.c

test.lex.c: test.l test.tab.h
	@echo "Flex:" $<
	@flex -otest.lex.c test.l

test.tab.c: test.y
	@echo "Bison:" $<
	@bison -d test.y


####################################################
# MISCELLANOUS PART :

clean:
	@rm -f $(LIBRARY)
	@rm -f $(OBJECTS) test.lex.o test.tab.o
	@rm -f $(DEPENDENCIES)
	@rm -f *~ test test.exe
	@rm -f test.tab.h test.tab.c test.lex.c 
	@rm -f core

backup:
	@mkdir -p ./backup
	@cp -f makefile ./backup
	@cp -f test.l ./backup
	@cp -f test.y ./backup
	@cp -f $(SOURCES:.c=.txt) ./backup
	@for header in $(HEADERS) ; do cp -f $$header ./backup ; done
	@for source in $(SOURCES) ; do cp -f $$source ./backup ; done

restore:
	@cp -f ./backup/makefile .
	@cp -f ./backup/test.l .
	@cp -f ./backup/test.y .
	@cp -f ./backup/$(SOURCES:.c=.txt)
	@for header in $(HEADERS) ; do cp -f ./backup/$$header . ; done
	@for source in $(SOURCES) ; do cp -f ./backup/$$source . ; done

dist: backup
	@mv backup $(PACKAGE)
	@tar czvf $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)/*
	@rm -f $(PACKAGE)/*
	@rmdir $(PACKAGE)

install: all
	@mkdir -p $(PREFIX)/libraries
	@cp $(LIBRARY) $(PREFIX)/libraries
	@mkdir -p $(PREFIX)/headers/$(PACKAGE)
	@for header in $(HEADERS) ; do cp $$header $(PREFIX)/headers/$(PACKAGE) ; done
	     
-include $(DEPENDENCIES)

