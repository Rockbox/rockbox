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
#include <stdlib.h>
#include "config.h"
#include "system.h"
#include "metadata.h"
#include "audio.h"
#ifdef RB_PROFILE
#include "profile.h"
#include "thread.h"
#endif
#if (CONFIG_CODEC == SWCODEC)
#if !defined(SIMULATOR) && defined(HAVE_RECORDING)
#include "pcm_record.h"
#endif
#include "dsp.h"
#endif
#include "settings.h"

#ifdef CODEC
#if defined(DEBUG) || defined(SIMULATOR)
#undef DEBUGF
#define DEBUGF  ci->debugf
#undef LDEBUGF
#define LDEBUGF ci->debugf
#else
#define DEBUGF(...)
#define LDEBUGF(...)
#endif

#ifdef ROCKBOX_HAS_LOGF
#undef LOGF
#define LOGF ci->logf
#else
#define LOGF(...)
#endif

#endif

/* magic for normal codecs */
#define CODEC_MAGIC 0x52434F44 /* RCOD */
/* magic for encoder codecs */
#define CODEC_ENC_MAGIC 0x52454E43 /* RENC */

/* increase this every time the api struct changes */
#define CODEC_API_VERSION 29

/* update this to latest version if a change to the api struct breaks
   backwards compatibility (and please take the opportunity to sort in any
   new function which are "waiting" at the end of the function table) */
#define CODEC_MIN_API_VERSION 29

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
       In case it is, codec must return immediately */
    bool stop_codec;
    /* Codec should periodically check if new_track is non zero.
       When it is, the codec should request a new track. */
    int new_track;
    /* If seek_time != 0, codec should seek to that song position (in ms)
       if codec supports seeking. */
    long seek_time;

    /* The dsp instance to be used for audio output */
    struct dsp_config *dsp;
    
    /* Returns buffer to malloc array. Only codeclib should need this. */
    void* (*codec_get_buffer)(size_t *size);
    /* Insert PCM data into audio buffer for playback. Playback will start
       automatically. */
    bool (*pcmbuf_insert)(const void *ch1, const void *ch2, int count);
    /* Set song position in WPS (value in ms). */
    void (*set_elapsed)(unsigned int value);
    
    /* Read next <size> amount bytes from file buffer to <ptr>.
       Will return number of bytes read or 0 if end of file. */
    size_t (*read_filebuf)(void *ptr, size_t size);
    /* Request pointer to file buffer which can be used to read
       <realsize> amount of data. <reqsize> tells the buffer system
       how much data it should try to allocate. If <realsize> is 0,
       end of file is reached. */
    void* (*request_buffer)(size_t *realsize, size_t reqsize);
    /* Advance file buffer position by <amount> amount of bytes. */
    void (*advance_buffer)(size_t amount);
    /* Advance file buffer to a pointer location inside file buffer. */
    void (*advance_buffer_loc)(void *ptr);
    /* Seek file buffer to position <newpos> beginning of file. */
    bool (*seek_buffer)(size_t newpos);
    /* Codec should call this function when it has done the seeking. */
    void (*seek_complete)(void);
    /* Request file change from file buffer. Returns true is next
       track is available and changed. If return value is false,
       codec should exit immediately with PLUGIN_OK status. */
    bool (*request_next_track)(void);
    /* Free the buffer area of the current codec after its loaded */
    void (*discard_codec)(void);
    
    void (*set_offset)(size_t value);
    /* Configure different codec buffer parameters. */
    void (*configure)(int setting, intptr_t value);

    /* kernel/ system */
    void (*sleep)(int ticks);
    void (*yield)(void);

#if NUM_CORES > 1
    unsigned int
        (*create_thread)(void (*function)(void), void* stack,
                         size_t stack_size, unsigned flags, const char *name
                         IF_PRIO(, int priority)
                         IF_COP(, unsigned int core));

    void (*thread_thaw)(unsigned int thread_id);
    void (*thread_wait)(unsigned int thread_id);
    void (*semaphore_init)(struct semaphore *s, int max, int start);
    void (*semaphore_wait)(struct semaphore *s);
    void (*semaphore_release)(struct semaphore *s);
#endif /* NUM_CORES */

#ifdef CACHE_FUNCTIONS_AS_CALL
    void (*flush_icache)(void);
    void (*invalidate_icache)(void);
#endif

    /* strings and memory */
    char* (*strcpy)(char *dst, const char *src);
    char* (*strncpy)(char *dst, const char *src, size_t length);
    size_t (*strlen)(const char *str);
    int (*strcmp)(const char *, const char *);
    char *(*strcat)(char *s1, const char *s2);
    void* (*memset)(void *dst, int c, size_t length);
    void* (*memcpy)(void *out, const void *in, size_t n);
    void* (*memmove)(void *out, const void *in, size_t n);
    int (*memcmp)(const void *s1, const void *s2, size_t n);
    void *(*memchr)(const void *s1, int c, size_t n);
    char *(*strcasestr) (const char* phaystack, const char* pneedle);

