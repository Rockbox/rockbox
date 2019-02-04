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
#ifndef _PLUGIN_H_
#define _PLUGIN_H_

/* instruct simulator code to not redefine any symbols when compiling plugins.
   (the PLUGIN macro is defined in apps/plugins/Makefile) */
#ifdef PLUGIN
#define NO_REDEFINES_PLEASE
#endif

#include <stdbool.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "string-extra.h"
#include "gcc_extensions.h"



/* on some platforms strcmp() seems to be a tricky define which
 * breaks if we write down strcmp's prototype */
#undef strcmp
#undef strncmp
#undef strchr
#undef strtok_r

char* strncpy(char *, const char *, size_t);
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
#include "load_code.h"
#include "usb.h"
#include "font.h"
#include "lcd.h"
#include "scroll_engine.h"
#include "metadata.h"
#include "sound.h"
#include "mpeg.h"
#include "audio.h"
#include "mp3_playback.h"
#include "root_menu.h"
#include "talk.h"
#ifdef PLUGIN
#include "lang_enum.h"
#endif
#ifdef RB_PROFILE
#include "profile.h"
#endif
#include "misc.h"
#include "pathfuncs.h"
#if (CONFIG_CODEC == SWCODEC)
#include "pcm_mixer.h"
#include "dsp-util.h"
#include "dsp_core.h"
#include "dsp_proc_settings.h"
#include "codecs.h"
#include "playback.h"
#include "codec_thread.h"
#ifdef HAVE_RECORDING
#include "recording.h"
#endif
#else
#include "mas35xx.h"
#endif /* CONFIG_CODEC == SWCODEC */
#include "settings.h"
#include "timer.h"
#include "playlist.h"
#ifdef HAVE_LCD_BITMAP
#include "screendump.h"
#include "scrollbar.h"
#include "jpeg_load.h"
#include "../recorder/bmp.h"
#endif
#include "statusbar.h"
#include "menu.h"
#include "rbunicode.h"
#include "list.h"
#include "tree.h"
#include "color_picker.h"
#include "buflib.h"
#include "buffering.h"
#include "tagcache.h"
#include "viewport.h"
#include "ata_idle_notify.h"
#include "settings_list.h"
#include "timefuncs.h"
#include "crc32.h"
#include "rbpaths.h"
#include "core_alloc.h"
#include "screen_access.h"

#ifdef HAVE_ALBUMART
#include "albumart.h"
#endif

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

#include "yesno.h"

#include "filetypes.h"

#ifdef USB_ENABLE_HID
#include "usbstack/usb_hid_usage_tables.h"
#endif


#ifdef PLUGIN

#if defined(DEBUG) || defined(SIMULATOR)
#undef DEBUGF
#define DEBUGF  rb->debugf
#undef LDEBUGF
#define LDEBUGF rb->debugf
#else
#undef DEBUGF
#define DEBUGF(...) do { } while(0)
#undef LDEBUGF
#define LDEBUGF(...) do { } while(0)
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
#define PLUGIN_API_VERSION 2350

/* update this to latest version if a change to the api struct breaks
   backwards compatibility (and please take the opportunity to sort in any
   new function which are "waiting" at the end of the function table) */
#define PLUGIN_MIN_API_VERSION 2350

