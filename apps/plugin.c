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
#include "plugin.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "i2c.h"
#include "lang.h"
#include "led.h"
#include "keyboard.h"
#include "backlight.h"
#include "sound_menu.h"
#include "mp3data.h"
#include "powermgmt.h"
#include "splash.h"
#include "logf.h"
#include "option_select.h"
#include "talk.h"
#include "version.h"
#include "storage.h"
#include "pcmbuf.h"
#include "errno.h"
#include "diacritic.h"
#include "filefuncs.h"
#include "load_code.h"

#if CONFIG_CHARGING
#include "power.h"
#endif

#ifdef HAVE_LCD_BITMAP
#include "scrollbar.h"
#include "peakmeter.h"
#include "bmp.h"
#include "bidi.h"
#endif

#ifdef USB_ENABLE_HID
#include "usbstack/usb_hid.h"
#endif

#if defined (SIMULATOR)
#define PREFIX(_x_) sim_ ## _x_
#elif defined (APPLICATION)
#define PREFIX(_x_) app_ ## _x_
#else
#define PREFIX(_x_) _x_
#endif

#if defined (APPLICATION)
/* For symmetry reasons (we want app_ and sim_ to behave similarly), some
 * wrappers are needed */
static int app_close(int fd)
{
    return close(fd);
}

static ssize_t app_read(int fd, void *buf, size_t count)
{
    return read(fd,buf,count);
}

static off_t app_lseek(int fd, off_t offset, int whence)
{
    return lseek(fd,offset,whence);
}

static ssize_t app_write(int fd, const void *buf, size_t count)
{
    return write(fd,buf,count);
}

static int app_ftruncate(int fd, off_t length)
{
    return ftruncate(fd,length);
}
#endif

#if defined(HAVE_PLUGIN_CHECK_OPEN_CLOSE) && (MAX_OPEN_FILES>32)
#warning "MAX_OPEN_FILES>32, disabling plugin file open/close checking"
#undef HAVE_PLUGIN_CHECK_OPEN_CLOSE
#endif

#ifdef HAVE_PLUGIN_CHECK_OPEN_CLOSE
static unsigned int open_files;
#endif

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
static unsigned char pluginbuf[PLUGIN_BUFFER_SIZE];
void sim_lcd_ex_init(unsigned long (*getpixel)(int, int));
void sim_lcd_ex_update_rect(int x, int y, int width, int height);
#else
extern unsigned char pluginbuf[];
#include "bitswap.h"
#endif

/* for actual plugins only, not for codecs */
static int  plugin_size = 0;
static bool (*pfn_tsr_exit)(bool reenter) = NULL; /* TSR exit callback */
static char current_plugin[MAX_PATH];
/* NULL if no plugin is loaded, otherwise the handle that lc_open() returned */
static void *current_plugin_handle;

char *plugin_get_current_filename(void);

/* Some wrappers used to monitor open and close and detect leaks*/
static int open_wrapper(const char* pathname, int flags, ...);
#ifdef HAVE_PLUGIN_CHECK_OPEN_CLOSE
static int close_wrapper(int fd);
static int creat_wrapper(const char *pathname, mode_t mode);
#endif

