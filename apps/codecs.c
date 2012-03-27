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
#include "rbpaths.h"

#define LOGF_ENABLE
#include "logf.h"

#if (CONFIG_PLATFORM & (PLATFORM_SDL|PLATFORM_MAEMO|PLATFORM_PANDORA))
#define PREFIX(_x_) sim_ ## _x_
#else
#define PREFIX(_x_) _x_
#endif

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
/* For PLATFORM_HOSTED this buffer must be define here. */
static unsigned char codecbuf[CODEC_SIZE];
#else
/* For PLATFORM_NATIVE this buffer is defined in *.lds files. */
extern unsigned char codecbuf[];
#endif

static size_t codec_size;

extern void* plugin_get_audio_buffer(size_t *buffer_size);

#if (CONFIG_PLATFORM & PLATFORM_NATIVE) && defined(HAVE_RECORDING)
#undef open
static int open(const char* pathname, int flags, ...)
{
    return file_open(pathname, flags);
}
#endif
struct codec_api ci = {

    0,    /* filesize */
    0,    /* curpos */
    NULL, /* id3 */
    ERR_HANDLE_NOT_FOUND, /* audio_hid */
    NULL, /* struct dsp_config *dsp */
    NULL, /* codec_get_buffer */
    NULL, /* pcmbuf_insert */
    NULL, /* set_elapsed */
    NULL, /* read_filebuf */
    NULL, /* request_buffer */
    NULL, /* advance_buffer */
    NULL, /* seek_buffer */
    NULL, /* seek_complete */
    NULL, /* set_offset */
    NULL, /* configure */
    NULL, /* get_command */
    NULL, /* loop_track */
    
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

    commit_dcache,
    commit_discard_dcache,
    commit_discard_idcache,

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
#if defined(DEBUG) || defined(SIMULATOR)
    debugf,
#endif
#ifdef ROCKBOX_HAS_LOGF
    logf,
#endif

    (qsort_func)qsort,

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
    snprintf(path, MAX_PATH-1, CODECS_DIR "/" CODEC_PREFIX "%s."
            CODEC_EXTENSION, codec_root_fn);
}

/* Returns pointer to and size of free codec RAM. Aligns to CACHEALIGN_SIZE. */
void *codec_get_buffer_callback(size_t *size)
{
    void *buf = &codecbuf[codec_size];
    ssize_t s = CODEC_SIZE - codec_size;

    if (s <= 0)
        return NULL;

    *size = s;
    ALIGN_BUFFER(buf, *size, CACHEALIGN_SIZE);

    return buf;
}

/** codec loading and call interface **/
static void *curr_handle = NULL;
static struct codec_header *c_hdr = NULL;

static int codec_load_ram(struct codec_api *api)
{
    struct lc_header *hdr;

    c_hdr = lc_get_header(curr_handle);
    hdr   = c_hdr ? &c_hdr->lc_hdr : NULL;

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
        lc_close(curr_handle);
        curr_handle = NULL;
        return CODEC_ERROR;
    }

    if (hdr->api_version > CODEC_API_VERSION
        || hdr->api_version < CODEC_MIN_API_VERSION) {
        logf("codec api version error");
        lc_close(curr_handle);
        curr_handle = NULL;
        return CODEC_ERROR;
    }

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    codec_size = hdr->end_addr - codecbuf;
#else
    codec_size = 0;
#endif

    *(c_hdr->api) = api;

    logf("Codec: calling entrypoint");
    return c_hdr->entry_point(CODEC_LOAD);
}

int codec_load_buf(int hid, struct codec_api *api)
{
    int rc = bufread(hid, CODEC_SIZE, codecbuf);

    if (rc < 0) {
        logf("Codec: cannot read buf handle");
        return CODEC_ERROR;
    }

    curr_handle = lc_open_from_mem(codecbuf, rc);

    if (curr_handle == NULL) {
        logf("Codec: load error");
        return CODEC_ERROR;
    }

    return codec_load_ram(api);
}

int codec_load_file(const char *plugin, struct codec_api *api)
{
    char path[MAX_PATH];

    codec_get_full_path(path, plugin);

    curr_handle = lc_open(path, codecbuf, CODEC_SIZE);

    if (curr_handle == NULL) {
        logf("Codec: cannot read file");
        return CODEC_ERROR;
    }

    return codec_load_ram(api);
}

int codec_run_proc(void)
{
    if (curr_handle == NULL) {
        logf("Codec: no codec to run");
        return CODEC_ERROR;
    }

    logf("Codec: entering run state");
    return c_hdr->run_proc();
}

int codec_close(void)
{
    int status = CODEC_OK;

    if (curr_handle != NULL) {
        logf("Codec: cleaning up");
        status = c_hdr->entry_point(CODEC_UNLOAD);
        lc_close(curr_handle);
        curr_handle = NULL;
    }

    return status;
}