/* plugin return codes */
/* internal returns start at 0x100 to make exit(1..255) work */
#define INTERNAL_PLUGIN_RETVAL_START 0x100
enum plugin_status {
    PLUGIN_OK = 0, /* PLUGIN_OK == EXIT_SUCCESS */
    /* 1...255 reserved for exit() */
    PLUGIN_USB_CONNECTED = INTERNAL_PLUGIN_RETVAL_START,
    PLUGIN_POWEROFF,
    PLUGIN_GOTO_WPS,
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
    
#ifdef HAVE_LCD_CONTRAST
    void (*lcd_set_contrast)(int x);
#endif
    void (*lcd_update)(void);
    void (*lcd_clear_display)(void);
    int  (*lcd_getstringsize)(const unsigned char *str, int *w, int *h);
    void (*lcd_putsxy)(int x, int y, const unsigned char *string);
    void (*lcd_putsxyf)(int x, int y, const unsigned char *fmt, ...);
    void (*lcd_puts)(int x, int y, const unsigned char *string);
    void (*lcd_putsf)(int x, int y, const unsigned char *fmt, ...);
    bool (*lcd_puts_scroll)(int x, int y, const unsigned char* string);
    void (*lcd_scroll_stop)(void);
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
    void (*lcd_set_viewport)(struct viewport* vp);
    void (*lcd_set_framebuffer)(fb_data *fb);
    void (*lcd_bmp_part)(const struct bitmap *bm, int src_x, int src_y,
                         int x, int y, int width, int height);
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
#if LCD_DEPTH >= 16
    void (*lcd_bitmap_transparent_part)(const fb_data *src,
            int src_x, int src_y, int stride,
            int x, int y, int width, int height);
    void (*lcd_bitmap_transparent)(const fb_data *src, int x, int y,
            int width, int height);
#if MEMORYSIZE > 2
    void (*lcd_blit_yuv)(unsigned char * const src[3],
                         int src_x, int src_y, int stride,
                         int x, int y, int width, int height);
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200) || defined(SANSA_C200) \
    || defined(IRIVER_H10) || defined(COWON_D2) || defined(PHILIPS_HDD1630) \
    || defined(SANSA_FUZE) || defined(SANSA_E200V2) || defined(SANSA_FUZEV2) \
    || defined(TOSHIBA_GIGABEAT_S) || defined(PHILIPS_SA9200)
    void (*lcd_yuv_set_options)(unsigned options);
#endif
#endif /* MEMORYSIZE > 2 */
#elif (LCD_DEPTH < 4) && (CONFIG_PLATFORM & PLATFORM_NATIVE)
    void (*lcd_blit_mono)(const unsigned char *data, int x, int by, int width,
                          int bheight, int stride);
    void (*lcd_blit_grey_phase)(unsigned char *values, unsigned char *phases,
                                int bx, int by, int bwidth, int bheight,
                                int stride);
#endif /* LCD_DEPTH */
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    void (*lcd_blit_pal256)(unsigned char *src, int src_x, int src_y, int x, int y,
                            int width, int height);
    void (*lcd_pal256_update_pal)(fb_data *palette);
#endif
#ifdef HAVE_LCD_INVERT
    void (*lcd_set_invert_display)(bool yesno);
#endif /* HAVE_LCD_INVERT */
#if defined(HAVE_LCD_MODES)
    void (*lcd_set_mode)(int mode);
#endif

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
    struct event_queue *button_queue;
#endif
    unsigned short *(*bidi_l2v)( const unsigned char *str, int orientation );
#ifdef HAVE_LCD_BITMAP
    bool (*is_diacritic)(const unsigned short char_code, bool *is_rtl);
#endif
    const unsigned char *(*font_get_bits)( struct font *pf, unsigned short char_code );
    int (*font_load)(const char *path);
    void (*font_unload)(int font_id);
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
    bool (*lcd_remote_puts_scroll)(int x, int y, const unsigned char* string);
    void (*lcd_remote_scroll_stop)(void);
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
    void (*lcd_remote_bitmap_part)(const fb_remote_data *src,
                                   int src_x, int src_y, int stride,
                                   int x, int y, int width, int height);
    void (*lcd_remote_bitmap)(const fb_remote_data *src, int x, int y,
                              int width, int height);
#endif
    void (*viewport_set_defaults)(struct viewport *vp,
                                  const enum screen_type screen);                                  
#ifdef HAVE_LCD_BITMAP
    void (*viewportmanager_theme_enable)(enum screen_type screen, bool enable,
                                         struct viewport *viewport);
    void (*viewportmanager_theme_undo)(enum screen_type screen, bool force_redraw);
    void (*viewport_set_fullscreen)(struct viewport *vp,
                                    const enum screen_type screen);