static const struct plugin_api rockbox_api = {

    /* lcd */
#ifdef HAVE_LCD_CONTRAST
    lcd_set_contrast,
#endif
    lcd_update,
    lcd_clear_display,
    lcd_getstringsize,
    lcd_putsxy,
    lcd_putsxyf,
    lcd_puts,
    lcd_putsf,
    lcd_puts_scroll,
    lcd_stop_scroll,
#ifdef HAVE_LCD_CHARCELLS
    lcd_define_pattern,
    lcd_get_locked_pattern,
    lcd_unlock_pattern,
    lcd_putc,
    lcd_put_cursor,
    lcd_remove_cursor,
    lcd_icon,
    lcd_double_height,
#else
    &lcd_static_framebuffer[0][0],
    lcd_update_rect,
    lcd_set_drawmode,
    lcd_get_drawmode,
    screen_helper_setfont,
    lcd_drawpixel,
    lcd_drawline,
    lcd_hline,
    lcd_vline,
    lcd_drawrect,
    lcd_fillrect,
    lcd_mono_bitmap_part,
    lcd_mono_bitmap,
#if LCD_DEPTH > 1
    lcd_set_foreground,
    lcd_get_foreground,
    lcd_set_background,
    lcd_get_background,
    lcd_bitmap_part,
    lcd_bitmap,
    lcd_get_backdrop,
    lcd_set_backdrop,
#endif
#if LCD_DEPTH == 16
    lcd_bitmap_transparent_part,
    lcd_bitmap_transparent,
#if MEMORYSIZE > 2
    lcd_blit_yuv,
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200) || defined(SANSA_C200) \
    || defined(IRIVER_H10) || defined(COWON_D2) || defined(PHILIPS_HDD1630) \
    || defined(SANSA_FUZE) || defined(SANSA_E200V2) || defined(SANSA_FUZEV2) \
    || defined(TOSHIBA_GIGABEAT_S) || defined(PHILIPS_SA9200)
    lcd_yuv_set_options,
#endif
#endif /* MEMORYSIZE > 2 */
#elif (LCD_DEPTH < 4) && (CONFIG_PLATFORM & PLATFORM_NATIVE)
    lcd_blit_mono,
    lcd_blit_grey_phase,
#endif /* LCD_DEPTH */
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    lcd_blit_pal256,
    lcd_pal256_update_pal,
#endif
    lcd_puts_style,
    lcd_puts_scroll_style,
#ifdef HAVE_LCD_INVERT
    lcd_set_invert_display,
#endif /* HAVE_LCD_INVERT */
#if defined(HAVE_LCD_MODES)
    lcd_set_mode,
#endif
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
    &button_queue,
#endif
    bidi_l2v,
#ifdef HAVE_LCD_BITMAP
    is_diacritic,
#endif
    font_get_bits,
    font_load,
    font_unload,
    font_get,
    font_getstringsize,
    font_get_width,
    screen_clear_area,
    gui_scrollbar_draw,
#endif /* HAVE_LCD_BITMAP */
    get_codepage_name,

    backlight_on,
    backlight_off,
    backlight_set_timeout,
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    backlight_set_brightness,
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#if CONFIG_CHARGING
    backlight_set_timeout_plugged,
#endif
    is_backlight_on,
    splash,
    splashf,

#ifdef HAVE_REMOTE_LCD
    /* remote lcd */
    lcd_remote_set_contrast,
    lcd_remote_clear_display,
    lcd_remote_puts,
    lcd_remote_puts_scroll,
    lcd_remote_stop_scroll,
    lcd_remote_set_drawmode,
    lcd_remote_get_drawmode,
    lcd_remote_setfont,
    lcd_remote_getstringsize,
    lcd_remote_drawpixel,
    lcd_remote_drawline,
    lcd_remote_hline,
    lcd_remote_vline,
    lcd_remote_drawrect,
    lcd_remote_fillrect,
    lcd_remote_mono_bitmap_part,
    lcd_remote_mono_bitmap,
    lcd_remote_putsxy,
    lcd_remote_puts_style,
    lcd_remote_puts_scroll_style,
    &lcd_remote_static_framebuffer[0][0],
    lcd_remote_update,
    lcd_remote_update_rect,

    remote_backlight_on,
    remote_backlight_off,
    remote_backlight_set_timeout,
#if CONFIG_CHARGING
    remote_backlight_set_timeout_plugged,
#endif
#endif /* HAVE_REMOTE_LCD */
#if NB_SCREENS == 2
    {&screens[SCREEN_MAIN], &screens[SCREEN_REMOTE]},
#else
    {&screens[SCREEN_MAIN]},
#endif
#if defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    lcd_remote_set_foreground,
    lcd_remote_get_foreground,
    lcd_remote_set_background,
    lcd_remote_get_background,
    lcd_remote_bitmap_part,
    lcd_remote_bitmap,
#endif
    viewport_set_defaults,
#ifdef HAVE_LCD_BITMAP
    viewportmanager_theme_enable,
    viewportmanager_theme_undo,
