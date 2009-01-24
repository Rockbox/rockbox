/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Bj�rn Stenberg
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
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* plugin_get_buffer(size_t *buffer_size);

#ifndef __PCTOOL__
#include "config.h"
#include "system.h"
#include "dir.h"
#include "general.h"
#include "kernel.h"
#include "thread.h"
#include "button.h"
#include "action.h"
#include "usb.h"
#include "font.h"
#include "lcd.h"
#include "metadata.h"
#include "sound.h"
#include "mpeg.h"
#include "audio.h"
#include "mp3_playback.h"
#include "talk.h"
#ifdef RB_PROFILE
#include "profile.h"
#endif
#include "misc.h"
#if (CONFIG_CODEC == SWCODEC)
#include "dsp.h"
#include "codecs.h"
#include "playback.h"
#include "metadata.h"
#ifdef HAVE_RECORDING
#include "recording.h"
#endif
#else
#include "mas.h"
#endif /* CONFIG_CODEC == SWCODEC */
#include "settings.h"
#include "timer.h"
#include "playlist.h"
#ifdef HAVE_LCD_BITMAP
#include "scrollbar.h"
#include "../recorder/bmp.h"
#endif
#include "statusbar.h"
#include "menu.h"
#include "rbunicode.h"
#include "list.h"
#include "tree.h"
#include "color_picker.h"
#include "buffering.h"
#include "tagcache.h"
#include "viewport.h"
#include "ata_idle_notify.h"
#include "settings_list.h"

#ifdef HAVE_ALBUMART
#include "albumart.h"
#endif

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

#include "yesno.h"

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

#define PLUGIN_MAGIC 0x526F634B /* RocK */

/* increase this every time the api struct changes */
#define PLUGIN_API_VERSION 139

/* update this to latest version if a change to the api struct breaks
   backwards compatibility (and please take the opportunity to sort in any
   new function which are "waiting" at the end of the function table) */
#define PLUGIN_MIN_API_VERSION 137

/* plugin return codes */
enum plugin_status {
    PLUGIN_OK = 0,
    PLUGIN_USB_CONNECTED,
    PLUGIN_ERROR = -1,
};

/* NOTE: To support backwards compatibility, only add new functions at
         the end of the structure.  Every time you add a new function,
         remember to increase PLUGIN_API_VERSION.  If you make changes to the
         existing APIs then also update PLUGIN_MIN_API_VERSION to current
         version
 */
struct plugin_api {

    /* lcd */
    void (*lcd_set_contrast)(int x);
    void (*lcd_update)(void);
    void (*lcd_clear_display)(void);
    int  (*lcd_getstringsize)(const unsigned char *str, int *w, int *h);
    void (*lcd_putsxy)(int x, int y, const unsigned char *string);
    void (*lcd_puts)(int x, int y, const unsigned char *string);
    void (*lcd_puts_scroll)(int x, int y, const unsigned char* string);
    void (*lcd_stop_scroll)(void);
#ifdef HAVE_LCD_CHARCELLS
    void (*lcd_define_pattern)(unsigned long ucs, const char *pattern);
    unsigned long (*lcd_get_locked_pattern)(void);
    void (*lcd_unlock_pattern)(unsigned long ucs);
    void (*lcd_putc)(int x, int y, unsigned long ucs);
    void (*lcd_put_cursor)(int x, int y, unsigned long ucs);
    void (*lcd_remove_cursor)(void);
    void (*lcd_icon)(int icon, bool enable);
    void (*lcd_double_height)(bool on);
#else /* HAVE_LCD_BITMAP */
    fb_data* lcd_framebuffer;
    void (*lcd_update_rect)(int x, int y, int width, int height);
    void (*lcd_set_drawmode)(int mode);
    int  (*lcd_get_drawmode)(void);
    void (*lcd_setfont)(int font);
    void (*lcd_drawpixel)(int x, int y);
    void (*lcd_drawline)(int x1, int y1, int x2, int y2);
    void (*lcd_hline)(int x1, int x2, int y);
    void (*lcd_vline)(int x, int y1, int y2);
    void (*lcd_drawrect)(int x, int y, int width, int height);
    void (*lcd_fillrect)(int x, int y, int width, int height);
    void (*lcd_mono_bitmap_part)(const unsigned char *src, int src_x, int src_y,
                                 int stride, int x, int y, int width, int height);
    void (*lcd_mono_bitmap)(const unsigned char *src, int x, int y,
                            int width, int height);
#if LCD_DEPTH > 1
    void     (*lcd_set_foreground)(unsigned foreground);
    unsigned (*lcd_get_foreground)(void);
    void     (*lcd_set_background)(unsigned foreground);
    unsigned (*lcd_get_background)(void);
    void (*lcd_bitmap_part)(const fb_data *src, int src_x, int src_y,
                            int stride, int x, int y, int width, int height);
    void (*lcd_bitmap)(const fb_data *src, int x, int y, int width,
                       int height);
    fb_data* (*lcd_get_backdrop)(void);
    void (*lcd_set_backdrop)(fb_data* backdrop);
#endif
#if LCD_DEPTH == 16
    void (*lcd_bitmap_transparent_part)(const fb_data *src,
            int src_x, int src_y, int stride,
            int x, int y, int width, int height);
    void (*lcd_bitmap_transparent)(const fb_data *src, int x, int y,
            int width, int height);
    void (*lcd_blit_yuv)(unsigned char * const src[3],
                         int src_x, int src_y, int stride,
                         int x, int y, int width, int height);
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200) || defined(SANSA_C200) \
    || defined(IRIVER_H10) || defined(COWON_D2)
    void (*lcd_yuv_set_options)(unsigned options);
