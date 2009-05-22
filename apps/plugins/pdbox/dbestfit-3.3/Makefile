
OBJS1 = bmalloc.o bysize.o mytest.o
TARGET1 = mytest

OBJS2 = dmalloc.o bmalloc.o bysize.o Malloc.o
TARGET2 = mtest

OBJS3 = dmalloc.o bmalloc.o bysize.o dmytest.o
TARGET3 = dmytest

CFLAGS = -g -DUNIX -DBMALLOC -Wall -pedantic -DDEBUG
CC = gcc

all: $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1): $(OBJS1)
	$(CC) -g -o $(TARGET1) $(OBJS1)

$(TARGET2): $(OBJS2)
	$(CC) -g -o $(TARGET2) $(OBJS2)

$(TARGET3): $(OBJS3)
	$(CC) -g -o $(TARGET3) $(OBJS3)

bmalloc.o: bmalloc.c
dmalloc.o: dmalloc.c
mytest.o: mytest.c
dmytest.o: dmytest.c
Malloc.o : Malloc.c
bysize.o : bysize.c

tgz:
	@(dir=`pwd`;name=`basename $$dir`;echo Creates $$name.tar.gz; cd .. ; \
	tar -cf $$name.tar `cat $$name/FILES | sed "s:^/:$$name/:g"` ; \
	gzip $$name.tar ; chmod a+r $$name.tar.gz ; mv $$name.tar.gz $$name/)

clean:
	rm -f *.o *~ $(TARGET1) $(TARGET2) $(TARGET3)

OBJS1 = bmalloc.o bysize.o mytest.o
TARGET1 = mytest

OBJS2 = dmalloc.o bmalloc.o bysize.o Malloc.o
TARGET2 = mtest

OBJS3 = dmalloc.o bmalloc.o bysize.o dmytest.o
TARGET3 = dmytest

CFLAGS = -g -DUNIX -DBMALLOC -Wall -pedantic -DDEBUG
CC = gcc

all: $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1): $(OBJS1)
	$(CC) -g -o $(TARGET1) $(OBJS1)

$(TARGET2): $(OBJS2)
	$(CC) -g -o $(TARGET2) $(OBJS2)

$(TARGET3): $(OBJS3)
	$(CC) -g -o $(TARGET3) $(OBJS3)

bmalloc.o: bmalloc.c
dmalloc.o: dmalloc.c
mytest.o: mytest.c
dmytest.o: dmytest.c
Malloc.o : Malloc.c
bysize.o : bysize.c

tgz:
	@(dir=`pwd`;name=`basename $$dir`;echo Creates $$name.tar.gz; cd .. ; \
	tar -cf $$name.tar `cat $$name/FILES | sed "s:^/:$$name/:g"` ; \
	gzip $$name.tar ; chmod a+r $$name.tar.gz ; mv $$name.tar.gz $$name/)

clean:
	rm -f *.o *~ $(TARGET1) $(TARGET2) $(TARGET3)
