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
#define DIRFUNCTIONS_DEFINED
#define FILEFUNCTIONS_DEFINED
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
#include "pathfuncs.h"
#include "load_code.h"
#include "file.h"

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

#define WRAPPER(_x_) _x_ ## _wrapper

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

static void* plugin_get_audio_buffer(size_t *buffer_size);
static void plugin_release_audio_buffer(void);
static void plugin_tsr(bool (*exit_callback)(bool));

#ifdef HAVE_PLUGIN_CHECK_OPEN_CLOSE
/* File handle leak prophylaxis */
#include "bitarray.h"
#include "file_internal.h" /* for MAX_OPEN_FILES */

#define PCOC_WRAPPER(_x_)   WRAPPER(_x_)

BITARRAY_TYPE_DECLARE(plugin_check_open_close_bitmap_t, open_files_bitmap,
                      MAX_OPEN_FILES)

static plugin_check_open_close_bitmap_t open_files_bitmap;

static void plugin_check_open_close__enter(void)
{
    if (!current_plugin_handle)
        open_files_bitmap_clear(&open_files_bitmap);
}

static void plugin_check_open_close__open(int fildes)
{
    if (fildes >= 0)
        open_files_bitmap_set_bit(&open_files_bitmap, fildes);
}

static void plugin_check_open_close__close(int fildes)
{
    if (fildes < 0)
        return;

    if (!open_files_bitmap_test_bit(&open_files_bitmap, fildes))
    {
        logf("double close from plugin");
    }

    open_files_bitmap_clear_bit(&open_files_bitmap, fildes);
}

static int WRAPPER(open)(const char *path, int oflag, ...)
{
    int fildes = FS_PREFIX(open)(path, oflag __OPEN_MODE_ARG);
    plugin_check_open_close__open(fildes);
    return fildes;
}

static int WRAPPER(creat)(const char *path, mode_t mode)
{
    int fildes = FS_PREFIX(creat)(path __CREAT_MODE_ARG);
    plugin_check_open_close__open(fildes);
    return fildes;
    (void)mode;
}

static int WRAPPER(close)(int fildes)
{
    int rc = FS_PREFIX(close)(fildes);
    if (rc >= 0)
        plugin_check_open_close__close(fildes);

    return rc;
}

static void plugin_check_open_close__exit(void)
{
    if (current_plugin_handle)
        return;

    if (open_files_bitmap_is_clear(&open_files_bitmap))
        return;

    logf("Plugin '%s' leaks file handles", current_plugin);

    static const char *lines[] =
        { ID2P(LANG_PLUGIN_ERROR), "#leak-file-handles" };
    static const struct text_message message = { lines, 2 };
    button_clear_queue(); /* Empty the keyboard buffer */
    gui_syncyesno_run(&message, NULL, NULL);

    FOR_EACH_BITARRAY_SET_BIT(&open_files_bitmap, fildes)
        WRAPPER(close)(fildes);
}

#else /* !HAVE_PLUGIN_CHECK_OPEN_CLOSE */

#define PCOC_WRAPPER(_x_) FS_PREFIX(_x_)
#define plugin_check_open_close__enter()
#define plugin_check_open_close__exit()

#endif /* HAVE_PLUGIN_CHECK_OPEN_CLOSE */

