#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
CFLAGS =

TARGETS = scramble.exe descramble.exe sh2d.exe convbdf.exe

all: $(TARGETS)

$(OBJDIR)/%.exe: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del $(TARGETS) *.obj

