
OBJS1 = mytest.o
TARGET1 = mytest

OBJS2 = Malloc.o
TARGET2 = mtest

OBJS3 = dmytest.o
TARGET3 = dmytest

# define this to talk a lot in runtime
# -DDEBUG_VERBOSE 
CFLAGS = -g -Wall -DDEBUG -I../../malloc
CC = gcc
AR = ar

LDFLAGS = -L../../malloc -ldmalloc

all: $(TARGET1) $(TARGET2) $(TARGET3)

clean:
	rm -f core *~ $(TARGET1) $(TARGET2) $(TARGET3) \
	$(OBJS1) $(OBJS2) $(OBJS3)

$(TARGET1): $(OBJS1)
	$(CC) -g -o $(TARGET1) $(OBJS1) $(LDFLAGS)

$(TARGET2): $(OBJS2)
	$(CC) -g -o $(TARGET2) $(OBJS2) $(LDFLAGS)

$(TARGET3): $(OBJS3)
	$(CC) -g -o $(TARGET3) $(OBJS3) $(LDFLAGS)

dmytest.o: dmytest.c
Malloc.o: Malloc.c
mytest.o: mytest.c