#endif
#elif (LCD_DEPTH < 4) && !defined(SIMULATOR)
    void (*lcd_blit_mono)(const unsigned char *data, int x, int by, int width,
                          int bheight, int stride);
    void (*lcd_blit_grey_phase)(unsigned char *values, unsigned char *phases,
                                int bx, int by, int bwidth, int bheight,
                                int stride);
#endif /* LCD_DEPTH */
    void (*lcd_puts_style)(int x, int y, const unsigned char *str, int style);
    void (*lcd_puts_scroll_style)(int x, int y, const unsigned char* string,
                                  int style);
#ifdef HAVE_LCD_INVERT
    void (*lcd_set_invert_display)(bool yesno);
#endif /* HAVE_LCD_INVERT */

#if defined(HAVE_LCD_ENABLE) && defined(HAVE_LCD_COLOR)
    void (*lcd_set_enable_hook)(void (*enable_hook)(void));
    struct event_queue *button_queue;
#endif
    unsigned short *(*bidi_l2v)( const unsigned char *str, int orientation );
    const unsigned char *(*font_get_bits)( struct font *pf, unsigned short char_code );
    struct font* (*font_load)(const char *path);
    struct font* (*font_get)(int font);
    int  (*font_getstringsize)(const unsigned char *str, int *w, int *h,
                               int fontnumber);
    int (*font_get_width)(struct font* pf, unsigned short char_code);
    void (*screen_clear_area)(struct screen * display, int xstart, int ystart,
                              int width, int height);
    void (*gui_scrollbar_draw)(struct screen * screen, int x, int y,
                               int width, int height, int items,
                               int min_shown, int max_shown,
                               unsigned flags);
#endif  /* HAVE_LCD_BITMAP */
    const char* (*get_codepage_name)(int cp);

    /* backlight */
    /* The backlight_* functions must be present in the API regardless whether
     * HAVE_BACKLIGHT is defined or not. The reason is that the stock Ondio has
     * no backlight but can be modded to have backlight (it's prepared on the
     * PCB). This makes backlight an all-target feature API wise, and keeps API
     * compatible between stock and modded Ondio.
     * For OLED targets like the Sansa Clip, the backlight_* functions control
     * the display enable, which has essentially the same effect. */
    void (*backlight_on)(void);
    void (*backlight_off)(void);
    void (*backlight_set_timeout)(int index);
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    void (*backlight_set_brightness)(int val);
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#if CONFIG_CHARGING
    void (*backlight_set_timeout_plugged)(int index);
#endif
    bool (*is_backlight_on)(bool ignore_always_off);
    void (*splash)(int ticks, const char *str);
    void (*splashf)(int ticks, const char *fmt, ...) ATTRIBUTE_PRINTF(2, 3);

