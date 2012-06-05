CC=gcc
CFLAGS=-Wall -O0 -g
TARGET=bflt_load

$(TARGET): bflt.o
	$(CC) $(CFLAGS) *.o main.c -o $(TARGET)

$(TARGET).o:
	$(CC) $(CFLAGS) $(TARGET).c -c

clean:
	rm -f $(TARGET) *.o
