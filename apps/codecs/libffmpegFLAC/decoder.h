#ifndef _FLAC_DECODER_H
#define _FLAC_DECODER_H
 
#include "bitstream.h"

#define MAX_CHANNELS 2       /* Maximum supported channels */
#define MAX_BLOCKSIZE 4608   /* Maxsize in samples of one uncompressed frame */
#define MAX_FRAMESIZE 32768  /* Maxsize in bytes of one compressed frame */

#define FLAC_OUTPUT_DEPTH 29 /* Provide samples left-shifted to 28 bits+sign */

enum decorrelation_type {
    INDEPENDENT,
    LEFT_SIDE,
    RIGHT_SIDE,
    MID_SIDE,
};

typedef struct FLACContext {
    GetBitContext gb;

    int min_blocksize, max_blocksize;
    int min_framesize, max_framesize;
    int samplerate, channels;
    int blocksize/*, last_blocksize*/;
    int bps, curr_bps;
    unsigned long samplenumber;
    unsigned long totalsamples;
    enum decorrelation_type decorrelation;

    int filesize;
    int length;
    int bitrate;
    int metadatalength;
    
    int bitstream_size;
    int bitstream_index;

    int sample_skip;
    int framesize;
} FLACContext;

int flac_decode_frame(FLACContext *s,
                      int32_t* decoded0,
                      int32_t* decoded1,
                      uint8_t *buf, int buf_size,
                      void (*yield)(void)) ICODE_ATTR_FLAC;

#endif