#ifdef HAVE_REMOTE_LCD
    /* remote lcd */
    void (*lcd_remote_set_contrast)(int x);
    void (*lcd_remote_clear_display)(void);
    void (*lcd_remote_puts)(int x, int y, const unsigned char *string);
    void (*lcd_remote_puts_scroll)(int x, int y, const unsigned char* string);
    void (*lcd_remote_stop_scroll)(void);
    void (*lcd_remote_set_drawmode)(int mode);
    int  (*lcd_remote_get_drawmode)(void);
    void (*lcd_remote_setfont)(int font);
    int  (*lcd_remote_getstringsize)(const unsigned char *str, int *w, int *h);
    void (*lcd_remote_drawpixel)(int x, int y);
    void (*lcd_remote_drawline)(int x1, int y1, int x2, int y2);
    void (*lcd_remote_hline)(int x1, int x2, int y);
    void (*lcd_remote_vline)(int x, int y1, int y2);
    void (*lcd_remote_drawrect)(int x, int y, int nx, int ny);
    void (*lcd_remote_fillrect)(int x, int y, int nx, int ny);
    void (*lcd_remote_mono_bitmap_part)(const unsigned char *src, int src_x,
                                        int src_y, int stride, int x, int y,
                                        int width, int height);
    void (*lcd_remote_mono_bitmap)(const unsigned char *src, int x, int y,
                                   int width, int height);
    void (*lcd_remote_putsxy)(int x, int y, const unsigned char *string);
    void (*lcd_remote_puts_style)(int x, int y, const unsigned char *str, int style);
    void (*lcd_remote_puts_scroll_style)(int x, int y, const unsigned char* string,
                                         int style);
    fb_remote_data* lcd_remote_framebuffer;
    void (*lcd_remote_update)(void);
    void (*lcd_remote_update_rect)(int x, int y, int width, int height);

    void (*remote_backlight_on)(void);
    void (*remote_backlight_off)(void);
    void (*remote_backlight_set_timeout)(int index);
#if CONFIG_CHARGING
    void (*remote_backlight_set_timeout_plugged)(int index);
#endif
#endif /* HAVE_REMOTE_LCD */
    struct screen* screens[NB_SCREENS];
#if defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    void     (*lcd_remote_set_foreground)(unsigned foreground);
    unsigned (*lcd_remote_get_foreground)(void);
    void     (*lcd_remote_set_background)(unsigned background);
    unsigned (*lcd_remote_get_background)(void);
    void (*lcd_remote_bitmap_part)(const fb_remote_data *src, int src_x, int src_y,
                                   int stride, int x, int y, int width, int height);
    void (*lcd_remote_bitmap)(const fb_remote_data *src, int x, int y, int width,
                              int height);
#endif
    void (*viewport_set_defaults)(struct viewport *vp, enum screen_type screen);
    bool (*viewportmanager_set_statusbar)(bool enabled);
    /* list */
    void (*gui_synclist_init)(struct gui_synclist * lists,
            list_get_name callback_get_item_name, void * data,
            bool scroll_all,int selected_size,
            struct viewport parent[NB_SCREENS]);
    void (*gui_synclist_set_nb_items)(struct gui_synclist * lists, int nb_items);
    void (*gui_synclist_set_icon_callback)(struct gui_synclist * lists, list_get_icon icon_callback);
    int (*gui_synclist_get_nb_items)(struct gui_synclist * lists);
    int  (*gui_synclist_get_sel_pos)(struct gui_synclist * lists);
    void (*gui_synclist_draw)(struct gui_synclist * lists);
    void (*gui_synclist_select_item)(struct gui_synclist * lists,
    int item_number);
    void (*gui_synclist_add_item)(struct gui_synclist * lists);
    void (*gui_synclist_del_item)(struct gui_synclist * lists);
    void (*gui_synclist_limit_scroll)(struct gui_synclist * lists, bool scroll);
    bool (*gui_synclist_do_button)(struct gui_synclist * lists,
                                   int *action, enum list_wrap wrap);
    void (*gui_synclist_set_title)(struct gui_synclist *lists, char* title, int icon);
    enum yesno_res (*gui_syncyesno_run)(const struct text_message * main_message,
                                        const struct text_message * yes_message,
                                        const struct text_message * no_message);
    void (*simplelist_info_init)(struct simplelist_info *info, char* title,
           int count, void* data);
    bool (*simplelist_show_list)(struct simplelist_info *info);

    /* button */
    long (*button_get)(bool block);
    long (*button_get_w_tmo)(int ticks);
    int (*button_status)(void);
#ifdef HAVE_BUTTON_DATA
    intptr_t (*button_get_data)(void);
#endif
    void (*button_clear_queue)(void);
    int (*button_queue_count)(void);
#ifdef HAS_BUTTON_HOLD
    bool (*button_hold)(void);
#endif
#ifdef HAVE_TOUCHSCREEN
    void (*touchscreen_set_mode)(enum touchscreen_mode);
#endif
#ifdef HAVE_BUTTON_LIGHT
    void (*buttonlight_set_timeout)(int value);
    void (*buttonlight_off)(void);
    void (*buttonlight_on)(void);
#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
    void (*buttonlight_set_brightness)(int val);
