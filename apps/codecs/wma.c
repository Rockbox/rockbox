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

#include "codeclib.h"
#include "libwma/asf.h"
#include "libwma/wmadec.h"

CODEC_HEADER

int packet_count=0;

/* The output buffer containing the decoded samples (channels 0 and 1)
   BLOCK_MAX_SIZE is 2048 (samples) and MAX_CHANNELS is 2.
 */

static uint32_t decoded[BLOCK_MAX_SIZE * MAX_CHANNELS];

/* NOTE: WMADecodeContext is 120152 bytes (on x86) */
static WMADecodeContext wmadec;

enum asf_error_e {
    ASF_ERROR_INTERNAL       = -1,  /* incorrect input to API calls */
    ASF_ERROR_OUTOFMEM       = -2,  /* some malloc inside program failed */
    ASF_ERROR_EOF            = -3,  /* unexpected end of file */
    ASF_ERROR_IO             = -4,  /* error reading or writing to file */
    ASF_ERROR_INVALID_LENGTH = -5,  /* length value conflict in input data */
    ASF_ERROR_INVALID_VALUE  = -6,  /* other value conflict in input data */
    ASF_ERROR_INVALID_OBJECT = -7,  /* ASF object missing or in wrong place */
    ASF_ERROR_OBJECT_SIZE    = -8,  /* invalid ASF object size (too small) */
    ASF_ERROR_SEEKABLE       = -9,  /* file not seekable */
    ASF_ERROR_SEEK           = -10  /* file is seekable but seeking failed */
};

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

static int asf_read_packet(uint8_t** audiobuf, int* audiobufsize, int* packetlength, asf_waveformatex_t* wfx)
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
    DEBUGF("Reading new packet at %d bytes ", (int)ci->curpos);

    if (ci->read_filebuf(&tmp8, 1) == 0) {
        return ASF_ERROR_EOF;
    }
    bytesread++;

    //DEBUGF("tmp8=0x%02x\n",tmp8);
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
    DEBUGF("and duration %d ms\n", duration);

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


 static int get_timestamp(int *duration){

     uint8_t tmp8, packet_flags, packet_property;
     //int stream_id;
     int ec_length, opaque_data, ec_length_type;
     int datalen;
     uint8_t data[18];
     uint8_t* datap;
     uint32_t length;
     uint32_t padding_length;
     uint32_t send_time;

     //uint16_t payload_count;
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

         return ASF_ERROR_EOF; }


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

     if (ci->read_filebuf(&packet_flags, 1) == 0) { DEBUGF("Detected end of stream 2\n"); return ASF_ERROR_EOF; }
     if (ci->read_filebuf(&packet_property, 1) == 0) {DEBUGF("Detected end of stream3\n"); return ASF_ERROR_EOF; }
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

     return send_time;
 }

/*entry point for seeks*/
 static int seek(int ms, asf_waveformatex_t* wfx){
    int time, duration, delta, temp, count=0;

    /*estimate packet number from bitrate*/
    int initial_packet = ci->curpos/wfx->packet_size;
    int packet_num = (ms*(wfx->bitrate>>3))/wfx->packet_size/1000;
    int last_packet = ci->id3->filesize / wfx->packet_size;

    if(packet_num > last_packet){
        packet_num = last_packet;
    }
    /*calculate byte address of the start of that packet*/
    int packet_offset = packet_num*wfx->packet_size;

    /*seek to estimated packet*/
    ci->seek_buffer(ci->id3->first_frame_offset+packet_offset);
    temp = ms;
    while(1){
        /*for very large files it can be difficult and unimportant to find the exact packet*/
        count++;

        /*check the time stamp of our packet*/
        time = get_timestamp(&duration);
        DEBUGF("seeked to %d ms with duration %d\n", time, duration);

        if(time < 0){
            /*unknown error, try to recover*/
            DEBUGF("UKNOWN SEEK ERROR\n");
            ci->seek_buffer(ci->id3->first_frame_offset+initial_packet*wfx->packet_size);
            return ms;
        }

        if((time+duration>=ms && time<=ms) || count > 10){
            /*the get_timestamp function advances us 12 bytes past the packet start*/
            ci->seek_buffer(ci->curpos-12);
            DEBUGF("Found our packet! Now at %d packet\n", packet_num);
            return time;
        }else {
            /*seek again*/
            delta = ms-time;
            /*estimate new packet number from bitrate and our current position*/
            temp += delta;
            packet_num = ((temp/1000)*(wfx->bitrate>>3) - (wfx->packet_size>>1))/wfx->packet_size;  //round down!
            packet_offset = packet_num*wfx->packet_size;
            ci->seek_buffer(ci->id3->first_frame_offset+packet_offset);
        }
     }
 }



