#include "bitstream.h"

#define SHN_OUTPUT_DEPTH 28

#define MAX_CHANNELS   2
#define MAX_PRED_ORDER 16
#define MAX_NWRAP      MAX_PRED_ORDER
#define MAX_NMEAN      4

/* NUM_DEC_LOOPS should be even number */
#define NUM_DEC_LOOPS      26
#define DEFAULT_BLOCK_SIZE 256
#define MAX_HEADER_SIZE    DEFAULT_BLOCK_SIZE*4
#define MAX_BUFFER_SIZE    2*DEFAULT_BLOCK_SIZE*NUM_DEC_LOOPS
#define MAX_DECODE_SIZE    ((DEFAULT_BLOCK_SIZE*NUM_DEC_LOOPS/2) + MAX_NWRAP)
#define MAX_OFFSET_SIZE    MAX_NMEAN

#define FN_DIFF0     0
#define FN_DIFF1     1
#define FN_DIFF2     2
#define FN_DIFF3     3
#define FN_QUIT      4
#define FN_BLOCKSIZE 5
#define FN_BITSHIFT  6
#define FN_QLPC      7
#define FN_ZERO      8
#define FN_VERBATIM  9
#define FN_ERROR     10

typedef struct ShortenContext {
    GetBitContext gb;
    int32_t lpcqoffset;
    uint32_t totalsamples;
    int header_bits;
    int channels;
    int sample_rate;
    int bits_per_sample;
    int version;
    int bitshift;
    int last_bitshift;
    int nmean;
    int nwrap;
    int blocksize;
    int bitindex;
} ShortenContext;

int shorten_init(ShortenContext* s, uint8_t *buf, int buf_size);
int shorten_decode_frames(ShortenContext *s, int *nsamples,
                         int32_t *decoded0, int32_t *decoded1,
                         int32_t *offset0, int32_t *offset1,
                         uint8_t *buf, int buf_size,
                         void (*yield)(void)) ICODE_ATTR;
