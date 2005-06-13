/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 David Bryant
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"

#include <codecs/libwavpack/wavpack.h>
#include "playback.h"
#include "lib/codeclib.h"

static struct plugin_api *rb;
static struct codec_api *ci;

#define BUFFER_SIZE 4096

static long temp_buffer [BUFFER_SIZE] IDATA_ATTR;

static long read_callback (void *buffer, long bytes)
{
    return ci->read_filebuf (buffer, bytes);
}

#ifndef SIMULATOR
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
#endif

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parm)
{
    WavpackContext *wpc;
    char error [80];
    int bps, nchans;

    /* Generic plugin initialisation */
    TEST_PLUGIN_API(api);

    rb = api;
    ci = (struct codec_api*) parm;

#ifndef SIMULATOR
    rb->memcpy(iramstart, iramcopy, iramend-iramstart);
#endif

    ci->configure(CODEC_SET_FILEBUF_LIMIT, (int *)(1024*1024*10));
    ci->configure(CODEC_SET_FILEBUF_WATERMARK, (int *)(1024*512));
    ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (int *)(1024*128));

    next_track:

    if (codec_init(api, ci))
        return PLUGIN_ERROR;

    /* Create a decoder instance */

    wpc = WavpackOpenFileInput (read_callback, error);

    if (!wpc)
        return PLUGIN_ERROR;

    bps = WavpackGetBytesPerSample (wpc);
    nchans = WavpackGetReducedChannels (wpc);

    ci->set_elapsed (0);

    /* The main decoder loop */

    while (1) {
        long nsamples;  

        if (ci->seek_time && ci->taginfo_ready && ci->id3->length) {
            int curpos_ms = (WavpackGetSampleIndex (wpc) + 220) / 441 * 10;
            int n, d, skip;

            if (ci->seek_time > curpos_ms) {
                n = ci->seek_time - curpos_ms;
                d = ci->id3->length - curpos_ms;
                skip = (int)((long long)(ci->filesize - ci->curpos) * n / d);
                ci->seek_buffer (ci->curpos + skip);
            }
            else {
                n = curpos_ms - ci->seek_time;
                d = curpos_ms;
                skip = (int)((long long) ci->curpos * n / d);
                ci->seek_buffer (ci->curpos - skip);
            }

            wpc = WavpackOpenFileInput (read_callback, error);
            ci->seek_time = 0;

            if (!wpc)
                break;

            ci->set_elapsed ((int)((long long) WavpackGetSampleIndex (wpc) * 1000 / 44100));
            rb->yield ();
        }

        nsamples = WavpackUnpackSamples (wpc, temp_buffer, BUFFER_SIZE / 2);  

        if (!nsamples || ci->stop_codec || ci->reload_codec)
            break;

        /* convert mono to stereo here, in place */

        if (nchans == 1) {
            long *dst = temp_buffer + (nsamples * 2);
            long *src = temp_buffer + nsamples;
            long count = nsamples;

            while (count--) {
                *--dst = *--src;
                *--dst = *src;
                if (!(count & 0x7f))
                    rb->yield ();
            }
        }

        if (bps == 1) {
            short *dst = (short *) temp_buffer;
            long *src = temp_buffer;
            long count = nsamples;

            while (count--) {
                *dst++ = *src++ << 8;
                *dst++ = *src++ << 8;
                if (!(count & 0x7f))
                    rb->yield ();
            }
        }
        else if (bps == 2) {
            short *dst = (short *) temp_buffer;
            long *src = temp_buffer;
            long count = nsamples;

            while (count--) {
                *dst++ = *src++;
                *dst++ = *src++;
                if (!(count & 0x7f))
                    rb->yield ();
            }
        }
        else {
            short *dst = (short *) temp_buffer;
            int shift = (bps - 2) * 8;
            long *src = temp_buffer;
            long count = nsamples;

            while (count--) {
                *dst++ = *src++ >> shift;
                *dst++ = *src++ >> shift;
                if (!(count & 0x7f))
                    rb->yield ();
            }
        }

        if (ci->stop_codec || ci->reload_codec)
            break;

        while (!ci->audiobuffer_insert ((char *) temp_buffer, nsamples * 4))
            rb->yield ();

        ci->set_elapsed ((WavpackGetSampleIndex (wpc) + 220) / 441 * 10);
    }

    if (ci->request_next_track())
        goto next_track;

    return PLUGIN_OK;
}