static const struct plugin_api rockbox_api = {
    .rbversion = rbversion,
    .global_settings = &global_settings,
    .global_status = &global_status,
    .language_strings = language_strings,
    /* lcd */
    .splash = splash,
    .splashf = splashf,
#ifdef HAVE_LCD_CONTRAST
    .lcd_set_contrast = lcd_set_contrast,
#endif
    .lcd_update = lcd_update,
    .lcd_clear_display = lcd_clear_display,
    .lcd_getstringsize = lcd_getstringsize,
    .lcd_putsxy = lcd_putsxy,
    .lcd_putsxyf = lcd_putsxyf,
    .lcd_puts = lcd_puts,
    .lcd_putsf = lcd_putsf,
    .lcd_puts_scroll = lcd_puts_scroll,
    .lcd_scroll_stop = lcd_scroll_stop,
#ifdef HAVE_LCD_CHARCELLS
    .lcd_define_pattern = lcd_define_pattern,
    .lcd_get_locked_pattern = lcd_get_locked_pattern,
    .lcd_unlock_pattern = lcd_unlock_pattern,
    .lcd_putc = lcd_putc,
    .lcd_put_cursor = lcd_put_cursor,
    .lcd_remove_cursor = lcd_remove_cursor,
    .lcd_icon = lcd_icon,
    .lcd_double_height = lcd_double_height,
#else /* HAVE_LCD_BITMAP */
    .lcd_framebuffer = &lcd_static_framebuffer[0][0],
    .lcd_set_viewport = lcd_set_viewport,
    .lcd_set_framebuffer = lcd_set_framebuffer,
    .lcd_bmp_part = lcd_bmp_part,
    .lcd_update_rect = lcd_update_rect,
    .lcd_set_drawmode = lcd_set_drawmode,
    .lcd_get_drawmode = lcd_get_drawmode,
    .lcd_setfont = screen_helper_setfont,
    .lcd_drawpixel = lcd_drawpixel,
    .lcd_drawline = lcd_drawline,
    .lcd_hline = lcd_hline,
    .lcd_vline = lcd_vline,
    .lcd_drawrect = lcd_drawrect,
    .lcd_fillrect = lcd_fillrect,
    .lcd_mono_bitmap_part = lcd_mono_bitmap_part,
    .lcd_mono_bitmap = lcd_mono_bitmap,
#if LCD_DEPTH > 1
    .lcd_set_foreground = lcd_set_foreground,
    .lcd_get_foreground = lcd_get_foreground,
    .lcd_set_background = lcd_set_background,
    .lcd_get_background = lcd_get_background,
    .lcd_bitmap_part = lcd_bitmap_part,
    .lcd_bitmap = lcd_bitmap,
    .lcd_get_backdrop = lcd_get_backdrop,
    .lcd_set_backdrop = lcd_set_backdrop,
#endif
#if LCD_DEPTH >= 16
    .lcd_bitmap_transparent_part = lcd_bitmap_transparent_part,
    .lcd_bitmap_transparent = lcd_bitmap_transparent,
#if MEMORYSIZE > 2
    .lcd_blit_yuv = lcd_blit_yuv,
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200) || defined(SANSA_C200) \
    || defined(IRIVER_H10) || defined(COWON_D2) || defined(PHILIPS_HDD1630) \
    || defined(SANSA_FUZE) || defined(SANSA_E200V2) || defined(SANSA_FUZEV2) \
    || defined(TOSHIBA_GIGABEAT_S) || defined(PHILIPS_SA9200)
    .lcd_set_yuv_options = lcd_yuv_set_options,
#endif
#endif /* MEMORYSIZE > 2 */
#elif (LCD_DEPTH < 4) && (CONFIG_PLATFORM & PLATFORM_NATIVE)
    .lcd_blit_mono = lcd_blit_mono,
    .lcd_blit_grey_phase = lcd_blit_grey_phase,
#endif /* LCD_DEPTH */
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    .lcd_bit_pal256 = lcd_blit_pal256,
    .lcd_pal256_update_pal = lcd_pal256_update_pal,
#endif
#ifdef HAVE_LCD_INVERT
    .lcd_set_invert_display = lcd_set_invert_display,
#endif /* HAVE_LCD_INVERT */
#if defined(HAVE_LCD_MODES)
    .lcd_set_mode = lcd_set_mode,
#endif
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
    .button_queue = &button_queue,
#endif
    .bidi_l2v = bidi_l2v,
    .is_diacritic = is_diacritic,
    .font_get_bits = font_get_bits,
    .font_load = font_load,
    .font_unload = font_unload,
    .font_get = font_get,
    .font_getstringsize = font_getstringsize,
    .font_get_width = font_get_width,
    .screen_clear_area = screen_clear_area,
    .gui_scrollbar_draw = gui_scrollbar_draw,
#endif /* HAVE_LCD_BITMAP */
    .get_codepage_name = get_codepage_name,

#ifdef HAVE_REMOTE_LCD
    /* remote lcd */
    .lcd_remote_set_contrast = lcd_remote_set_contrast,
    .lcd_remote_clear_display = lcd_remote_clear_display,
    .lcd_remote_puts = lcd_remote_puts,
    .lcd_remote_puts_scroll = lcd_remote_puts_scroll,
    .lcd_remote_scroll_stop = lcd_remote_scroll_stop,
    .lcd_remote_set_drawmode = lcd_remote_set_drawmode,
    .lcd_remote_get_drawmode = lcd_remote_get_drawmode,
    .lcd_remote_setfont = lcd_remote_setfont,
    .lcd_remote_getstringsize = lcd_remote_getstringsize,
    .lcd_remote_drawpixel = lcd_remote_drawpixel,
    .lcd_remote_drawline = lcd_remote_drawline,
    .lcd_remote_hline = lcd_remote_hline,
    .lcd_remote_vline = lcd_remote_vline,
    .lcd_remote_drawrect = lcd_remote_drawrect,
    .lcd_remote_fillrect = lcd_remote_fillrect,
    .lcd_remote_mono_bitmap_part = lcd_remote_mono_bitmap_part,
    .lcd_remote_mono_bitmap = lcd_remote_mono_bitmap,
    .lcd_remote_putsxy = lcd_remote_putsxy,
    .lcd_remote_framebuffer = &lcd_remote_static_framebuffer[0][0],
    .lcd_remote_update = lcd_remote_update,
    .lcd_remote_update_rect = lcd_remote_update_rect,
