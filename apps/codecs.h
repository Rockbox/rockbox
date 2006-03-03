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
#ifndef _CODECS_H_
#define _CODECS_H_

/* instruct simulator code to not redefine any symbols when compiling codecs.
   (the CODEC macro is defined in apps/codecs/Makefile) */
#ifdef CODEC
#define NO_REDEFINES_PLEASE
#endif

#ifndef MEM
#define MEM 2
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "config.h"
#include "dir.h"
#include "kernel.h"
#include "button.h"
#include "font.h"
#include "system.h"
#include "id3.h"
#include "mpeg.h"
#include "audio.h"
#include "mp3_playback.h"
#ifdef RB_PROFILE
#include "profile.h"
#endif
#if (CONFIG_CODEC == SWCODEC)
#include "dsp.h"
#include "playback.h"
#endif
#include "settings.h"
#include "thread.h"
#include "playlist.h"
#include "sound.h"

#ifdef CODEC

#if defined(DEBUG) || defined(SIMULATOR)
#undef DEBUGF
#define DEBUGF  rb->debugf
#undef LDEBUGF
#define LDEBUGF rb->debugf
#else
#define DEBUGF(...)
#define LDEBUGF(...)
#endif

#ifdef ROCKBOX_HAS_LOGF
#undef LOGF
#define LOGF rb->logf
#else
#define LOGF(...)
#endif

#endif

#ifdef SIMULATOR
#define PREFIX(_x_) sim_ ## _x_
#else
#define PREFIX(_x_) _x_
#endif

#define CODEC_MAGIC 0x52434F44 /* RCOD */

/* increase this every time the api struct changes */
#define CODEC_API_VERSION 4

/* update this to latest version if a change to the api struct breaks
   backwards compatibility (and please take the opportunity to sort in any
   new function which are "waiting" at the end of the function table) */
#define CODEC_MIN_API_VERSION 3

/* codec return codes */
enum codec_status {
    CODEC_OK = 0,
    CODEC_USB_CONNECTED,
    CODEC_ERROR = -1,
};

/* NOTE: To support backwards compatibility, only add new functions at
         the end of the structure.  Every time you add a new function,
         remember to increase CODEC_API_VERSION.  If you make changes to the
         existing APIs then also update CODEC_MIN_API_VERSION to current
         version
 */
struct codec_api {

    off_t  filesize;          /* Total file length */
    off_t  curpos;            /* Current buffer position */
    
    /* For gapless mp3 */
    struct mp3entry *id3;     /* TAG metadata pointer */
    bool *taginfo_ready;      /* Is metadata read */
    
    /* Codec should periodically check if stop_codec is set to true.
       In case it's, codec must return with PLUGIN_OK status immediately. */
    bool stop_codec;
    /* Codec should periodically check if reload_codec is set to true.
       In case it's, codec should reload itself without exiting. */
    bool reload_codec;
    /* If seek_time != 0, codec should seek to that song position (in ms)
       if codec supports seeking. */
    long seek_time;
    
    /* Returns buffer to malloc array. Only codeclib should need this. */
    void* (*get_codec_memory)(long *size);
    /* Insert PCM data into audio buffer for playback. Playback will start
       automatically. */
    bool (*pcmbuf_insert)(const char *data, size_t length);
    bool (*pcmbuf_insert_split)(const void *ch1, const void *ch2, size_t length);
    /* Set song position in WPS (value in ms). */
    void (*set_elapsed)(unsigned int value);
    
