/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
 * Copyright (C) 2009 Yoshihisa Uchida
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "codeclib.h"
#include "support_formats.h"

/*
 * ITU-T G.711 A-law mu-law
 */

static const int16_t alaw2linear16[256] ICONST_ATTR = {
     -5504,   -5248,   -6016,   -5760,   -4480,   -4224,   -4992,
     -4736,   -7552,   -7296,   -8064,   -7808,   -6528,   -6272,
     -7040,   -6784,   -2752,   -2624,   -3008,   -2880,   -2240,
     -2112,   -2496,   -2368,   -3776,   -3648,   -4032,   -3904,
     -3264,   -3136,   -3520,   -3392,  -22016,  -20992,  -24064,
    -23040,  -17920,  -16896,  -19968,  -18944,  -30208,  -29184,
    -32256,  -31232,  -26112,  -25088,  -28160,  -27136,  -11008,
    -10496,  -12032,  -11520,   -8960,   -8448,   -9984,   -9472,
    -15104,  -14592,  -16128,  -15616,  -13056,  -12544,  -14080,
    -13568,    -344,    -328,    -376,    -360,    -280,    -264,
      -312,    -296,    -472,    -456,    -504,    -488,    -408,
      -392,    -440,    -424,     -88,     -72,    -120,    -104,
       -24,      -8,     -56,     -40,    -216,    -200,    -248,
      -232,    -152,    -136,    -184,    -168,   -1376,   -1312,
     -1504,   -1440,   -1120,   -1056,   -1248,   -1184,   -1888,
     -1824,   -2016,   -1952,   -1632,   -1568,   -1760,   -1696,
      -688,    -656,    -752,    -720,    -560,    -528,    -624,
      -592,    -944,    -912,   -1008,    -976,    -816,    -784,
      -880,    -848,    5504,    5248,    6016,    5760,    4480,
      4224,    4992,    4736,    7552,    7296,    8064,    7808,
      6528,    6272,    7040,    6784,    2752,    2624,    3008,
      2880,    2240,    2112,    2496,    2368,    3776,    3648,
      4032,    3904,    3264,    3136,    3520,    3392,   22016,
     20992,   24064,   23040,   17920,   16896,   19968,   18944,
     30208,   29184,   32256,   31232,   26112,   25088,   28160,
     27136,   11008,   10496,   12032,   11520,    8960,    8448,
      9984,    9472,   15104,   14592,   16128,   15616,   13056,
     12544,   14080,   13568,     344,     328,     376,     360,
       280,     264,     312,     296,     472,     456,     504,
       488,     408,     392,     440,     424,      88,      72,
       120,     104,      24,       8,      56,      40,     216,
       200,     248,     232,     152,     136,     184,     168,
      1376,    1312,    1504,    1440,    1120,    1056,    1248,
      1184,    1888,    1824,    2016,    1952,    1632,    1568,
      1760,    1696,     688,     656,     752,     720,     560,
       528,     624,     592,     944,     912,    1008,     976,
       816,     784,     880,     848
};

