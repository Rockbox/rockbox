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
#include <sys/types.h>
#include "config.h"
#include "dir.h"
#include "kernel.h"
#include "button.h"
#include "font.h"
#include "system.h"
#include "lcd.h"
#include "id3.h"
#include "mpeg.h"
#include "audio.h"
#include "mp3_playback.h"
#if (HWCODEC == MASNONE)
#include "pcm_playback.h"
#endif
#include "settings.h"
#include "thread.h"
#include "playlist.h"
#ifdef HAVE_LCD_BITMAP
#include "widgets.h"
#endif
#include "sound.h"

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

#ifdef PLUGIN

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

/* These three sizes must match the ones set in plugins/plugin.lds */
#if MEM >= 32
#define PLUGIN_BUFFER_SIZE 0xC0000
#else
#define PLUGIN_BUFFER_SIZE 0x8000
#endif

#ifdef SIMULATOR
#define PREFIX(_x_) sim_ ## _x_
#else
#define PREFIX(_x_) _x_
#endif

/* increase this every time the api struct changes */
#define PLUGIN_API_VERSION 41

/* update this to latest version if a change to the api struct breaks
   backwards compatibility (and please take the opportunity to sort in any 
   new function which are "waiting" at the end of the function table) */
#define PLUGIN_MIN_API_VERSION 41

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
    void (*lcd_set_contrast)(int x);
    void (*lcd_clear_display)(void);
    void (*lcd_puts)(int x, int y, const unsigned char *string);
    void (*lcd_puts_scroll)(int x, int y, const unsigned char* string);
    void (*lcd_stop_scroll)(void);
#ifdef HAVE_LCD_CHARCELLS
    void (*lcd_define_pattern)(int which,const char *pattern);
    unsigned char (*lcd_get_locked_pattern)(void);
    void (*lcd_unlock_pattern)(unsigned char pat);
    void (*lcd_putc)(int x, int y, unsigned short ch);
    void (*lcd_put_cursor)(int x, int y, char cursor_char);
    void (*lcd_remove_cursor)(void);
    void (*PREFIX(lcd_icon))(int icon, bool enable);
#else
#ifndef SIMULATOR
    void (*lcd_roll)(int pixels);
#endif
    void (*lcd_set_drawmode)(int mode);
    int  (*lcd_get_drawmode)(void);
    void (*lcd_setfont)(int font);
    int  (*lcd_getstringsize)(const unsigned char *str, int *w, int *h);
    void (*lcd_drawpixel)(int x, int y);
    void (*lcd_drawline)(int x1, int y1, int x2, int y2);
    void (*lcd_drawrect)(int x, int y, int nx, int ny);
    void (*lcd_fillrect)(int x, int y, int nx, int ny);
    void (*lcd_bitmap)(const unsigned char *src, int x, int y,
                       int nx, int ny, bool clear);
    void (*lcd_putsxy)(int x, int y, const unsigned char *string);
    void (*lcd_puts_style)(int x, int y, const unsigned char *str, int style);
    void (*lcd_puts_scroll_style)(int x, int y, const unsigned char* string,
                                  int style);
    unsigned char* lcd_framebuffer;
    void (*lcd_blit) (const unsigned char* p_data, int x, int y, int width,
                      int height, int stride);
    void (*lcd_update)(void);
    void (*lcd_update_rect)(int x, int y, int width, int height);
    void (*scrollbar)(int x, int y, int width, int height, int items,
                      int min_shown, int max_shown, int orientation);
    void (*checkbox)(int x, int y, int width, int height, bool checked);
    struct font* (*font_get)(int font);
#endif
    void (*backlight_on)(void);
    void (*backlight_off)(void);
    void (*backlight_set_timeout)(int index);
    void (*splash)(int ticks, bool center, const char *fmt, ...);

#ifdef HAVE_REMOTE_LCD
    /* remote lcd */
    void (*remote_clear_display)(void);
    void (*remote_puts)(int x, int y, const unsigned char *string);
    void (*remote_lcd_puts_scroll)(int x, int y, const unsigned char* string);
    void (*remote_lcd_stop_scroll)(void);
    void (*remote_set_contrast)(int x);

    void (*remote_putsxy)(int x, int y, const unsigned char *string);
    void (*remote_puts_style)(int x, int y, const unsigned char *str, int style);
    void (*remote_puts_scroll_style)(int x, int y, const unsigned char* string,
                                  int style);
    void (*remote_bitmap)(const unsigned char *src, int x, int y,
                       int nx, int ny, bool clear);
    void (*remote_drawline)(int x1, int y1, int x2, int y2);
    void (*remote_clearline)(int x1, int y1, int x2, int y2);
    void (*remote_drawpixel)(int x, int y);
    void (*remote_clearpixel)(int x, int y);
    void (*remote_setfont)(int font);
    struct font* (*remote_font_get)(int font);
    void (*remote_clearrect)(int x, int y, int nx, int ny);
    void (*remote_fillrect)(int x, int y, int nx, int ny);
    void (*remote_drawrect)(int x, int y, int nx, int ny);
    void (*remote_invertrect)(int x, int y, int nx, int ny);
    int  (*remote_getstringsize)(const unsigned char *str, int *w, int *h);
    void (*remote_update)(void);
    void (*remote_update_rect)(int x, int y, int width, int height);

    void (*remote_backlight_on)(void);
    void (*remote_backlight_off)(void);
    unsigned char* lcd_remote_framebuffer;
