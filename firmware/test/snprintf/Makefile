
TARGET = snprintf

OBJS = snprintf.o test.o

CFLAGS = -I../../include

$(TARGET): $(OBJS)

snprintf.o: ../../common/sprintf.c
	$(CC) -c $< -o $@

test.o: test.c

clean:
	rm -f $(OBJS)