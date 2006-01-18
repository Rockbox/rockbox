/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "codeclib.h"
#include "Tremor/ivorbisfile.h"
#include "Tremor/ogg.h"

CODEC_HEADER

static struct codec_api *rb;

/* Some standard functions and variables needed by Tremor */

int errno;

size_t read_handler(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    (void)datasource;
    return rb->read_filebuf(ptr, nmemb*size);
}

int initial_seek_handler(void *datasource, ogg_int64_t offset, int whence)
{
    (void)datasource;
    (void)offset;
    (void)whence;
    return -1;
}

int seek_handler(void *datasource, ogg_int64_t offset, int whence)
{
    (void)datasource;

    if (whence == SEEK_CUR) {
        offset += rb->curpos;
    } else if (whence == SEEK_END) {
        offset += rb->filesize;
    }

    if (rb->seek_buffer(offset)) {
        return 0;
    }

    return -1;
}

int close_handler(void *datasource)
{
    (void)datasource;
    return 0;
}

long tell_handler(void *datasource)
{
    (void)datasource;
    return rb->curpos;
}

/* This sets the DSP parameters based on the current logical bitstream
 * (sampling rate, number of channels, etc).  It also tries to guess
 * reasonable buffer parameters based on the current quality setting.
 */
bool vorbis_set_codec_parameters(OggVorbis_File *vf)
{
    vorbis_info *vi;

    vi = ov_info(vf, -1);

    if (vi == NULL) {
        //rb->splash(HZ*2, true, "Vorbis Error");
        return false;
    }

    rb->configure(DSP_SET_FREQUENCY, (int *)rb->id3->frequency);
    codec_set_replaygain(rb->id3);

    if (vi->channels == 2) {
          rb->configure(DSP_SET_STEREO_MODE, (int *)STEREO_NONINTERLEAVED);
    } else if (vi->channels == 1) {
          rb->configure(DSP_SET_STEREO_MODE, (int *)STEREO_MONO);
    }

    return true;
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
    ov_callbacks callbacks;
    OggVorbis_File vf;
    ogg_int32_t **pcm;

    int error;
    long n;
    int current_section;
    int previous_section = -1;
    int eof;
    ogg_int64_t vf_offsets[2];
    ogg_int64_t vf_dataoffsets;
    ogg_uint32_t vf_serialnos;
    ogg_int64_t vf_pcmlengths[2];

    rb = api;

    #ifdef USE_IRAM
    rb->memcpy(iramstart, iramcopy, iramend - iramstart);
    rb->memset(iedata, 0, iend - iedata);
    #endif

    rb->configure(CODEC_DSP_ENABLE, (bool *)true);
    rb->configure(DSP_DITHER, (bool *)false);
    rb->configure(DSP_SET_SAMPLE_DEPTH, (long *)24);
    rb->configure(DSP_SET_CLIP_MAX, (long *)((1 << 24) - 1));
    rb->configure(DSP_SET_CLIP_MIN, (long *)-((1 << 24) - 1));
    /* Note: These are sane defaults for these values.  Perhaps
     * they should be set differently based on quality setting
     */

    /* The chunk size below is magic.  If set any lower, resume
     * doesn't work properly (ov_raw_seek() does the wrong thing).
     */
    rb->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (long *)(1024*256));

/* We need to flush reserver memory every track load. */
next_track:
    if (codec_init(rb)) {
        error = CODEC_ERROR;
        goto exit;
    }

    while (!*rb->taginfo_ready && !rb->stop_codec)
        rb->sleep(1);

    /* Create a decoder instance */
    callbacks.read_func = read_handler;
    callbacks.seek_func = initial_seek_handler;
    callbacks.tell_func = tell_handler;
    callbacks.close_func = close_handler;

    /* Open a non-seekable stream */
    error = ov_open_callbacks(rb, &vf, NULL, 0, callbacks);
    
    /* If the non-seekable open was successful, we need to supply the missing
     * data to make it seekable.  This is a hack, but it's reasonable since we
     * don't want to run the whole file through the buffer before we start
     * playing.  Using Tremor's seekable open routine would cause us to do
     * this, so we pretend not to be seekable at first.  Then we fill in the
     * missing fields of vf with 1) information in rb->id3, and 2) info
     * obtained by Tremor in the above ov_open call.
     *
     * Note that this assumes there is only ONE logical Vorbis bitstream in our
     * physical Ogg bitstream.  This is verified in metadata.c, well before we
     * get here.
     */
    if (!error) {
         vf.offsets = vf_offsets;
         vf.dataoffsets = &vf_dataoffsets;
         vf.serialnos = &vf_serialnos;
         vf.pcmlengths = vf_pcmlengths;

         vf.offsets[0] = 0;
         vf.offsets[1] = rb->id3->filesize;
         vf.dataoffsets[0] = vf.offset;
         vf.pcmlengths[0] = 0;
         vf.pcmlengths[1] = rb->id3->samples;
         vf.serialnos[0] = vf.current_serialno;
         vf.callbacks.seek_func = seek_handler;
         vf.seekable = 1;
         vf.end = rb->id3->filesize;
         vf.ready_state = OPENED;
         vf.links = 1;
    } else {
         //rb->logf("ov_open: %d", error);
         error = CODEC_ERROR;
         goto exit;
    }

    if (rb->id3->offset) {
        rb->advance_buffer(rb->id3->offset);
        ov_raw_seek(&vf, rb->id3->offset);
        rb->set_elapsed(ov_time_tell(&vf));
        rb->set_offset(ov_raw_tell(&vf));
    }

    eof = 0;
    while (!eof) {
        rb->yield();
        if (rb->stop_codec || rb->reload_codec)
            break;

        if (rb->seek_time) {
            if (ov_time_seek(&vf, rb->seek_time - 1)) {
                //rb->logf("ov_time_seek failed");
            }
            rb->seek_complete();
        }

        /* Read host-endian signed 24-bit PCM samples */
        n = ov_read_fixed(&vf, &pcm, 1024, &current_section);

        /* Change DSP and buffer settings for this bitstream */
        if (current_section != previous_section) {
            if (!vorbis_set_codec_parameters(&vf)) {
                error = CODEC_ERROR;
                goto exit;
            } else {
                previous_section = current_section;
            }
        }

        if (n == 0) {
            eof = 1;
        } else if (n < 0) {
            DEBUGF("Error decoding frame\n");
        } else {
            while (!rb->pcmbuf_insert_split(pcm[0], pcm[1], 
                   n*sizeof(ogg_int32_t))) {
                rb->sleep(1);
            }
            rb->set_offset(ov_raw_tell(&vf));
            rb->set_elapsed(ov_time_tell(&vf));
        }
    }
    
    if (rb->request_next_track()) {
        /* Clean things up for the next track */
        vf.dataoffsets = NULL;
        vf.offsets = NULL;
        vf.serialnos = NULL;
        vf.pcmlengths = NULL;
        ov_clear(&vf);
        previous_section = -1;
        goto next_track;
    }
        
    error = CODEC_OK;
exit:
    return error;
}