#endif /* HAVE_BUTTONLIGHT_BRIGHTNESS */
#endif /* HAVE_BUTTON_LIGHT */

    /* file */
    int (*open)(const char* pathname, int flags);
    int (*close)(int fd);
    ssize_t (*read)(int fd, void* buf, size_t count);
    off_t (*lseek)(int fd, off_t offset, int whence);
    int (*creat)(const char *pathname);
    ssize_t (*write)(int fd, const void* buf, size_t count);
    int (*remove)(const char* pathname);
    int (*rename)(const char* path, const char* newname);
    int (*ftruncate)(int fd, off_t length);
    off_t (*filesize)(int fd);
    int (*fdprintf)(int fd, const char *fmt, ...) ATTRIBUTE_PRINTF(2, 3);
    int (*read_line)(int fd, char* buffer, int buffer_size);
    bool (*settings_parseline)(char* line, char** name, char** value);
    void (*storage_sleep)(void);
    void (*storage_spin)(void);
    void (*storage_spindown)(int seconds);
#if USING_STORAGE_CALLBACK
    void (*register_storage_idle_func)(storage_idle_notify function);
    void (*unregister_storage_idle_func)(storage_idle_notify function, bool run);
#endif /* USING_STORAGE_CALLBACK */
    void (*reload_directory)(void);
    char *(*create_numbered_filename)(char *buffer, const char *path,
                                      const char *prefix, const char *suffix,
                                      int numberlen IF_CNFN_NUM_(, int *num));
    bool (*file_exists)(const char *file);


    /* dir */
    DIR* (*opendir)(const char* name);
    int (*closedir)(DIR* dir);
    struct dirent* (*readdir)(DIR* dir);
    int (*mkdir)(const char *name);
    int (*rmdir)(const char *name);
    bool (*dir_exists)(const char *path);

    /* kernel/ system */
    void (*sleep)(int ticks);
    void (*yield)(void);
    volatile long* current_tick;
    long (*default_event_handler)(long event);
    long (*default_event_handler_ex)(long event, void (*callback)(void *), void *parameter);
    unsigned int (*create_thread)(void (*function)(void), void* stack,
                                  size_t stack_size, unsigned flags,
                                  const char *name
                                  IF_PRIO(, int priority)
                                  IF_COP(, unsigned int core));
    void (*thread_exit)(void);
    void (*thread_wait)(unsigned int thread_id);
#if CONFIG_CODEC == SWCODEC
    void (*thread_thaw)(unsigned int thread_id);
#ifdef HAVE_PRIORITY_SCHEDULING
    int (*thread_set_priority)(unsigned int thread_id, int priority);
#endif
    void (*mutex_init)(struct mutex *m);
    void (*mutex_lock)(struct mutex *m);
    void (*mutex_unlock)(struct mutex *m);
#endif

    void (*reset_poweroff_timer)(void);
#ifndef SIMULATOR
    int (*system_memory_guard)(int newmode);
    long *cpu_frequency;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#ifdef CPU_BOOST_LOGGING
    void (*cpu_boost_)(bool on_off,char*location,int line);
#else
    void (*cpu_boost)(bool on_off);
#endif
#endif /* HAVE_ADJUSTABLE_CPU_FREQ */
#endif /* !SIMULATOR */
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    void (*trigger_cpu_boost)(void);
    void (*cancel_cpu_boost)(void);
#endif
#ifdef CACHE_FUNCTIONS_AS_CALL
    void (*flush_icache)(void);
    void (*invalidate_icache)(void);
#endif
    bool (*timer_register)(int reg_prio, void (*unregister_callback)(void),
                           long cycles, int int_prio,
                           void (*timer_callback)(void) IF_COP(, int core));
    void (*timer_unregister)(void);
    bool (*timer_set_period)(long count);

    void (*queue_init)(struct event_queue *q, bool register_queue);
    void (*queue_delete)(struct event_queue *q);
    void (*queue_post)(struct event_queue *q, long id, intptr_t data);
    void (*queue_wait_w_tmo)(struct event_queue *q, struct queue_event *ev,
            int ticks);
#if CONFIG_CODEC == SWCODEC
    void (*queue_enable_queue_send)(struct event_queue *q,
                                    struct queue_sender_list *send,
                                    unsigned int thread_id);
    bool (*queue_empty)(const struct event_queue *q);
    void (*queue_wait)(struct event_queue *q, struct queue_event *ev);
    intptr_t (*queue_send)(struct event_queue *q, long id,
                           intptr_t data);
    void (*queue_reply)(struct event_queue *q, intptr_t retval);
#endif /* CONFIG_CODEC == SWCODEC */

    void (*usb_acknowledge)(long id);
#ifdef RB_PROFILE
    void (*profile_thread)(void);
    void (*profstop)(void);
    void (*profile_func_enter)(void *this_fn, void *call_site);
    void (*profile_func_exit)(void *this_fn, void *call_site);