#endif
    /* list */
    void (*gui_synclist_init)(struct gui_synclist * lists,
            list_get_name callback_get_item_name, void * data,
            bool scroll_all,int selected_size,
            struct viewport parent[NB_SCREENS]);
    void (*gui_synclist_set_nb_items)(struct gui_synclist * lists, int nb_items);
    void (*gui_synclist_set_voice_callback)(struct gui_synclist * lists, list_speak_item voice_callback);
    void (*gui_synclist_set_icon_callback)(struct gui_synclist * lists,
                                           list_get_icon icon_callback);
    int (*gui_synclist_get_nb_items)(struct gui_synclist * lists);
    int  (*gui_synclist_get_sel_pos)(struct gui_synclist * lists);
    void (*gui_synclist_draw)(struct gui_synclist * lists);
    void (*gui_synclist_speak_item)(struct gui_synclist * lists);
    void (*gui_synclist_select_item)(struct gui_synclist * lists,
                                     int item_number);
    void (*gui_synclist_add_item)(struct gui_synclist * lists);
    void (*gui_synclist_del_item)(struct gui_synclist * lists);
    void (*gui_synclist_limit_scroll)(struct gui_synclist * lists, bool scroll);
    bool (*gui_synclist_do_button)(struct gui_synclist * lists,
                                   int *action, enum list_wrap wrap);
    void (*gui_synclist_set_title)(struct gui_synclist *lists, char* title,
                                   enum themable_icons icon);
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
    int (*button_status_wdata)(int *pdata);
#endif
    void (*button_clear_queue)(void);
    int (*button_queue_count)(void);
#ifdef HAS_BUTTON_HOLD
    bool (*button_hold)(void);
#endif
#ifdef HAVE_TOUCHSCREEN
    void (*touchscreen_set_mode)(enum touchscreen_mode);
    enum touchscreen_mode (*touchscreen_get_mode)(void);
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
    int (*open_utf8)(const char* pathname, int flags);
    int (*open)(const char *path, int oflag, ...);
    int (*creat)(const char *path, mode_t mode);
    int (*close)(int fildes);
    ssize_t (*read)(int fildes, void *buf, size_t nbyte);
    off_t (*lseek)(int fildes, off_t offset, int whence);
    ssize_t (*write)(int fildes, const void *buf, size_t nbyte);
    int (*remove)(const char *path);
    int (*rename)(const char *old, const char *new);
    int (*ftruncate)(int fildes, off_t length);
    off_t (*filesize)(int fildes);
    int (*fdprintf)(int fildes, const char *fmt, ...) ATTRIBUTE_PRINTF(2, 3);
    int (*read_line)(int fd, char* buffer, int buffer_size);
    bool (*settings_parseline)(char* line, char** name, char** value);
    void (*storage_sleep)(void);
    void (*storage_spin)(void);
    void (*storage_spindown)(int seconds);
#if USING_STORAGE_CALLBACK
    void (*register_storage_idle_func)(void (*function)(void));
    void (*unregister_storage_idle_func)(void (*function)(void), bool run);
#endif /* USING_STORAGE_CALLBACK */
    void (*reload_directory)(void);
    char *(*create_numbered_filename)(char *buffer, const char *path,
                                      const char *prefix, const char *suffix,
                                      int numberlen IF_CNFN_NUM_(, int *num));
    bool (*file_exists)(const char *path);
    char* (*strip_extension)(char* buffer, int buffer_size, const char *filename);
    uint32_t (*crc_32)(const void *src, uint32_t len, uint32_t crc32);

    int (*filetype_get_attr)(const char* file);



    /* dir */
    DIR * (*opendir)(const char *dirname);
    int (*closedir)(DIR *dirp);
    struct dirent * (*readdir)(DIR *dirp);
    int (*mkdir)(const char *path);
    int (*rmdir)(const char *path);
    bool (*dir_exists)(const char *dirname);
    struct dirinfo (*dir_get_info)(DIR *dirp, struct dirent *entry);

    /* browsing */
    void (*browse_context_init)(struct browse_context *browse,
                                int dirfilter, unsigned flags,
                                char *title, enum themable_icons icon,
                                const char *root, const char *selected);
    int (*rockbox_browse)(struct browse_context *browse);

    /* talking */
    int (*talk_id)(int32_t id, bool enqueue);
    int (*talk_file)(const char *root, const char *dir, const char *file,
                     const char *ext, const long *prefix_ids, bool enqueue);
    int (*talk_file_or_spell)(const char *dirname, const char* filename,
                              const long *prefix_ids, bool enqueue);
    int (*talk_dir_or_spell)(const char* filename,
                             const long *prefix_ids, bool enqueue);
    int (*talk_number)(long n, bool enqueue);
    int (*talk_value)(long n, int unit, bool enqueue);
    int (*talk_spell)(const char* spell, bool enqueue);
    void (*talk_time)(const struct tm *tm, bool enqueue);
    void (*talk_date)(const struct tm *tm, bool enqueue);
    void (*talk_disable)(bool disable);
    void (*talk_shutup)(void);
    void (*talk_force_shutup)(void);
    void (*talk_force_enqueue_next)(void);

    /* kernel/ system */
