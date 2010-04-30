STD = c99
LINK = -lm
CFLAGS = -Wall -std=$(STD)
TARGET = test
OBJS = wmaprodec.c wma.c dsputil.c mdct.c fft.c bitstream.c libavutil/log.c libavutil/mem.c libavutil/mathematics.c
$(TARGET): $(OBJS)
	gcc $(CFLAGS) $(OBJS) $(LINK) -o $(TARGET)
	
clean:
	rm -f $(TARGET) *~ *.o libavutil/*.o

