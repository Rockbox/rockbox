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
#ifndef _PLUGIN_H_
#define _PLUGIN_H_

/* instruct simulator code to not redefine any symbols when compiling plugins.
   (the PLUGIN macro is defined in apps/plugins/Makefile) */
#ifdef PLUGIN
#define NO_REDEFINES_PLEASE
#endif

#ifndef MEM
#define MEM 2
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "dir.h"
#include "kernel.h"
#include "button.h"
#include "font.h"
#include "system.h"
#include "lcd.h"
#include "id3.h"
#include "mpeg.h"
#include "mp3_playback.h"
#include "settings.h"
#include "thread.h"

#ifdef PLUGIN
#if defined(DEBUG) || defined(SIMULATOR)
#define DEBUGF	rb->debugf
#define LDEBUGF rb->debugf
#else
#define DEBUGF(...)
#define LDEBUGF(...)
#endif
#endif

/* increase this every time the api struct changes */
#define PLUGIN_API_VERSION 14

/* update this to latest version if a change to the api struct breaks
   backwards compatibility (and please take the opportunity to sort in any 
   new function which are "waiting" at the end of the function table) */
#define PLUGIN_MIN_API_VERSION 10

/* plugin return codes */
enum plugin_status {
    PLUGIN_OK = 0,
    PLUGIN_USB_CONNECTED,

    PLUGIN_WRONG_API_VERSION = -1,
    PLUGIN_WRONG_MODEL = -2,
    PLUGIN_ERROR = -3,
};

/* different (incompatible) plugin models */
enum model {
    PLAYER,
    RECORDER
};

#ifdef HAVE_LCD_CHARCELLS
#define MODEL PLAYER
#else
#define MODEL RECORDER
#endif

/* compatibility test macro */
#define TEST_PLUGIN_API(_api_) \
do { \
 int _rc_ = _api_->plugin_test(PLUGIN_API_VERSION, MODEL, MEM); \
 if (_rc_<0) \
     return _rc_; \
} while(0)

/* NOTE: To support backwards compatibility, only add new functions at
         the end of the structure.  Every time you add a new function,
         remember to increase PLUGIN_API_VERSION.  If you make changes to the
         existing APIs then also update PLUGIN_MIN_API_VERSION to current
         version
 */
struct plugin_api {
    /* these two fields must always be first, to ensure
       TEST_PLUGIN_API will always work */
    int version;
    int (*plugin_test)(int api_version, int model, int memsize);

    /* lcd */
    void (*lcd_clear_display)(void);
    void (*lcd_puts)(int x, int y, unsigned char *string);
    void (*lcd_puts_scroll)(int x, int y, unsigned char* string);
    void (*lcd_stop_scroll)(void);
#ifdef HAVE_LCD_CHARCELLS
    void (*lcd_define_pattern)(int which,char *pattern);
    unsigned char (*lcd_get_locked_pattern)(void);
    void (*lcd_unlock_pattern)(unsigned char pat);
    void (*lcd_putc)(int x, int y, unsigned short ch);
#else
    void (*lcd_putsxy)(int x, int y, unsigned char *string);
    void (*lcd_bitmap)(unsigned char *src, int x, int y,
                       int nx, int ny, bool clear);
    void (*lcd_drawline)(int x1, int y1, int x2, int y2);
    void (*lcd_clearline)(int x1, int y1, int x2, int y2);
    void (*lcd_drawpixel)(int x, int y);
    void (*lcd_clearpixel)(int x, int y);
    void (*lcd_setfont)(int font);
    void (*lcd_clearrect)(int x, int y, int nx, int ny);
    void (*lcd_fillrect)(int x, int y, int nx, int ny);
    void (*lcd_drawrect)(int x, int y, int nx, int ny);
    void (*lcd_invertrect)(int x, int y, int nx, int ny);
    int  (*lcd_getstringsize)(unsigned char *str, int *w, int *h);
    void (*lcd_update)(void);
    void (*lcd_update_rect)(int x, int y, int width, int height);
    void (*progressbar)(int x, int y, int width, int height,
                        int percent, int direction);
    void (*slidebar)(int x, int y, int width, int height,
                     int percent, int direction);
    void (*scrollbar)(int x, int y, int width, int height, int items,
                      int min_shown, int max_shown, int orientation);
#ifndef SIMULATOR
    void (*lcd_roll)(int pixels);
#endif
#endif

    /* button */
    int (*button_get)(bool block);
    int (*button_get_w_tmo)(int ticks);

    /* file */
    int (*open)(const char* pathname, int flags);
    int (*close)(int fd);
    ssize_t (*read)(int fd, void* buf, size_t count);
    off_t (*lseek)(int fd, off_t offset, int whence);
    int (*creat)(const char *pathname, mode_t mode);
    ssize_t (*write)(int fd, const void* buf, size_t count);
    int (*remove)(const char* pathname);
    int (*rename)(const char* path, const char* newname);
    int (*ftruncate)(int fd, off_t length);
    int (*filesize)(int fd);
    int (*fprintf)(int fd, const char *fmt, ...);
    int (*read_line)(int fd, char* buffer, int buffer_size);
    