#endif

#ifdef SIMULATOR
    /* special simulator hooks */
#if defined(HAVE_LCD_BITMAP) && LCD_DEPTH < 8
    void (*sim_lcd_ex_init)(int shades, unsigned long (*getpixel)(int, int));
    void (*sim_lcd_ex_update_rect)(int x, int y, int width, int height);
#endif
#endif

    /* strings and memory */
    int (*snprintf)(char *buf, size_t size, const char *fmt, ...)
                    ATTRIBUTE_PRINTF(3, 4);
    int (*vsnprintf)(char *buf, int size, const char *fmt, va_list ap);
    char* (*strcpy)(char *dst, const char *src);
    char* (*strncpy)(char *dst, const char *src, size_t length);
    size_t (*strlen)(const char *str);
    char * (*strrchr)(const char *s, int c);
    int (*strcmp)(const char *, const char *);
    int (*strncmp)(const char *, const char *, size_t);
    int (*strcasecmp)(const char *, const char *);
    int (*strncasecmp)(const char *s1, const char *s2, size_t n);
    void* (*memset)(void *dst, int c, size_t length);
    void* (*memcpy)(void *out, const void *in, size_t n);
    void* (*memmove)(void *out, const void *in, size_t n);
    const unsigned char *_ctype_;
    int (*atoi)(const char *str);
    char *(*strchr)(const char *s, int c);
    char *(*strcat)(char *s1, const char *s2);
    void *(*memchr)(const void *s1, int c, size_t n);
    int (*memcmp)(const void *s1, const void *s2, size_t n);
    char *(*strcasestr) (const char* phaystack, const char* pneedle);
    char* (*strtok_r)(char *ptr, const char *sep, char **end);
    /* unicode stuff */
    const unsigned char* (*utf8decode)(const unsigned char *utf8, unsigned short *ucs);
    unsigned char* (*iso_decode)(const unsigned char *iso, unsigned char *utf8, int cp, int count);
    unsigned char* (*utf16LEdecode)(const unsigned char *utf16, unsigned char *utf8, int count);
    unsigned char* (*utf16BEdecode)(const unsigned char *utf16, unsigned char *utf8, int count);
    unsigned char* (*utf8encode)(unsigned long ucs, unsigned char *utf8);
    unsigned long (*utf8length)(const unsigned char *utf8);
    int (*utf8seek)(const unsigned char* utf8, int offset);

    /* sound */
    void (*sound_set)(int setting, int value);
    int (*sound_default)(int setting);
    int (*sound_min)(int setting);
    int (*sound_max)(int setting);
    const char * (*sound_unit)(int setting);
    int (*sound_val2phys)(int setting, int value);
#ifndef SIMULATOR
    void (*mp3_play_data)(const unsigned char* start, int size, void (*get_more)(unsigned char** start, size_t* size));
    void (*mp3_play_pause)(bool play);
    void (*mp3_play_stop)(void);
    bool (*mp3_is_playing)(void);
#if CONFIG_CODEC != SWCODEC
    void (*bitswap)(unsigned char *data, int length);
#endif
#endif /* !SIMULATOR */
#if CONFIG_CODEC == SWCODEC
    const unsigned long *audio_master_sampr_list;
    const unsigned long *hw_freq_sampr;
    void (*pcm_apply_settings)(void);
    void (*pcm_play_data)(pcm_more_callback_type get_more,
            unsigned char* start, size_t size);
    void (*pcm_play_stop)(void);
    void (*pcm_set_frequency)(unsigned int frequency);
    bool (*pcm_is_playing)(void);
    bool (*pcm_is_paused)(void);
    void (*pcm_play_pause)(bool play);
    size_t (*pcm_get_bytes_waiting)(void);
    void (*pcm_calculate_peaks)(int *left, int *right);
    void (*pcm_play_lock)(void);
    void (*pcm_play_unlock)(void);
#ifdef HAVE_RECORDING
    const unsigned long *rec_freq_sampr;
    void (*pcm_init_recording)(void);
    void (*pcm_close_recording)(void);
    void (*pcm_record_data)(pcm_more_callback_type2 more_ready,
                            void *start, size_t size);
    void (*pcm_record_more)(void *start, size_t size);
    void (*pcm_stop_recording)(void);
    void (*pcm_calculate_rec_peaks)(int *left, int *right);
    void (*audio_set_recording_gain)(int left, int right, int type);
#endif /* HAVE_RECORDING */
#if INPUT_SRC_CAPS != 0
    void (*audio_set_output_source)(int monitor);
    void (*audio_set_input_source)(int source, unsigned flags);
