CC=gcc
CFLAGS=-g -Wall -O0 -m32
TARGET=elfload

$(TARGET): elfload.o main.o
	$(CC) $(CFLAGS) *.o -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o $(TARGET)