    /* Read next <size> amount bytes from file buffer to <ptr>.
       Will return number of bytes read or 0 if end of file. */
    long (*read_filebuf)(void *ptr, long size);
    /* Request pointer to file buffer which can be used to read
       <realsize> amount of data. <reqsize> tells the buffer system
       how much data it should try to allocate. If <realsize> is 0,
       end of file is reached. */
    void* (*request_buffer)(long *realsize, long reqsize);
    /* Advance file buffer position by <amount> amount of bytes. */
    void (*advance_buffer)(long amount);
    /* Advance file buffer to a pointer location inside file buffer. */
    void (*advance_buffer_loc)(void *ptr);
    /* Seek file buffer to position <newpos> beginning of file. */
    bool (*seek_buffer)(off_t newpos);
    /* Codec should call this function when it has done the seeking. */
    void (*seek_complete)(void);
    /* Calculate mp3 seek position from given time data in ms. */
    off_t (*mp3_get_filepos)(int newtime);
    /* Request file change from file buffer. Returns true is next
       track is available and changed. If return value is false,
       codec should exit immediately with PLUGIN_OK status. */
    bool (*request_next_track)(void);
    
    void (*set_offset)(unsigned int value);
    /* Configure different codec buffer parameters. */
    void (*configure)(int setting, void *value);

    void (*splash)(int ticks, bool center, const unsigned char *fmt, ...);

    /* file */
    int (*PREFIX(open))(const char* pathname, int flags);
    int (*close)(int fd);
    ssize_t (*read)(int fd, void* buf, size_t count);
    off_t (*PREFIX(lseek))(int fd, off_t offset, int whence);
    int (*PREFIX(creat))(const char *pathname, mode_t mode);
    ssize_t (*write)(int fd, const void* buf, size_t count);
    int (*PREFIX(remove))(const char* pathname);
    int (*PREFIX(rename))(const char* path, const char* newname);
    int (*PREFIX(ftruncate))(int fd, off_t length);

    int (*fdprintf)(int fd, const char *fmt, ...);
    int (*read_line)(int fd, char* buffer, int buffer_size);
    bool (*settings_parseline)(char* line, char** name, char** value);
#ifndef SIMULATOR
    void (*ata_sleep)(void);
#endif
    
    /* dir */
    DIR* (*PREFIX(opendir))(const char* name);
    int (*PREFIX(closedir))(DIR* dir);
    struct dirent* (*PREFIX(readdir))(DIR* dir);
    int (*PREFIX(mkdir))(const char *name, int mode);

    /* kernel/ system */
    void (*PREFIX(sleep))(int ticks);
    void (*yield)(void);
    long* current_tick;
    long (*default_event_handler)(long event);
    long (*default_event_handler_ex)(long event, void (*callback)(void *), void *parameter);
    int (*create_thread)(void (*function)(void), void* stack, int stack_size, const char *name);
    void (*remove_thread)(int threadnum);
    void (*reset_poweroff_timer)(void);
#ifndef SIMULATOR
    int (*system_memory_guard)(int newmode);
    long *cpu_frequency;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    void (*cpu_boost)(bool on_off);
#endif
#endif

    /* strings and memory */
    int (*snprintf)(char *buf, size_t size, const char *fmt, ...);
    char* (*strcpy)(char *dst, const char *src);
    char* (*strncpy)(char *dst, const char *src, size_t length);
    size_t (*strlen)(const char *str);
    char * (*strrchr)(const char *s, int c);
    int (*strcmp)(const char *, const char *);
    int (*strcasecmp)(const char *, const char *);
    int (*strncasecmp)(const char *s1, const char *s2, size_t n);
    void* (*memset)(void *dst, int c, size_t length);
    void* (*memcpy)(void *out, const void *in, size_t n);
    const char *_ctype_;
    int (*atoi)(const char *str);
    char *(*strchr)(const char *s, int c);
    char *(*strcat)(char *s1, const char *s2);
    int (*memcmp)(const void *s1, const void *s2, size_t n);
    char *(*strcasestr) (const char* phaystack, const char* pneedle);
    void *(*memchr)(const void *s1, int c, size_t n);

    /* sound */
    void (*sound_set)(int setting, int value);
#ifndef SIMULATOR
    void (*mp3_play_data)(const unsigned char* start, int size, void (*get_more)(unsigned char** start, int* size));
    void (*mp3_play_pause)(bool play);
    void (*mp3_play_stop)(void);
    bool (*mp3_is_playing)(void);
#endif /* !SIMULATOR */

