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
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "system.h"
#include "dir.h"
#include "kernel.h"
#include "thread.h"
#include "button.h"
#include "action.h"
#include "usb.h"
#include "font.h"
#include "lcd.h"
#include "id3.h"
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
#endif
#include "statusbar.h"
#include "menu.h"
#include "rbunicode.h"
#include "list.h"
#include "tree.h"
#include "color_picker.h"

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

#ifdef SIMULATOR
#define PREFIX(_x_) sim_ ## _x_
#else
#define PREFIX(_x_) _x_
#endif

#define PLUGIN_MAGIC 0x526F634B /* RocK */

/* increase this every time the api struct changes */
#define PLUGIN_API_VERSION 81

/* update this to latest version if a change to the api struct breaks
   backwards compatibility (and please take the opportunity to sort in any
   new function which are "waiting" at the end of the function table) */
#define PLUGIN_MIN_API_VERSION 79

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
    void (*lcd_setmargins)(int x, int y);
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
#else
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
#endif
    unsigned short *(*bidi_l2v)( const unsigned char *str, int orientation );
    const unsigned char *(*font_get_bits)( struct font *pf, unsigned short char_code );
    struct font* (*font_load)(const char *path);
    void (*lcd_puts_style)(int x, int y, const unsigned char *str, int style);
    void (*lcd_puts_scroll_style)(int x, int y, const unsigned char* string,
                                  int style);
    fb_data* lcd_framebuffer;
    void (*lcd_blit) (const fb_data* data, int x, int by, int width,
                      int bheight, int stride);
    void (*lcd_update_rect)(int x, int y, int width, int height);
    void (*gui_scrollbar_draw)(struct screen * screen, int x, int y,
                               int width, int height, int items,
                               int min_shown, int max_shown,
                               unsigned flags);
    struct font* (*font_get)(int font);
    int  (*font_getstringsize)(const unsigned char *str, int *w, int *h,
                               int fontnumber);
    int (*font_get_width)(struct font* pf, unsigned short char_code);
    void (*screen_clear_area)(struct screen * display, int xstart, int ystart,
                              int width, int height);
#endif
    void (*backlight_on)(void);
    void (*backlight_off)(void);
    void (*backlight_set_timeout)(int index);
#if CONFIG_CHARGING
    void (*backlight_set_timeout_plugged)(int index);
#endif
    void (*splash)(int ticks, const char *fmt, ...) ATTRIBUTE_PRINTF(2, 3);

#ifdef HAVE_REMOTE_LCD
    /* remote lcd */
    void (*lcd_remote_set_contrast)(int x);
    void (*lcd_remote_clear_display)(void);
    void (*lcd_remote_puts)(int x, int y, const unsigned char *string);
    void (*lcd_remote_lcd_puts_scroll)(int x, int y, const unsigned char* string);
    void (*lcd_remote_lcd_stop_scroll)(void);
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
#endif
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
#if defined(HAVE_LCD_COLOR)
    void (*lcd_yuv_blit)(unsigned char * const src[3],
                         int src_x, int src_y, int stride,
                         int x, int y, int width, int height);
#endif

    /* list */
    void (*gui_synclist_init)(struct gui_synclist * lists,
            list_get_name callback_get_item_name,void * data,
            bool scroll_all,int selected_size);
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
                                         unsigned *action, enum list_wrap wrap);
    void (*gui_synclist_set_title)(struct gui_synclist *lists, char* title, int icon);

    /* button */
    long (*button_get)(bool block);
    long (*button_get_w_tmo)(int ticks);
    int (*button_status)(void);
    void (*button_clear_queue)(void);
#ifdef HAS_BUTTON_HOLD
    bool (*button_hold)(void);
