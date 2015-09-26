#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
CFLAGS += -Wall

OUTPUT = mk6gboot

# inputs
LIBSOURCES := dualboot.c mkdfu.c
SOURCES := $(LIBSOURCES) main.c

include ../libtools.make

# explicit dependencies on dualboot.{c,h} and mkdfu.h
$(OBJDIR)mkdfu.o: dualboot.h dualboot.c mkdfu.c mkdfu.h
$(OBJDIR)main.o: dualboot.h dualboot.c main.c mkdfu.h
