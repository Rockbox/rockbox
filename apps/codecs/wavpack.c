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

static struct codec_api *ci;

#define BUFFER_SIZE 4096

static long temp_buffer [BUFFER_SIZE] IBSS_ATTR;

static long read_callback (void *buffer, long bytes)
{
    long retval = ci->read_filebuf (buffer, bytes);
    ci->id3->offset = ci->curpos;
    return retval;
}

#ifdef USE_IRAM 
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
extern char iedata[];
extern char iend[];
#endif

/* this is the codec entry point */
enum codec_status codec_start(struct codec_api* api)
{
    WavpackContext *wpc;
    char error [80];
    int bps, nchans, sr_100;
    int retval;

    /* Generic codec initialisation */
    ci = api;

#ifdef USE_IRAM
    ci->memcpy(iramstart, iramcopy, iramend-iramstart);
    ci->memset(iedata, 0, iend - iedata);
#endif

    ci->configure(CODEC_SET_FILEBUF_WATERMARK, (int *)(1024*512));
    ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (int *)(1024*128));
  
    ci->configure(DSP_DITHER, (bool *)false);
    ci->configure(DSP_SET_SAMPLE_DEPTH, (int *)(27));   // should be 28...

    next_track:

    if (codec_init(api)) {
        retval = CODEC_ERROR;
        goto exit;
    }

    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);
        
    ci->configure(CODEC_DSP_ENABLE, (bool *)true);
    ci->configure(DSP_SET_FREQUENCY, (long *)(ci->id3->frequency));
    codec_set_replaygain(ci->id3);
   
    /* Create a decoder instance */
    wpc = WavpackOpenFileInput (read_callback, error);

    if (!wpc) {
        retval = CODEC_ERROR;
        goto exit;
    }

    bps = WavpackGetBytesPerSample (wpc);
    nchans = WavpackGetReducedChannels (wpc);
    ci->configure(DSP_SET_STEREO_MODE, nchans == 2 ? (int *)STEREO_INTERLEAVED : (int *)STEREO_MONO);
    sr_100 = ci->id3->frequency / 100;

    ci->set_elapsed (0);

    /* The main decoder loop */

    while (1) {
        long nsamples;  

        if (ci->seek_time && ci->taginfo_ready && ci->id3->length) {
            ci->seek_time--;
            int curpos_ms = WavpackGetSampleIndex (wpc) / sr_100 * 10;
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
            ci->seek_complete();

            if (!wpc)
                break;

            ci->set_elapsed (WavpackGetSampleIndex (wpc) / sr_100 * 10);
            ci->yield ();
        }

        nsamples = WavpackUnpackSamples (wpc, temp_buffer, BUFFER_SIZE / nchans);  

        if (!nsamples || ci->stop_codec || ci->reload_codec)
            break;

        ci->yield ();

        if (ci->stop_codec || ci->reload_codec)
            break;

        while (!ci->pcmbuf_insert ((char *) temp_buffer, nsamples * nchans * 4))
            ci->sleep (1);

        ci->set_elapsed (WavpackGetSampleIndex (wpc) / sr_100 * 10);
        ci->yield ();
    }

    if (ci->request_next_track())
        goto next_track;

    retval = CODEC_OK;
exit:
    return retval;
}
