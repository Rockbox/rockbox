
TARGET = libdmalloc.a

LIBOBJS = dmalloc.o bmalloc.o bysize.o

OBJS1 = mytest.o
TARGET1 = mytest

OBJS2 = Malloc.o
TARGET2 = mtest

OBJS3 = dmytest.o
TARGET3 = dmytest

# define this to talk a lot in runtime
# -DDEBUG_VERBOSE 
CFLAGS = -g -DUNIX -DBMALLOC -Wall -DDEBUG
CC = gcc
AR = ar

LDFLAGS = -L. -ldmalloc

all: $(TARGET) $(TARGET1) $(TARGET2) $(TARGET3)

clean:
	rm -f core *~ $(TARGET) $(TARGET1) $(TARGET2) $(TARGET3) \
	$(LIBOBJS) $(OBJS1) $(OBJS2) $(OBJS3)

$(TARGET): $(LIBOBJS)
	$(AR) ruv $(TARGET) $(LIBOBJS)

$(TARGET1): $(OBJS1)
	$(CC) -g -o $(TARGET1) $(OBJS1) $(LDFLAGS)

$(TARGET2): $(OBJS2)
	$(CC) -g -o $(TARGET2) $(OBJS2) $(LDFLAGS)

$(TARGET3): $(OBJS3)
	$(CC) -g -o $(TARGET3) $(OBJS3) $(LDFLAGS)

bmalloc.o: bmalloc.c bysize.h
bysize.o: bysize.c
dmalloc.o: dmalloc.c
dmytest.o: dmytest.c dmalloc.h bmalloc.h
Malloc.o: Malloc.c
mytest.o: mytest.c bmalloc.h

tgz:
	@(dir=`pwd`;name=`basename $$dir`;echo Creates $$name.tar.gz; cd .. ; \
	tar -cf $$name.tar `cat $$name/FILES | sed "s:^/:$$name/:g"` ; \
	gzip $$name.tar ; chmod a+r $$name.tar.gz ; mv $$name.tar.gz $$name/)