#if defined(CPU_ARM) && CONFIG_PLATFORM & PLATFORM_NATIVE
    void (*__div0)(void);
#endif
    unsigned (*sleep)(unsigned ticks);
    void (*yield)(void);
    volatile long* current_tick;
    long (*default_event_handler)(long event);
    long (*default_event_handler_ex)(long event,
            void (*callback)(void *), void *parameter);
    unsigned int (*create_thread)(void (*function)(void), void* stack,
                                  size_t stack_size, unsigned flags,
                                  const char *name
                                  IF_PRIO(, int priority)
                                  IF_COP(, unsigned int core));
    unsigned int (*thread_self)(void);
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
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    int (*system_memory_guard)(int newmode);
    long *cpu_frequency;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#ifdef CPU_BOOST_LOGGING
    void (*cpu_boost_)(bool on_off,char*location,int line);
#else
    void (*cpu_boost)(bool on_off);
#endif
#endif /* HAVE_ADJUSTABLE_CPU_FREQ */
#endif /* PLATFORM_NATIVE */
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    void (*trigger_cpu_boost)(void);
    void (*cancel_cpu_boost)(void);
#endif

    void (*commit_dcache)(void);
    void (*commit_discard_dcache)(void);
    void (*commit_discard_idcache)(void);

    /* load code api for overlay */
    void* (*lc_open)(const char *filename, unsigned char *buf, size_t buf_size);
    void* (*lc_open_from_mem)(void* addr, size_t blob_size);
    void* (*lc_get_header)(void *handle);
    void  (*lc_close)(void *handle);

    bool (*timer_register)(int reg_prio, void (*unregister_callback)(void),
                           long cycles, void (*timer_callback)(void)
                           IF_COP(, int core));
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
#ifdef USB_ENABLE_HID
    void (*usb_hid_send)(usage_page_t usage_page, int id);
#endif
#ifdef RB_PROFILE
    void (*profile_thread)(void);
    void (*profstop)(void);
    void (*profile_func_enter)(void *this_fn, void *call_site);
    void (*profile_func_exit)(void *this_fn, void *call_site);
#endif
    /* event api */
    bool (*add_event)(unsigned short id, void (*handler)(unsigned short id, void *data));
    void (*remove_event)(unsigned short id, void (*handler)(unsigned short id, void *data));
    void (*send_event)(unsigned short id, void *data);

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
    /* special simulator hooks */
#if defined(HAVE_LCD_BITMAP) && LCD_DEPTH < 8
    void (*sim_lcd_ex_init)(unsigned long (*getpixel)(int, int));
    void (*sim_lcd_ex_update_rect)(int x, int y, int width, int height);