/* this is the codec entry point */
enum codec_status codec_main(void)
{
    uint32_t elapsedtime;
    int retval;
    asf_waveformatex_t wfx;
    uint32_t currentframe;
    size_t resume_offset;
    int i;
    int wmares, res;
    uint8_t* audiobuf;
    int audiobufsize;
    int packetlength;
    int errcount = 0;

    /* Generic codec initialisation */
    ci->configure(CODEC_SET_FILEBUF_WATERMARK, 1024*512);

    ci->configure(DSP_SET_SAMPLE_DEPTH, 30);

next_track:

    /* Wait for the metadata to be read */
    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    retval = CODEC_OK;

    /* Remember the resume position - when the codec is opened, the
       playback engine will reset it. */
    resume_offset = ci->id3->offset;

    if (codec_init()) {
        LOGF("WMA: Error initialising codec\n");
        retval = CODEC_ERROR;
        goto exit;
    }

    /* Copy the format metadata we've stored in the id3 TOC field.  This
       saves us from parsing it again here. */
    memcpy(&wfx, ci->id3->toc, sizeof(wfx));

    if (wma_decode_init(&wmadec,&wfx) < 0) {
        LOGF("WMA: Unsupported or corrupt file\n");
        retval = CODEC_ERROR;
        goto exit;
    }

    /* Now advance the file position to the first frame */
    ci->seek_buffer(ci->id3->first_frame_offset);

    ci->configure(DSP_SWITCH_FREQUENCY, wfx.rate);
    ci->configure(DSP_SET_STEREO_MODE, wfx.channels == 1 ?
                  STEREO_MONO : STEREO_INTERLEAVED);
    codec_set_replaygain(ci->id3);

    /* The main decoding loop */

    currentframe = 0;
    elapsedtime = 0;

    DEBUGF("**************** IN WMA.C ******************\n");
    wma_decode_init(&wmadec,&wfx);

    res = 1;
    while (res >= 0)
    {
        ci->yield();
        if (ci->stop_codec || ci->new_track) {
            goto done;
        }

        /* Deal with any pending seek requests */
        if (ci->seek_time){

            if (ci->seek_time == 1) {
                ci->seek_complete();
                goto next_track; /* Pretend you never saw this... */
            }

            elapsedtime = seek(ci->seek_time, &wfx);
            if(elapsedtime < 1){
                ci->seek_complete();
                goto next_track;
            }
            DEBUGF("Seek returned %d\n", (int)time);
            ci->set_elapsed(elapsedtime);

            /*flush the wma decoder state*/
            wmadec.last_superframe_len = 0;
            wmadec.last_bitoffset = 0;
            ci->seek_complete();
        }
        errcount = 0;
new_packet:
        res = asf_read_packet(&audiobuf, &audiobufsize, &packetlength, &wfx);
        if(res < 0){

        /* We'll try to recover from a parse error a certain number of
        * times. If we succeed, the error counter will be reset.
        */

        errcount++;
        DEBUGF("WMA decode error %d, errcount %d\n",wmares, errcount);
        if (errcount > 5) {
            goto done;
        } else {
            ci->advance_buffer(packetlength);
            goto new_packet;
        }

        }else if (res > 0) {
            wma_decode_superframe_init(&wmadec,
                                       audiobuf, audiobufsize);

            for (i=0; i < wmadec.nb_frames; i++)
            {
                wmares = wma_decode_superframe_frame(&wmadec,
                                                     decoded,
                                                     audiobuf, audiobufsize);

                ci->yield ();

                 if (wmares < 0){
                    /* Do the above, but for errors in decode.
                     */
                    errcount++;
                    DEBUGF("WMA decode error %d, errcount %d\n",wmares, errcount);
                    if (errcount > 5) {
                        goto done;
                    } else {
                        ci->advance_buffer(packetlength);
                        goto new_packet;
                    }
                } else if (wmares > 0) {
                    ci->pcmbuf_insert(decoded, NULL, wmares);
                    elapsedtime += (wmares*10)/(wfx.rate/100);
                    ci->set_elapsed(elapsedtime);
                }
                ci->yield();
            }
        }

        ci->advance_buffer(packetlength);
    }
    retval = CODEC_OK;

done:
    LOGF("WMA: Decoded %ld samples\n",elapsedtime*wfx.rate/1000);

    if (ci->request_next_track())
        goto next_track;
exit:
    return retval;
}
