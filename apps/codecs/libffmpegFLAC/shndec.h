#include "bitstream.h"

#define SHN_OUTPUT_DEPTH 28
#define DEFAULT_BLOCK_SIZE 256
#define MAX_FRAMESIZE 1024
#define MAX_CHANNELS 2
#define MAX_NWRAP 3
#define MAX_NMEAN 4

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
    int nmean;
    int nwrap;
    int blocksize;
    int bitindex;
/* Not needed...
    int bit_rate;
    int block_align;
    int chunk_size;
*/
} ShortenContext;

int shorten_init(ShortenContext* s, uint8_t *buf, int buf_size);
int shorten_decode_frame(ShortenContext *s,
                         int32_t *decoded,
                         int32_t *offset,
                         uint8_t *buf,
                         int buf_size) ICODE_ATTR;
