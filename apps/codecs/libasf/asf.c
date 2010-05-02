/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
 *
 * ASF parsing code based on libasf by Juho Vähä-Herttua
 * http://code.google.com/p/libasf/  libasf itself was based on the ASF
 * parser in VLC - http://www.videolan.org/
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
#include <inttypes.h>
#include "codeclib.h"
#include "asf.h"

/* Read an unaligned 32-bit little endian long from buffer. */
static unsigned long get_long_le(void* buf)
{
    unsigned char* p = (unsigned char*) buf;

    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

/* Read an unaligned 16-bit little endian short from buffer. */
static unsigned short get_short_le(void* buf)
{
    unsigned char* p = (unsigned char*) buf;

    return p[0] | (p[1] << 8);
}

#define GETLEN2b(bits) (((bits) == 0x03) ? 4 : bits)

#define GETVALUE2b(bits, data) \
        (((bits) != 0x03) ? ((bits) != 0x02) ? ((bits) != 0x01) ? \
         0 : *(data) : get_short_le(data) : get_long_le(data))

int asf_read_packet(uint8_t** audiobuf, int* audiobufsize, int* packetlength, 
                    asf_waveformatex_t* wfx, struct codec_api* ci)
{
    uint8_t tmp8, packet_flags, packet_property;
    int stream_id;
    int ec_length, opaque_data, ec_length_type;
    int datalen;
    uint8_t data[18];
    uint8_t* datap;
    uint32_t length;
    uint32_t padding_length;
    uint32_t send_time;
    uint16_t duration;
    uint16_t payload_count;
    int payload_length_type;
    uint32_t payload_hdrlen;
    int payload_datalen;
    int multiple;
    uint32_t replicated_length;
    uint32_t media_object_number;
    uint32_t media_object_offset;
    uint32_t bytesread = 0;
    uint8_t* buf;
    size_t bufsize;
    int i;
    /*DEBUGF("Reading new packet at %d bytes ", (int)ci->curpos);*/

    if (ci->read_filebuf(&tmp8, 1) == 0) {
        return ASF_ERROR_EOF;
    }
    bytesread++;

    /* TODO: We need a better way to detect endofstream */
    if (tmp8 != 0x82) {
    DEBUGF("Read failed:  packet did not sync\n");
    return -1;
    }


    if (tmp8 & 0x80) {
       ec_length = tmp8 & 0x0f;
       opaque_data = (tmp8 >> 4) & 0x01;
       ec_length_type = (tmp8 >> 5) & 0x03;

       if (ec_length_type != 0x00 || opaque_data != 0 || ec_length != 0x02) {
            DEBUGF("incorrect error correction flags\n");
            return ASF_ERROR_INVALID_VALUE;
       }

       /* Skip ec_data */
       ci->advance_buffer(ec_length);
       bytesread += ec_length;
    } else {
        ec_length = 0;
    }

    if (ci->read_filebuf(&packet_flags, 1) == 0) { return ASF_ERROR_EOF; }
    if (ci->read_filebuf(&packet_property, 1) == 0) { return ASF_ERROR_EOF; }
    bytesread += 2;

    datalen = GETLEN2b((packet_flags >> 1) & 0x03) +
              GETLEN2b((packet_flags >> 3) & 0x03) +
              GETLEN2b((packet_flags >> 5) & 0x03) + 6;

#if 0
    if (datalen > sizeof(data)) {
        DEBUGF("Unexpectedly long datalen in data - %d\n",datalen);
        return ASF_ERROR_OUTOFMEM;
    }
#endif

    if (ci->read_filebuf(data, datalen) == 0) {
        return ASF_ERROR_EOF;
    }

    bytesread += datalen;

    datap = data;
    length = GETVALUE2b((packet_flags >> 5) & 0x03, datap);
    datap += GETLEN2b((packet_flags >> 5) & 0x03);
    /* sequence value is not used */
    GETVALUE2b((packet_flags >> 1) & 0x03, datap);
    datap += GETLEN2b((packet_flags >> 1) & 0x03);
    padding_length = GETVALUE2b((packet_flags >> 3) & 0x03, datap);
    datap += GETLEN2b((packet_flags >> 3) & 0x03);
    send_time = get_long_le(datap);
    datap += 4;
    duration = get_short_le(datap);
    datap += 2;
    /*DEBUGF("and duration %d ms\n", duration);*/

    /* this is really idiotic, packet length can (and often will) be
     * undefined and we just have to use the header packet size as the size
     * value */
    if (!((packet_flags >> 5) & 0x03)) {
         length = wfx->packet_size;
    }

    /* this is also really idiotic, if packet length is smaller than packet
     * size, we need to manually add the additional bytes into padding length
     */
    if (length < wfx->packet_size) {
        padding_length += wfx->packet_size - length;
        length = wfx->packet_size;
    }

    if (length > wfx->packet_size) {
        DEBUGF("packet with too big length value\n");
        return ASF_ERROR_INVALID_LENGTH;
    }

    /* check if we have multiple payloads */
    if (packet_flags & 0x01) {
        if (ci->read_filebuf(&tmp8, 1) == 0) {
            return ASF_ERROR_EOF;
        }
        payload_count = tmp8 & 0x3f;
        payload_length_type = (tmp8 >> 6) & 0x03;
        bytesread++;
    } else {
        payload_count = 1;
        payload_length_type = 0x02; /* not used */
    }

    if (length < bytesread) {
        DEBUGF("header exceeded packet size, invalid file - length=%d, bytesread=%d\n",(int)length,(int)bytesread);
        /* FIXME: should this be checked earlier? */
        return ASF_ERROR_INVALID_LENGTH;
    }


    /* We now parse the individual payloads, and move all payloads
       belonging to our audio stream to a contiguous block, starting at
       the location of the first payload.
    */

    *audiobuf = NULL;
    *audiobufsize = 0;
    *packetlength = length - bytesread;

    buf = ci->request_buffer(&bufsize, length);
    datap = buf;

    if (bufsize != length) {
        /* This should only happen with packets larger than 32KB (the
           guard buffer size).  All the streams I've seen have
           relatively small packets less than about 8KB), but I don't
           know what is expected.
        */
        DEBUGF("Could not read packet (requested %d bytes, received %d), curpos=%d, aborting\n",
               (int)length,(int)bufsize,(int)ci->curpos);
        return -1;
    }

    for (i=0; i<payload_count; i++) {
        stream_id = datap[0]&0x7f;
        datap++;
        bytesread++;

        payload_hdrlen = GETLEN2b(packet_property & 0x03) +
                         GETLEN2b((packet_property >> 2) & 0x03) +
                         GETLEN2b((packet_property >> 4) & 0x03);

        //DEBUGF("payload_hdrlen = %d\n",payload_hdrlen);
#if 0
        /* TODO */
        if (payload_hdrlen > size) {
            return ASF_ERROR_INVALID_LENGTH;
        }
#endif
        if (payload_hdrlen > sizeof(data)) {
            DEBUGF("Unexpectedly long datalen in data - %d\n",datalen);
            return ASF_ERROR_OUTOFMEM;
        }

        bytesread += payload_hdrlen;
        media_object_number = GETVALUE2b((packet_property >> 4) & 0x03, datap);
        datap += GETLEN2b((packet_property >> 4) & 0x03);
        media_object_offset = GETVALUE2b((packet_property >> 2) & 0x03, datap);
        datap += GETLEN2b((packet_property >> 2) & 0x03);
        replicated_length = GETVALUE2b(packet_property & 0x03, datap);
        datap += GETLEN2b(packet_property & 0x03);

        /* TODO: Validate replicated_length */
        /* TODO: Is the content of this important for us? */
        datap += replicated_length;
        bytesread += replicated_length;

        multiple = packet_flags & 0x01;


        if (multiple) {
            int x;

            x = GETLEN2b(payload_length_type);

            if (x != 2) {
                /* in multiple payloads datalen should be a word */
                return ASF_ERROR_INVALID_VALUE;
            }

#if 0
            if (skip + tmp > datalen) {
                /* not enough data */
                return ASF_ERROR_INVALID_LENGTH;
            }
#endif
            payload_datalen = GETVALUE2b(payload_length_type, datap);
            datap += x;
            bytesread += x;
        } else {
            payload_datalen = length - bytesread - padding_length;
        }

        if (replicated_length==1)
            datap++;

        if (stream_id == wfx->audiostream)
        {
            if (*audiobuf == NULL) {
                /* The first payload can stay where it is */
                *audiobuf = datap;
                *audiobufsize = payload_datalen;
            } else {
                /* The second and subsequent payloads in this packet
                   that belong to the audio stream need to be moved to be
                   contiguous with the first payload.
                */
                memmove(*audiobuf + *audiobufsize, datap, payload_datalen);
                *audiobufsize += payload_datalen;
            }
        }
        datap += payload_datalen;
        bytesread += payload_datalen;
    }

    if (*audiobuf != NULL)
        return 1;
    else
        return 0;
}


int asf_get_timestamp(int *duration, struct codec_api* ci)
{
    uint8_t tmp8, packet_flags, packet_property;
    int ec_length, opaque_data, ec_length_type;
    int datalen;
    uint8_t data[18];
    uint8_t* datap;
    uint32_t length;
    uint32_t padding_length;
    uint32_t send_time;
    static int packet_count = 0;

    uint32_t bytesread = 0;
    packet_count++;
    if (ci->read_filebuf(&tmp8, 1) == 0) {
        DEBUGF("ASF ERROR (EOF?)\n");
        return ASF_ERROR_EOF;
    }
    bytesread++;

    /* TODO: We need a better way to detect endofstream */
    if (tmp8 != 0x82) {
        DEBUGF("Get timestamp:  Detected end of stream\n");
        return ASF_ERROR_EOF;
    }


    if (tmp8 & 0x80) {
        ec_length = tmp8 & 0x0f;
        opaque_data = (tmp8 >> 4) & 0x01;
        ec_length_type = (tmp8 >> 5) & 0x03;

        if (ec_length_type != 0x00 || opaque_data != 0 || ec_length != 0x02) {
             DEBUGF("incorrect error correction flags\n");
             return ASF_ERROR_INVALID_VALUE;
        }

        /* Skip ec_data */
        ci->advance_buffer(ec_length);
        bytesread += ec_length;
    } else {
        ec_length = 0;
    }

    if (ci->read_filebuf(&packet_flags, 1) == 0) {
        DEBUGF("Detected end of stream 2\n");
        return ASF_ERROR_EOF;
    }

    if (ci->read_filebuf(&packet_property, 1) == 0) {
        DEBUGF("Detected end of stream3\n");
        return ASF_ERROR_EOF;
    }
    bytesread += 2;

    datalen = GETLEN2b((packet_flags >> 1) & 0x03) +
              GETLEN2b((packet_flags >> 3) & 0x03) +
              GETLEN2b((packet_flags >> 5) & 0x03) + 6;

    if (ci->read_filebuf(data, datalen) == 0) {
        DEBUGF("Detected end of stream4\n");
        return ASF_ERROR_EOF;
    }

    bytesread += datalen;

    datap = data;
    length = GETVALUE2b((packet_flags >> 5) & 0x03, datap);
    datap += GETLEN2b((packet_flags >> 5) & 0x03);

    /* sequence value is not used */
    GETVALUE2b((packet_flags >> 1) & 0x03, datap);
    datap += GETLEN2b((packet_flags >> 1) & 0x03);
    padding_length = GETVALUE2b((packet_flags >> 3) & 0x03, datap);
    datap += GETLEN2b((packet_flags >> 3) & 0x03);
    send_time = get_long_le(datap);
    datap += 4;
    *duration = get_short_le(datap);

    /*the asf_get_timestamp function advances us 12-13 bytes past the packet start,
      need to undo this here so that we stay synced with the packet*/
    ci->seek_buffer(ci->curpos-bytesread);

    return send_time;
}
