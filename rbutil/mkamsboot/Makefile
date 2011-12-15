#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

# We use the UCL code available in the Rockbox tools/ directory
CFLAGS += -I../../tools/ucl/include -Wall

OUTPUT = mkamsboot
LIBUCL = libucl$(RBARCH).a

# inputs
LIBSOURCES := dualboot.c md5.c mkamsboot.c
SOURCES := $(LIBSOURCES) main.c
# additional link dependencies for the standalone executable
EXTRADEPS := $(LIBUCL)

include ../libtools.make
# explicit dependencies on dualboot.{c,h} and mkamsboot.h
$(OBJDIR)mkamsboot.o: dualboot.h dualboot.c mkamsboot.c mkamsboot.h
$(OBJDIR)main.o: dualboot.h dualboot.c main.c mkamsboot.h

