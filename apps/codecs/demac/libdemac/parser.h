/*

libdemac - A Monkey's Audio decoder

$Id:$

Copyright (C) Dave Chapman 2007

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

#ifndef _APE_PARSER_H
#define _APE_PARSER_H

#include <inttypes.h>

#ifdef ROCKBOX
/* Include the Rockbox Codec API when building for Rockbox */
#define APE_OUTPUT_DEPTH 29
#ifndef ROCKBOX_PLUGIN
#include "../lib/codeclib.h"
#include <codecs.h>
#endif
#else
#define APE_OUTPUT_DEPTH (ape_ctx->bps)
#define IBSS_ATTR
#define ICONST_ATTR
#define ICODE_ATTR
#endif

/* The earliest and latest file formats supported by this library */
#define APE_MIN_VERSION 3970
#define APE_MAX_VERSION 3990

#define MAC_FORMAT_FLAG_8_BIT                 1    // is 8-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_CRC                   2    // uses the new CRC32 error detection [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_PEAK_LEVEL        4    // uint32 nPeakLevel after the header [OBSOLETE]
#define MAC_FORMAT_FLAG_24_BIT                8    // is 24-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS    16    // has the number of seek elements after the peak level
#define MAC_FORMAT_FLAG_CREATE_WAV_HEADER    32    // create the wave header on decompression (not stored)


/* Special frame codes:

   MONO_SILENCE - All PCM samples in frame are zero (mono streams only)
   LEFT_SILENCE - All PCM samples for left channel in frame are zero (stereo streams)
   RIGHT_SILENCE - All PCM samples for left channel in frame are zero (stereo streams)
   PSEUDO_STEREO - Left and Right channels are identical

*/

#define APE_FRAMECODE_MONO_SILENCE    1
#define APE_FRAMECODE_STEREO_SILENCE  3
#define APE_FRAMECODE_PSEUDO_STEREO   4

#define HISTORY_SIZE 512
#define PREDICTOR_ORDER 8

struct predictor_t
{
    /* Adaption co-efficients */
    int32_t coeffsA[4];
    int32_t coeffsB[5];

    /* Filter histories */
    int32_t historybuffer[HISTORY_SIZE + PREDICTOR_ORDER * 4];
    int32_t* delayA;
    int32_t* delayB;
    int32_t* adaptcoeffsA;
    int32_t* adaptcoeffsB;

    int32_t lastA;

    int32_t filterA;
    int32_t filterB;
};

struct ape_ctx_t
{
    /* Derived fields */
    uint32_t      junklength;
    uint32_t      firstframe;
    uint32_t      totalsamples;

    /* Info from Descriptor Block */
    char          magic[4];
    int16_t       fileversion;
    int16_t       padding1;
    uint32_t      descriptorlength;
    uint32_t      headerlength;
    uint32_t      seektablelength;
    uint32_t      wavheaderlength;
    uint32_t      audiodatalength;
    uint32_t      audiodatalength_high;
    uint32_t      wavtaillength;
    uint8_t       md5[16];

    /* Info from Header Block */
    uint16_t      compressiontype;
    uint16_t      formatflags;
    uint32_t      blocksperframe;
    uint32_t      finalframeblocks;
    uint32_t      totalframes;
    uint16_t      bps;
    uint16_t      channels;
    uint32_t      samplerate;

    /* Seektable */
    uint32_t*     seektable;

    /* Decoder state */
    uint32_t      CRC;
    int           frameflags;
    int           currentframeblocks;
    int           blocksdecoded;
    struct predictor_t predictorY;
    struct predictor_t predictorX;
};

int ape_parseheader(int fd, struct ape_ctx_t* ape_ctx);
int ape_parseheaderbuf(unsigned char* buf, struct ape_ctx_t* ape_ctx);
void ape_dumpinfo(struct ape_ctx_t* ape_ctx);

#endif