    /* dir */
    DIR* (*opendir)(const char* name);
    int (*closedir)(DIR* dir);
    struct dirent* (*readdir)(DIR* dir);

    /* kernel */
    void (*sleep)(int ticks);
    void (*usb_screen)(void);
    long* current_tick;

    /* strings and memory */
    int    (*snprintf)(char *buf, size_t size, const char *fmt, ...);
    char*  (*strcpy)(char *dst, const char *src);
    size_t (*strlen)(const char *str);
    void*  (*memset)(void *dst, int c, size_t length);
    void*  (*memcpy)(void *out, const void *in, size_t n);

    /* sound */
#ifndef SIMULATOR
#ifdef HAVE_MAS3587F
    int (*mas_codec_readreg)(int reg);
#endif
#endif

    /* misc */
    void (*srand)(unsigned int seed);
    int  (*rand)(void);
    void (*splash)(int ticks, bool center, char *fmt, ...);
    void (*qsort)(void *base, size_t nmemb, size_t size,
                  int(*compar)(const void *, const void *));
    int (*kbd_input)(char* buffer, int buflen);
    struct mp3entry* (*mpeg_current_track)(void);
    int (*atoi)(const char *str);
    struct tm* (*get_time)(void);
    void* (*plugin_get_buffer)(int* buffer_size);

    /* new stuff, sort in next time the API gets broken! */
#ifndef HAVE_LCD_CHARCELLS
    unsigned char* lcd_framebuffer;
    void (*lcd_blit) (unsigned char* p_data, int x, int y, int width, int height, int stride);
#endif
    void (*yield)(void);

    void* (*plugin_get_mp3_buffer)(int* buffer_size);
    void (*mpeg_sound_set)(int setting, int value);
#ifndef SIMULATOR
    void (*mp3_play_init)(void); /* FIXME: remove this next time we break compatibility */
    void (*mp3_play_data)(unsigned char* start, int size, void (*get_more)(unsigned char** start, int* size));
    void (*mp3_play_pause)(bool play);
    void (*mp3_play_stop)(void);
    bool (*mp3_is_playing)(void);
    void (*bitswap)(unsigned char *data, int length);
#endif
    struct user_settings* global_settings;
    void (*backlight_set_timeout)(int index);
#ifndef SIMULATOR
    int (*ata_sleep)(void);
#endif
#ifdef HAVE_LCD_BITMAP
    void (*checkbox)(int x, int y, int width, int height, bool checked);
#endif
#ifndef SIMULATOR
    int (*plugin_register_timer)(int cycles, int prio, void (*timer_callback)(void));
    void (*plugin_unregister_timer)(void);
#endif
    void (*plugin_tsr)(void (*exit_callback)(void));
    int (*create_thread)(void* function, void* stack, int stack_size, char *name);
    void (*remove_tread)(int threadnum);
    void (*lcd_set_contrast)(int x);

    /* playback control */
    void (*mpeg_play)(int offset);
    void (*mpeg_stop)(void);
    void (*mpeg_pause)(void);
    void (*mpeg_resume)(void);
    void (*mpeg_next)(void);
    void (*mpeg_prev)(void);
    void (*mpeg_ff_rewind)(int newtime);
    struct mp3entry* (*mpeg_next_track)(void);
    bool (*mpeg_has_changed_track)(void);
    int (*mpeg_status)(void);
    
#ifdef HAVE_LCD_BITMAP
    struct font* (*font_get)(int font);
#endif
#if defined(DEBUG) || defined(SIMULATOR)
   void (*debugf)(char *fmt, ...);
#endif
   bool (*mp3info)(struct mp3entry *entry, char *filename) ;
   int (*count_mp3_frames)(int fd, int startpos, int filesize,
                           void (*progressfunc)(int));
   int (*create_xing_header)(int fd, int startpos, int filesize,
                             unsigned char *buf, int num_frames,
                             unsigned long header_template,
                             void (*progressfunc)(int), bool generate_toc);
};

/* defined by the plugin loader (plugin.c) */
int plugin_load(char* plugin, void* parameter);
void* plugin_get_buffer(int *buffer_size);
void* plugin_get_mp3_buffer(int *buffer_size);
int plugin_register_timer(int cycles, int prio, void (*timer_callback)(void));
void plugin_unregister_timer(void);
void plugin_tsr(void (*exit_callback)(void));

/* defined by the plugin */
enum plugin_status plugin_start(struct plugin_api* rockbox, void* parameter)
    __attribute__ ((section (".entry")));

#endif