#endif
#endif

    /* strings and memory */
    int (*snprintf)(char *buf, size_t size, const char *fmt, ...)
                    ATTRIBUTE_PRINTF(3, 4);
    int (*vsnprintf)(char *buf, size_t size, const char *fmt, va_list ap);
    char* (*strcpy)(char *dst, const char *src);
    size_t (*strlcpy)(char *dst, const char *src, size_t length);
    size_t (*strlen)(const char *str);
    char * (*strrchr)(const char *s, int c);
    int (*strcmp)(const char *, const char *);
    int (*strncmp)(const char *, const char *, size_t);
    int (*strcasecmp)(const char *, const char *);
    int (*strncasecmp)(const char *s1, const char *s2, size_t n);
    void* (*memset)(void *dst, int c, size_t length);
    void* (*memcpy)(void *out, const void *in, size_t n);
    void* (*memmove)(void *out, const void *in, size_t n);
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    const unsigned char *_rbctype_;
#endif
    int (*atoi)(const char *str);
    char *(*strchr)(const char *s, int c);
    char *(*strcat)(char *s1, const char *s2);
    size_t (*strlcat)(char *dst, const char *src, size_t length);
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

    /* the buflib memory management library */
    void   (*buflib_init)(struct buflib_context* ctx, void* buf, size_t size);
    size_t (*buflib_available)(struct buflib_context* ctx);
    int    (*buflib_alloc)(struct buflib_context* ctx, size_t size);
    int    (*buflib_alloc_ex)(struct buflib_context* ctx, size_t size,
                              const char* name, struct buflib_callbacks *ops);
    int    (*buflib_alloc_maximum)(struct buflib_context* ctx, const char* name,
                                   size_t* size, struct buflib_callbacks *ops);
    void   (*buflib_buffer_in)(struct buflib_context* ctx, int size);
    void*  (*buflib_buffer_out)(struct buflib_context* ctx, size_t* size);
    int    (*buflib_free)(struct buflib_context* ctx, int handle);
    bool   (*buflib_shrink)(struct buflib_context* ctx, int handle,
                            void* new_start, size_t new_size);
    void*  (*buflib_get_data)(struct buflib_context* ctx, int handle);
    const char* (*buflib_get_name)(struct buflib_context* ctx, int handle);

    /* sound */
    void (*sound_set)(int setting, int value);
    int (*sound_default)(int setting);
    int (*sound_min)(int setting);
    int (*sound_max)(int setting);
    const char * (*sound_unit)(int setting);
    int (*sound_val2phys)(int setting, int value);
#ifdef AUDIOHW_HAVE_EQ
    int (*sound_enum_hw_eq_band_setting)(unsigned int band,
                                         unsigned int band_setting);
#endif /* AUDIOHW_HAVE_EQ */
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    void (*mp3_play_data)(const void* start, size_t size,
                          mp3_play_callback_t get_more);
    void (*mp3_play_pause)(bool play);
    void (*mp3_play_stop)(void);
    bool (*mp3_is_playing)(void);
#if CONFIG_CODEC != SWCODEC
    void (*bitswap)(unsigned char *data, int length);
#endif
#endif /* PLATFORM_NATIVE */
#if CONFIG_CODEC == SWCODEC
    const unsigned long *audio_master_sampr_list;
    const unsigned long *hw_freq_sampr;
    void (*pcm_apply_settings)(void);
    void (*pcm_play_data)(pcm_play_callback_type get_more,
                          pcm_status_callback_type status_cb,
                          const void *start, size_t size);
    void (*pcm_play_stop)(void);
    void (*pcm_set_frequency)(unsigned int frequency);
    bool (*pcm_is_playing)(void);
    bool (*pcm_is_paused)(void);
    void (*pcm_play_pause)(bool play);
    size_t (*pcm_get_bytes_waiting)(void);
    void (*pcm_calculate_peaks)(int *left, int *right);
    const void* (*pcm_get_peak_buffer)(int *count);
    void (*pcm_play_lock)(void);
    void (*pcm_play_unlock)(void);
    void (*beep_play)(unsigned int frequency, unsigned int duration,
                      unsigned int amplitude);
#ifdef HAVE_RECORDING
    const unsigned long *rec_freq_sampr;
    void (*pcm_init_recording)(void);
    void (*pcm_close_recording)(void);
    void (*pcm_record_data)(pcm_rec_callback_type more_ready,
                            pcm_status_callback_type status_cb,
                            void *start, size_t size);
    void (*pcm_stop_recording)(void);
    void (*pcm_calculate_rec_peaks)(int *left, int *right);
    void (*audio_set_recording_gain)(int left, int right, int type);
