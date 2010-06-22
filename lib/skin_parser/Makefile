#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

BUILDDIR ?= .
ifndef V
SILENT = @
endif

SRC = skin_buffer.c skin_parser.c skin_debug.c skin_scan.c tag_table.c
OBJ := $(patsubst %.c,$(BUILDDIR)/%.o,$(SRC))
OUT = $(BUILDDIR)/libskin_parser.a
CC = gcc
AR = ar
INCLUDES = -I.

default: $(OUT)

$(BUILDDIR)/%.o: %.c
	@echo CC $<
	$(SILENT)$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	@echo AR $(notdir $(OUT))
	$(SILENT)$(AR) rcs $(OUT) $(OBJ)

clean:
	rm -f $(OBJ) $(OUT)
