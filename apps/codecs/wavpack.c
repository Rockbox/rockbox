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

#include "codeclib.h"
#include "libwavpack/wavpack.h"

CODEC_HEADER

#define BUFFER_SIZE 4096

static int32_t temp_buffer [BUFFER_SIZE] IBSS_ATTR;

static int32_t read_callback (void *buffer, int32_t bytes)
{
    int32_t retval = ci->read_filebuf (buffer, bytes);
    ci->id3->offset = ci->curpos;
    return retval;
}

/* this is the codec entry point */
enum codec_status codec_main(void)
{
    WavpackContext *wpc;
    char error [80];
    int bps, nchans, sr_100;
    int retval;

    /* Generic codec initialisation */
    ci->configure(CODEC_SET_FILEBUF_WATERMARK, 1024*512);
  
    ci->configure(DSP_SET_SAMPLE_DEPTH, 28);

    next_track:

    if (codec_init()) {
        retval = CODEC_ERROR;
        goto exit;
    }

    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);
        
    /* Create a decoder instance */
    wpc = WavpackOpenFileInput (read_callback, error);

    if (!wpc) {
        retval = CODEC_ERROR;
        goto done;
    }

    ci->configure(DSP_SWITCH_FREQUENCY, WavpackGetSampleRate (wpc));
    codec_set_replaygain(ci->id3);
    bps = WavpackGetBytesPerSample (wpc);
    nchans = WavpackGetReducedChannels (wpc);
    ci->configure(DSP_SET_STEREO_MODE, nchans == 2 ? STEREO_INTERLEAVED : STEREO_MONO);
    sr_100 = ci->id3->frequency / 100;

    ci->set_elapsed (0);

    /* The main decoder loop */

    while (1) {
        int32_t nsamples;  

        if (ci->seek_time && ci->taginfo_ready && ci->id3->length) {
            ci->seek_time--;
            int curpos_ms = WavpackGetSampleIndex (wpc) / sr_100 * 10;
            int n, d, skip;

            if (ci->seek_time > curpos_ms) {
                n = ci->seek_time - curpos_ms;
                d = ci->id3->length - curpos_ms;
                skip = (int)((int64_t)(ci->filesize - ci->curpos) * n / d);
                ci->seek_buffer (ci->curpos + skip);
            }
            else {
                n = curpos_ms - ci->seek_time;
                d = curpos_ms;
                skip = (int)((int64_t) ci->curpos * n / d);
                ci->seek_buffer (ci->curpos - skip);
            }

            wpc = WavpackOpenFileInput (read_callback, error);
            ci->seek_complete();

            if (!wpc)
                break;

            ci->set_elapsed (WavpackGetSampleIndex (wpc) / sr_100 * 10);
            ci->yield ();
        }

        nsamples = WavpackUnpackSamples (wpc, temp_buffer, BUFFER_SIZE / nchans);  

        if (!nsamples || ci->stop_codec || ci->new_track)
            break;

        ci->yield ();

        if (ci->stop_codec || ci->new_track)
            break;

        ci->pcmbuf_insert (temp_buffer, NULL, nsamples);

        ci->set_elapsed (WavpackGetSampleIndex (wpc) / sr_100 * 10);
        ci->yield ();
    }
    retval = CODEC_OK;

done:
    if (ci->request_next_track())
        goto next_track;

exit:
    return retval;
}
