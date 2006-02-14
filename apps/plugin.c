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
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <atoi.h>
#include <timefuncs.h>
#include <ctype.h>
#include "debug.h"
#include "button.h"
#include "lcd.h"
#include "dir.h"
#include "file.h"
#include "kernel.h"
#include "usb.h"
#include "sprintf.h"
#include "logf.h"
#include "screens.h"
#include "misc.h"
#include "mas.h"
#include "plugin.h"
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
#include "timer.h"
#include "sound.h"
#include "database.h"
#include "splash.h"
#if (CONFIG_CODEC == SWCODEC)
#include "pcm_playback.h"
#endif

#ifdef HAVE_CHARGING
#include "power.h"
#endif

#ifdef HAVE_LCD_BITMAP
#include "peakmeter.h"
#include "widgets.h"
#include "bmp.h"
#endif

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

#ifdef SIMULATOR
static unsigned char pluginbuf[PLUGIN_BUFFER_SIZE];
void *sim_plugin_load(char *plugin, int *fd);
void sim_plugin_close(int fd);
#else
#define sim_plugin_close(x)
extern unsigned char pluginbuf[];
#include "bitswap.h"
#endif

/* for actual plugins only, not for codecs */
static bool plugin_loaded = false;
static int  plugin_size = 0;
static void (*pfn_tsr_exit)(void) = NULL; /* TSR exit callback */

static const struct plugin_api rockbox_api = {

    /* lcd */
    lcd_set_contrast,
    lcd_clear_display,
    lcd_puts,
    lcd_puts_scroll,
    lcd_stop_scroll,
#ifdef HAVE_LCD_CHARCELLS
    lcd_define_pattern,
    lcd_get_locked_pattern,
    lcd_unlock_pattern,
    lcd_putc,
    lcd_put_cursor,
    lcd_remove_cursor,
    PREFIX(lcd_icon),
    lcd_double_height,
#else
#ifndef SIMULATOR
    lcd_roll,
#endif
    lcd_set_drawmode,
    lcd_get_drawmode,
    lcd_setfont,
    lcd_getstringsize,
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
#endif
#if LCD_DEPTH == 16
    lcd_bitmap_transparent_part,
    lcd_bitmap_transparent,
#endif
    lcd_putsxy,
    lcd_puts_style,
    lcd_puts_scroll_style,
    &lcd_framebuffer[0][0],
    lcd_blit,
    lcd_update,
    lcd_update_rect,
    scrollbar,
    checkbox,
    font_get,
    font_getstringsize,
    font_get_width,
#endif
    backlight_on,
    backlight_off,
    backlight_set_timeout,
    gui_syncsplash,
#ifdef HAVE_REMOTE_LCD
    /* remote lcd */
    lcd_remote_set_contrast,
    lcd_remote_clear_display,
    lcd_remote_puts,
    lcd_remote_puts_scroll,
    lcd_remote_stop_scroll,
#ifndef SIMULATOR
    lcd_remote_roll,
#endif
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
    &lcd_remote_framebuffer[0][0],
    lcd_remote_update,
    lcd_remote_update_rect,

    remote_backlight_on,
    remote_backlight_off,
#endif
    /* button */
    button_get,
    button_get_w_tmo,
    button_status,
    button_clear_queue,
#if CONFIG_KEYPAD == IRIVER_H100_PAD
    button_hold,
#endif

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
    PREFIX(filesize),
    fdprintf,
    read_line,
    settings_parseline,
#ifndef SIMULATOR
    ata_sleep,
	ata_disk_is_active,
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
    timer_register,
    timer_unregister,
    timer_set_period,
#endif
    queue_init,
    queue_delete,
    queue_post,
    queue_wait_w_tmo,
    usb_acknowledge,
#ifdef RB_PROFILE
    profile_thread,
    profstop,
    profile_func_enter,
    profile_func_exit,
#endif

    /* strings and memory */
    snprintf,
    strcpy,
    strncpy,
    strlen,
    strrchr,
    strcmp,
    strncmp,
    strcasecmp,
    strncasecmp,
    memset,
    memcpy,
    memmove,
    _ctype_,
    atoi,
    strchr,
    strcat,
    memcmp,
    strcasestr,
    /* unicode stuff */
    utf8decode,
    iso_decode,
    utf16LEdecode,
    utf16BEdecode,
    utf8encode,
    utf8length,

    /* sound */
    sound_set,
    sound_min,
    sound_max,
#ifndef SIMULATOR
    mp3_play_data,
    mp3_play_pause,
    mp3_play_stop,
    mp3_is_playing,
#if CONFIG_CODEC != SWCODEC
    bitswap,
#endif
#if CONFIG_CODEC == SWCODEC
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
#if !defined(SIMULATOR) && (CONFIG_CODEC != SWCODEC)
    mpeg_get_last_header,
#endif
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
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
#endif
#endif /* !SIMULATOR && CONFIG_CODEC != SWCODEC */

    /* tag database */
    &tagdbheader,
    &tagdb_fd,
    &tagdb_initialized,
    tagdb_init,
    /* runtime database */
    &rundbheader,
    &rundb_fd,
    &rundb_initialized,            

    /* menu */
    menu_init,
    menu_exit,
    menu_show,
    menu_run,
    menu_cursor,
    menu_description,
    menu_delete,
    menu_count,
    menu_moveup,
    menu_movedown,
    menu_draw,
    menu_insert,
    menu_set_cursor,

    /* power */
    battery_level,
    battery_level_safe,
    battery_time,
#ifndef SIMULATOR
	battery_voltage,
#endif
#ifdef HAVE_CHARGING
    charger_inserted,
# ifdef HAVE_CHARGE_STATE
    charging_state,
# endif
#endif
#ifdef HAVE_USB_POWER
        usb_powered,
#endif

    /* misc */
    srand,
    rand,
    (qsort_func)qsort,
    kbd_input,
    get_time,
    set_time,
    plugin_get_buffer,
    plugin_get_audio_buffer,
    plugin_tsr,
#if defined(DEBUG) || defined(SIMULATOR)
    debugf,
#endif
#ifdef ROCKBOX_HAS_LOGF
    logf,
#endif
    &global_settings,
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
    screen_dump_set_hook,
#endif
    show_logo,

    /* new stuff at the end, sort into place next time
       the API gets incompatible */

};

