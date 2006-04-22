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
#include <inttypes.h>  /* Needed by a52.h */
#include <codecs/liba52/config-a52.h>
#include <codecs/liba52/a52.h>

CODEC_HEADER

#define BUFFER_SIZE 4096

#define A52_SAMPLESPERFRAME (6*256)

struct codec_api *ci;

static a52_state_t *state;
unsigned long samplesdone;
unsigned long frequency;

/* used outside liba52 */
static uint8_t buf[3840] IBSS_ATTR;

void output_audio(sample_t *samples)
{
    do {
        ci->yield();
    } while (!ci->pcmbuf_insert_split(&samples[0], &samples[256], 
                                      256*sizeof(sample_t)));
}

void a52_decode_data(uint8_t *start, uint8_t *end)
{
    static uint8_t *bufptr = buf;
    static uint8_t *bufpos = buf + 7;
    /*
     * sample_rate and flags are static because this routine could
     * exit between the a52_syncinfo() and the ao_setup(), and we want
     * to have the same values when we get back !
     */
    static int sample_rate;
    static int flags;
    int bit_rate;
    int len;

    while (1) {
        len = end - start;
        if (!len)
            break;
        if (len > bufpos - bufptr)
            len = bufpos - bufptr;
        memcpy(bufptr, start, len);
        bufptr += len;
        start += len;
        if (bufptr == bufpos) {
            if (bufpos == buf + 7) {
                int length;

                length = a52_syncinfo(buf, &flags, &sample_rate, &bit_rate);
                if (!length) {
                    //DEBUGF("skip\n");
                    for (bufptr = buf; bufptr < buf + 6; bufptr++)
                        bufptr[0] = bufptr[1];
                    continue;
                }
                bufpos = buf + length;
            } else {
                /* Unity gain is 1 << 26, and we want to end up on 28 bits
                   of precision instead of the default 30.
                 */
                level_t level = 1 << 24;
                sample_t bias = 0;
                int i;

                /* This is the configuration for the downmixing: */
                flags = A52_STEREO | A52_ADJUST_LEVEL;

                if (a52_frame(state, buf, &flags, &level, bias))
                    goto error;
                a52_dynrng(state, NULL, NULL);
                frequency = sample_rate;

                /* An A52 frame consists of 6 blocks of 256 samples
                   So we decode and output them one block at a time */
                for (i = 0; i < 6; i++) {
                    if (a52_block(state))
                        goto error;
                    output_audio(a52_samples(state));
                    samplesdone += 256;
                }
                ci->set_elapsed(samplesdone/(frequency/1000));
                bufptr = buf;
                bufpos = buf + 7;
                continue;
            error:
                //logf("Error decoding A52 stream\n");
                bufptr = buf;
                bufpos = buf + 7;
            }
        }   
    }
}

#ifdef USE_IRAM
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
extern char iedata[];
extern char iend[];
#endif

/* this is the codec entry point */
enum codec_status codec_start(struct codec_api *api)
{
    size_t n;
    unsigned char *filebuf;
    int sample_loc;
    int retval;

    /* Generic codec initialisation */
    ci = api;

    #ifdef USE_IRAM 
    ci->memcpy(iramstart, iramcopy, iramend - iramstart);
    ci->memset(iedata, 0, iend - iedata);
    #endif

    ci->configure(DSP_DITHER, (bool *)false);
    ci->configure(DSP_SET_STEREO_MODE, (long *)STEREO_NONINTERLEAVED);
    ci->configure(DSP_SET_SAMPLE_DEPTH, (long *)28);
    ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (long *)(1024*128));

next_track:
    if (codec_init(api)) {
        retval = CODEC_ERROR;
        goto exit;
    }

    while (!ci->taginfo_ready)
        ci->yield();
    
    ci->configure(DSP_SET_FREQUENCY, (long *)(ci->id3->frequency));
    
    /* Intialise the A52 decoder and check for success */
    state = a52_init(0);

    /* The main decoding loop */
    samplesdone = 0;
    while (1) {
        if (ci->stop_codec || ci->new_track)
            break;

        if (ci->seek_time) {
            sample_loc = (ci->seek_time - 1)/1000 * ci->id3->frequency;

            if (ci->seek_buffer((sample_loc/A52_SAMPLESPERFRAME)*ci->id3->bytesperframe)) {
                samplesdone = sample_loc;
                ci->set_elapsed(samplesdone/(ci->id3->frequency/1000));
            }
            ci->seek_complete();
        }

        filebuf = ci->request_buffer(&n, BUFFER_SIZE);

        if (n == 0) /* End of Stream */
            break;
  
        a52_decode_data(filebuf, filebuf + n);
        ci->advance_buffer(n);
    }
    retval = CODEC_OK;

    if (ci->request_next_track())
        goto next_track;

exit:
    a52_free(state);
    return retval;
}