#endif

    /* file */
    int (*PREFIX(open))(const char* pathname, int flags);
    int (*PREFIX(close))(int fd);
    ssize_t (*PREFIX(read))(int fd, void* buf, size_t count);
    off_t (*PREFIX(lseek))(int fd, off_t offset, int whence);
    int (*PREFIX(creat))(const char *pathname);
    ssize_t (*PREFIX(write))(int fd, const void* buf, size_t count);
    int (*PREFIX(remove))(const char* pathname);
    int (*PREFIX(rename))(const char* path, const char* newname);
    int (*PREFIX(ftruncate))(int fd, off_t length);
    off_t (*PREFIX(filesize))(int fd);
    int (*fdprintf)(int fd, const char *fmt, ...) ATTRIBUTE_PRINTF(2, 3);
    int (*read_line)(int fd, char* buffer, int buffer_size);
    bool (*settings_parseline)(char* line, char** name, char** value);
#ifndef SIMULATOR
    void (*ata_sleep)(void);
    bool (*ata_disk_is_active)(void);
#endif
    void (*ata_spindown)(int seconds);
    void (*reload_directory)(void);
    char *(*create_numbered_filename)(char *buffer, const char *path,
                                      const char *prefix, const char *suffix,
                                      int numberlen IF_CNFN_NUM_(, int *num));

    /* dir */
    DIR* (*opendir)(const char* name);
    int (*closedir)(DIR* dir);
    struct dirent* (*readdir)(DIR* dir);
    int (*mkdir)(const char *name);
    int (*rmdir)(const char *name);

    /* kernel/ system */
    void (*PREFIX(sleep))(int ticks);
    void (*yield)(void);
#ifdef HAVE_PRIORITY_SCHEDULING
    void (*priority_yield)(void);
#endif
    volatile long* current_tick;
    long (*default_event_handler)(long event);
    long (*default_event_handler_ex)(long event, void (*callback)(void *), void *parameter);
    struct thread_entry* threads;
    struct thread_entry* (*create_thread)(void (*function)(void), void* stack,
                                          int stack_size, const char *name
                                          IF_PRIO(, int priority)
					  IF_COP(, unsigned int core, bool fallback));
    void (*remove_thread)(struct thread_entry *thread);
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
#endif
#endif
    bool (*timer_register)(int reg_prio, void (*unregister_callback)(void),
                           long cycles, int int_prio,
                           void (*timer_callback)(void));
    void (*timer_unregister)(void);
    bool (*timer_set_period)(long count);

    void (*queue_init)(struct event_queue *q, bool register_queue);
    void (*queue_delete)(struct event_queue *q);
    void (*queue_post)(struct event_queue *q, long id, intptr_t data);
    void (*queue_wait_w_tmo)(struct event_queue *q, struct event *ev,
            int ticks);
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
#if CONFIG_CODEC == SWCODEC
    int (*sound_default)(int setting);
#endif
    void (*sound_set)(int setting, int value);
    int (*sound_min)(int setting);
    int (*sound_max)(int setting);
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
#endif /* CONFIG_CODEC == SWCODC */

    /* playback control */
    int (*playlist_amount)(void);
    int (*playlist_resume)(void);
    int (*playlist_start)(int start_index, int offset);
    void (*PREFIX(audio_play))(long offset);
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
    int  (*i2c_write)(int address, unsigned char* buf, int count );
#endif
#endif

    /* menu */
    int (*do_menu)(const struct menu_item_ex *menu, int *start_selected);

    /* scroll bar */
    struct gui_syncstatusbar *statusbars;
    void (*gui_syncstatusbar_draw)(struct gui_syncstatusbar * bars, bool force_redraw);
    
    /* options */
    const struct settings_list* (*find_setting)(void* variable, int *id);
    bool (*option_screen)(struct settings_list *setting,
                          bool use_temp_var, unsigned char* option_title);
    bool (*set_option)(const char* string, void* variable,
                       enum optiontype type, const struct opt_items* options,
                       int numoptions, void (*function)(int));
    bool (*set_bool_options)(const char* string, bool* variable,
                             const char* yes_str, int yes_voice,
                             const char* no_str, int no_voice,
                             void (*function)(bool));
    bool (*set_int)(const unsigned char* string, const char* unit, int voice_unit,
                    int* variable, void (*function)(int), int step, int min,
                    int max, void (*formatter)(char*, size_t, int, const char*) );
    bool (*set_bool)(const char* string, bool* variable );

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
#ifdef IRAM_STEAL
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
    int (*read_bmp_file)(char* filename, struct bitmap *bm, int maxsize,
                         int format);
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
    /* new stuff at the end, sort into place next time
       the API gets incompatible */