#if (LCD_REMOTE_DEPTH > 1)
    .lcd_remote_set_foreground = lcd_remote_set_foreground,
    .lcd_remote_get_foreground = lcd_remote_get_foreground,
    .lcd_remote_set_background = lcd_remote_set_background,
    .lcd_remote_set_background = lcd_remote_get_background,
    .lcd_remote_bitmap_part = lcd_remote_bitmap_part,
    .lcd_remote_bitmap = lcd_remote_bitmap,
#endif
#endif /* HAVE_REMOTE_LCD */

#if NB_SCREENS == 2
    .screens = {&screens[SCREEN_MAIN], &screens[SCREEN_REMOTE]},
#else
    .screens = {&screens[SCREEN_MAIN]},
#endif

    .viewport_set_defaults = viewport_set_defaults,
#ifdef HAVE_LCD_BITMAP
    .viewportmanager_theme_enable = viewportmanager_theme_enable,
    .viewportmanager_theme_undo = viewportmanager_theme_undo,
    .viewport_set_fullscreen = viewport_set_fullscreen,
#endif

    /* lcd backlight */
    /* The backlight_* functions must be present in the API regardless whether
     * HAVE_BACKLIGHT is defined or not. The reason is that the stock Ondio has
     * no backlight but can be modded to have backlight (it's prepared on the
     * PCB). This makes backlight an all-target feature API wise, and keeps API
     * compatible between stock and modded Ondio.
     * For OLED targets like the Sansa Clip, the backlight_* functions control
     * the display enable, which has essentially the same effect. */
    .is_backlight_on = is_backlight_on,
    .backlight_on = backlight_on,
    .backlight_off = backlight_off,
    .backlight_set_timeout = backlight_set_timeout,
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    .backlight_set_brightness = backlight_set_brightness,
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#if CONFIG_CHARGING
    .backlight_set_timeout_plugged = backlight_set_timeout_plugged,
#endif

#ifdef HAVE_REMOTE_LCD
    .remote_backlight_on = remote_backlight_on,
    .remote_backlight_off = remote_backlight_off,
    .remote_backlight_set_timeout = remote_backlight_set_timeout,
#if CONFIG_CHARGING
    .remote_backlight_set_timeout_plugged = remote_backlight_set_timeout_plugged,
#endif
#endif /* HAVE_REMOTE_LCD */

    /* list */
    .gui_synclist_init = gui_synclist_init,
    .gui_synclist_set_nb_items = gui_synclist_set_nb_items,
    .gui_synclist_set_voice_callback = gui_synclist_set_voice_callback,
    .gui_synclist_set_icon_callback = gui_synclist_set_icon_callback,
    .gui_synclist_get_nb_items = gui_synclist_get_nb_items,
    .gui_synclist_get_sel_pos = gui_synclist_get_sel_pos,
    .gui_synclist_draw = gui_synclist_draw,
    .gui_synclist_speak_item = gui_synclist_speak_item,
    .gui_synclist_select_item = gui_synclist_select_item,
    .gui_synclist_add_item = gui_synclist_add_item,
    .gui_synclist_del_item = gui_synclist_del_item,
    .gui_synclist_limit_scroll = gui_synclist_limit_scroll,
    .gui_synclist_do_button = gui_synclist_do_button,
    .gui_synclist_set_title = gui_synclist_set_title,
    .gui_syncyesno_run = gui_syncyesno_run,
    .simplelist_info_init = simplelist_info_init,
    .simplelist_show_list = simplelist_show_list,

    /* action handling */
    .get_custom_action = get_custom_action,
    .get_action = get_action,
#ifdef HAVE_TOUCHSCREEN
    .action_get_touchscreen_press = action_get_touchscreen_press,
#endif
    .action_userabort = action_userabort,

    /* button */
    .button_get = button_get,
    .button_get_w_tmo = button_get_w_tmo,
    .button_status = button_status,