#endif
    void (*dsp_set_crossfeed)(bool enable);
    void (*dsp_set_eq)(bool enable);
    void (*dsp_dither_enable)(bool enable);
    intptr_t (*dsp_configure)(struct dsp_config *dsp, int setting,
                              intptr_t value);
    int (*dsp_process)(struct dsp_config *dsp, char *dest,
                       const char *src[], int count);
#endif /* CONFIG_CODEC == SWCODC */

    /* playback control */
    int (*playlist_amount)(void);
    int (*playlist_resume)(void);
    int (*playlist_start)(int start_index, int offset);
    void (*audio_play)(long offset);
    void (*audio_stop)(void);
    void (*audio_pause)(void);
    void (*audio_resume)(void);
    void (*audio_next)(void);
    void (*audio_prev)(void);
    void (*audio_ff_rewind)(long newtime);
    struct mp3entry* (*audio_next_track)(void);
    int (*audio_status)(void);
    bool (*audio_has_changed_track)(void);
    struct mp3entry* (*audio_current_track)(void);
    void (*audio_flush_and_reload_tracks)(void);
    int (*audio_get_file_pos)(void);
#if !defined(SIMULATOR) && (CONFIG_CODEC != SWCODEC)
    unsigned long (*mpeg_get_last_header)(void);
#endif
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) || \
    (CONFIG_CODEC == SWCODEC)
    void (*sound_set_pitch)(int pitch);
#endif

    /* MAS communication */
#if !defined(SIMULATOR) && (CONFIG_CODEC != SWCODEC)
    int (*mas_readmem)(int bank, int addr, unsigned long* dest, int len);
    int (*mas_writemem)(int bank, int addr, const unsigned long* src, int len);
    int (*mas_readreg)(int reg);
    int (*mas_writereg)(int reg, unsigned int val);
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    int (*mas_codec_writereg)(int reg, unsigned int val);
    int (*mas_codec_readreg)(int reg);
    void (*i2c_begin)(void);
    void (*i2c_end)(void);
    int  (*i2c_write)(int address, const unsigned char* buf, int count );
#endif
#endif

    /* menu */
    int (*do_menu)(const struct menu_item_ex *menu, int *start_selected,
                   struct viewport parent[NB_SCREENS], bool hide_bars);

    /* scroll bar */
    struct gui_syncstatusbar *statusbars;
    void (*gui_syncstatusbar_draw)(struct gui_syncstatusbar * bars, bool force_redraw);

    /* options */
    const struct settings_list* (*get_settings_list)(int*count);
    const struct settings_list* (*find_setting)(const void* variable, int *id);
    bool (*option_screen)(const struct settings_list *setting,
                          struct viewport parent[NB_SCREENS],
                          bool use_temp_var, unsigned char* option_title);
    bool (*set_option)(const char* string, const void* variable,
                       enum optiontype type, const struct opt_items* options,
                       int numoptions, void (*function)(int));
    bool (*set_bool_options)(const char* string, const bool* variable,
                             const char* yes_str, int yes_voice,
                             const char* no_str, int no_voice,
                             void (*function)(bool));
    bool (*set_int)(const unsigned char* string, const char* unit, int voice_unit,
                    const int* variable, void (*function)(int), int step,
                    int min, int max,
                    void (*formatter)(char*, size_t, int, const char*) );
    bool (*set_bool)(const char* string, const bool* variable );

#ifdef HAVE_LCD_COLOR
    bool (*set_color)(struct screen *display, char *title, unsigned *color,
                   unsigned banned_color);
#endif
    /* action handling */
    int (*get_custom_action)(int context,int timeout,
                          const struct button_mapping* (*get_context_map)(int));
    int (*get_action)(int context, int timeout);
    bool (*action_userabort)(int timeout);

    /* power */
    int (*battery_level)(void);
    bool (*battery_level_safe)(void);
    int (*battery_time)(void);
#ifndef SIMULATOR
    unsigned int (*battery_voltage)(void);
#endif
#if CONFIG_CHARGING
    bool (*charger_inserted)(void);
# if CONFIG_CHARGING == CHARGING_MONITOR
    bool (*charging_state)(void);
# endif
#endif
#ifdef HAVE_USB_POWER
    bool (*usb_powered)(void);
#endif

    /* misc */
    void (*srand)(unsigned int seed);
    int  (*rand)(void);
    void (*qsort)(void *base, size_t nmemb, size_t size,
                  int(*compar)(const void *, const void *));
    int (*kbd_input)(char* buffer, int buflen);
    struct tm* (*get_time)(void);
    int  (*set_time)(const struct tm *tm);
