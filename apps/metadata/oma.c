/*
 * Sony OpenMG (OMA) demuxer
 *
 * Copyright (c) 2008 Maxim Poliakovski
 *               2008 Benjamin Larsson
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file oma.c
 * This is a demuxer for Sony OpenMG Music files
 *
 * Known file extensions: ".oma", "aa3"
 * The format of such files consists of three parts:
 * - "ea3" header carrying overall info and metadata.
 * - "EA3" header is a Sony-specific header containing information about
 *   the OpenMG file: codec type (usually ATRAC, can also be MP3 or WMA),
 *   codec specific info (packet size, sample rate, channels and so on)
 *   and DRM related info (file encryption, content id).
 * - Sound data organized in packets follow the EA3 header
 *   (can be encrypted using the Sony DRM!).
 *
 * LIMITATIONS: This version supports only plain (unencrypted) OMA files.
 * If any DRM-protected (encrypted) file is encountered you will get the
 * corresponding error message. Try to remove the encryption using any
 * Sony software (for example SonicStage).
 * CODEC SUPPORT: Only ATRAC3 codec is currently supported!
 */

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "metadata.h"
#include "metadata_parsers.h"

#define EA3_HEADER_SIZE 96

#if 0
#define DEBUGF printf
#else
#define DEBUGF(...)
#endif

/* Various helper macros taken from ffmpeg for reading *
 * and writing buffers with a specified endianess.     */
#   define AV_RB16(x)                           \
    ((((const uint8_t*)(x))[0] << 8) |          \
      ((const uint8_t*)(x))[1])
#   define AV_RB24(x)                           \
    ((((const uint8_t*)(x))[0] << 16) |         \
     (((const uint8_t*)(x))[1] <<  8) |         \
      ((const uint8_t*)(x))[2])
#   define AV_RB32(x)                           \
    ((((const uint8_t*)(x))[0] << 24) |         \
     (((const uint8_t*)(x))[1] << 16) |         \
     (((const uint8_t*)(x))[2] <<  8) |         \
      ((const uint8_t*)(x))[3])
#   define AV_WL32(p, d) do {                   \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
        ((uint8_t*)(p))[3] = (d)>>24;           \
    } while(0)
#   define AV_WL16(p, d) do {                   \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
    } while(0)

/* Different codecs that could be present in a Sony OMA *
 * container file.                                      */
enum {
    OMA_CODECID_ATRAC3  = 0,
    OMA_CODECID_ATRAC3P = 1,
    OMA_CODECID_MP3     = 3,
    OMA_CODECID_LPCM    = 4,
    OMA_CODECID_WMA     = 5,
};

/* FIXME: This functions currently read different file *
 *        parameters required for decoding. It still   *
 *        does not read the metadata - which should be *
 *        present in the ea3 (first) header. The       *
 *        metadata in ea3 is stored as a variation of  *
 *        the ID3v2 metadata format.                   */
int oma_read_header(int fd, struct mp3entry* id3)
{
    static const uint16_t srate_tab[6] = {320,441,480,882,960,0};
    int     ret, ea3_taglen, EA3_pos, jsflag;
    uint32_t codec_params;
    int16_t eid;
    uint8_t buf[EA3_HEADER_SIZE];

    ret = read(fd, buf, 10);
    if (ret != 10)
        return -1;

    ea3_taglen = ((buf[6] & 0x7f) << 21) | ((buf[7] & 0x7f) << 14) | ((buf[8] & 0x7f) << 7) | (buf[9] & 0x7f);

    EA3_pos = ea3_taglen + 10;
    if (buf[5] & 0x10)
        EA3_pos += 10;

    lseek(fd, EA3_pos, SEEK_SET);
    ret = read(fd, buf, EA3_HEADER_SIZE);
    if (ret != EA3_HEADER_SIZE)
        return -1;

    if (memcmp(buf, ((const uint8_t[]){'E', 'A', '3'}),3) || buf[4] != 0 || buf[5] != EA3_HEADER_SIZE) {
        DEBUGF("Couldn't find the EA3 header !\n");
        return -1;
    }

    eid = AV_RB16(&buf[6]);
    if (eid != -1 && eid != -128) {
        DEBUGF("Encrypted file! Eid: %d\n", eid);
        return -1;
    }

    codec_params = AV_RB24(&buf[33]);

    switch (buf[32]) {
        case OMA_CODECID_ATRAC3:
            id3->frequency = srate_tab[(codec_params >> 13) & 7]*100;
            if (id3->frequency != 44100) {
                DEBUGF("Unsupported sample rate, send sample file to developers: %d\n", id3->frequency);
                return -1;
            }

            id3->bytesperframe = (codec_params & 0x3FF) * 8;
            id3->codectype = AFMT_OMA_ATRAC3;
            jsflag = (codec_params >> 17) & 1; /* get stereo coding mode, 1 for joint-stereo */

            id3->bitrate = id3->frequency * id3->bytesperframe * 8 / (1024 * 1000);

            /* fake the atrac3 extradata (wav format, makes stream copy to wav work) */
            /* ATRAC3 expects and extra-data size of 14 bytes for wav format, and *
             * looks for that in the id3v2buf.                                    */
            id3->extradata_size = 14;
            AV_WL16(&id3->id3v2buf[0],  1);             // always 1
            AV_WL32(&id3->id3v2buf[2],  id3->frequency);    // samples rate
            AV_WL16(&id3->id3v2buf[6],  jsflag);        // coding mode
            AV_WL16(&id3->id3v2buf[8],  jsflag);        // coding mode
            AV_WL16(&id3->id3v2buf[10], 1);             // always 1
            AV_WL16(&id3->id3v2buf[12], 0);             // always 0

            id3->channels = 2;
            DEBUGF("sample_rate = %d\n", id3->frequency);
            DEBUGF("frame_size = %d\n", id3->bytesperframe);
            DEBUGF("stereo_coding_mode = %d\n", jsflag);
            break;
        default:
            DEBUGF("Unsupported codec %d!\n",buf[32]);
            return -1;
            break;
    }

    /* Store the the offset of the first audio frame, to be able to seek to it *
     * directly in atrac3_oma.codec.                                           */
    id3->first_frame_offset = EA3_pos + EA3_HEADER_SIZE; 
    return 0;
}

bool get_oma_metadata(int fd, struct mp3entry* id3)
{
    if(oma_read_header(fd, id3) < 0)
        return false;

    /* Currently, there's no means of knowing the duration *
     * directly from the the file so we calculate it.      */
    id3->filesize = filesize(fd);
    id3->length   = ((id3->filesize - id3->first_frame_offset) * 8) / id3->bitrate;
    return true;
}