#ifdef HAVE_BUTTON_DATA
    .button_get_data = button_get_data,
    .button_status_wdata = button_status_wdata,
#endif
    .button_clear_queue = button_clear_queue,
    .button_queue_count = button_queue_count,
#ifdef HAS_BUTTON_HOLD
    .button_hold = button_hold,
#endif
#ifdef HAVE_SW_POWEROFF
    .button_set_sw_poweroff_state = button_set_sw_poweroff_state,
    .button_get_sw_poweroff_state = button_get_sw_poweroff_state,
#endif
#ifdef HAVE_TOUCHSCREEN
    .touchscreen_set_mode = touchscreen_set_mode,
    .touchscreen_get_mode = touchscreen_get_mode,
#endif

#ifdef HAVE_BUTTON_LIGHT
    .buttonlight_set_timeout = buttonlight_set_timeout,
    .buttonlight_off = buttonlight_off,
    .buttonlight_on = buttonlight_on,
#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
    .buttonlight_set_brightness = buttonlight_set_brightness,
#endif /* HAVE_BUTTONLIGHT_BRIGHTNESS */
#endif /* HAVE_BUTTON_LIGHT */

    /* file */
    .open_utf8 = open_utf8,
    .open = PCOC_WRAPPER(open),
    .creat = PCOC_WRAPPER(creat),
    .close = PCOC_WRAPPER(close),
    .read = FS_PREFIX(read),
    .lseek = FS_PREFIX(lseek),
    .write = FS_PREFIX(write),
    .remove = FS_PREFIX(remove),
    .rename = FS_PREFIX(rename),
    .ftruncate = FS_PREFIX(ftruncate),
    .filesize = FS_PREFIX(filesize),
    .fdprintf = fdprintf,
    .read_line = read_line,
    .settings_parseline = settings_parseline,
    .storage_sleep = storage_sleep,
    .storage_spin = STORAGE_FUNCTION(spin),
    .storage_spindown = STORAGE_FUNCTION(spindown),
#if USING_STORAGE_CALLBACK
    .register_storage_idle_func = register_storage_idle_func,
    .unregister_storage_idle_func = unregister_storage_idle_func,
#endif /* USING_STORAGE_CALLBACK */
    .reload_directory = reload_directory,
    .create_numbered_filename = create_numbered_filename,
    .file_exists = FS_PREFIX(file_exists),
    .strip_extension = strip_extension,
    .crc_32 = crc_32,
    .filetype_get_attr = filetype_get_attr,

    /* dir */
    .opendir = FS_PREFIX(opendir),
    .closedir = FS_PREFIX(closedir),
    .readdir = FS_PREFIX(readdir),
    .mkdir = FS_PREFIX(mkdir),
    .rmdir = FS_PREFIX(rmdir),
    .dir_exists = FS_PREFIX(dir_exists),
    .dir_get_info = dir_get_info,

    /* browsing */
    .browse_context_init = browse_context_init,
    .rockbox_browse = rockbox_browse,
    .tree_get_context = tree_get_context,
    .tree_get_entries = tree_get_entries,
    .tree_get_entry_at = tree_get_entry_at,
    .set_current_file = set_current_file,
    .set_dirfilter = set_dirfilter,

    /* talking */
    .talk_id = talk_id,
    .talk_file = talk_file,
    .talk_file_or_spell = talk_file_or_spell,
    .talk_dir_or_spell = talk_dir_or_spell,
    .talk_number = talk_number,
    .talk_value = talk_value,
    .talk_spell = talk_spell,
    .talk_time = talk_time,
    .talk_date = talk_date,
    .talk_disable = talk_disable,
    .talk_shutup = talk_shutup,
    .talk_force_shutup = talk_force_shutup,
    .talk_force_enqueue_next = talk_force_enqueue_next,

    /* kernel/ system */
#if defined(CPU_ARM) && CONFIG_PLATFORM & PLATFORM_NATIVE
    .__div0 = __div0,
#endif
    .sleep = sleep,
    .yield = yield,
    .current_tick = &current_tick,
    .default_event_handler = default_event_handler,
    .default_event_handler_ex = default_event_handler_ex,
    .create_thread = create_thread,
    .thread_self = thread_self,
    .thread_exit = thread_exit,
    .thread_wait = thread_wait,
#if (CONFIG_CODEC == SWCODEC)
    .thread_thaw = thread_thaw,
#ifdef HAVE_PRIORITY_SCHEDULING
    .thread_set_priority = thread_set_priority,
