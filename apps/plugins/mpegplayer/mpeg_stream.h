/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Stream definitions for MPEG
 *
 * Copyright (c) 2007 Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef MPEG_STREAM_H
#define MPEG_STREAM_H

/* Codes for various header byte sequences - MSB represents lowest memory
   address */
#define PACKET_START_CODE_PREFIX    0x00000100ul
#define END_CODE                    0x000001b9ul
#define PACK_START_CODE             0x000001baul
#define SYSTEM_HEADER_START_CODE    0x000001bbul

/* p = base pointer, b0 - b4 = byte offsets from p */
/* We only care about the MS 32 bits of the 33 and so the ticks are 45kHz */
#define TS_FROM_HEADER(p, b0) \
        ((uint32_t)((((p)[(b0)+0] & 0x0e) << 28) | \
                    (((p)[(b0)+1]       ) << 21) | \
                    (((p)[(b0)+2] & 0xfe) << 13) | \
                    (((p)[(b0)+3]       ) <<  6) | \
                    (((p)[(b0)+4]       ) >>  2)))

#define TS_CHECK_MARKERS(p, b0) \
        (((((p)[(b0)+0] & 0x01) << 2) |         \
          (((p)[(b0)+2] & 0x01) << 1) |         \
          (((p)[(b0)+4] & 0x01)     )) == 0x07)

/* Get the SCR in our 45kHz ticks. Ignore the 9-bit extension */
#define MPEG2_PACK_HEADER_SCR(p, b0) \
    ((uint32_t)((((p)[(b0)+0] & 0x38) << 26) | \
                (((p)[(b0)+0] & 0x03) << 27) | \
                (((p)[(b0)+1]       ) << 19) | \
                (((p)[(b0)+2] & 0xf8) << 11) | \
                (((p)[(b0)+2] & 0x03) << 12) | \
                (((p)[(b0)+3]       ) <<  4) | \
                (((p)[(b0)+4]       ) >>  4)))

#define MPEG2_CHECK_PACK_SCR_MARKERS(ph, b0) \
         (((((ph)[(b0)+0] & 0x04)     ) |        \
           (((ph)[(b0)+2] & 0x04) >> 1) |        \
           (((ph)[(b0)+4] & 0x04) >> 2)) == 0x07)

#define INVALID_TIMESTAMP (~(uint32_t)0)
#define MAX_TIMESTAMP     (INVALID_TIMESTAMP-1)
#define TS_SECOND (45000) /* Timestamp ticks per second */
#define TC_SECOND (27000000) /* MPEG timecode ticks per second */

/* These values immediately follow the start code prefix '00 00 01' */

/* Video start codes */
#define MPEG_START_PICTURE              0x00
#define MPEG_START_SLICE_FIRST          0x01
#define MPEG_START_SLICE_LAST           0xaf
#define MPEG_START_RESERVED_1           0xb0
#define MPEG_START_RESERVED_2           0xb1
#define MPEG_START_USER_DATA            0xb2
#define MPEG_START_SEQUENCE_HEADER      0xb3
#define MPEG_START_SEQUENCE_ERROR       0xb4
#define MPEG_START_EXTENSION            0xb5
#define MPEG_START_RESERVED_3           0xb6
#define MPEG_START_SEQUENCE_END         0xb7
#define MPEG_START_GOP                  0xb8

/* Stream IDs */
#define MPEG_STREAM_PROGRAM_END         0xb9
#define MPEG_STREAM_PACK_HEADER         0xba
#define MPEG_STREAM_SYSTEM_HEADER       0xbb
#define MPEG_STREAM_PROGRAM_STREAM_MAP  0xbc
#define MPEG_STREAM_PRIVATE_1           0xbd
#define MPEG_STREAM_PADDING             0xbe
#define MPEG_STREAM_PRIVATE_2           0xbf
#define MPEG_STREAM_AUDIO_FIRST         0xc0
#define MPEG_STREAM_AUDIO_LAST          0xcf
#define MPEG_STREAM_VIDEO_FIRST         0xe0
#define MPEG_STREAM_VIDEO_LAST          0xef
#define MPEG_STREAM_ECM                 0xf0
#define MPEG_STREAM_EMM                 0xf1
/* ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex A or
 * ISO/IEC 13818-6_DSMCC_stream */
#define MPEG_STREAM_MISC_1              0xf2
/* ISO/IEC_13522_stream */
#define MPEG_STREAM_MISC_2              0xf3
/* ITU-T Rec. H.222.1 type A - E */
#define MPEG_STREAM_MISC_3              0xf4
#define MPEG_STREAM_MISC_4              0xf5
#define MPEG_STREAM_MISC_5              0xf6
#define MPEG_STREAM_MISC_6              0xf7
#define MPEG_STREAM_MISC_7              0xf8
#define MPEG_STREAM_ANCILLARY           0xf9
#define MPEG_STREAM_RESERVED_FIRST      0xfa
#define MPEG_STREAM_RESERVED_LAST       0xfe
/* Program stream directory */
#define MPEG_STREAM_PROGRAM_DIRECTORY   0xff

#define STREAM_IS_AUDIO(s) (((s) & 0xf0) == 0xc0)
#define STREAM_IS_VIDEO(s) (((s) & 0xf0) == 0xe0)

#define MPEG_MAX_PACKET_SIZE (64*1024+16)

/* Largest MPEG audio frame - MPEG1, Layer II, 384kbps, 32kHz, pad */
#define MPA_MAX_FRAME_SIZE 1729

#endif /* MPEG_STREAM_H */