#endif
    
    /* list */
    gui_synclist_init,
    gui_synclist_set_nb_items,
    gui_synclist_set_icon_callback,
    gui_synclist_get_nb_items,
    gui_synclist_get_sel_pos,
    gui_synclist_draw,
    gui_synclist_select_item,
    gui_synclist_add_item,
    gui_synclist_del_item,
    gui_synclist_limit_scroll,
    gui_synclist_do_button,
    gui_synclist_set_title,
    gui_syncyesno_run,
    simplelist_info_init,
    simplelist_show_list,

    /* button */
    button_get,
    button_get_w_tmo,
    button_status,
#ifdef HAVE_BUTTON_DATA
    button_get_data,
    button_status_wdata,
#endif
    button_clear_queue,
    button_queue_count,
#ifdef HAS_BUTTON_HOLD
    button_hold,
#endif
#ifdef HAVE_TOUCHSCREEN
    touchscreen_set_mode,
    touchscreen_get_mode,
#endif
    
#ifdef HAVE_BUTTON_LIGHT
    buttonlight_set_timeout,
    buttonlight_off,
    buttonlight_on,
#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
    buttonlight_set_brightness,
#endif /* HAVE_BUTTONLIGHT_BRIGHTNESS */
#endif /* HAVE_BUTTON_LIGHT */

    /* file */
    open_utf8,
    (open_func)open_wrapper,
#ifdef HAVE_PLUGIN_CHECK_OPEN_CLOSE
    close_wrapper,
#else
    PREFIX(close),
#endif
    (read_func)PREFIX(read),
    PREFIX(lseek),
#ifdef HAVE_PLUGIN_CHECK_OPEN_CLOSE
    (creat_func)creat_wrapper,
#else
    PREFIX(creat),
#endif
    (write_func)PREFIX(write),
    PREFIX(remove),
    PREFIX(rename),
    PREFIX(ftruncate),
    filesize,
    fdprintf,
    read_line,
    settings_parseline,
    storage_sleep,
    storage_spin,
    storage_spindown,
#if USING_STORAGE_CALLBACK
    register_storage_idle_func,
    unregister_storage_idle_func,
#endif /* USING_STORAGE_CALLBACK */
    reload_directory,
    create_numbered_filename,
    file_exists,
    strip_extension,
    crc_32,
    filetype_get_attr,

    /* dir */
    (opendir_func)opendir,
    (closedir_func)closedir,
    (readdir_func)readdir,
    mkdir,
    rmdir,
    dir_exists,
    dir_get_info,

    /* browsing */
    browse_context_init,
    rockbox_browse,

    /* kernel/ system */
#if defined(CPU_ARM) && CONFIG_PLATFORM & PLATFORM_NATIVE
    __div0,
#endif
    sleep,
    yield,
    &current_tick,
    default_event_handler,
    default_event_handler_ex,
    create_thread,
    thread_self,
    thread_exit,
    thread_wait,
#if (CONFIG_CODEC == SWCODEC)
    thread_thaw,
#ifdef HAVE_PRIORITY_SCHEDULING
    thread_set_priority,
#endif
    mutex_init,
    mutex_lock,
    mutex_unlock,
#endif

    reset_poweroff_timer,
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    system_memory_guard,
    &cpu_frequency,

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#ifdef CPU_BOOST_LOGGING
    cpu_boost_,
#else
    cpu_boost,
#endif
#endif /* HAVE_ADJUSTABLE_CPU_FREQ */
#endif /* PLATFORM_NATIVE */
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    trigger_cpu_boost,
    cancel_cpu_boost,
#endif

    commit_dcache,
    commit_discard_dcache,
    commit_discard_idcache,

    lc_open,
    lc_open_from_mem,
    lc_get_header,
    lc_close,

    timer_register,
    timer_unregister,
    timer_set_period,

    queue_init,
    queue_delete,
    queue_post,
    queue_wait_w_tmo,
#if CONFIG_CODEC == SWCODEC
    queue_enable_queue_send,
    queue_empty,
    queue_wait,
    queue_send,
    queue_reply,
