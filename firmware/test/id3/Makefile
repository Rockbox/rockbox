TARGET = id3test

OBJS = id3.o
CFLAGS = -g -W -Wall -DDEBUG_STANDALONE -DSIMULATOR -c -o

$(TARGET) : $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

id3.o:../../id3.c
	$(CC) $(CFLAGS) $@ $<

clean:
	rm -f $(OBJS) $(TARGET) *~