#endif /* HAVE_RECORDING */
#if INPUT_SRC_CAPS != 0
    void (*audio_set_output_source)(int monitor);
    void (*audio_set_input_source)(int source, unsigned flags);
#endif
    void (*dsp_set_crossfeed_type)(int type);
    void (*dsp_eq_enable)(bool enable);
    void (*dsp_dither_enable)(bool enable);
#ifdef HAVE_PITCHCONTROL
    void (*dsp_set_timestretch)(int32_t percent);
#endif
    intptr_t (*dsp_configure)(struct dsp_config *dsp,
                              unsigned int setting, intptr_t value);
    struct dsp_config * (*dsp_get_config)(enum dsp_ids id);
    void (*dsp_process)(struct dsp_config *dsp, struct dsp_buffer *src,
                        struct dsp_buffer *dst);

    enum channel_status (*mixer_channel_status)(enum pcm_mixer_channel channel);
    const void * (*mixer_channel_get_buffer)(enum pcm_mixer_channel channel,
                                             int *count);
    void (*mixer_channel_calculate_peaks)(enum pcm_mixer_channel channel,
                                          struct pcm_peaks *peaks);
    void (*mixer_channel_play_data)(enum pcm_mixer_channel channel,
                                    pcm_play_callback_type get_more,
                                    const void *start, size_t size);
    void (*mixer_channel_play_pause)(enum pcm_mixer_channel channel, bool play);
    void (*mixer_channel_stop)(enum pcm_mixer_channel channel);
    void (*mixer_channel_set_amplitude)(enum pcm_mixer_channel channel,
                                        unsigned int amplitude);
    size_t (*mixer_channel_get_bytes_waiting)(enum pcm_mixer_channel channel);
    void (*mixer_channel_set_buffer_hook)(enum pcm_mixer_channel channel,
                                          chan_buffer_hook_fn_type fn);
    void (*mixer_set_frequency)(unsigned int samplerate);
    unsigned int (*mixer_get_frequency)(void);
    void (*pcmbuf_fade)(bool fade, bool in);
    void (*system_sound_play)(enum system_sound sound);
    void (*keyclick_click)(bool rawbutton, int action);
#endif /* CONFIG_CODEC == SWCODC */

    /* playback control */
    int (*playlist_amount)(void);
    int (*playlist_resume)(void);
    void (*playlist_resume_track)(int start_index, unsigned int crc,
                                  unsigned long elapsed, unsigned long offset);
    void (*playlist_start)(int start_index, unsigned long elapsed,
                           unsigned long offset);
    int (*playlist_add)(const char *filename);
    void (*playlist_sync)(struct playlist_info* playlist);
    int (*playlist_remove_all_tracks)(struct playlist_info *playlist);
    int (*playlist_create)(const char *dir, const char *file);
    int (*playlist_insert_track)(struct playlist_info* playlist,
            const char *filename, int position, bool queue, bool sync);
    int (*playlist_insert_directory)(struct playlist_info* playlist,
                              const char *dirname, int position, bool queue,
                              bool recurse);
    int (*playlist_shuffle)(int random_seed, int start_index);
    void (*audio_play)(unsigned long elapsed, unsigned long offset);
    void (*audio_stop)(void);
    void (*audio_pause)(void);
    void (*audio_resume)(void);
    void (*audio_next)(void);
    void (*audio_prev)(void);
    void (*audio_ff_rewind)(long newtime);
    struct mp3entry* (*audio_next_track)(void);
    int (*audio_status)(void);
    struct mp3entry* (*audio_current_track)(void);
    void (*audio_flush_and_reload_tracks)(void);
    int (*audio_get_file_pos)(void);
#if !defined(SIMULATOR) && (CONFIG_CODEC != SWCODEC)
    unsigned long (*mpeg_get_last_header)(void);