#endif
    usb_acknowledge,
#ifdef USB_ENABLE_HID
    usb_hid_send,
#endif
#ifdef RB_PROFILE
    profile_thread,
    profstop,
    __cyg_profile_func_enter,
    __cyg_profile_func_exit,
#endif
    add_event,
    remove_event,
    send_event,

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
    /* special simulator hooks */
#if defined(HAVE_LCD_BITMAP) && LCD_DEPTH < 8
    sim_lcd_ex_init,
    sim_lcd_ex_update_rect,
#endif
#endif

    /* strings and memory */
    snprintf,
    vsnprintf,
    strcpy,
    strlcpy,
    strlen,
    strrchr,
    strcmp,
    strncmp,
    strcasecmp,
    strncasecmp,
    memset,
    memcpy,
    memmove,
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    _ctype_,
#endif
    atoi,
    strchr,
    strcat,
    strlcat,
    memchr,
    memcmp,
    strcasestr,
    strtok_r,
    /* unicode stuff */
    utf8decode,
    iso_decode,
    utf16LEdecode,
    utf16BEdecode,
    utf8encode,
    utf8length,
    utf8seek,
    
    /* the buflib memory management library */
    buflib_init,
    buflib_available,
    buflib_alloc,
    buflib_alloc_ex,
    buflib_alloc_maximum,
    buflib_buffer_in,
    buflib_buffer_out,
    buflib_free,
    buflib_shrink,
    buflib_get_data,
    buflib_get_name,

    /* sound */
    sound_set,
    sound_default,
    sound_min,
    sound_max,
    sound_unit,
    sound_val2phys,
#ifdef AUDIOHW_HAVE_EQ
    sound_enum_hw_eq_band_setting,
#endif
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    mp3_play_data,
    mp3_play_pause,
    mp3_play_stop,
    mp3_is_playing,
#if CONFIG_CODEC != SWCODEC
    bitswap,
#endif
#endif
#if CONFIG_CODEC == SWCODEC
    &audio_master_sampr_list[0],
    &hw_freq_sampr[0],
    pcm_apply_settings,
    pcm_play_data,
    pcm_play_stop,
    pcm_set_frequency,
    pcm_is_playing,
    pcm_is_paused,
    pcm_play_pause,
    pcm_get_bytes_waiting,
    pcm_calculate_peaks,
    pcm_get_peak_buffer,
    pcm_play_lock,
    pcm_play_unlock,
    beep_play,
#ifdef HAVE_RECORDING
    &rec_freq_sampr[0],
    pcm_init_recording,
    pcm_close_recording,
    pcm_record_data,
    pcm_stop_recording,
    pcm_calculate_rec_peaks,
    audio_set_recording_gain,
#endif /* HAVE_RECORDING */
#if INPUT_SRC_CAPS != 0
    audio_set_output_source,
    audio_set_input_source,
#endif
    dsp_set_crossfeed_type ,
    dsp_eq_enable,
    dsp_dither_enable,
#ifdef HAVE_PITCHCONTROL
    dsp_set_timestretch,
#endif
    dsp_configure,
    dsp_get_config,
    dsp_process,

    mixer_channel_status,
    mixer_channel_get_buffer,
    mixer_channel_calculate_peaks,
    mixer_channel_play_data,
    mixer_channel_play_pause,
    mixer_channel_stop,
    mixer_channel_set_amplitude,
    mixer_channel_get_bytes_waiting,

    system_sound_play,
    keyclick_click,
#endif /* CONFIG_CODEC == SWCODEC */
    /* playback control */
    playlist_amount,
    playlist_resume,
    playlist_start,
    playlist_add,
    playlist_sync,
    playlist_remove_all_tracks,
    playlist_create,
    playlist_insert_track,
    playlist_insert_directory,
    playlist_shuffle,
    audio_play,
    audio_stop,
    audio_pause,
    audio_resume,
    audio_next,
    audio_prev,
    audio_ff_rewind,
    audio_next_track,
    audio_status,
    audio_current_track,
    audio_flush_and_reload_tracks,
    audio_get_file_pos,
