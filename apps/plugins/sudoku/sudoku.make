#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

SUDOKUSRCDIR := $(APPSDIR)/plugins/sudoku
SUDOKUBUILDDIR := $(BUILDDIR)/apps/plugins/sudoku

ROCKS += $(SUDOKUBUILDDIR)/sudoku.rock

SUDOKU_SRC := $(call preprocess, $(SUDOKUSRCDIR)/SOURCES)
SUDOKU_OBJ := $(call c2obj, $(SUDOKU_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(SUDOKU_SRC)

$(SUDOKUBUILDDIR)/sudoku.rock: $(SUDOKU_OBJ)