#endif
#if ((CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) || \
     (CONFIG_CODEC == SWCODEC)) && defined (HAVE_PITCHCONTROL)
    void (*sound_set_pitch)(int32_t pitch);
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
                   struct viewport parent[NB_SCREENS], bool hide_theme);

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
                    const char* (*formatter)(char*, size_t, int, const char*) );
    bool (*set_int_ex)(const unsigned char* string, const char* unit, int voice_unit,
                       const int* variable, void (*function)(int), int step,
                       int min, int max,
                       const char* (*formatter)(char*, size_t, int, const char*) ,
                       int32_t (*get_talk_id)(int, int));
    bool (*set_bool)(const char* string, const bool* variable );

#ifdef HAVE_LCD_COLOR
    bool (*set_color)(struct screen *display, char *title,
                      unsigned *color, unsigned banned_color);
#endif
    /* action handling */
    int (*get_custom_action)(int context,int timeout,
                          const struct button_mapping* (*get_context_map)(int));
    int (*get_action)(int context, int timeout);
#ifdef HAVE_TOUCHSCREEN
    int (*action_get_touchscreen_press)(short *x, short *y);
#endif
    bool (*action_userabort)(int timeout);

    /* power */
    int (*battery_level)(void);
    bool (*battery_level_safe)(void);
    int (*battery_time)(void);
    int (*battery_voltage)(void);
#if CONFIG_CHARGING
    bool (*charger_inserted)(void);
# if CONFIG_CHARGING >= CHARGING_MONITOR
    bool (*charging_state)(void);
# endif
#endif
    /* usb */
    bool (*usb_inserted)(void);

    /* misc */
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    int * (*__errno)(void);
#endif
    void (*srand)(unsigned int seed);
    int  (*rand)(void);
    void (*qsort)(void *base, size_t nmemb, size_t size,
                  int(*compar)(const void *, const void *));
    int (*kbd_input)(char* buffer, int buflen);
    struct tm* (*get_time)(void);
    int  (*set_time)(const struct tm *tm);
    struct tm * (*gmtime_r)(const time_t *timep, struct tm *tm);
#if CONFIG_RTC
    time_t (*mktime)(struct tm *t);
#endif
    void* (*plugin_get_buffer)(size_t *buffer_size);
    void* (*plugin_get_audio_buffer)(size_t *buffer_size);
    void (*plugin_release_audio_buffer)(void);
    void (*plugin_tsr)(bool (*exit_callback)(bool reenter));
    char* (*plugin_get_current_filename)(void);
#ifdef PLUGIN_USE_IRAM
    void (*audio_hard_stop)(void);
#endif
#if defined(DEBUG) || defined(SIMULATOR)
    void (*debugf)(const char *fmt, ...) ATTRIBUTE_PRINTF(1, 2);
#endif
#ifdef ROCKBOX_HAS_LOGF
    void (*logf)(const char *fmt, ...) ATTRIBUTE_PRINTF(1, 2);
#endif
    struct user_settings* global_settings;
    struct system_status *global_status;
    unsigned char **language_strings;
#if CONFIG_CODEC == SWCODEC
    void (*codec_thread_do_callback)(void (*fn)(void),
                                     unsigned int *audio_thread_id);
    int (*codec_load_file)(const char* codec, struct codec_api *api);
    int (*codec_run_proc)(void);
    int (*codec_close)(void);
    const char *(*get_codec_filename)(int cod_spec);
    void ** (*find_array_ptr)(void **arr, void *ptr);
    int (*remove_array_ptr)(void **arr, void *ptr);
    int (*round_value_to_list32)(unsigned long value,
                                 const unsigned long list[],
                                 int count,
                                 bool signd);
#endif /* CONFIG_CODEC == SWCODEC */
    bool (*get_metadata)(struct mp3entry* id3, int fd, const char* trackname);
    bool (*mp3info)(struct mp3entry *entry, const char *filename);
    int (*count_mp3_frames)(int fd,  int startpos,  int filesize,
                     void (*progressfunc)(int),
                     unsigned char* buf, size_t buflen);
    int (*create_xing_header)(int fd, long startpos, long filesize,
            unsigned char *buf, unsigned long num_frames,
            unsigned long rec_time, unsigned long header_template,
            void (*progressfunc)(int), bool generate_toc,
            unsigned char* tempbuf, size_t tempbuf_len);
    unsigned long (*find_next_frame)(int fd, long *offset,
            long max_offset, unsigned long reference_header);

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    unsigned short (*peak_meter_scale_value)(unsigned short val,
                                             int meterwidth);
    void (*peak_meter_set_use_dbfs)(bool use);
    bool (*peak_meter_get_use_dbfs)(void);
