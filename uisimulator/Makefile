##############################################################################
# Rockbox UI simulator
#############################################################################

TARGET = rockboxui

CC = gcc
RM = rm

CFLAGS = -g -O2 
CPPFLAGS =  -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS
LDFLAGS = -lX11 -lm -lXt -lXmu -lsocket -lnsl

DEPEND = .depends

OBJS=   alpha.o hsv.o screenhack.o yarandom.o uibasic.o resources.o visual.o\
	usleep.o

SRCS = $(OBJS:%.o=%.c)
HDRS = $(OBJS:%.o=%.h)

all: $(DEPEND) $(TARGET)

clean:
	$(RM) -f $(OBJS) *~ core $(TARGET) $(CLIENTS) $(DEPEND)

distclean: clean
	$(RM) config.cache

.c.o:
	$(CC) $(CPPFLAGS) $(CCFLAGS) -c $<

$(DEPEND):
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPEND)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(LDFLAGS) $(OBJS)

# Dependencies
include $(DEPEND)