int plugin_load(const char* plugin, void* parameter)
{
    int fd, rc;
    struct plugin_header *hdr;
#ifndef SIMULATOR
    ssize_t readsize;
#endif
#ifdef HAVE_LCD_BITMAP
    int xm, ym;
#endif
#ifdef HAVE_LCD_COLOR
    fb_data* old_backdrop;
#endif

    if (pfn_tsr_exit != NULL) /* if we have a resident old plugin: */
    {
        pfn_tsr_exit(); /* force it to exit now */
        pfn_tsr_exit = NULL;
        plugin_loaded = false;
    }

    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
    xm = lcd_getxmargin();
    ym = lcd_getymargin();
    lcd_setmargins(0,0);
    lcd_update();
#endif
#ifdef SIMULATOR
    hdr = sim_plugin_load((char *)plugin, &fd);
    if (!fd) {
        gui_syncsplash(HZ*2, true, str(LANG_PLUGIN_CANT_OPEN), plugin);
        return -1;
    }
    if (hdr == NULL
        || hdr->magic != PLUGIN_MAGIC
        || hdr->target_id != TARGET_ID) {
        sim_plugin_close(fd);
        gui_syncsplash(HZ*2, true,  str(LANG_PLUGIN_WRONG_MODEL));
        return -1;
    }
    if (hdr->api_version > PLUGIN_API_VERSION
        || hdr->api_version < PLUGIN_MIN_API_VERSION) {
        sim_plugin_close(fd);
        gui_syncsplash(HZ*2, true,  str(LANG_PLUGIN_WRONG_VERSION));
        return -1;
    }
#else
    fd = open(plugin, O_RDONLY);
    if (fd < 0) {
        gui_syncsplash(HZ*2, true, str(LANG_PLUGIN_CANT_OPEN), plugin);
        return fd;
    }
    /* zero out plugin buffer to ensure a properly zeroed bss area */
    memset(pluginbuf, 0, PLUGIN_BUFFER_SIZE);

    readsize = read(fd, pluginbuf, PLUGIN_BUFFER_SIZE);
    close(fd);

    if (readsize < 0) {
        gui_syncsplash(HZ*2, true, str(LANG_READ_FAILED), plugin);
        return -1;
    }
    hdr = (struct plugin_header *)pluginbuf;

    if ((unsigned)readsize <= sizeof(struct plugin_header)
        || hdr->magic != PLUGIN_MAGIC
        || hdr->target_id != TARGET_ID
        || hdr->load_addr != pluginbuf
        || hdr->end_addr > pluginbuf + PLUGIN_BUFFER_SIZE) {
        gui_syncsplash(HZ*2, true,  str(LANG_PLUGIN_WRONG_MODEL));
        return -1;
    }
    if (hdr->api_version > PLUGIN_API_VERSION
        || hdr->api_version < PLUGIN_MIN_API_VERSION) {
        gui_syncsplash(HZ*2, true,  str(LANG_PLUGIN_WRONG_VERSION));
        return -1;
    }
    plugin_size = hdr->end_addr - pluginbuf;
#endif

    plugin_loaded = true;

#ifdef HAVE_LCD_COLOR
    old_backdrop = lcd_get_backdrop();
    lcd_set_backdrop(NULL);
    lcd_update();
#endif

    invalidate_icache();

    rc = hdr->entry_point((struct plugin_api*) &rockbox_api, parameter);
    /* explicitly casting the pointer here to avoid touching every plugin. */

    button_clear_queue();
#ifdef HAVE_LCD_BITMAP
#if LCD_DEPTH > 1
    lcd_set_drawinfo(DRMODE_SOLID, LCD_DEFAULT_FG, LCD_DEFAULT_BG);
#else /* LCD_DEPTH == 1 */
    lcd_set_drawmode(DRMODE_SOLID);
#endif /* LCD_DEPTH */
    /* restore margins */
    lcd_setmargins(xm,ym);
#endif /* HAVE_LCD_BITMAP */
#ifdef HAVE_LCD_COLOR
    lcd_set_backdrop(old_backdrop);
#endif
    
    if (pfn_tsr_exit == NULL)
        plugin_loaded = false;

    sim_plugin_close(fd);

    switch (rc) {
        case PLUGIN_OK:
            break;

        case PLUGIN_USB_CONNECTED:
            return PLUGIN_USB_CONNECTED;

        default:
            gui_syncsplash(HZ*2, true, str(LANG_PLUGIN_ERROR));
            break;
    }

    return PLUGIN_OK;
}

/* Returns a pointer to the portion of the plugin buffer that is not already
   being used.  If no plugin is loaded, returns the entire plugin buffer */
void* plugin_get_buffer(int* buffer_size)
{
    int buffer_pos;

    if (plugin_loaded)
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
   Playback gets stopped, to avoid conflicts. */
void* plugin_get_audio_buffer(int* buffer_size)
{
    audio_stop();
    talk_buffer_steal(); /* we use the mp3 buffer, need to tell */
    *buffer_size = audiobufend - audiobuf;
    return audiobuf;
}

/* The plugin wants to stay resident after leaving its main function, e.g.
   runs from timer or own thread. The callback is registered to later 
   instruct it to free its resources before a new plugin gets loaded. */
void plugin_tsr(void (*exit_callback)(void))
{
    pfn_tsr_exit = exit_callback; /* remember the callback for later */
}
