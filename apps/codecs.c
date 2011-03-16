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
#include "config.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <timefuncs.h>
#include <ctype.h>
#include <stdarg.h>
#include "string-extra.h"
#include "load_code.h"
#include "debug.h"
#include "button.h"
#include "dir.h"
#include "file.h"
#include "kernel.h"
#include "screens.h"
#include "misc.h"
#include "codecs.h"
#include "lang.h"
#include "keyboard.h"
#include "buffering.h"
#include "mp3_playback.h"
#include "backlight.h"
#include "storage.h"
#include "talk.h"
#include "mp3data.h"
#include "powermgmt.h"
#include "system.h"
#include "sound.h"
#include "splash.h"
#include "general.h"

#define LOGF_ENABLE
#include "logf.h"

#if (CONFIG_PLATFORM & (PLATFORM_SDL|PLATFORM_MAEMO|PLATFORM_PANDORA))
#define PREFIX(_x_) sim_ ## _x_
#else
#define PREFIX(_x_) _x_
#endif

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
#if CONFIG_CODEC == SWCODEC
unsigned char codecbuf[CODEC_SIZE];
#endif
#endif

size_t codec_size;

extern void* plugin_get_audio_buffer(size_t *buffer_size);

#if (CONFIG_PLATFORM & PLATFORM_NATIVE) && defined(HAVE_RECORDING)
#undef open
static int open(const char* pathname, int flags, ...)
{
    return file_open(pathname, flags);
}
#endif
struct codec_api ci = {

    0, /* filesize */
    0, /* curpos */
    NULL, /* id3 */
    NULL, /* taginfo_ready */
    false, /* stop_codec */
    0, /* new_track */
    0, /* seek_time */
    NULL, /* struct dsp_config *dsp */
    NULL, /* codec_get_buffer */
    NULL, /* pcmbuf_insert */
    NULL, /* set_elapsed */
    NULL, /* read_filebuf */
    NULL, /* request_buffer */
    NULL, /* advance_buffer */
    NULL, /* seek_buffer */
    NULL, /* seek_complete */
    NULL, /* request_next_track */
    NULL, /* set_offset */
    NULL, /* configure */
    
    /* kernel/ system */
#if defined(CPU_ARM) && CONFIG_PLATFORM & PLATFORM_NATIVE
    __div0,
#endif
    sleep,
    yield,

#if NUM_CORES > 1
    create_thread,
    thread_thaw,
    thread_wait,
    semaphore_init,
    semaphore_wait,
    semaphore_release,
#endif

    cpucache_flush,
    cpucache_invalidate,

    /* strings and memory */
    strcpy,
    strlen,
    strcmp,
    strcat,
    memset,
    memcpy,
    memmove,
    memcmp,
    memchr,
    strcasestr,
#if defined(DEBUG) || defined(SIMULATOR)
    debugf,
#endif
#ifdef ROCKBOX_HAS_LOGF
    logf,
#endif

    (qsort_func)qsort,
    &global_settings,

#ifdef RB_PROFILE
    profile_thread,
    profstop,
    __cyg_profile_func_enter,
    __cyg_profile_func_exit,
#endif

#ifdef HAVE_RECORDING
    enc_get_inputs,
    enc_set_parameters,
    enc_get_chunk,
    enc_finish_chunk,
    enc_get_pcm_data,
    enc_unget_pcm_data,

    /* file */
    (open_func)PREFIX(open),
    PREFIX(close),
    (read_func)PREFIX(read),
    PREFIX(lseek),
    (write_func)PREFIX(write),
    round_value_to_list32,

#endif /* HAVE_RECORDING */

    /* new stuff at the end, sort into place next time
       the API gets incompatible */
};

void codec_get_full_path(char *path, const char *codec_root_fn)
{
    snprintf(path, MAX_PATH-1, "%s/%s." CODEC_EXTENSION,
             CODECS_DIR, codec_root_fn);
}

static void * codec_load_ram(void *handle, struct codec_api *api)
{
    struct codec_header *c_hdr = lc_get_header(handle);
    struct lc_header    *hdr   = c_hdr ? &c_hdr->lc_hdr : NULL;

    if (hdr == NULL
        || (hdr->magic != CODEC_MAGIC
#ifdef HAVE_RECORDING
             && hdr->magic != CODEC_ENC_MAGIC
#endif
            )
        || hdr->target_id != TARGET_ID
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
        || hdr->load_addr != codecbuf
        || hdr->end_addr > codecbuf + CODEC_SIZE
#endif
        )
    {
        logf("codec header error");
        lc_close(handle);
        return NULL;
    }

    if (hdr->api_version > CODEC_API_VERSION
        || hdr->api_version < CODEC_MIN_API_VERSION) {
        logf("codec api version error");
        lc_close(handle);
        return NULL;
    }

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    codec_size = hdr->end_addr - codecbuf;
#else
    codec_size = 0;
#endif

    *(c_hdr->api) = api;

    return handle;
}

void * codec_load_buf(int hid, struct codec_api *api)
{
    int rc;
    void *handle;
    rc = bufread(hid, CODEC_SIZE, codecbuf);
    if (rc < 0) {
        logf("Codec: cannot read buf handle");
        return NULL;
    }

    handle = lc_open_from_mem(codecbuf, rc);

    if (handle == NULL) {
        logf("error loading codec");
        return NULL;
    }

    return codec_load_ram(handle, api);
}

void * codec_load_file(const char *plugin, struct codec_api *api)
{
    char path[MAX_PATH];
    void *handle;

    codec_get_full_path(path, plugin);

    handle = lc_open(path, codecbuf, CODEC_SIZE);

    if (handle == NULL) {
        logf("Codec: cannot read file");
        return NULL;
    }

    return codec_load_ram(handle, api);
}

int codec_begin(void *handle)
{
    int status = CODEC_ERROR;
    struct codec_header *c_hdr;

    c_hdr = lc_get_header(handle);

    if (c_hdr != NULL) {
        logf("Codec: calling entry_point");
        status = c_hdr->entry_point();
    }

    return status;
}

void codec_close(void *handle)
{
    if (handle)
        lc_close(handle);
}
