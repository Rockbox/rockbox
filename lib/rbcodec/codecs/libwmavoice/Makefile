CC = gcc -o
INC = -I.
OUTPUT = wmavoice
STD = c99
LIBS = -lm
CFLAGS = -Wall -ggdb -std=$(STD) $(INC)

SOURCES = \
acelp_filters.c\
acelp_vectors.c\
avfft.c\
bitstream.c\
celp_filters.c\
celp_math.c\
dct.c\
fft.c\
lsp.c\
mdct.c\
rdft.c\
utils.c\
wmavoice.c\
libavutil/log.c\
libavutil/lzo.c\
libavutil/mem.c\
libavutil/mathematics.c

HEADERS = \
acelp_vectors.h\
celp_math.h\
get_bits.h\
wmavoice_data.h\
avcodec.h\
fft.h\
acelp_filters.h\
celp_filters.h\
put_bits.h\
lsp.h\
internal.h\
avfft.h\
mathops.h\
mdct_tablegen.h\
dct32.c\
libavutil/avutil.h\
libavutil/attributes.h\
libavutil/lzo.h\
libavutil/mem.h\
libavutil/log.h\
libavutil/internal.h\
libavutil/common.h\
libavutil/intreadwrite.h\
libavutil/bswap.h\
libavutil/mathematics.h

OBJECTS = $(SOURCES:.c=.o)

all:$(OUTPUT)

$(OUTPUT):$(SOURCES) $(HEADERS)
	$(CC) $@ $(CFLAGS) $(SOURCES) $(LIBS)
	@echo "Done."

clean:
	rm -f *.o $(OUTPUT) *~