static const int16_t ulaw2linear16[256] ICONST_ATTR = {
    -32124,  -31100,  -30076,  -29052,  -28028,  -27004,  -25980,
    -24956,  -23932,  -22908,  -21884,  -20860,  -19836,  -18812,
    -17788,  -16764,  -15996,  -15484,  -14972,  -14460,  -13948,
    -13436,  -12924,  -12412,  -11900,  -11388,  -10876,  -10364,
     -9852,   -9340,   -8828,   -8316,   -7932,   -7676,   -7420,
     -7164,   -6908,   -6652,   -6396,   -6140,   -5884,   -5628,
     -5372,   -5116,   -4860,   -4604,   -4348,   -4092,   -3900,
     -3772,   -3644,   -3516,   -3388,   -3260,   -3132,   -3004,
     -2876,   -2748,   -2620,   -2492,   -2364,   -2236,   -2108,
     -1980,   -1884,   -1820,   -1756,   -1692,   -1628,   -1564,
     -1500,   -1436,   -1372,   -1308,   -1244,   -1180,   -1116,
     -1052,    -988,    -924,    -876,    -844,    -812,    -780,
      -748,    -716,    -684,    -652,    -620,    -588,    -556,
      -524,    -492,    -460,    -428,    -396,    -372,    -356,
      -340,    -324,    -308,    -292,    -276,    -260,    -244,
      -228,    -212,    -196,    -180,    -164,    -148,    -132,
      -120,    -112,    -104,     -96,     -88,     -80,     -72,
       -64,     -56,     -48,     -40,     -32,     -24,     -16,
        -8,       0,   32124,   31100,   30076,   29052,   28028,
     27004,   25980,   24956,   23932,   22908,   21884,   20860,
     19836,   18812,   17788,   16764,   15996,   15484,   14972,
     14460,   13948,   13436,   12924,   12412,   11900,   11388,
     10876,   10364,    9852,    9340,    8828,    8316,    7932,
      7676,    7420,    7164,    6908,    6652,    6396,    6140,
      5884,    5628,    5372,    5116,    4860,    4604,    4348,
      4092,    3900,    3772,    3644,    3516,    3388,    3260,
      3132,    3004,    2876,    2748,    2620,    2492,    2364,
      2236,    2108,    1980,    1884,    1820,    1756,    1692,
      1628,    1564,    1500,    1436,    1372,    1308,    1244,
      1180,    1116,    1052,     988,     924,     876,     844,
       812,     780,     748,     716,     684,     652,     620,
       588,     556,     524,     492,     460,     428,     396,
       372,     356,     340,     324,     308,     292,     276,
       260,     244,     228,     212,     196,     180,     164,
       148,     132,     120,     112,     104,      96,      88,
        80,      72,      64,      56,      48,      40,      32,
        24,      16,       8,       0
};

static struct pcm_format *fmt;

static bool set_format(struct pcm_format *format)
{
    fmt = format;

    if (fmt->bitspersample != 8)
    {
        DEBUGF("CODEC_ERROR: alaw and mulaw must have 8 bitspersample: %d\n",
                             fmt->bitspersample);
        return false;
    }

    if (fmt->totalsamples == 0)
    {
        fmt->bytespersample = 1;
        fmt->totalsamples = fmt->numbytes / (fmt->bytespersample * fmt->channels);
    }

    fmt->samplesperblock = fmt->blockalign / (fmt->bytespersample * fmt->channels);

    /* chunksize = about 1/50[sec] data */
    fmt->chunksize = (ci->id3->frequency / (50 * fmt->samplesperblock))
                                         * fmt->blockalign;

    return true;
}

static struct pcm_pos *get_seek_pos(long seek_time,
                                    uint8_t *(*read_buffer)(size_t *realsize))
{
    static struct pcm_pos newpos;
    uint32_t newblock = ((uint64_t)seek_time * ci->id3->frequency)
                              / (1000LL * fmt->samplesperblock);

    (void)read_buffer;
    newpos.pos     = newblock * fmt->blockalign;
    newpos.samples = newblock * fmt->samplesperblock;
    return &newpos;
}

static int decode_alaw(const uint8_t *inbuf, size_t inbufsize,
                       int32_t *outbuf, int *outbufsize)
{
    uint32_t i;

    for (i = 0; i < inbufsize; i++)
        outbuf[i] = alaw2linear16[inbuf[i]] << (PCM_OUTPUT_DEPTH - 16);

    *outbufsize = (fmt->channels == 2) ? (inbufsize >> 1) : inbufsize;

    return CODEC_OK;
}

static int decode_mulaw(const uint8_t *inbuf, size_t inbufsize,
                        int32_t *outbuf, int *outbufsize)
{
    uint32_t i;

    for (i = 0; i < inbufsize; i++)
        outbuf[i] = ulaw2linear16[inbuf[i]] << (PCM_OUTPUT_DEPTH - 16);

    *outbufsize = (fmt->channels == 2) ? (inbufsize >> 1) : inbufsize;

    return CODEC_OK;
}

static const struct pcm_codec alaw_codec = {
                                               set_format,
                                               get_seek_pos,
                                               decode_alaw,
                                           };

static const struct pcm_codec mulaw_codec = {
                                                set_format,
                                                get_seek_pos,
                                                decode_mulaw,
                                            };

const struct pcm_codec *get_itut_g711_alaw_codec(void)
{
    return &alaw_codec;
}

const struct pcm_codec *get_itut_g711_mulaw_codec(void)
{
    return &mulaw_codec;
}