#if defined(DEBUG) || defined(SIMULATOR)
    void (*debugf)(const char *fmt, ...) ATTRIBUTE_PRINTF(1, 2);
#endif
#ifdef ROCKBOX_HAS_LOGF
    void (*logf)(const char *fmt, ...) ATTRIBUTE_PRINTF(1, 2);
#endif

    /* Tremor requires qsort */
    void (*qsort)(void *base, size_t nmemb, size_t size,
                  int(*compar)(const void *, const void *));

    /* The ADX codec accesses global_settings to test for REPEAT_ONE mode */
    struct user_settings* global_settings;

#ifdef RB_PROFILE
    void (*profile_thread)(void);
    void (*profstop)(void);
    void (*profile_func_enter)(void *this_fn, void *call_site);
    void (*profile_func_exit)(void *this_fn, void *call_site);
#endif
 
#if defined(HAVE_RECORDING) && !defined(SIMULATOR)
    volatile bool   stop_encoder;
    volatile int    enc_codec_loaded; /* <0=error, 0=pending, >0=ok */
    void            (*enc_get_inputs)(struct enc_inputs *inputs);
    void            (*enc_set_parameters)(struct enc_parameters *params);
    struct enc_chunk_hdr * (*enc_get_chunk)(void);
    void            (*enc_finish_chunk)(void);
    unsigned char * (*enc_get_pcm_data)(size_t size);
    size_t          (*enc_unget_pcm_data)(size_t size);

    /* file */
    int (*open)(const char* pathname, int flags);
    int (*close)(int fd);
    ssize_t (*read)(int fd, void* buf, size_t count);
    off_t (*lseek)(int fd, off_t offset, int whence);
    ssize_t (*write)(int fd, const void* buf, size_t count);
    int (*round_value_to_list32)(unsigned long value,
                                 const unsigned long list[],
                                 int count,
                                 bool signd);
#endif

    /* new stuff at the end, sort into place next time
       the API gets incompatible */     

};

/* codec header */
struct codec_header {
    unsigned long magic; /* RCOD or RENC */
    unsigned short target_id;
    unsigned short api_version;
    unsigned char *load_addr;
    unsigned char *end_addr;
    enum codec_status(*entry_point)(struct codec_api*);
};

extern unsigned char codecbuf[];
extern size_t codec_size;

#ifdef CODEC
#ifndef SIMULATOR
/* plugin_* is correct, codecs use the plugin linker script */
extern unsigned char plugin_start_addr[];
extern unsigned char plugin_end_addr[];
/* decoders */
#define CODEC_HEADER \
        const struct codec_header __header \
        __attribute__ ((section (".header")))= { \
        CODEC_MAGIC, TARGET_ID, CODEC_API_VERSION, \
        plugin_start_addr, plugin_end_addr, codec_start };
/* encoders */
#define CODEC_ENC_HEADER \
        const struct codec_header __header \
        __attribute__ ((section (".header")))= { \
        CODEC_ENC_MAGIC, TARGET_ID, CODEC_API_VERSION, \
        plugin_start_addr, plugin_end_addr, codec_start };

#else /* def SIMULATOR */
/* decoders */
#define CODEC_HEADER \
        const struct codec_header __header \
        __attribute__((visibility("default"))) = { \
        CODEC_MAGIC, TARGET_ID, CODEC_API_VERSION, \
        NULL, NULL, codec_start };
/* encoders */
#define CODEC_ENC_HEADER \
        const struct codec_header __header = { \
        CODEC_ENC_MAGIC, TARGET_ID, CODEC_API_VERSION, \
        NULL, NULL, codec_start };
#endif /* SIMULATOR */
#endif /* CODEC */

/* create full codec path from root filenames in audio_formats[]
   assumes buffer size is MAX_PATH */
void codec_get_full_path(char *path, const char *codec_root_fn);

/* defined by the codec loader (codec.c) */
int codec_load_buf(unsigned int hid, struct codec_api *api);
int codec_load_file(const char* codec, struct codec_api *api);

/* defined by the codec */
enum codec_status codec_start(struct codec_api* rockbox);
enum codec_status codec_main(void);

#ifndef CACHE_FUNCTION_WRAPPERS

#ifdef CACHE_FUNCTIONS_AS_CALL
#define CACHE_FUNCTION_WRAPPERS(api) \
        void flush_icache(void) \
        { \
            (api)->flush_icache(); \
        } \
        void invalidate_icache(void) \
        { \
            (api)->invalidate_icache(); \
        }
#else
#define CACHE_FUNCTION_WRAPPERS(api)
#endif /* CACHE_FUNCTIONS_AS_CALL */

#endif /* CACHE_FUNCTION_WRAPPERS */

#endif