#endif

    /* button */
    long (*button_get)(bool block);
    long (*button_get_w_tmo)(int ticks);
    int (*button_status)(void);
    void (*button_clear_queue)(void);
#if CONFIG_KEYPAD == IRIVER_H100_PAD
    bool (*button_hold)(void);
#endif

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
    off_t (*PREFIX(filesize))(int fd);
    int (*fdprintf)(int fd, const char *fmt, ...);
    int (*read_line)(int fd, char* buffer, int buffer_size);
    bool (*settings_parseline)(char* line, char** name, char** value);
#ifndef SIMULATOR
    int (*ata_sleep)(void);
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
    int (*create_thread)(void* function, void* stack, int stack_size, const char *name);
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

    /* sound */
    void (*sound_set)(int setting, int value);
#ifndef SIMULATOR
    void (*mp3_play_data)(const unsigned char* start, int size, void (*get_more)(unsigned char** start, int* size));
    void (*mp3_play_pause)(bool play);
    void (*mp3_play_stop)(void);
    bool (*mp3_is_playing)(void);
#if CONFIG_HWCODEC != MASNONE
    void (*bitswap)(unsigned char *data, int length);
#endif
#if CONFIG_HWCODEC == MASNONE
    void (*pcm_play_data)(const unsigned char *start, int size,
		    void (*get_more)(unsigned char** start, long*size));
    void (*pcm_play_stop)(void);
    void (*pcm_set_frequency)(unsigned int frequency);
    bool (*pcm_is_playing)(void);
    void (*pcm_play_pause)(bool play);
#endif
#endif /* !SIMULATOR */

    /* playback control */
    void (*PREFIX(audio_play))(int offset);
    void (*audio_stop)(void);
    void (*audio_pause)(void);
    void (*audio_resume)(void);
    void (*audio_next)(void);
    void (*audio_prev)(void);
    void (*audio_ff_rewind)(int newtime);
    struct mp3entry* (*audio_next_track)(void);
    int (*playlist_amount)(void);
    int (*audio_status)(void);
    bool (*audio_has_changed_track)(void);
    struct mp3entry* (*audio_current_track)(void);
    void (*audio_flush_and_reload_tracks)(void);
    int (*audio_get_file_pos)(void);
#if !defined(SIMULATOR) && (CONFIG_HWCODEC != MASNONE)
    unsigned long (*mpeg_get_last_header)(void);
#endif
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    void (*sound_set_pitch)(int pitch);        
#endif

    /* MAS communication */
#if !defined(SIMULATOR) && (CONFIG_HWCODEC != MASNONE)
    int (*mas_readmem)(int bank, int addr, unsigned long* dest, int len);
    int (*mas_writemem)(int bank, int addr, const unsigned long* src, int len);
    int (*mas_readreg)(int reg);
    int (*mas_writereg)(int reg, unsigned int val);
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    int (*mas_codec_writereg)(int reg, unsigned int val);
    int (*mas_codec_readreg)(int reg);
#endif
#endif

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
    void* (*plugin_get_buffer)(int* buffer_size);
    void* (*plugin_get_audio_buffer)(int* buffer_size);
#ifndef SIMULATOR
    int (*plugin_register_timer)(int cycles, int prio, void (*timer_callback)(void));
    void (*plugin_unregister_timer)(void);
#endif
    void (*plugin_tsr)(void (*exit_callback)(void));
#if defined(DEBUG) || defined(SIMULATOR)
    void (*debugf)(const char *fmt, ...);
#endif
    struct user_settings* global_settings;
    bool (*mp3info)(struct mp3entry *entry, const char *filename, bool v1first);
    int (*count_mp3_frames)(int fd, int startpos, int filesize,
                            void (*progressfunc)(int));
    int (*create_xing_header)(int fd, int startpos, int filesize,
                              unsigned char *buf, int num_frames,
                              unsigned long header_template,
                              void (*progressfunc)(int), bool generate_toc);
    unsigned long (*find_next_frame)(int fd, long *offset,
                                     long max_offset, unsigned long last_header);
    int (*battery_level)(void);
    bool (*battery_level_safe)(void);
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    unsigned short (*peak_meter_scale_value)(unsigned short val,
                                             int meterwidth);
    void (*peak_meter_set_use_dbfs)(int use);
    int (*peak_meter_get_use_dbfs)(void);
#endif
#ifdef HAVE_LCD_BITMAP
    int (*read_bmp_file)(char* filename, int *get_width, int *get_height,
                         char *bitmap, int maxsize);
#endif

    /* new stuff at the end, sort into place next time
       the API gets incompatible */     
       
#ifdef ROCKBOX_HAS_LOGF
    void (*logf)(const char *fmt, ...);
#endif
};

int plugin_load(const char* plugin, void* parameter);
void* plugin_get_buffer(int *buffer_size);
void* plugin_get_audio_buffer(int *buffer_size);
int plugin_register_timer(int cycles, int prio, void (*timer_callback)(void));
void plugin_unregister_timer(void);
void plugin_tsr(void (*exit_callback)(void));

/* defined by the plugin */
enum plugin_status plugin_start(struct plugin_api* rockbox, void* parameter)
    __attribute__ ((section (".entry")));

#endif
