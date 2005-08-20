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
#include "config.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <atoi.h>
#include <timefuncs.h>
#include <ctype.h>
#include "debug.h"
#include "button.h"
#include "dir.h"
#include "file.h"
#include "kernel.h"
#include "sprintf.h"
#include "logf.h"
#include "screens.h"
#include "misc.h"
#include "mas.h"
#include "codecs.h"
#include "lang.h"
#include "keyboard.h"
#include "mpeg.h"
#include "buffer.h"
#include "mp3_playback.h"
#include "backlight.h"
#include "ata.h"
#include "talk.h"
#include "mp3data.h"
#include "powermgmt.h"
#include "system.h"
#include "sound.h"
#include "database.h"
#if (CONFIG_HWCODEC == MASNONE)
#include "pcm_playback.h"
#endif

#ifdef SIMULATOR
#if CONFIG_HWCODEC == MASNONE
unsigned char codecbuf[CODEC_SIZE];
#endif
void *sim_codec_load_ram(char* codecptr, int size,
        void* ptr2, int bufwrap, int *pd);
void sim_codec_close(int pd);
#else
#define sim_codec_close(x)
extern unsigned char codecbuf[];
#endif

extern void* plugin_get_audio_buffer(int *buffer_size);

static int codec_test(int api_version, int model, int memsize);

struct codec_api ci_voice;

struct codec_api ci = {
    CODEC_API_VERSION,
    codec_test,

    0, /* filesize */
    0, /* curpos */
    NULL, /* id3 */
    NULL, /* taginfo_ready */
    false, /* stop_codec */
    false, /* reload_codec */
    0, /* seek_time */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    
    splash,

    /* file */
    (open_func)PREFIX(open),
    close,
    (read_func)read,
    PREFIX(lseek),
    (creat_func)PREFIX(creat),
    (write_func)write,
    PREFIX(remove),
    PREFIX(rename),
    PREFIX(ftruncate),
    fdprintf,
    read_line,
    settings_parseline,
#ifndef SIMULATOR
    ata_sleep,
#endif

    /* dir */
    PREFIX(opendir),
    PREFIX(closedir),
    PREFIX(readdir),
    PREFIX(mkdir),

    /* kernel/ system */
    PREFIX(sleep),
    yield,
    &current_tick,
    default_event_handler,
    default_event_handler_ex,
    create_thread,
    remove_thread,
    reset_poweroff_timer,
#ifndef SIMULATOR
    system_memory_guard,
    &cpu_frequency,
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    cpu_boost,
#endif
#endif

    /* strings and memory */
    snprintf,
    strcpy,
    strncpy,
    strlen,
    strrchr,
    strcmp,
    strcasecmp,
    strncasecmp,
    memset,
    memcpy,
    _ctype_,
    atoi,
    strchr,
    strcat,
    memcmp,
    strcasestr,

    /* sound */
    sound_set,
#ifndef SIMULATOR
    mp3_play_data,
    mp3_play_pause,
    mp3_play_stop,
    mp3_is_playing,
#if CONFIG_HWCODEC != MASNONE
    bitswap,
#endif
#if CONFIG_HWCODEC == MASNONE
    pcm_play_data,    
    pcm_play_stop,
    pcm_set_frequency,
    pcm_is_playing,
    pcm_play_pause,
#endif
#endif

    /* playback control */
    PREFIX(audio_play),
    audio_stop,
    audio_pause,
    audio_resume,
    audio_next,
    audio_prev,
    audio_ff_rewind,
    audio_next_track,
    playlist_amount,
    audio_status,
    audio_has_changed_track,
    audio_current_track,
    audio_flush_and_reload_tracks,
    audio_get_file_pos,
#if !defined(SIMULATOR) && (CONFIG_HWCODEC != MASNONE)
    mpeg_get_last_header,
#endif
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    sound_set_pitch,
#endif

#if !defined(SIMULATOR) && (CONFIG_HWCODEC != MASNONE)
    /* MAS communication */
    mas_readmem,
    mas_writemem,
    mas_readreg,
    mas_writereg,
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    mas_codec_writereg,
    mas_codec_readreg,
#endif
#endif /* !simulator and HWCODEC != MASNONE */

    /* tag database */
    &tagdbheader,
    &tagdb_fd,
    &tagdb_initialized,
    tagdb_init,

    /* misc */
    srand,
    rand,
    (qsort_func)qsort,
    kbd_input,
    get_time,
    set_time,
    plugin_get_audio_buffer,

#if defined(DEBUG) || defined(SIMULATOR)
    debugf,
#endif
    &global_settings,
    mp3info,
    count_mp3_frames,
    create_xing_header,
    find_next_frame,
    battery_level,
    battery_level_safe,
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    peak_meter_scale_value,
    peak_meter_set_use_dbfs,
    peak_meter_get_use_dbfs,
#endif

    /* new stuff at the end, sort into place next time
       the API gets incompatible */

#ifdef ROCKBOX_HAS_LOGF
    logf,
#endif

    memchr,
    NULL,
};

int codec_load_ram(char* codecptr, int size, void* ptr2, int bufwrap,
                   struct codec_api *api)
{
    enum codec_status (*codec_start)(const struct codec_api* api);
    int status;
#ifndef SIMULATOR
    int copy_n;
    
    if ((char *)&codecbuf[0] != codecptr) {
        /* zero out codec buffer to ensure a properly zeroed bss area */
        memset(codecbuf, 0, CODEC_SIZE);
        
        size = MIN(size, CODEC_SIZE);
        copy_n = MIN(size, bufwrap);
        memcpy(codecbuf, codecptr, copy_n);         
        size -= copy_n;
        if (size > 0) {
            memcpy(&codecbuf[copy_n], ptr2, size);
        }
    }
    codec_start = (void*)&codecbuf;
        
#else /* SIMULATOR */
    int pd;
    
    codec_start = sim_codec_load_ram(codecptr, size, ptr2, bufwrap, &pd);
    if (pd < 0)
        return CODEC_ERROR;
#endif /* SIMULATOR */

    invalidate_icache();
    status = codec_start(api);
#ifdef SIMULATOR
    sim_codec_close(pd);
#endif
    
    return status;
}

int codec_load_file(const char *plugin, struct codec_api *api)
{
    char msgbuf[80];
    int fd;
    int rc;
    
    /* zero out codec buffer to ensure a properly zeroed bss area */
    memset(codecbuf, 0, CODEC_SIZE);

    fd = open(plugin, O_RDONLY);
    if (fd < 0) {
        snprintf(msgbuf, sizeof(msgbuf)-1, "Couldn't load codec: %s", plugin);
        logf("Codec load error:%d", fd);
        splash(HZ*2, true, msgbuf);
        return fd;
    }
    
    rc = read(fd, &codecbuf[0], CODEC_SIZE);
    close(fd);
    if (rc <= 0) {
        logf("Codec read error");
        return CODEC_ERROR;
    }

    return codec_load_ram(codecbuf, (size_t)rc, NULL, 0, api);
}

static int codec_test(int api_version, int model, int memsize)
{
    if (api_version < CODEC_MIN_API_VERSION ||
        api_version > CODEC_API_VERSION)
        return CODEC_WRONG_API_VERSION;

    (void)model;
#if 0
    if (model != MODEL)
        return CODEC_WRONG_MODEL;
#endif

    if (memsize != MEM)
        return CODEC_WRONG_MODEL;
    
    return CODEC_OK;
}