#if CONFIG_RTC
    time_t (*mktime)(struct tm *t);
#endif
    void* (*plugin_get_buffer)(size_t *buffer_size);
    void* (*plugin_get_audio_buffer)(size_t *buffer_size);
    void (*plugin_tsr)(bool (*exit_callback)(bool reenter));
    char* (*plugin_get_current_filename)(void);
#ifdef PLUGIN_USE_IRAM
    void (*plugin_iram_init)(char *iramstart, char *iramcopy, size_t iram_size,
                             char *iedata, size_t iedata_size);
#endif
#if defined(DEBUG) || defined(SIMULATOR)
    void (*debugf)(const char *fmt, ...) ATTRIBUTE_PRINTF(1, 2);
#endif
#ifdef ROCKBOX_HAS_LOGF
    void (*logf)(const char *fmt, ...) ATTRIBUTE_PRINTF(1, 2);
#endif
    struct user_settings* global_settings;
    struct system_status *global_status;
    void (*talk_disable)(bool disable);
#if CONFIG_CODEC == SWCODEC
    void (*codec_thread_do_callback)(void (*fn)(void),
                                     unsigned int *audio_thread_id);
    int (*codec_load_file)(const char* codec, struct codec_api *api);
    const char *(*get_codec_filename)(int cod_spec);
    bool (*get_metadata)(struct mp3entry* id3, int fd, const char* trackname);
#endif
    bool (*mp3info)(struct mp3entry *entry, const char *filename);
    int (*count_mp3_frames)(int fd, int startpos, int filesize,
                            void (*progressfunc)(int));
    int (*create_xing_header)(int fd, long startpos, long filesize,
            unsigned char *buf, unsigned long num_frames,
            unsigned long rec_time, unsigned long header_template,
            void (*progressfunc)(int), bool generate_toc);
    unsigned long (*find_next_frame)(int fd, long *offset,
            long max_offset, unsigned long last_header);

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    unsigned short (*peak_meter_scale_value)(unsigned short val,
                                             int meterwidth);
    void (*peak_meter_set_use_dbfs)(bool use);
    bool (*peak_meter_get_use_dbfs)(void);
#endif
#ifdef HAVE_LCD_BITMAP
    int (*read_bmp_file)(const char* filename, struct bitmap *bm, int maxsize,
                         int format, const struct custom_format *cformat);
    void (*screen_dump_set_hook)(void (*hook)(int fh));
#endif
    int (*show_logo)(void);
    struct tree_context* (*tree_get_context)(void);
    void (*set_current_file)(char* path);
    void (*set_dirfilter)(int l_dirfilter);

#ifdef HAVE_WHEEL_POSITION
    int (*wheel_status)(void);
    void (*wheel_send_events)(bool send);
#endif

#ifdef IRIVER_H100_SERIES
    /* Routines for the iriver_flash -plugin. */
    bool (*detect_original_firmware)(void);
    bool (*detect_flashed_ramimage)(void);
    bool (*detect_flashed_romimage)(void);
#endif

    void (*led)(bool on);

#if (CONFIG_CODEC == SWCODEC)
    /* buffering API */
    int (*bufopen)(const char *file, size_t offset, enum data_type type);
    int (*bufalloc)(const void *src, size_t size, enum data_type type);
    bool (*bufclose)(int handle_id);
    int (*bufseek)(int handle_id, size_t newpos);
    int (*bufadvance)(int handle_id, off_t offset);
    ssize_t (*bufread)(int handle_id, size_t size, void *dest);
    ssize_t (*bufgetdata)(int handle_id, size_t size, void **data);
    ssize_t (*bufgettail)(int handle_id, size_t size, void **data);
    ssize_t (*bufcuttail)(int handle_id, size_t size);

    ssize_t (*buf_get_offset)(int handle_id, void *ptr);
    ssize_t (*buf_handle_offset)(int handle_id);
    void (*buf_request_buffer_handle)(int handle_id);
    void (*buf_set_base_handle)(int handle_id);
    size_t (*buf_used)(void);
#endif

#ifdef HAVE_TAGCACHE
    bool (*tagcache_search)(struct tagcache_search *tcs, int tag);
    void (*tagcache_search_set_uniqbuf)(struct tagcache_search *tcs,
           void *buffer, long length);
    bool (*tagcache_search_add_filter)(struct tagcache_search *tcs,
                                    int tag, int seek);
    bool (*tagcache_get_next)(struct tagcache_search *tcs);
    bool (*tagcache_retrieve)(struct tagcache_search *tcs, int idxid,
                           int tag, char *buf, long size);
    void (*tagcache_search_finish)(struct tagcache_search *tcs);
    long (*tagcache_get_numeric)(const struct tagcache_search *tcs, int tag);