#if !defined(SIMULATOR) && (CONFIG_CODEC != SWCODEC)
    mpeg_get_last_header,
#endif
#if ((CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) || \
     (CONFIG_CODEC == SWCODEC)) && defined (HAVE_PITCHCONTROL)
    sound_set_pitch,
#endif

#if !defined(SIMULATOR) && (CONFIG_CODEC != SWCODEC)
    /* MAS communication */
    mas_readmem,
    mas_writemem,
    mas_readreg,
    mas_writereg,
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    mas_codec_writereg,
    mas_codec_readreg,
    i2c_begin,
    i2c_end,
    i2c_write,
#endif
#endif /* !SIMULATOR && CONFIG_CODEC != SWCODEC */

    /* menu */
    do_menu,
    /* statusbars */
    &statusbars,
    gui_syncstatusbar_draw,

    /* options */
    get_settings_list,
    find_setting,
    option_screen,
    set_option,
    set_bool_options,
    set_int,
    set_bool,
#ifdef HAVE_LCD_COLOR
    set_color,
#endif

    /* action handling */
    get_custom_action,
    get_action,
#ifdef HAVE_TOUCHSCREEN
    action_get_touchscreen_press,
#endif
    action_userabort,

    /* power */
    battery_level,
    battery_level_safe,
    battery_time,
    battery_voltage,
#if CONFIG_CHARGING
    charger_inserted,
# if CONFIG_CHARGING >= CHARGING_MONITOR
    charging_state,
# endif
#endif
#ifdef HAVE_USB_POWER
    usb_powered,
#endif

    /* misc */
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    &errno,
#endif
    srand,
    rand,
    (qsort_func)qsort,
    kbd_input,
    get_time,
    set_time,
#if CONFIG_RTC
    mktime,
#endif
    plugin_get_buffer,
    plugin_get_audio_buffer,
    plugin_tsr,
    plugin_get_current_filename,
#if defined(DEBUG) || defined(SIMULATOR)
    debugf,
#endif
#ifdef ROCKBOX_HAS_LOGF
    _logf,
#endif
    &global_settings,
    &global_status,
    talk_disable,
#if CONFIG_CODEC == SWCODEC
    codec_thread_do_callback,
    codec_load_file,
    codec_run_proc,
    codec_close,
    get_codec_filename,
    find_array_ptr,
    remove_array_ptr,
    round_value_to_list32,
#endif /* CONFIG_CODEC == SWCODEC */
    get_metadata,
    mp3info,
    count_mp3_frames,
    create_xing_header,
    find_next_frame,
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    peak_meter_scale_value,
    peak_meter_set_use_dbfs,
    peak_meter_get_use_dbfs,
#endif
#ifdef HAVE_LCD_BITMAP
    read_bmp_file,
    read_bmp_fd,
#ifdef HAVE_JPEG
    read_jpeg_file,
    read_jpeg_fd,
#endif
    screen_dump_set_hook,
#endif
    show_logo,
    tree_get_context,
    tree_get_entries,
    tree_get_entry_at,
    set_current_file,
    set_dirfilter,

#ifdef HAVE_WHEEL_POSITION
    wheel_status,
    wheel_send_events,
#endif

#ifdef IRIVER_H100_SERIES
    /* Routines for the iriver_flash -plugin. */
    detect_original_firmware,
    detect_flashed_ramimage,
    detect_flashed_romimage,
#endif
    led,
#if (CONFIG_CODEC == SWCODEC)
    bufopen,
    bufalloc,
    bufclose,
    bufseek,
    bufadvance,
    bufread,
    bufgetdata,
    bufgettail,
    bufcuttail,

    buf_handle_offset,
    buf_set_base_handle,
    buf_used,
#endif

#ifdef HAVE_TAGCACHE
    tagcache_search,
    tagcache_search_set_uniqbuf,
    tagcache_search_add_filter,
    tagcache_get_next,
    tagcache_retrieve,
    tagcache_search_finish,
    tagcache_get_numeric,
#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
    tagcache_fill_tags,
#endif
#endif