#endif
#ifdef HAVE_LCD_BITMAP
    int (*read_bmp_file)(const char* filename, struct bitmap *bm, int maxsize,
                         int format, const struct custom_format *cformat);
    int (*read_bmp_fd)(int fd, struct bitmap *bm, int maxsize,
                       int format, const struct custom_format *cformat);
#ifdef HAVE_JPEG
    int (*read_jpeg_file)(const char* filename, struct bitmap *bm, int maxsize,
                          int format, const struct custom_format *cformat);
    int (*read_jpeg_fd)(int fd, struct bitmap *bm, int maxsize,
                        int format, const struct custom_format *cformat);
#endif
    void (*screen_dump_set_hook)(void (*hook)(int fh));
#endif
    int (*show_logo)(void);
    struct tree_context* (*tree_get_context)(void);
    struct entry* (*tree_get_entries)(struct tree_context* t);
    struct entry* (*tree_get_entry_at)(struct tree_context* t, int index);

    void (*set_current_file)(const char* path);
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
#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
    bool (*tagcache_fill_tags)(struct mp3entry *id3, const char *filename);
#endif
#endif

#ifdef HAVE_ALBUMART
    bool (*search_albumart_files)(const struct mp3entry *id3, const char *size_string,
                                  char *buf, int buflen);
#endif

#ifdef HAVE_SEMAPHORE_OBJECTS
    void (*semaphore_init)(struct semaphore *s, int max, int start);
    int  (*semaphore_wait)(struct semaphore *s, int timeout);
    void (*semaphore_release)(struct semaphore *s);
#endif

    const char *rbversion;
    struct menu_table *(*root_menu_get_options)(int *nb_options);
    void (*root_menu_set_default)(void* setting, void* defaultval);
    char* (*root_menu_write_to_cfg)(void* setting, char*buf, int buf_len);
    void (*root_menu_load_from_cfg)(void* setting, char *value);
    int (*settings_save)(void);

    /* new stuff at the end, sort into place next time
       the API gets incompatible */
};

/* plugin header */
struct plugin_header {
    struct lc_header lc_hdr; /* must be the first */
    enum plugin_status(*entry_point)(const void*);
    const struct plugin_api **api;
};

#ifdef PLUGIN
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
extern unsigned char plugin_start_addr[];
extern unsigned char plugin_end_addr[];
#define PLUGIN_HEADER \
        const struct plugin_api *rb DATA_ATTR; \
        const struct plugin_header __header \
        __attribute__ ((section (".header")))= { \
        { PLUGIN_MAGIC, TARGET_ID, PLUGIN_API_VERSION, \
        plugin_start_addr, plugin_end_addr }, plugin__start, &rb };
#else /* PLATFORM_HOSTED */
#define PLUGIN_HEADER \
        const struct plugin_api *rb DATA_ATTR; \
        const struct plugin_header __header \
        __attribute__((visibility("default"))) = { \
        { PLUGIN_MAGIC, TARGET_ID, PLUGIN_API_VERSION, NULL, NULL }, \
        plugin__start, &rb };
#endif /* CONFIG_PLATFORM */
#endif /* PLUGIN */

/*
 * The str() macro/functions is how to access strings that might be
 * translated. Use it like str(MACRO) and expect a string to be
 * returned!
 */
#define str(x) language_strings[x]

int plugin_load(const char* plugin, const void* parameter);

/* defined by the plugin */
extern const struct plugin_api *rb;
enum plugin_status plugin_start(const void* parameter);
enum plugin_status plugin__start(const void* parameter)
    NO_PROF_ATTR;

#endif /* __PCTOOL__ */
#endif