#endif
    .mutex_init = mutex_init,
    .mutex_lock = mutex_lock,
    .mutex_unlock = mutex_unlock,
#endif
#ifdef HAVE_SEMAPHORE_OBJECTS
    .semaphore_init = semaphore_init,
    .semaphore_wait = semaphore_wait,
    .semaphore_release = semaphore_release,
#endif
    .reset_poweroff_timer = reset_poweroff_timer,
    .set_sleeptimer_duration = set_sleeptimer_duration, /*stub*/
    .get_sleep_timer = get_sleep_timer, /*stub*/
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    .system_memory_guard = system_memory_guard,
    .cpu_frequency = &cpu_frequency,

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#ifdef CPU_BOOST_LOGGING
    .cpu_boost_ = cpu_boost_,
#else
    .cpu_boost = cpu_boost,
#endif
#endif /* HAVE_ADJUSTABLE_CPU_FREQ */
#endif /* PLATFORM_NATIVE */
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    .trigger_cpu_boost = trigger_cpu_boost,
    .cancel_cpu_boost = cancel_cpu_boost,
#endif

    .commit_dcache = commit_dcache,
    .commit_discard_dcache = commit_discard_dcache,
    .commit_discard_idcache = commit_discard_idcache,

    .lc_open = lc_open,
    .lc_open_from_mem = lc_open_from_mem,
    .lc_get_header = lc_get_header,
    .lc_close = lc_close,

    .timer_register = timer_register,
    .timer_unregister = timer_unregister,
    .timer_set_period = timer_set_period,

    .queue_init = queue_init,
    .queue_delete = queue_delete,
    .queue_post = queue_post,
    .queue_wait_w_tmo = queue_wait_w_tmo,
#if CONFIG_CODEC == SWCODEC
    .queue_enable_queue_send = queue_enable_queue_send,
    .queue_empty = queue_empty,
    .queue_wait = queue_wait,
    .queue_send = queue_send,
    .queue_reply = queue_reply,
#endif

#ifdef RB_PROFILE
    .profile_thread = profile_thread,
    .profstop = profstop,
    .__cyg_profile_func_enter = __cyg_profile_func_enter,
    .__cyg_profile_func_exit = __cyg_profile_func_exit,
#endif
    .add_event = add_event,
    .remove_event = remove_event,
    .send_event = send_event,

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
    /* special simulator hooks */
#if defined(HAVE_LCD_BITMAP) && LCD_DEPTH < 8
    .sim_lcd_ex_init = sim_lcd_ex_init,
    .sim_lcd_ex_update_rect = sim_lcd_ex_update_rect,
#endif
#endif

    /* strings and memory */
    .snprintf = snprintf,
    .vsnprintf = vsnprintf,
    .strcpy = strcpy,
    .strlcpy = strlcpy,
    .strlen = strlen,
    .strrchr = strrchr,
    .strcmp = strcmp,
    .strncmp = strncmp,
    .strcasecmp = strcasecmp,
    .strncasecmp = strncasecmp,
    .memset = memset,
    .memcpy = memcpy,
    .memmove = memmove,
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    ._rbctype_ = _ctype_,
#endif
    .atoi = atoi,
    .strchr = strchr,
    .strcat = strcat,
    .strlcat = strlcat,
    .memchr = memchr,
    .memcmp = memcmp,
    .strcasestr = strcasestr,
    .strtok_r = strtok_r,
    /* unicode stuff */
    .utf8decode = utf8decode,
    .iso_decode = iso_decode,
    .utf16LEdecode = utf16LEdecode,
    .utf16BEdecode = utf16BEdecode,
    .utf8encode = utf8encode,
    .utf8length = utf8length,
    .utf8seek = utf8seek,

    /* the buflib memory management library */
    .buflib_init = buflib_init,
    .buflib_available = buflib_available,
    .buflib_alloc = buflib_alloc,
    .buflib_alloc_ex = buflib_alloc_ex,
    .buflib_alloc_maximum = buflib_alloc_maximum,
    .buflib_buffer_in = buflib_buffer_in,
    .buflib_buffer_out = buflib_buffer_out,
    .buflib_free = buflib_free,
    .buflib_shrink = buflib_shrink,
    .buflib_get_data = buflib_get_data,
    .buflib_get_name = buflib_get_name,

    /* sound */
    .sound_set = sound_set,
    .sound_current = sound_current, /*stub*/
    .sound_default = sound_default,
    .sound_min = sound_min,
    .sound_max = sound_max,
    .sound_unit = sound_unit,
    .sound_val2phys = sound_val2phys,