#ifdef HAVE_ALBUMART
    search_albumart_files,
#endif

#ifdef HAVE_SEMAPHORE_OBJECTS
    semaphore_init,
    semaphore_wait,
    semaphore_release,
#endif

    rbversion,

    /* new stuff at the end, sort into place next time
       the API gets incompatible */
};

int plugin_load(const char* plugin, const void* parameter)
{
    struct plugin_header *p_hdr;
    struct lc_header     *hdr;

    if (current_plugin_handle && pfn_tsr_exit)
    {    /* if we have a resident old plugin and a callback */
        if (pfn_tsr_exit(!strcmp(current_plugin, plugin)) == false )
        {
            /* not allowing another plugin to load */
            return PLUGIN_OK;
        }
        lc_close(current_plugin_handle);
        current_plugin_handle = pfn_tsr_exit = NULL;
    }

    splash(0, ID2P(LANG_WAIT));
    strcpy(current_plugin, plugin);

    current_plugin_handle = lc_open(plugin, pluginbuf, PLUGIN_BUFFER_SIZE);
    if (current_plugin_handle == NULL) {
        splashf(HZ*2, str(LANG_PLUGIN_CANT_OPEN), plugin);
        return -1;
    }

    p_hdr = lc_get_header(current_plugin_handle);

    hdr = p_hdr ? &p_hdr->lc_hdr : NULL;
    

    if (hdr == NULL
        || hdr->magic != PLUGIN_MAGIC
        || hdr->target_id != TARGET_ID
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
        || hdr->load_addr != pluginbuf
        || hdr->end_addr > pluginbuf + PLUGIN_BUFFER_SIZE
#endif
        )
    {
        lc_close(current_plugin_handle);
        splash(HZ*2, str(LANG_PLUGIN_WRONG_MODEL));
        return -1;
    }
    if (hdr->api_version > PLUGIN_API_VERSION
        || hdr->api_version < PLUGIN_MIN_API_VERSION)
    {
        lc_close(current_plugin_handle);
        splash(HZ*2, str(LANG_PLUGIN_WRONG_VERSION));
        return -1;
    }
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    plugin_size = hdr->end_addr - pluginbuf;
#else
    plugin_size = 0;
#endif

    *(p_hdr->api) = &rockbox_api;

    lcd_clear_display();
    lcd_update();

#ifdef HAVE_REMOTE_LCD
    lcd_remote_clear_display();
    lcd_remote_update();
#endif
    push_current_activity(ACTIVITY_PLUGIN);
    /* some plugins assume the entry cache doesn't move and save pointers to it
     * they should be fixed properly instead of this lock */
    tree_lock_cache(tree_get_context());

    FOR_NB_SCREENS(i)
       viewportmanager_theme_enable(i, false, NULL);
    
#ifdef HAVE_TOUCHSCREEN
    touchscreen_set_mode(TOUCHSCREEN_BUTTON);
#endif

#ifdef HAVE_PLUGIN_CHECK_OPEN_CLOSE
    open_files = 0;
#endif

    int rc = p_hdr->entry_point(parameter);
    
    tree_unlock_cache(tree_get_context());
    pop_current_activity();

    if (!pfn_tsr_exit)
    {   /* close handle if plugin is no tsr one */
        lc_close(current_plugin_handle);
        current_plugin_handle = NULL;
    }

    /* Go back to the global setting in case the plugin changed it */
#ifdef HAVE_TOUCHSCREEN
    touchscreen_set_mode(global_settings.touch_mode);
#endif

#ifdef HAVE_LCD_BITMAP
    screen_helper_setfont(FONT_UI);
#if LCD_DEPTH > 1
#ifdef HAVE_LCD_COLOR
    lcd_set_drawinfo(DRMODE_SOLID, global_settings.fg_color,
                                   global_settings.bg_color);
#else
    lcd_set_drawinfo(DRMODE_SOLID, LCD_DEFAULT_FG, LCD_DEFAULT_BG);
#endif
#else /* LCD_DEPTH == 1 */
    lcd_set_drawmode(DRMODE_SOLID);
#endif /* LCD_DEPTH */
#endif /* HAVE_LCD_BITMAP */


#ifdef HAVE_REMOTE_LCD
#if LCD_REMOTE_DEPTH > 1
    lcd_remote_set_drawinfo(DRMODE_SOLID, LCD_REMOTE_DEFAULT_FG,
                            LCD_REMOTE_DEFAULT_BG);
#else
    lcd_remote_set_drawmode(DRMODE_SOLID);
#endif
#endif

    lcd_clear_display();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_clear_display();
#endif

    FOR_NB_SCREENS(i)
        viewportmanager_theme_undo(i, true);

#ifdef HAVE_PLUGIN_CHECK_OPEN_CLOSE
    if(open_files != 0 && !current_plugin_handle)
    {
        int fd;
        logf("Plugin '%s' leaks file handles", plugin);
        
        static const char *lines[] = 
            { ID2P(LANG_PLUGIN_ERROR),
              "#leak-file-handles" };
        static const struct text_message message={ lines, 2 };
        button_clear_queue(); /* Empty the keyboard buffer */
        gui_syncyesno_run(&message, NULL, NULL);
        
        for(fd=0; fd < MAX_OPEN_FILES; fd++)
            if(open_files & (1<<fd))
                close_wrapper(fd);
    }
#endif

    if (rc == PLUGIN_ERROR)
        splash(HZ*2, str(LANG_PLUGIN_ERROR));

    return rc;
}