#endif

#ifdef HAVE_ALBUMART
    bool (*find_albumart)(const struct mp3entry *id3, char *buf, int buflen);
    bool (*search_albumart_files)(const struct mp3entry *id3, const char *size_string,
                                  char *buf, int buflen);
#endif

#ifdef HAVE_SEMAPHORE_OBJECTS
    void (*semaphore_init)(struct semaphore *s, int max, int start);
    void (*semaphore_wait)(struct semaphore *s);
    void (*semaphore_release)(struct semaphore *s);
#endif

	const char *appsversion;
    /* new stuff at the end, sort into place next time
       the API gets incompatible */
#ifdef CPU_ARM
    void (*__div0)(void);
#endif
    char* (*strip_extension)(char* buffer, int buffer_size, const char *filename);
};

/* plugin header */
struct plugin_header {
    unsigned long magic;
    unsigned short target_id;
    unsigned short api_version;
    unsigned char *load_addr;
    unsigned char *end_addr;
    enum plugin_status(*entry_point)(const void*);
    const struct plugin_api **api;
};

#ifdef PLUGIN
#ifndef SIMULATOR
extern unsigned char plugin_start_addr[];
extern unsigned char plugin_end_addr[];
#define PLUGIN_HEADER \
        const struct plugin_api *rb __attribute__ ((section (".data"))); \
        const struct plugin_header __header \
        __attribute__ ((section (".header")))= { \
        PLUGIN_MAGIC, TARGET_ID, PLUGIN_API_VERSION, \
        plugin_start_addr, plugin_end_addr, plugin_start, &rb };
#else /* SIMULATOR */
#define PLUGIN_HEADER \
        const struct plugin_api *rb __attribute__ ((section (".data"))); \
        const struct plugin_header __header \
        __attribute__((visibility("default"))) = { \
        PLUGIN_MAGIC, TARGET_ID, PLUGIN_API_VERSION, \
        NULL, NULL, plugin_start, &rb };
#endif /* SIMULATOR */

#ifdef PLUGIN_USE_IRAM
/* Declare IRAM variables */
#define PLUGIN_IRAM_DECLARE \
    extern char iramcopy[]; \
    extern char iramstart[]; \
    extern char iramend[]; \
    extern char iedata[]; \
    extern char iend[];
/* Initialize IRAM */
#define PLUGIN_IRAM_INIT(api) \
    (api)->plugin_iram_init(iramstart, iramcopy, iramend-iramstart, \
                            iedata, iend-iedata);
#else
#define PLUGIN_IRAM_DECLARE
#define PLUGIN_IRAM_INIT(api)
#endif /* PLUGIN_USE_IRAM */
#endif /* PLUGIN */

int plugin_load(const char* plugin, const void* parameter);
void* plugin_get_audio_buffer(size_t *buffer_size);
#ifdef PLUGIN_USE_IRAM
void plugin_iram_init(char *iramstart, char *iramcopy, size_t iram_size,
                      char *iedata, size_t iedata_size);
#endif

/* plugin_tsr,
    callback returns true to allow the new plugin to load,
    reenter means the currently running plugin is being reloaded */
void plugin_tsr(bool (*exit_callback)(bool reenter));

/* defined by the plugin */
extern const struct plugin_api *rb;
enum plugin_status plugin_start(const void* parameter)
    NO_PROF_ATTR;

/* Use this macro in plugins where gcc tries to optimize by calling
 * these functions directly */
#define MEM_FUNCTION_WRAPPERS \
        void *memcpy(void *dest, const void *src, size_t n) \
        { \
            return rb->memcpy(dest, src, n); \
        } \
        void *memset(void *dest, int c, size_t n) \
        { \
            return rb->memset(dest, c, n); \
        } \
        void *memmove(void *dest, const void *src, size_t n) \
        { \
            return rb->memmove(dest, src, n); \
        } \
        int memcmp(const void *s1, const void *s2, size_t n) \
        { \
            return rb->memcmp(s1, s2, n); \
        }

#undef CACHE_FUNCTION_WRAPPERS
#ifdef CACHE_FUNCTIONS_AS_CALL
#define CACHE_FUNCTION_WRAPPERS \
        void flush_icache(void) \
        { \
            rb->flush_icache(); \
        } \
        void invalidate_icache(void) \
        { \
            rb->invalidate_icache(); \
        }
#else
#define CACHE_FUNCTION_WRAPPERS
#endif /* CACHE_FUNCTIONS_AS_CALL */

#endif /* __PCTOOL__ */
#endif
