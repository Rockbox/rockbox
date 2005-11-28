#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
TARGET = libucl.a

CFLAGS = -I../include

SOURCES = alloc.c io.c n2b_99.c n2b_d.c n2b_ds.c n2b_to.c n2d_99.c	\
	n2d_d.c n2d_ds.c n2d_to.c n2e_99.c n2e_d.c n2e_ds.c n2e_to.c ucl_crc.c	\
	ucl_dll.c ucl_init.c ucl_ptr.c ucl_str.c ucl_util.c

OBJS = alloc.o io.o n2b_99.o n2b_d.o n2b_ds.o n2b_to.o n2d_99.o	\
	n2d_d.o n2d_ds.o n2d_to.o n2e_99.o n2e_d.o n2e_ds.o n2e_to.o ucl_crc.o	\
	ucl_dll.o ucl_init.o ucl_ptr.o ucl_str.o ucl_util.o

$(TARGET): $(OBJS)
	ar ruv $(TARGET) $(OBJS)


clean:
	rm -f $(OBJS) $(TARGET)