/* Returns a pointer to the portion of the plugin buffer that is not already
   being used.  If no plugin is loaded, returns the entire plugin buffer */
void* plugin_get_buffer(size_t *buffer_size)
{
    int buffer_pos;

    if (current_plugin_handle)
    {
        if (plugin_size >= PLUGIN_BUFFER_SIZE)
            return NULL;

        *buffer_size = PLUGIN_BUFFER_SIZE-plugin_size;
        buffer_pos = plugin_size;
    }
    else
    {
        *buffer_size = PLUGIN_BUFFER_SIZE;
        buffer_pos = 0;
    }

    return &pluginbuf[buffer_pos];
}

/* Returns a pointer to the mp3 buffer.
   Playback gets stopped, to avoid conflicts.
   Talk buffer is stolen as well.
 */
void* plugin_get_audio_buffer(size_t *buffer_size)
{
    audio_stop();
    return audio_get_buffer(true, buffer_size);
}

/* The plugin wants to stay resident after leaving its main function, e.g.
   runs from timer or own thread. The callback is registered to later
   instruct it to free its resources before a new plugin gets loaded. */
void plugin_tsr(bool (*exit_callback)(bool))
{
    pfn_tsr_exit = exit_callback; /* remember the callback for later */
}

char *plugin_get_current_filename(void)
{
    return current_plugin;
}

static int open_wrapper(const char* pathname, int flags, ...)
{
/* we don't have an 'open' function. it's a define. and we need
 * the real file_open, hence PREFIX() doesn't work here */
    int fd;
#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
    if (flags & O_CREAT)
    {
        va_list ap;
        va_start(ap, flags);
        fd = open(pathname, flags, va_arg(ap, unsigned int));
        va_end(ap);
    }
    else
        fd = open(pathname, flags);
#else
    fd = file_open(pathname,flags);
#endif

#ifdef HAVE_PLUGIN_CHECK_OPEN_CLOSE
    if(fd >= 0)
        open_files |= 1<<fd;
#endif
    return fd;
}

#ifdef HAVE_PLUGIN_CHECK_OPEN_CLOSE
static int close_wrapper(int fd)
{
    if((~open_files) & (1<<fd))
    {
        logf("double close from plugin");
    }
    if(fd >= 0)
        open_files &= (~(1<<fd));

    return PREFIX(close)(fd);
}

static int creat_wrapper(const char *pathname, mode_t mode)
{
    (void)mode;

    int fd = PREFIX(creat)(pathname, mode);

    if(fd >= 0)
        open_files |= (1<<fd);

    return fd;
}
#endif /* HAVE_PLUGIN_CHECK_OPEN_CLOSE */
