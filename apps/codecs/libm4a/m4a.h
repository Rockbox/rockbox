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
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _M4A_H
#define _M4A_H

#include <codecs.h>
#include <inttypes.h>

typedef struct {
  struct codec_api* ci;
  int eof;
} stream_t;

typedef uint32_t fourcc_t;

typedef struct
{
    uint16_t num_channels;
    uint16_t sample_size;
    uint32_t sample_rate;
    fourcc_t format;
    void *buf;

    struct {
        uint32_t sample_count;
        uint32_t sample_duration;
    } *time_to_sample;
    uint32_t num_time_to_samples;

    uint32_t *sample_byte_size;
    uint32_t num_sample_byte_sizes;

    uint32_t codecdata_len;
    void *codecdata;

    int mdat_offset;
    uint32_t mdat_len;
#if 0
    void *mdat;
#endif
} demux_res_t;

int qtmovie_read(stream_t *stream, demux_res_t *demux_res);

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ( \
    ( (int32_t)(char)(ch0) << 24 ) | \
    ( (int32_t)(char)(ch1) << 16 ) | \
    ( (int32_t)(char)(ch2) << 8 ) | \
    ( (int32_t)(char)(ch3) ) )
#endif

#ifndef SLPITFOURCC
/* splits it into ch0, ch1, ch2, ch3 - use for printf's */
#define SPLITFOURCC(code) \
    (char)((int32_t)code >> 24), \
    (char)((int32_t)code >> 16), \
    (char)((int32_t)code >> 8), \
    (char)code
#endif

void stream_read(stream_t *stream, size_t len, void *buf);

int32_t stream_tell(stream_t *stream);
int32_t stream_read_int32(stream_t *stream);
uint32_t stream_read_uint32(stream_t *stream);

int16_t stream_read_int16(stream_t *stream);
uint16_t stream_read_uint16(stream_t *stream);

int8_t stream_read_int8(stream_t *stream);
uint8_t stream_read_uint8(stream_t *stream);

void stream_skip(stream_t *stream, size_t skip);

int stream_eof(stream_t *stream);

void stream_create(stream_t *stream,struct codec_api* ci);
int get_sample_info(demux_res_t *demux_res, uint32_t samplenum,
                    uint32_t *sample_duration,
                    uint32_t *sample_byte_size);
unsigned int alac_seek (demux_res_t* demux_res,
                        stream_t* stream,
                        unsigned int sample_loc, 
                        uint32_t* samplesdone, int* currentblock);

#endif /* STREAM_H */