#ifdef AUDIOHW_HAVE_EQ
    .sound_enum_hw_eq_band_setting = sound_enum_hw_eq_band_setting,
#endif
#if ((CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) || \
     (CONFIG_CODEC == SWCODEC)) && defined (HAVE_PITCHCONTROL)
    .sound_set_pitch = sound_set_pitch,
#endif
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    .mp3_play_data = mp3_play_data,
    .mp3_play_pause = mp3_play_pause,
    .mp3_play_stop = mp3_play_stop,
    .mp3_is_playing = mp3_is_playing,
#if CONFIG_CODEC != SWCODEC
    .bitswap = bitswap,
#endif
#endif
#if CONFIG_CODEC == SWCODEC
    .audio_master_sampr_list = &audio_master_sampr_list[0],
    .hw_freq_sampr = &hw_freq_sampr[0],
    .pcm_apply_settings = pcm_apply_settings,
    .pcm_play_data = pcm_play_data,
    .pcm_play_stop = pcm_play_stop,
    .pcm_set_frequency = pcm_set_frequency,
    .pcm_is_playing = pcm_is_playing,
    .pcm_is_paused = pcm_is_paused,
    .pcm_play_pause = pcm_play_pause,
    .pcm_get_bytes_waiting = pcm_get_bytes_waiting,
    .pcm_calculate_peaks = pcm_calculate_peaks,
    .pcm_get_peak_buffer = pcm_get_peak_buffer,
    .pcm_play_lock = pcm_play_lock,
    .pcm_play_unlock = pcm_play_unlock,
    .beep_play = beep_play,
#ifdef HAVE_RECORDING
    .rec_freq_sampr = &rec_freq_sampr[0],
    .pcm_init_recording = pcm_init_recording,
    .pcm_close_recording = pcm_close_recording,
    .pcm_record_data = pcm_record_data,
    .pcm_stop_recording = pcm_stop_recording,
    .pcm_calculate_rec_peaks = pcm_calculate_rec_peaks,
    .audio_set_recording_gain = audio_set_recording_gain,
#endif /* HAVE_RECORDING */
#if INPUT_SRC_CAPS != 0
    .audio_set_output_source = audio_set_output_source,
    .audio_set_input_source = audio_set_input_source,
#endif
    .dsp_set_crossfeed_type = dsp_set_crossfeed_type,
    .dsp_eq_enable = dsp_eq_enable,
    .dsp_dither_enable = dsp_dither_enable,
#ifdef HAVE_PITCHCONTROL
    .dsp_set_timestretch = dsp_set_timestretch,
#endif
    .dsp_configure = dsp_configure,
    .dsp_get_config = dsp_get_config,
    .dsp_process = dsp_process,

    .mixer_channel_status = mixer_channel_status,
    .mixer_channel_get_buffer = mixer_channel_get_buffer,
    .mixer_channel_calculate_peaks = mixer_channel_calculate_peaks,
    .mixer_channel_play_data = mixer_channel_play_data,
    .mixer_channel_play_pause = mixer_channel_play_pause,
    .mixer_channel_stop = mixer_channel_stop,
    .mixer_channel_set_amplitude = mixer_channel_set_amplitude,
    .mixer_channel_get_bytes_waiting = mixer_channel_get_bytes_waiting,
    .mixer_channel_set_buffer_hook = mixer_channel_set_buffer_hook,
    .mixer_set_frequency = mixer_set_frequency,
    .mixer_get_frequency = mixer_get_frequency,

    .pcmbuf_fade = pcmbuf_fade,
    .system_sound_play = system_sound_play,
    .keyclick_click = keyclick_click,
#endif /* CONFIG_CODEC == SWCODEC */

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    .peak_meter_scale_value = peak_meter_scale_value,
    .peak_meter_set_use_dbfs = peak_meter_set_use_dbfs,
    .peak_meter_get_use_dbfs = peak_meter_get_use_dbfs,
#endif


    /* metadata */
    .get_metadata = get_metadata,
    .mp3info = mp3info,
    .count_mp3_frames = count_mp3_frames,
    .create_xing_header = create_xing_header,
    .find_next_frame = find_next_frame,
#ifdef HAVE_TAGCACHE
    .tagcache_search = tagcache_search,
    .tagcache_search_set_uniqbuf = tagcache_search_set_uniqbuf,
    .tagcache_search_add_filter = tagcache_search_add_filter,
    .tagcache_get_next = tagcache_get_next,
    .tagcache_retrieve = tagcache_retrieve,
    .tagcache_search_finish = tagcache_search_finish,
    .tagcache_get_numeric = tagcache_get_numeric,
