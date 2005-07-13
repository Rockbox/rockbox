/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Thom Johansen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "codec.h"
#include "playback.h"
#include "lib/codeclib.h"
#include <codecs/libmusepack/musepack.h>

mpc_decoder decoder;

/* Our implementations of the mpc_reader callback functions. */
mpc_int32_t read_impl(void *data, void *ptr, mpc_int32_t size)
{
    struct codec_api *ci = (struct codec_api*)data;

    return ((mpc_int32_t)(ci->read_filebuf(ptr, size)));
}

bool seek_impl(void *data, mpc_int32_t offset)
{  
    struct codec_api *ci = (struct codec_api*)data;

    /* WARNING: assumes we don't need to skip too far into the past,
       this might not be supported by the buffering layer yet */
    return ci->seek_buffer(offset);
}

mpc_int32_t tell_impl(void *data)
{
    struct codec_api *ci = (struct codec_api*)data;

    return ci->curpos;
}

mpc_int32_t get_size_impl(void *data)
{
    struct codec_api *ci = (struct codec_api*)data;
    
    return ci->filesize;
}

bool canseek_impl(void *data)
{
    (void)data;
    
    /* doesn't much matter, libmusepack ignores this anyway */
    return true;
}

inline int shift_signed(MPC_SAMPLE_FORMAT val, int shift)
{
    if (shift > 0)
        val <<= shift;
    else if (shift < 0)
        val >>= -shift;
    return (int)val;
}

#define OUTPUT_BUFFER_SIZE 65536 /* Must be an integer multiple of 4. */

unsigned char OutputBuffer[OUTPUT_BUFFER_SIZE];
/* temporary, we probably have better use for iram than this */
MPC_SAMPLE_FORMAT sample_buffer[MPC_DECODER_BUFFER_LENGTH] IDATA_ATTR;
unsigned char *OutputPtr;
const unsigned char *OutputBufferEnd;

#ifdef USE_IRAM
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
#endif

/* this is the codec entry point */
enum codec_status codec_start(struct codec_api *api)
{
    struct codec_api *ci = api;
    unsigned short Sample;
    unsigned long samplesdone;
    unsigned long frequency;
    unsigned status = 1;
    unsigned int i;
    mpc_reader reader;

    /* Generic codec inititialisation */
    TEST_CODEC_API(api);

    #ifndef SIMULATOR
    ci->memcpy(iramstart, iramcopy, iramend - iramstart);
    #endif

    ci->configure(CODEC_SET_FILEBUF_LIMIT, (int *)(1024*1024*2));
    ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (int *)(1024*16));

    next_track:
    if (codec_init(api))
        return CODEC_ERROR;

    /* Create a decoder instance */
    reader.read = read_impl;
    reader.seek = seek_impl;
    reader.tell = tell_impl;
    reader.get_size = get_size_impl;
    reader.canseek = canseek_impl;
    reader.data = ci;

    /* read file's streaminfo data */
    mpc_streaminfo info;
    mpc_streaminfo_init(&info);
    if (mpc_streaminfo_read(&info, &reader) != ERROR_CODE_OK)
        return CODEC_ERROR;  
    frequency = info.sample_freq;

    /* instantiate a decoder with our file reader */
    mpc_decoder_setup(&decoder, &reader);
    if (!mpc_decoder_initialize(&decoder, &info))
        return CODEC_ERROR;

    /* Initialise the output buffer. */
    OutputPtr = OutputBuffer;
    OutputBufferEnd = OutputBuffer + OUTPUT_BUFFER_SIZE;

    /* This is the decoding loop. */
    samplesdone = 0;
    while (status != 0) {
        if (ci->stop_codec || ci->reload_codec)
            break;

        status = mpc_decoder_decode(&decoder, sample_buffer, 0, 0);
        if (status == (unsigned)(-1)) {
            /* decode error */
            return CODEC_ERROR;
        } else { /* status > 0 */
            /* Convert musepack's numbers to an array of 16-bit BE signed integers */
            for (i = 0; i < status*info.channels; i += info.channels) {
                /* Left channel */
                Sample = shift_signed(sample_buffer[i], 16 - MPC_FIXED_POINT_SCALE_SHIFT);
                *(OutputPtr++) = Sample >> 8;
                *(OutputPtr++) = Sample & 0xff;

                /* Right channel. If the decoded stream is monophonic then
                 * the right output channel is the same as the left one.
                 */
                if (info.channels == 2) {
                    Sample = shift_signed(sample_buffer[i + 1], 16 - MPC_FIXED_POINT_SCALE_SHIFT);
                }
                *(OutputPtr++) = Sample >> 8;
                *(OutputPtr++) = Sample & 0xff;
                samplesdone++;

                /* Flush the buffer if it is full. */
                if (OutputPtr == OutputBufferEnd) {
                    ci->yield();
                    while (!ci->pcmbuf_insert(OutputBuffer, OUTPUT_BUFFER_SIZE))
                        ci->yield();
                    ci->set_elapsed(samplesdone/(frequency/1000));
                    OutputPtr = OutputBuffer;
                }
            }
        }
    }

    /* Flush the remaining data in the output buffer */
    if (OutputPtr > OutputBuffer) {
        ci->yield();
        while (!ci->pcmbuf_insert(OutputBuffer, OutputPtr - OutputBuffer))
            ci->yield();
    }

    if (ci->request_next_track())
        goto next_track;

    return CODEC_OK;
}