    /* playback control */
    void (*PREFIX(audio_play))(long offset);
    void (*audio_stop)(void);
    void (*audio_pause)(void);
    void (*audio_resume)(void);
    void (*audio_next)(void);
    void (*audio_prev)(void);
    void (*audio_ff_rewind)(long newtime);
    struct mp3entry* (*audio_next_track)(void);
    int (*playlist_amount)(void);
    int (*audio_status)(void);
    bool (*audio_has_changed_track)(void);
    struct mp3entry* (*audio_current_track)(void);
    void (*audio_flush_and_reload_tracks)(void);
    int (*audio_get_file_pos)(void);

    /* tag database */
    struct tagdb_header *tagdbheader;
    int *tagdb_fd;
    int *tagdb_initialized;
    int (*tagdb_init) (void);

    /* misc */
    void (*srand)(unsigned int seed);
    int  (*rand)(void);
    void (*qsort)(void *base, size_t nmemb, size_t size,
                  int(*compar)(const void *, const void *));
    int (*kbd_input)(char* buffer, int buflen);
    struct tm* (*get_time)(void);
    int  (*set_time)(const struct tm *tm);
    void* (*plugin_get_audio_buffer)(int* buffer_size);

#if defined(DEBUG) || defined(SIMULATOR)
    void (*debugf)(const char *fmt, ...);
#endif
#ifdef ROCKBOX_HAS_LOGF
    void (*logf)(const char *fmt, ...);
#endif
    struct user_settings* global_settings;
    bool (*mp3info)(struct mp3entry *entry, const char *filename, bool v1first);
    int (*count_mp3_frames)(int fd, int startpos, int filesize,
                            void (*progressfunc)(int));
    int (*create_xing_header)(int fd, long startpos, long filesize,
                              unsigned char *buf, unsigned long num_frames,
                              unsigned long rec_time, unsigned long header_template,
                              void (*progressfunc)(int), bool generate_toc);
    unsigned long (*find_next_frame)(int fd, long *offset,
                                     long max_offset, unsigned long last_header);
    int (*battery_level)(void);
    bool (*battery_level_safe)(void);

#ifdef RB_PROFILE
    void (*profile_thread)(void);
    void (*profstop)(void);
    void (*profile_func_enter)(void *this_fn, void *call_site);
    void (*profile_func_exit)(void *this_fn, void *call_site);
#endif

    /* new stuff at the end, sort into place next time
       the API gets incompatible */     

    void* (*memmove)(void *out, const void *in, size_t n);
};

/* codec header */
struct codec_header {
    unsigned long magic;
    unsigned short target_id;
    unsigned short api_version;
    unsigned char *load_addr;
    unsigned char *end_addr;
    enum codec_status(*entry_point)(struct codec_api*);
};
#ifdef CODEC
#ifndef SIMULATOR
/* plugin_* is correct, codecs use the plugin linker script */
extern unsigned char plugin_start_addr[];
extern unsigned char plugin_end_addr[];
#define CODEC_HEADER \
        const struct codec_header __header \
        __attribute__ ((section (".header")))= { \
        CODEC_MAGIC, TARGET_ID, CODEC_API_VERSION, \
        plugin_start_addr, plugin_end_addr, codec_start };
#else /* SIMULATOR */
#define CODEC_HEADER \
        const struct codec_header __header = { \
        CODEC_MAGIC, TARGET_ID, CODEC_API_VERSION, \
        NULL, NULL, codec_start };
#endif
#endif

/* defined by the codec loader (codec.c) */
int codec_load_ram(char* codecptr, int size, void* ptr2, int bufwrap,
                   struct codec_api *api);
int codec_load_file(const char* codec, struct codec_api *api);

/* defined by the codec */
enum codec_status codec_start(struct codec_api* rockbox);

#endif