#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
    .tagcache_fill_tags = tagcache_fill_tags,
#endif
#endif /* HAVE_TAGCACHE */

#ifdef HAVE_ALBUMART
    .search_albumart_files = search_albumart_files,
#endif

    /* playback control */
    .playlist_amount = playlist_amount,
    .playlist_resume = playlist_resume,
    .playlist_resume_track = playlist_resume_track,
    .playlist_start = playlist_start,
    .playlist_add = playlist_add,
    .playlist_sync = playlist_sync,
    .playlist_remove_all_tracks = playlist_remove_all_tracks,
    .playlist_create = playlist_create,
    .playlist_insert_track = playlist_insert_track,
    .playlist_insert_directory = playlist_insert_directory,
    .playlist_shuffle = playlist_shuffle,
    .audio_play = audio_play,
    .audio_stop = audio_stop,
    .audio_pause = audio_pause,
    .audio_resume = audio_resume,
    .audio_next = audio_next,
    .audio_prev = audio_prev,
    .audio_ff_rewind = audio_ff_rewind,
    .audio_next_track = audio_next_track,
    .audio_status = audio_status,
    .audio_current_track = audio_current_track,
    .audio_flush_and_reload_tracks = audio_flush_and_reload_tracks,
    .audio_get_file_pos = audio_get_file_pos,
#if !defined(SIMULATOR) && (CONFIG_CODEC != SWCODEC)
    .mpeg_get_last_header = mpeg_get_last_header,
#endif

#if !defined(SIMULATOR) && (CONFIG_CODEC != SWCODEC)
    /* MAS communication */
    .mas_readmem = mas_readmem,
    .mas_writemem = mas_writemem,
    .mas_readreg = mas_readreg,
    .mas_writereg = mas_writereg,
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    .mas_codec_writereg = mas_codec_writereg,
    .mas_codec_readreg = mas_codec_readreg,
    .i2c_begin = i2c_begin,
    .i2c_end = i2c_end,
    .i2c_write = i2c_write,
#endif
#endif /* !SIMULATOR && CONFIG_CODEC != SWCODEC */

    /* menu */
    .root_menu_get_options = root_menu_get_options,
    .do_menu = do_menu,
    .root_menu_set_default = root_menu_set_default,
    .root_menu_write_to_cfg = root_menu_write_to_cfg,
    .root_menu_load_from_cfg = root_menu_load_from_cfg,

    /* statusbars */
    .statusbars = &statusbars,
    .gui_syncstatusbar_draw = gui_syncstatusbar_draw,

    /* options */
    .get_settings_list = get_settings_list,
    .find_setting = find_setting,
    .settings_save = settings_save,
    .option_screen = option_screen,
    .set_option = set_option,
    .set_bool_options = set_bool_options,
    .set_int = set_int,
    .set_int_ex = set_int_ex,
    .set_bool = set_bool,
#ifdef HAVE_LCD_COLOR
    .set_color = set_color,
#endif

    /* power */
    .battery_level = battery_level,
    .battery_level_safe = battery_level_safe,
    .battery_time = battery_time,
    .battery_voltage = battery_voltage,
#if CONFIG_CHARGING
    .charger_inserted = charger_inserted,
# if CONFIG_CHARGING >= CHARGING_MONITOR
    .charging_state = charging_state,
# endif
#endif
    /* usb */
    .usb_inserted = usb_inserted,
    .usb_acknowledge = usb_acknowledge,
#ifdef USB_ENABLE_HID
    .usb_hid_send = usb_hid_send,
#endif
    /* misc */
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    .__errno = __errno,
#endif
    .srand = srand,
    .rand = rand,
    .qsort = (void *)qsort,
    .kbd_input = kbd_input,
    .get_time = get_time,
    .set_time = set_time,
    .gmtime_r = gmtime_r,
#if CONFIG_RTC
    .mktime = mktime,
#endif

#if defined(DEBUG) || defined(SIMULATOR)
    .debugf = debugf,
#endif
#ifdef ROCKBOX_HAS_LOGF
    ._logf = _logf,
#endif
#if CONFIG_CODEC == SWCODEC
    .codec_thread_do_callback = codec_thread_do_callback,
    .codec_load_file = codec_load_file,
    .codec_run_proc = codec_run_proc,
    .codec_close = codec_close,
    .get_codec_filename = get_codec_filename,
    .find_array_ptr = find_array_ptr,
    .remove_array_ptr = remove_array_ptr,
    .round_value_to_list32 = round_value_to_list32,
