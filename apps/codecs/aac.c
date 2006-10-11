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
#include "libm4a/m4a.h"
#include "libfaad/common.h"
#include "libfaad/structs.h"
#include "libfaad/decoder.h"

CODEC_HEADER

#ifndef SIMULATOR
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
extern char iedata[];
extern char iend[];
#endif

struct codec_api* rb;
struct codec_api* ci;

/* this is the codec entry point */
enum codec_status codec_start(struct codec_api* api)
{
    /* Note that when dealing with QuickTime/MPEG4 files, terminology is
     * a bit confusing. Files with sound are split up in chunks, where
     * each chunk contains one or more samples. Each sample in turn
     * contains a number of "sound samples" (the kind you refer to with
     * the sampling frequency).
     */
    size_t n;
    static demux_res_t demux_res;
    stream_t input_stream;
    uint32_t sound_samples_done;
    uint32_t elapsed_time;
    uint32_t sample_duration;
    uint32_t sample_byte_size;
    int file_offset;
    unsigned int i;
    unsigned char* buffer;
    static NeAACDecFrameInfo frame_info;
    NeAACDecHandle decoder;
    int err;
    uint32_t s = 0;
    unsigned char c = 0;

    /* Generic codec initialisation */
    rb = api;
    ci = api;

#ifndef SIMULATOR
    ci->memcpy(iramstart, iramcopy, iramend-iramstart);
    ci->memset(iedata, 0, iend - iedata);
#endif

    ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (int *)(1024*16));
    ci->configure(CODEC_SET_FILEBUF_WATERMARK, (int *)(1024*512));

    ci->configure(DSP_DITHER, (bool *)false);
    ci->configure(DSP_SET_STEREO_MODE, (int *)STEREO_NONINTERLEAVED);
    ci->configure(DSP_SET_SAMPLE_DEPTH, (int *)(29));

next_track:
    err = CODEC_OK;

    if (codec_init(api)) {
        LOGF("FAAD: Codec init error\n");
        err = CODEC_ERROR;
        goto exit;
    }

    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);
  
    sound_samples_done = ci->id3->offset;

    ci->configure(DSP_SET_FREQUENCY, (long *)(rb->id3->frequency));

    stream_create(&input_stream,ci);

    /* if qtmovie_read returns successfully, the stream is up to
     * the movie data, which can be used directly by the decoder */
    if (!qtmovie_read(&input_stream, &demux_res)) {
        LOGF("FAAD: File init error\n");
        err = CODEC_ERROR;
        goto done;
    }

    /* initialise the sound converter */
    decoder = NeAACDecOpen();

    if (!decoder) {
        LOGF("FAAD: Decode open error\n");
        err = CODEC_ERROR;
        goto done;
    }

    NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(decoder);
    conf->outputFormat = FAAD_FMT_24BIT; /* irrelevant, we don't convert */
    NeAACDecSetConfiguration(decoder, conf);

    err = NeAACDecInit2(decoder, demux_res.codecdata, demux_res.codecdata_len, &s, &c);
    if (err) {
        LOGF("FAAD: DecInit: %d, %d\n", err, decoder->object_type);
        err = CODEC_ERROR;
        goto done;
    }

    ci->id3->frequency = s;

    i = 0;
    
    if (sound_samples_done > 0) {
        if (alac_seek_raw(&demux_res, &input_stream, sound_samples_done,
                          &sound_samples_done, (int*) &i)) {
            elapsed_time = (sound_samples_done * 10) / (ci->id3->frequency / 100);
            ci->set_elapsed(elapsed_time);
        } else {
            sound_samples_done = 0;
        }
    }

    /* The main decoding loop */
    while (i < demux_res.num_sample_byte_sizes) {
        rb->yield();

        if (ci->stop_codec || ci->new_track) {
            break;
        }

        /* Deal with any pending seek requests */
        if (ci->seek_time) {
            if (alac_seek(&demux_res, &input_stream,
                          ((ci->seek_time-1)/10)*(ci->id3->frequency/100),
                          &sound_samples_done, (int*) &i)) {
                elapsed_time = (sound_samples_done * 10) / (ci->id3->frequency / 100);
                ci->set_elapsed(elapsed_time);
            }
            ci->seek_complete();
        }

        /* Lookup the length (in samples and bytes) of block i */
        if (!get_sample_info(&demux_res, i, &sample_duration, 
                             &sample_byte_size)) {
            LOGF("AAC: get_sample_info error\n");
            err = CODEC_ERROR;
            goto done;
        }

        /* There can be gaps between chunks, so skip ahead if needed. It
         * doesn't seem to happen much, but it probably means that a 
         * "proper" file can have chunks out of order. Why one would want
         * that an good question (but files with gaps do exist, so who 
         * knows?), so we don't support that - for now, at least.
         */        
        file_offset = get_sample_offset(&demux_res, i);
        
        if (file_offset > ci->curpos)
        {
            ci->advance_buffer(file_offset - ci->curpos);
        }
        
        /* Request the required number of bytes from the input buffer */
        buffer=ci->request_buffer(&n,sample_byte_size);

        /* Decode one block - returned samples will be host-endian */
        NeAACDecDecode(decoder, &frame_info, buffer, n);
        /* Ignore return value, we access samples in the decoder struct 
         * directly.
         */
        if (frame_info.error > 0) {
            LOGF("FAAD: decode error '%s'\n", NeAACDecGetErrorMessage(frame_info.error));
            err = CODEC_ERROR;
            goto done;
        }

        /* Advance codec buffer */
        ci->advance_buffer(n);

        /* Output the audio */
        rb->yield();
        while (!rb->pcmbuf_insert_split(decoder->time_out[0],
                                        decoder->time_out[1],
                                        frame_info.samples * 2))
        {
            rb->sleep(1);
        }

        /* Update the elapsed-time indicator */
        sound_samples_done += sample_duration;
        elapsed_time = (sound_samples_done * 10) / (ci->id3->frequency / 100);
        ci->set_elapsed(elapsed_time);

        /* Keep track of current position - for resuming */
        ci->set_offset(elapsed_time);

        i++;
    }

    err = CODEC_OK;

done:
    LOGF("AAC: Decoded %d samples, %d frames\n", sound_samples_done);

    if (ci->request_next_track())
        goto next_track;

exit:
    return err;
}