#if (CONFIG_CODEC == SWCODEC)
    void (*spinlock_init)(struct mutex *m);
    void (*spinlock_lock)(struct mutex *m);
    void (*spinlock_unlock)(struct mutex *m);

    int (*codec_load_file)(const char* codec, struct codec_api *api);
    const char *(*get_codec_filename)(int cod_spec);
    bool (*get_metadata)(struct mp3entry* id3, int fd, const char* trackname);
#endif
    void (*led)(bool on);

#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200)
    void (*lcd_yuv_set_options)(unsigned options);
#endif

#ifdef CACHE_FUNCTIONS_AS_CALL
    void (*flush_icache)(void);
    void (*invalidate_icache)(void);
#endif

    void (*talk_disable_menus)(void);
    void (*talk_enable_menus)(void);

    int (*button_queue_count)(void);   
#ifdef HAVE_REMOTE_LCD
    void (*lcd_remote_setmargins)(int x, int y);
#endif
};

/* plugin header */
struct plugin_header {
    unsigned long magic;
    unsigned short target_id;
    unsigned short api_version;
    unsigned char *load_addr;
    unsigned char *end_addr;
    enum plugin_status(*entry_point)(struct plugin_api*, void*);
};

#ifdef PLUGIN
#ifndef SIMULATOR
extern unsigned char plugin_start_addr[];
extern unsigned char plugin_end_addr[];
#define PLUGIN_HEADER \
        const struct plugin_header __header \
        __attribute__ ((section (".header")))= { \
        PLUGIN_MAGIC, TARGET_ID, PLUGIN_API_VERSION, \
        plugin_start_addr, plugin_end_addr, plugin_start };
#else /* SIMULATOR */
#define PLUGIN_HEADER \
        const struct plugin_header __header \
        __attribute__((visibility("default"))) = { \
        PLUGIN_MAGIC, TARGET_ID, PLUGIN_API_VERSION, \
        NULL, NULL, plugin_start };
#endif /* SIMULATOR */

#ifdef USE_IRAM
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
#endif /* USE_IRAM */
#endif /* PLUGIN */

int plugin_load(const char* plugin, void* parameter);
void* plugin_get_buffer(size_t *buffer_size);
void* plugin_get_audio_buffer(size_t *buffer_size);
#ifdef IRAM_STEAL
void plugin_iram_init(char *iramstart, char *iramcopy, size_t iram_size,
                      char *iedata, size_t iedata_size);
#endif

/* plugin_tsr,
    callback returns true to allow the new plugin to load,
    reenter means the currently running plugin is being reloaded */
void plugin_tsr(bool (*exit_callback)(bool reenter));

/* defined by the plugin */
enum plugin_status plugin_start(struct plugin_api* rockbox, void* parameter)
    NO_PROF_ATTR;

/* Use this macro in plugins where gcc tries to optimize by calling
 * these functions directly */
#define MEM_FUNCTION_WRAPPERS(api) \
        void *memcpy(void *dest, const void *src, size_t n) \
        { \
            return (api)->memcpy(dest, src, n); \
        } \
        void *memset(void *dest, int c, size_t n) \
        { \
            return (api)->memset(dest, c, n); \
        } \
        void *memmove(void *dest, const void *src, size_t n) \
        { \
            return (api)->memmove(dest, src, n); \
        } \
        int memcmp(const void *s1, const void *s2, size_t n) \
        { \
            return (api)->memcmp(s1, s2, n); \
        }

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
