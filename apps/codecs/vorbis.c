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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "codeclib.h"
#include "libtremor/ivorbisfile.h"
#include "libtremor/ogg.h"

CODEC_HEADER

#if defined(CPU_ARM) || defined(CPU_COLDFIRE)
#include <setjmp.h>
jmp_buf rb_jump_buf;
#endif

/* Some standard functions and variables needed by Tremor */

static size_t read_handler(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    (void)datasource;
    return ci->read_filebuf(ptr, nmemb*size);
}

static int initial_seek_handler(void *datasource, ogg_int64_t offset, int whence)
{
    (void)datasource;
    (void)offset;
    (void)whence;
    return -1;
}

static int seek_handler(void *datasource, ogg_int64_t offset, int whence)
{
    (void)datasource;

    if (whence == SEEK_CUR) {
        offset += ci->curpos;
    } else if (whence == SEEK_END) {
        offset += ci->filesize;
    }

    if (ci->seek_buffer(offset)) {
        return 0;
    }

    return -1;
}

static int close_handler(void *datasource)
{
    (void)datasource;
    return 0;
}

static long tell_handler(void *datasource)
{
    (void)datasource;
    return ci->curpos;
}

/* This sets the DSP parameters based on the current logical bitstream
 * (sampling rate, number of channels, etc).  It also tries to guess
 * reasonable buffer parameters based on the current quality setting.
 */
static bool vorbis_set_codec_parameters(OggVorbis_File *vf)
{
    vorbis_info *vi;

    vi = ov_info(vf, -1);

    if (vi == NULL) {
        //ci->splash(HZ*2, "Vorbis Error");
        return false;
    }

    ci->configure(DSP_SWITCH_FREQUENCY, ci->id3->frequency);
    codec_set_replaygain(ci->id3);

    if (vi->channels == 2) {
          ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);
    } else if (vi->channels == 1) {
          ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
    }

    return true;
}

/* this is the codec entry point */
enum codec_status codec_main(void)
{
    ov_callbacks callbacks;
    OggVorbis_File vf;
    ogg_int32_t **pcm;

    int error;
    long n;
    int current_section;
    int previous_section;
    int eof;
    ogg_int64_t vf_offsets[2];
    ogg_int64_t vf_dataoffsets;
    ogg_uint32_t vf_serialnos;
    ogg_int64_t vf_pcmlengths[2];

    ci->configure(DSP_SET_SAMPLE_DEPTH, 24);
    /* Note: These are sane defaults for these values.  Perhaps
     * they should be set differently based on quality setting
     */

#if defined(CPU_ARM) || defined(CPU_COLDFIRE)
    if (setjmp(rb_jump_buf) != 0)
    {
        /* malloc failed; skip to next track */
        error = CODEC_ERROR;
        goto done;
    }
#endif

/* We need to flush reserver memory every track load. */
next_track:
    if (codec_init()) {
        error = CODEC_ERROR;
        goto exit;
    }
    ogg_malloc_init();

    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    /* Create a decoder instance */
    callbacks.read_func = read_handler;
    callbacks.seek_func = initial_seek_handler;
    callbacks.tell_func = tell_handler;
    callbacks.close_func = close_handler;

    /* Open a non-seekable stream */
    error = ov_open_callbacks(ci, &vf, NULL, 0, callbacks);
    
    /* If the non-seekable open was successful, we need to supply the missing
     * data to make it seekable.  This is a hack, but it's reasonable since we
     * don't want to run the whole file through the buffer before we start
     * playing.  Using Tremor's seekable open routine would cause us to do
     * this, so we pretend not to be seekable at first.  Then we fill in the
     * missing fields of vf with 1) information in ci->id3, and 2) info
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
         vf.offsets[1] = ci->id3->filesize;
         vf.dataoffsets[0] = vf.offset;
         vf.pcmlengths[0] = 0;
         vf.pcmlengths[1] = ci->id3->samples;
         vf.serialnos[0] = vf.current_serialno;
         vf.callbacks.seek_func = seek_handler;
         vf.seekable = 1;
         vf.end = ci->id3->filesize;
         vf.ready_state = OPENED;
         vf.links = 1;
    } else {
         //ci->logf("ov_open: %d", error);
         error = CODEC_ERROR;
         goto done;
    }

    if (ci->id3->offset) {
        ci->advance_buffer(ci->id3->offset);
        ov_raw_seek(&vf, ci->id3->offset);
        ci->set_elapsed(ov_time_tell(&vf));
        ci->set_offset(ov_raw_tell(&vf));
    }

    previous_section = -1;
    eof = 0;
    while (!eof) {
        ci->yield();
        if (ci->stop_codec || ci->new_track)
            break;

        if (ci->seek_time) {
            if (ov_time_seek(&vf, ci->seek_time - 1)) {
                //ci->logf("ov_time_seek failed");
            }
            ci->seek_complete();
        }

        /* Read host-endian signed 24-bit PCM samples */
        n = ov_read_fixed(&vf, &pcm, 1024, &current_section);

        /* Change DSP and buffer settings for this bitstream */
        if (current_section != previous_section) {
            if (!vorbis_set_codec_parameters(&vf)) {
                error = CODEC_ERROR;
                goto done;
            } else {
                previous_section = current_section;
            }
        }

        if (n == 0) {
            eof = 1;
        } else if (n < 0) {
            DEBUGF("Error decoding frame\n");
        } else {
            ci->pcmbuf_insert(pcm[0], pcm[1], n);
            ci->set_offset(ov_raw_tell(&vf));
            ci->set_elapsed(ov_time_tell(&vf));
        }
    }
    error = CODEC_OK;
    
done:
    if (ci->request_next_track()) {
        /* Clean things up for the next track */
        vf.dataoffsets = NULL;
        vf.offsets = NULL;
        vf.serialnos = NULL;
        vf.pcmlengths = NULL;
        ov_clear(&vf);
        goto next_track;
    }
        
exit:
    return error;
}