#endif /* CONFIG_CODEC == SWCODEC */

#ifdef HAVE_LCD_BITMAP
    .read_bmp_file = read_bmp_file,
    .read_bmp_fd = read_bmp_fd,
#ifdef HAVE_JPEG
    .read_jpeg_file = read_jpeg_file,
    .read_jpeg_fd = read_jpeg_fd,
#endif
    .screen_dump_set_hook = screen_dump_set_hook,
#endif
    .show_logo = show_logo,

#ifdef HAVE_WHEEL_POSITION
    .wheel_status = wheel_status,
    .wheel_send_events = wheel_send_events,
#endif

#ifdef IRIVER_H100_SERIES
    /* Routines for the iriver_flash -plugin. */
    .detect_original_firmware = detect_original_firmware,
    .detect_flashed_ramimage = detect_flashed_ramimage,
    .detect_flashed_romimage = detect_flashed_romimage,
#endif
    .led = led,

    /*plugin*/
    .plugin_get_buffer = plugin_get_buffer,
    .plugin_get_audio_buffer = plugin_get_audio_buffer,     /* defined in plugin.c */
    .plugin_release_audio_buffer = plugin_release_audio_buffer, /* defined in plugin.c */
    .plugin_tsr = plugin_tsr,                  /* defined in plugin.c */
    .plugin_get_current_filename = plugin_get_current_filename,
#ifdef PLUGIN_USE_IRAM
    .audio_hard_stop = audio_hard_stop,
#endif

    /* new stuff at the end, sort into place next time
       the API gets incompatible */
};

static int plugin_buffer_handle;
static size_t plugin_buffer_size;

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
        if (plugin_buffer_handle > 0)
            plugin_buffer_handle = core_free(plugin_buffer_handle);
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
        splash(HZ*2, ID2P(LANG_PLUGIN_WRONG_MODEL));
        return -1;
    }
    if (hdr->api_version > PLUGIN_API_VERSION
        || hdr->api_version < PLUGIN_MIN_API_VERSION)
    {
        lc_close(current_plugin_handle);
        splash(HZ*2, ID2P(LANG_PLUGIN_WRONG_VERSION));
        return -1;
    }
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    /* tlsf crashes observed on arm with 0x4 aligned addresses */
    plugin_size = ALIGN_UP(hdr->end_addr - pluginbuf, 0x8);
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

    /* allow voice to back off if the plugin needs lots of memory */
    if (!global_settings.talk_menu)
        talk_buffer_set_policy(TALK_BUFFER_LOOSE);

    plugin_check_open_close__enter();

    int rc = p_hdr->entry_point(parameter);

    tree_unlock_cache(tree_get_context());
    pop_current_activity();

    if (!pfn_tsr_exit)
    {   /* close handle if plugin is no tsr one */
        lc_close(current_plugin_handle);
        current_plugin_handle = NULL;
        if (plugin_buffer_handle > 0)
            plugin_buffer_handle = core_free(plugin_buffer_handle);
    }

    talk_buffer_set_policy(TALK_BUFFER_DEFAULT);

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

    plugin_check_open_close__exit();

    status_save();

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
static void* plugin_get_audio_buffer(size_t *buffer_size)
{
    /* dummy ops with no callbacks, needed because by
     * default buflib buffers can be moved around which must be avoided */
    static struct buflib_callbacks dummy_ops;
    if (plugin_buffer_handle <= 0)
    {
        plugin_buffer_handle = core_alloc_maximum("plugin audio buf",
                                                  &plugin_buffer_size,
                                                  &dummy_ops);
    }

    if (buffer_size)
        *buffer_size = plugin_buffer_size;

    return core_get_data(plugin_buffer_handle);
}

static void plugin_release_audio_buffer(void)
{
    if (plugin_buffer_handle > 0)
    {
        plugin_buffer_handle = core_free(plugin_buffer_handle);
        plugin_buffer_size = 0;
    }
}

/* The plugin wants to stay resident after leaving its main function, e.g.
   runs from timer or own thread. The callback is registered to later
   instruct it to free its resources before a new plugin gets loaded. */
static void plugin_tsr(bool (*exit_callback)(bool))
{
    pfn_tsr_exit = exit_callback; /* remember the callback for later */
}

char *plugin_get_current_filename(void)
{
    return current_plugin;
}
