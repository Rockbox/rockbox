/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by wavey@wavey.org
 * RTC config saving code (C) 2002 by hessu@hes.iki.fi
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include "inttypes.h"
#include "config.h"
#include "action.h"
#include "crc32.h"
#include "settings.h"
#include "debug.h"
#include "usb.h"
#include "backlight.h"
#include "audio.h"
#include "mpeg.h"
#include "talk.h"
#include "string.h"
#include "rtc.h"
#include "power.h"
#include "ata_idle_notify.h"
#include "atoi.h"
#include "screens.h"
#include "ctype.h"
#include "file.h"
#include "system.h"
#include "misc.h"
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "font.h"
#include "peakmeter.h"
#endif
#include "lang.h"
#include "language.h"
#include "gwps.h"
#include "powermgmt.h"
#include "sprintf.h"
#include "keyboard.h"
#include "version.h"
#include "sound.h"
#include "rbunicode.h"
#include "dircache.h"
#include "statusbar.h"
#include "splash.h"
#include "list.h"
#include "settings_list.h"
#if LCD_DEPTH > 1
#include "backdrop.h"
#endif

#if CONFIG_TUNER
#include "radio.h"
#endif

#if CONFIG_CODEC == MAS3507D
void dac_line_in(bool enable);
#endif
struct user_settings global_settings;
struct system_status global_status;

#ifdef HAVE_RECORDING
const char rec_base_directory[] = REC_BASE_DIR;
#endif
#if CONFIG_CODEC == SWCODEC
#include "pcmbuf.h"
#include "pcm_playback.h"
#include "dsp.h"
#ifdef HAVE_RECORDING
#include "enc_config.h"
#endif
#endif /* CONFIG_CODEC == SWCODEC */

#define NVRAM_BLOCK_SIZE 44

#ifdef HAVE_LCD_BITMAP
#define MAX_LINES 10
#else
#define MAX_LINES 2
#endif

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

long lasttime = 0;

/** NVRAM stuff, if the target doesnt have NVRAM it is saved in ROCKBOX_DIR /nvram.bin **/
/* NVRAM is set out as
[0] 'R'
[1] 'b'
[2] version
[3] stored variable count
[4-7] crc32 checksum
[8-NVRAM_BLOCK_SIZE] data
*/
#define NVRAM_DATA_START 8
#define NVRAM_FILE ROCKBOX_DIR "/nvram.bin"
static char nvram_buffer[NVRAM_BLOCK_SIZE];

static bool read_nvram_data(char* buf, int max_len)
{
    unsigned crc32 = 0xffffffff;
    int var_count = 0, i = 0, buf_pos = 0;
#ifndef HAVE_RTC_RAM
    int fd = open(NVRAM_FILE,O_RDONLY);
    if (fd < 0)
        return false;
    memset(buf,0,max_len);
    if (read(fd,buf,max_len) < 8) /* min is 8 bytes,magic, ver, vars, crc32 */
        return false;
    close(fd);
#else
    memset(buf,0,max_len);
    /* read rtc block */
    for (i=0; i < max_len; i++ )
        buf[i] = rtc_read(0x14+i);
#endif
    /* check magic, version */
    if ((buf[0] != 'R') || (buf[1] != 'b') 
        || (buf[2] != NVRAM_CONFIG_VERSION))
        return false;
    /* check crc32 */
    crc32 = crc_32(&buf[NVRAM_DATA_START],
                    max_len-NVRAM_DATA_START-1,0xffffffff);
    if (memcmp(&crc32,&buf[4],4))
        return false;
    /* all good, so read in the settings */
    var_count = buf[3];
    buf_pos = NVRAM_DATA_START;
    for(i=0; (i<nb_settings) && (var_count>0) && (buf_pos<max_len); i++)
    {
        int nvram_bytes = (settings[i].flags&F_NVRAM_BYTES_MASK)
                                >>F_NVRAM_MASK_SHIFT;
        if (nvram_bytes)
        {
            memcpy(settings[i].setting,&buf[buf_pos],nvram_bytes);
            buf_pos += nvram_bytes;
            var_count--;
        }
    }
    return true;
}
static bool write_nvram_data(char* buf, int max_len)
{
    unsigned crc32 = 0xffffffff;
    int i = 0, buf_pos = 0;
    char var_count = 0;
#ifndef HAVE_RTC_RAM
    int fd;
#endif
    memset(buf,0,max_len);
    /* magic, version */
    buf[0] = 'R'; buf[1] = 'b';
    buf[2] = NVRAM_CONFIG_VERSION;
    buf_pos = NVRAM_DATA_START;
    for(i=0; (i<nb_settings) && (buf_pos<max_len); i++)
    {
        int nvram_bytes = (settings[i].flags&F_NVRAM_BYTES_MASK)
                                >>F_NVRAM_MASK_SHIFT;
        if (nvram_bytes)
        {
            memcpy(&buf[buf_pos],settings[i].setting,nvram_bytes);
            buf_pos += nvram_bytes;
            var_count++;
        }
    }
    /* count and crc32 */
    buf[3] = var_count;
    crc32 = crc_32(&buf[NVRAM_DATA_START],
                    max_len-NVRAM_DATA_START-1,0xffffffff);
    memcpy(&buf[4],&crc32,4);
#ifndef HAVE_RTC_RAM
    fd = open(NVRAM_FILE,O_CREAT|O_TRUNC|O_WRONLY);
    if (fd >= 0)
    {
        int len = write(fd,buf,max_len);
        close(fd);
        if (len < 8)
            return false;
    }
#else
    /* FIXME: okay, it _would_ be cleaner and faster to implement rtc_write so
       that it would write a number of bytes at a time since the RTC chip
       supports that, but this will have to do for now 8-) */
    for (i=0; i < NVRAM_BLOCK_SIZE; i++ ) {
        int r = rtc_write(0x14+i, buf[i]);
        if (r) {
            DEBUGF( "save_config_buffer: rtc_write failed at addr 0x%02x: %d\n",
                    14+i, r );
            return false;
        }
    }
#endif
    return true;
}

/** Reading from a config file **/
/*
 * load settings from disk or RTC RAM
 */
void settings_load(int which)
{
    DEBUGF( "reload_all_settings()\n" );
    if (which&SETTINGS_RTC)
        read_nvram_data(nvram_buffer,NVRAM_BLOCK_SIZE);
    if (which&SETTINGS_HD)
    {
        settings_load_config(CONFIGFILE,false);
        settings_load_config(FIXEDSETTINGSFILE,false);
    }
}
#ifdef HAVE_LCD_COLOR
/*
 * Helper function to convert a string of 6 hex digits to a native colour
 */

#define hex2dec(c) (((c) >= '0' && ((c) <= '9')) ? (toupper(c)) - '0' : \
                                                   (toupper(c)) - 'A' + 10)

static int hex_to_rgb(const char* hex)
{   int ok = 1;
    int i;
    int red, green, blue;

    if (strlen(hex) == 6) {
        for (i=0; i < 6; i++ ) {
           if (!isxdigit(hex[i])) {
              ok=0;
              break;
           }
        }

        if (ok) {
            red = (hex2dec(hex[0]) << 4) | hex2dec(hex[1]);
            green = (hex2dec(hex[2]) << 4) | hex2dec(hex[3]);
            blue = (hex2dec(hex[4]) << 4) | hex2dec(hex[5]);
            return LCD_RGBPACK(red,green,blue);
        }
    }

    return 0;
}
#endif /* HAVE_LCD_COLOR */

static bool cfg_string_to_int(int setting_id, int* out, char* str)
{
    const char* start = settings[setting_id].cfg_vals;
    char* end = NULL;
    char temp[MAX_PATH];
    int count = 0;
    while (1)
    {
        end = strchr(start, ',');
        if (!end)
        {
            if (!strcmp(str, start))
            {
                *out = count;
                return true;
            }
            else return false;
        }
        strncpy(temp, start, end-start);
        temp[end-start] = '\0';
        if (!strcmp(str, temp))
        {
            *out = count;
            return true;
        }
        start = end +1;
        count++;
    }
    return false;
}

bool settings_load_config(const char* file, bool apply)
{
    int fd;
    char line[128];
    char* name;
    char* value;
    int i;
    fd = open(file, O_RDONLY);
    if (fd < 0)
        return false;

    while (read_line(fd, line, sizeof line) > 0)
    {
        if (!settings_parseline(line, &name, &value))
            continue;
        for(i=0; i<nb_settings; i++)
        {
            if (settings[i].cfg_name == NULL)
                continue;
            if (!strcasecmp(name,settings[i].cfg_name))
            {
                switch (settings[i].flags&F_T_MASK)
                {
                    case F_T_INT:
                    case F_T_UINT:
#ifdef HAVE_LCD_COLOR
                        if (settings[i].flags&F_RGB)
                            *(int*)settings[i].setting = hex_to_rgb(value);
                        else 
#endif 
                            if (settings[i].cfg_vals == NULL)
                        {
                            *(int*)settings[i].setting = atoi(value);
                        }
                        else
                        {
                            cfg_string_to_int(i,(int*)settings[i].setting,value);
                        }
                        break;
                    case F_T_BOOL:
                    {
                        int temp;
                        if (cfg_string_to_int(i,&temp,value))
                            *(bool*)settings[i].setting = (temp==0?false:true);
                        break;
                    }
                    case F_T_CHARPTR:
                    case F_T_UCHARPTR:
                    {
                        char storage[MAX_PATH];
                        if (settings[i].filename_setting->prefix)
                        {
                            int len = strlen(settings[i].filename_setting->prefix);
                            if (!strncasecmp(value,
                                             settings[i].filename_setting->prefix,
                                             len))
                            {
                                strncpy(storage,&value[len],MAX_PATH);
                            }
                            else strncpy(storage,value,MAX_PATH);
                        }
                        else strncpy(storage,value,MAX_PATH);
                        if (settings[i].filename_setting->suffix)
                        {
                            char *s = strcasestr(storage,settings[i].filename_setting->suffix);
                            if (s) *s = '\0';
                        }
                        strncpy((char*)settings[i].setting,storage,
                                settings[i].filename_setting->max_len);
                        ((char*)settings[i].setting)
                            [settings[i].filename_setting->max_len-1] = '\0';
                        break;
                    }
                }
                break;
            } /* if (!strcmp(name,settings[i].cfg_name)) */
        } /* for(...) */
    } /* while(...) */

    close(fd);
    settings_save();
    if (apply)
        settings_apply();
    return true;
}

/** Writing to a config file and saving settings **/

bool cfg_int_to_string(int setting_id, int val, char* buf, int buf_len)
{
    const char* start = settings[setting_id].cfg_vals;
    char* end = NULL;
    int count = 0;
    while (count < val)
    {
        start = strchr(start,',');
        if (!start)
            return false;
        count++;
        start++;
    }
    end = strchr(start,',');
    if (end == NULL)
        strncpy(buf, start, buf_len);
    else 
    {
        int len = (buf_len > (end-start))? end-start: buf_len;
        strncpy(buf, start, len);
        buf[len] = '\0';
    }
    return true;
}
static bool is_changed(int setting_id)
{
    const struct settings_list *setting = &settings[setting_id];
    switch (setting->flags&F_T_MASK)
    {
    case F_T_INT:
    case F_T_UINT:
        if (setting->flags&F_DEF_ISFUNC)
        {
            if (*(int*)setting->setting == setting->default_val.func())
                return false;
        }
        else if (setting->flags&F_T_SOUND)
        {
            if (*(int*)setting->setting ==
                sound_default(setting->sound_setting->setting))
                return false;
        }
        else if (*(int*)setting->setting == setting->default_val.int_)
            return false;
        break;
    case F_T_BOOL:
        if (*(bool*)setting->setting == setting->default_val.bool_)
            return false;
        break;
    case F_T_CHARPTR:
    case F_T_UCHARPTR:
        if (!strcmp((char*)setting->setting, setting->default_val.charptr))
            return false;
        break;
    }
    return true;
}

static bool settings_write_config(char* filename, int options)
{
    int i;
    int fd;
    char value[MAX_PATH];
    fd = open(filename,O_CREAT|O_TRUNC|O_WRONLY);
    if (fd < 0)
        return false;
    fdprintf(fd, "# .cfg file created by rockbox %s - "
                 "http://www.rockbox.org\r\n\r\n", appsversion);
    for(i=0; i<nb_settings; i++)
    {
        if (settings[i].cfg_name == NULL)
            continue;
        value[0] = '\0';
        
        if ((options == SETTINGS_SAVE_CHANGED) &&
            !is_changed(i))
            continue;
        else if ((options == SETTINGS_SAVE_THEME) &&
                 ((settings[i].flags&F_THEMESETTING) == 0))
            continue;
        
        switch (settings[i].flags&F_T_MASK)
        {
            case F_T_INT:
            case F_T_UINT:
#ifdef HAVE_LCD_COLOR
                if (settings[i].flags&F_RGB)
                {
                    int colour = *(int*)settings[i].setting;
                    snprintf(value,MAX_PATH,"%02x%02x%02x",
                                (int)RGB_UNPACK_RED(colour),
                                (int)RGB_UNPACK_GREEN(colour),
                                (int)RGB_UNPACK_BLUE(colour));
                }
                else 
#endif
                if (settings[i].cfg_vals == NULL)
                {
                    snprintf(value,MAX_PATH,"%d",*(int*)settings[i].setting);
                }
                else
                {
                    cfg_int_to_string(i, *(int*)settings[i].setting,
                                        value, MAX_PATH);
                }
                break;
            case F_T_BOOL:
                cfg_int_to_string(i,
                        *(bool*)settings[i].setting==false?0:1, value, MAX_PATH);
                break;
            case F_T_CHARPTR:
            case F_T_UCHARPTR:
                if (((char*)settings[i].setting)[0] == '\0')
                    break;
                if (settings[i].filename_setting->prefix)
                {
                    snprintf(value,MAX_PATH,"%s%s%s",
                        settings[i].filename_setting->prefix,
                        (char*)settings[i].setting,
                        settings[i].filename_setting->suffix);
                }
                else strncpy(value,(char*)settings[i].setting,
                                settings[i].filename_setting->max_len);
                break;
        } /* switch () */
        if (value[0])
            fdprintf(fd,"%s: %s\r\n",settings[i].cfg_name,value);
    } /* for(...) */
    close(fd);
    return true;
}
#ifndef HAVE_RTC_RAM
static bool flush_global_status_callback(void)
{
    return write_nvram_data(nvram_buffer,NVRAM_BLOCK_SIZE);
}
#endif
static bool flush_config_block_callback(void)
{
    bool r1, r2;
    r1 = write_nvram_data(nvram_buffer,NVRAM_BLOCK_SIZE);
    r2 = settings_write_config(CONFIGFILE, SETTINGS_SAVE_CHANGED);
    return r1 || r2;
}

/*
 * persist all runtime user settings to disk or RTC RAM
 */
static void update_runtime(void)
{
    int elapsed_secs;

    elapsed_secs = (current_tick - lasttime) / HZ;
    global_status.runtime += elapsed_secs;
    lasttime += (elapsed_secs * HZ);

    if ( global_status.runtime > global_status.topruntime )
        global_status.topruntime = global_status.runtime;
}

void status_save( void )
{
    update_runtime();
#ifdef HAVE_RTC_RAM
    /* this will be done in the ata_callback if
       target doesnt have rtc ram */
    write_nvram_data(nvram_buffer,NVRAM_BLOCK_SIZE);
#else
    register_ata_idle_func(flush_global_status_callback);
#endif
}

int settings_save( void )
{
    update_runtime();
#ifdef HAVE_RTC_RAM
    /* this will be done in the ata_callback if
       target doesnt have rtc ram */
    write_nvram_data(nvram_buffer,NVRAM_BLOCK_SIZE);
#endif
    if(!register_ata_idle_func(flush_config_block_callback))
    {
        int i;
        FOR_NB_SCREENS(i)
        {
            screens[i].clear_display();
#ifdef HAVE_LCD_CHARCELLS
            screens[i].puts(0, 0, str(LANG_SETTINGS_SAVE_PLAYER));
            screens[i].puts(0, 1, str(LANG_SETTINGS_BATTERY_PLAYER));
#else
            screens[i].puts(4, 2, str(LANG_SETTINGS_SAVE_RECORDER));
            screens[i].puts(2, 4, str(LANG_SETTINGS_BATTERY_RECORDER));
            screens[i].update();
#endif
        }
        sleep(HZ*2);
        return -1;
    }
    return 0;
}
bool settings_save_config(int options)
{
    char filename[MAX_PATH];

    create_numbered_filename(filename, ROCKBOX_DIR, "config", ".cfg", 2
                             IF_CNFN_NUM_(, NULL));

    /* allow user to modify filename */
    while (true) {
        if (!kbd_input(filename, sizeof filename)) {
            break;
        }
        else {
            gui_syncsplash(HZ, str(LANG_MENU_SETTING_CANCEL));
            return false;
        }
    }

    if (settings_write_config(filename, options))
        gui_syncsplash(HZ, str(LANG_SETTINGS_SAVED));
    else
        gui_syncsplash(HZ, str(LANG_FAILED));
    return true;
}

/** Apply and Reset settings **/


#ifdef HAVE_LCD_BITMAP
/*
 * Applies the range infos stored in global_settings to
 * the peak meter.
 */
void settings_apply_pm_range(void)
{
    int pm_min, pm_max;

    /* depending on the scale mode (dBfs or percent) the values
       of global_settings.peak_meter_dbfs have different meanings */
    if (global_settings.peak_meter_dbfs)
    {
        /* convert to dBfs * 100          */
        pm_min = -(((int)global_settings.peak_meter_min) * 100);
        pm_max = -(((int)global_settings.peak_meter_max) * 100);
    }
    else
    {
        /* percent is stored directly -> no conversion */
        pm_min = global_settings.peak_meter_min;
        pm_max = global_settings.peak_meter_max;
    }

    /* apply the range */
    peak_meter_init_range(global_settings.peak_meter_dbfs, pm_min, pm_max);
}
#endif /* HAVE_LCD_BITMAP */

void sound_settings_apply(void)
{
#if CONFIG_CODEC == SWCODEC
    sound_set_dsp_callback(dsp_callback);
#endif
    sound_set(SOUND_BASS, global_settings.bass);
    sound_set(SOUND_TREBLE, global_settings.treble);
    sound_set(SOUND_BALANCE, global_settings.balance);
    sound_set(SOUND_VOLUME, global_settings.volume);
    sound_set(SOUND_CHANNELS, global_settings.channel_config);
    sound_set(SOUND_STEREO_WIDTH, global_settings.stereo_width);
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    sound_set(SOUND_LOUDNESS, global_settings.loudness);
    sound_set(SOUND_AVC, global_settings.avc);
    sound_set(SOUND_MDB_STRENGTH, global_settings.mdb_strength);
    sound_set(SOUND_MDB_HARMONICS, global_settings.mdb_harmonics);
    sound_set(SOUND_MDB_CENTER, global_settings.mdb_center);
    sound_set(SOUND_MDB_SHAPE, global_settings.mdb_shape);
    sound_set(SOUND_MDB_ENABLE, global_settings.mdb_enable);
    sound_set(SOUND_SUPERBASS, global_settings.superbass);
#endif

#ifdef HAVE_USB_POWER
#if CONFIG_CHARGING
    usb_charging_enable(global_settings.usb_charging);
#endif
#endif
}

void settings_apply(void)
{
    char buf[64];
#if CONFIG_CODEC == SWCODEC
    int i;
#endif

    DEBUGF( "settings_apply()\n" );
    sound_settings_apply();

    audio_set_buffer_margin(global_settings.buffer_margin);

#ifdef HAVE_LCD_CONTRAST
    lcd_set_contrast(global_settings.contrast);
#endif
    lcd_scroll_speed(global_settings.scroll_speed);
#ifdef HAVE_REMOTE_LCD
    lcd_remote_set_contrast(global_settings.remote_contrast);
    lcd_remote_set_invert_display(global_settings.remote_invert);
    lcd_remote_set_flip(global_settings.remote_flip_display);
    lcd_remote_scroll_speed(global_settings.remote_scroll_speed);
    lcd_remote_scroll_step(global_settings.remote_scroll_step);
    lcd_remote_scroll_delay(global_settings.remote_scroll_delay);
    lcd_remote_bidir_scroll(global_settings.remote_bidir_limit);
#ifdef HAVE_REMOTE_LCD_TICKING
    lcd_remote_emireduce(global_settings.remote_reduce_ticking);
#endif
    remote_backlight_set_timeout(global_settings.remote_backlight_timeout);
#if CONFIG_CHARGING
    remote_backlight_set_timeout_plugged(global_settings.remote_backlight_timeout_plugged);
#endif
#ifdef HAS_REMOTE_BUTTON_HOLD
    remote_backlight_set_on_button_hold(global_settings.remote_backlight_on_button_hold);
#endif
#endif /* HAVE_REMOTE_LCD */
#if CONFIG_BACKLIGHT
    backlight_set_timeout(global_settings.backlight_timeout);
#if CONFIG_CHARGING
    backlight_set_timeout_plugged(global_settings.backlight_timeout_plugged);
#endif
#if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
    backlight_set_fade_in(global_settings.backlight_fade_in);
    backlight_set_fade_out(global_settings.backlight_fade_out);
#endif
#endif
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    backlight_set_brightness(global_settings.brightness);
#endif
    ata_spindown(global_settings.disk_spindown);
#if (CONFIG_CODEC == MAS3507D) && !defined(SIMULATOR)
    dac_line_in(global_settings.line_in);
#endif
    mpeg_id3_options(global_settings.id3_v1_first);

    set_poweroff_timeout(global_settings.poweroff);

    set_battery_capacity(global_settings.battery_capacity);
#if BATTERY_TYPES_COUNT > 1
    set_battery_type(global_settings.battery_type);
#endif

#ifdef HAVE_LCD_BITMAP
    lcd_set_invert_display(global_settings.invert);
    lcd_set_flip(global_settings.flip_display);
    button_set_flip(global_settings.flip_display);
    lcd_update(); /* refresh after flipping the screen */
    settings_apply_pm_range();
    peak_meter_init_times(
        global_settings.peak_meter_release, global_settings.peak_meter_hold,
        global_settings.peak_meter_clip_hold);
#endif

#if LCD_DEPTH > 1
    unload_wps_backdrop();
#endif
    if ( global_settings.wps_file[0] &&
         global_settings.wps_file[0] != 0xff ) {
        snprintf(buf, sizeof buf, WPS_DIR "/%s.wps",
                 global_settings.wps_file);
        wps_data_load(gui_wps[0].data, buf, true);
    }
    else
    {
        wps_data_init(gui_wps[0].data);
    }

#if LCD_DEPTH > 1
    if ( global_settings.backdrop_file[0] &&
         global_settings.backdrop_file[0] != 0xff ) {
        snprintf(buf, sizeof buf, BACKDROP_DIR "/%s.bmp",
                 global_settings.backdrop_file);
        load_main_backdrop(buf);
    } else {
        unload_main_backdrop();
    }
    show_main_backdrop();
#endif

#ifdef HAVE_LCD_COLOR
    screens[SCREEN_MAIN].set_foreground(global_settings.fg_color);
    screens[SCREEN_MAIN].set_background(global_settings.bg_color);
#endif

#if defined(HAVE_REMOTE_LCD) && (NB_SCREENS > 1)
    if ( global_settings.rwps_file[0]) {
        snprintf(buf, sizeof buf, WPS_DIR "/%s.rwps",
                 global_settings.rwps_file);
        wps_data_load(gui_wps[1].data, buf, true);
    }
    else
        wps_data_init(gui_wps[1].data);
#endif

#ifdef HAVE_LCD_BITMAP
    if ( global_settings.font_file[0]) {
        snprintf(buf, sizeof buf, FONT_DIR "/%s.fnt",
                 global_settings.font_file);
        font_load(buf);
    }
    else
        font_reset();

    if ( global_settings.kbd_file[0]) {
        snprintf(buf, sizeof buf, ROCKBOX_DIR "/%s.kbd",
                 global_settings.kbd_file);
        load_kbd(buf);
    }
    else
        load_kbd(NULL);

    lcd_scroll_step(global_settings.scroll_step);
    gui_list_screen_scroll_step(global_settings.screen_scroll_step);
    gui_list_screen_scroll_out_of_view(global_settings.offset_out_of_view);
#else
    lcd_jump_scroll(global_settings.jump_scroll);
    lcd_jump_scroll_delay(global_settings.jump_scroll_delay);
#endif
    lcd_bidir_scroll(global_settings.bidir_limit);
    lcd_scroll_delay(global_settings.scroll_delay);

    if ( global_settings.lang_file[0]) {
        snprintf(buf, sizeof buf, LANG_DIR "/%s.lng",
                 global_settings.lang_file);
        lang_load(buf);
        talk_init(); /* use voice of same language */
    }

    set_codepage(global_settings.default_codepage);

#if CONFIG_CODEC == SWCODEC
    audio_set_crossfade(global_settings.crossfade);
    dsp_set_replaygain();
    dsp_set_crossfeed(global_settings.crossfeed);
    dsp_set_crossfeed_direct_gain(global_settings.crossfeed_direct_gain);
    dsp_set_crossfeed_cross_params(global_settings.crossfeed_cross_gain,
                                   global_settings.crossfeed_hf_attenuation,
                                   global_settings.crossfeed_hf_cutoff);

    /* Configure software equalizer, hardware eq is handled in audio_init() */
    dsp_set_eq(global_settings.eq_enabled);
    dsp_set_eq_precut(global_settings.eq_precut);
    for(i = 0; i < 5; i++) {
        dsp_set_eq_coefs(i);
    }

    dsp_dither_enable(global_settings.dithering_enabled);
#endif

#ifdef HAVE_SPDIF_POWER
    spdif_power_enable(global_settings.spdif_enable);
#endif

#if CONFIG_BACKLIGHT
    set_backlight_filter_keypress(global_settings.bl_filter_first_keypress);
#ifdef HAVE_REMOTE_LCD
    set_remote_backlight_filter_keypress(global_settings.remote_bl_filter_first_keypress);
#endif
#ifdef HAS_BUTTON_HOLD
    backlight_set_on_button_hold(global_settings.backlight_on_button_hold);
#endif
#ifdef HAVE_LCD_SLEEP
    lcd_set_sleep_after_backlight_off(global_settings.lcd_sleep_after_backlight_off);
#endif
#endif /* CONFIG_BACKLIGHT */

    /* This should stay last */
#if defined(HAVE_RECORDING) && CONFIG_CODEC == SWCODEC
    enc_global_settings_apply();
#endif
}




/*
 * reset all settings to their default value
 */
void settings_reset(void) {

    int i;
    DEBUGF( "settings_reset()\n" );

    for(i=0; i<nb_settings; i++)
    {
        switch (settings[i].flags&F_T_MASK)
        {
            case F_T_INT:
            case F_T_UINT:
                if (settings[i].flags&F_DEF_ISFUNC)
                    *(int*)settings[i].setting = settings[i].default_val.func();
                else if (settings[i].flags&F_T_SOUND)
                    *(int*)settings[i].setting = 
                            sound_default(settings[i].sound_setting->setting);
                else *(int*)settings[i].setting = settings[i].default_val.int_;
                break;
            case F_T_BOOL:
                *(bool*)settings[i].setting = settings[i].default_val.bool_;
                break;
            case F_T_CHARPTR:
            case F_T_UCHARPTR:
                strncpy((char*)settings[i].setting,
                        settings[i].default_val.charptr,MAX_FILENAME);
                break;
        }
    } /* for(...) */
#if defined (HAVE_RECORDING) && CONFIG_CODEC == SWCODEC
    enc_global_settings_reset();
#endif
}

/** Changing setting values **/
const struct settings_list* find_setting(void* variable, int *id)
{
    int i;
    for(i=0;i<nb_settings;i++)
    {
        if (settings[i].setting == variable)
        {
            if (id)
                *id = i;
            return &settings[i];
        }
    }
    return NULL;
}

void talk_setting(void *global_settings_variable)
{
    const struct settings_list *setting;
    if (global_settings.talk_menu == 0)
        return;
    setting = find_setting(global_settings_variable, NULL);
    if (setting == NULL)
        return;
    if (setting->lang_id)
        talk_id(setting->lang_id,false);
}

static int selected_setting; /* Used by the callback */

static void dec_sound_formatter(char *buffer, int buffer_size, 
        int val, const char *unit)
{
    val = sound_val2phys(selected_setting, val);
    char sign = ' ';
    if(val < 0)
    {
        sign = '-';
        val = abs(val);
    }
    int integer = val / 10;
    int dec = val % 10;
    snprintf(buffer, buffer_size, "%c%d.%d %s", sign, integer, dec, unit);
}

bool set_sound(const unsigned char * string,
               int* variable,
               int setting)
{
    int talkunit = UNIT_INT;
    const char* unit = sound_unit(setting);
    int numdec = sound_numdecimals(setting);
    int steps = sound_steps(setting);
    int min = sound_min(setting);
    int max = sound_max(setting);
    sound_set_type* sound_callback = sound_get_fn(setting);
    if (*unit == 'd') /* crude reconstruction */
        talkunit = UNIT_DB;
    else if (*unit == '%')
        talkunit = UNIT_PERCENT;
    else if (*unit == 'H')
        talkunit = UNIT_HERTZ;
    if (!numdec)
        return set_int(string, unit, talkunit,  variable, sound_callback,
                       steps, min, max, NULL );
    else
    {/* Decimal number */
        selected_setting=setting;
        return set_int(string, unit, talkunit,  variable, sound_callback,
                       steps, min, max, &dec_sound_formatter );
    }
}

bool set_bool(const char* string, bool* variable )
{
    return set_bool_options(string, variable,
                            (char *)STR(LANG_SET_BOOL_YES),
                            (char *)STR(LANG_SET_BOOL_NO),
                            NULL);
}

/* wrapper to convert from int param to bool param in set_option */
static void (*boolfunction)(bool);
static void bool_funcwrapper(int value)
{
    if (value)
        boolfunction(true);
    else
        boolfunction(false);
}

bool set_bool_options(const char* string, bool* variable,
                      const char* yes_str, int yes_voice,
                      const char* no_str, int no_voice,
                      void (*function)(bool))
{
    struct opt_items names[] = {
        {(unsigned char *)no_str, no_voice},
        {(unsigned char *)yes_str, yes_voice}
    };
    bool result;

    boolfunction = function;
    result = set_option(string, variable, BOOL, names, 2,
                        function ? bool_funcwrapper : NULL);
    return result;
}

static void talk_unit(int unit, int value, long (*get_talk_id)(int value))
{
    if (global_settings.talk_menu)
    {
        if (get_talk_id)
        {
            talk_id(get_talk_id(value),false);
        }
        else if (unit < UNIT_LAST)
        {   /* use the available unit definition */
            talk_value(value, unit, false);
        }
        else
        {   /* say the number, followed by an arbitrary voice ID */
            talk_number(value, false);
            talk_id(unit, true);
        }
    }
}

struct value_setting_data {
    enum optiontype type;
    /* used for "value" settings.. */
    int max;
    int step;
    int voice_unit;
    const char * unit;
    void (*formatter)(char* dest, int dest_length,
                          int value, const char* unit);
    long (*get_talk_id)(int value);
    /* used for BOOL and "choice" settings */
    struct opt_items* options;
};

static char * value_setting_get_name_cb(int selected_item,void * data, char *buffer)
{
    struct value_setting_data* cb_data =
            (struct value_setting_data*)data;
    if (cb_data->type == INT && !cb_data->options)
    {
        int item = cb_data->max -(selected_item*cb_data->step);
        if (cb_data->formatter)
            cb_data->formatter(buffer, MAX_PATH,item,cb_data->unit);
        else
            snprintf(buffer, MAX_PATH,"%d %s",item,cb_data->unit);
    }
    else strcpy(buffer,P2STR(cb_data->options[selected_item].string));
    return buffer;
}
#define type_fromvoidptr(type, value) \
    (type == INT)? \
        (int)(*(int*)(value)) \
    : \
        (bool)(*(bool*)(value))
static bool do_set_setting(const unsigned char* string, void *variable,
                           int nb_items,int selected,
                           struct value_setting_data *cb_data,
                           void (*function)(int))
{
    int action;
    bool done = false;
    struct gui_synclist lists;
    int oldvalue;
    bool allow_wrap = true;

    if (cb_data->type == INT)
    {
         oldvalue = *(int*)variable;
         if (variable == &global_settings.volume)
             allow_wrap = false;
    }
    else oldvalue = *(bool*)variable;

    gui_synclist_init(&lists,value_setting_get_name_cb,(void*)cb_data,false,1);
    gui_synclist_set_title(&lists, (char*)string,
#ifdef HAVE_LCD_BITMAP
        bitmap_icons_6x8[Icon_Questionmark]
#else
        NOICON
#endif
    );
    gui_synclist_set_icon_callback(&lists,NULL);
    gui_synclist_set_nb_items(&lists,nb_items);
    gui_synclist_limit_scroll(&lists,true);
    gui_synclist_select_item(&lists, selected);

    if (global_settings.talk_menu)
    {
        if (cb_data->type == INT && !cb_data->options)
            talk_unit(cb_data->voice_unit, *(int*)variable, cb_data->get_talk_id);
        else talk_id(cb_data->options[selected].voice_id, false);
    }

    gui_synclist_draw(&lists);
    action_signalscreenchange();
    while (!done)
    {

        action = get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
        if (action == ACTION_NONE)
            continue;
        if (gui_synclist_do_button(&lists,action,
                                   allow_wrap?LIST_WRAP_UNLESS_HELD:LIST_WRAP_OFF))
        {
            if (global_settings.talk_menu)
            {
                int value;
                if (cb_data->type == INT && !cb_data->options)
                {
                    value = cb_data->max -
                            gui_synclist_get_sel_pos(&lists)*cb_data->step;
                    talk_unit(cb_data->voice_unit, value, cb_data->get_talk_id);
                }
                else
                {
                    value = gui_synclist_get_sel_pos(&lists);
                    talk_id(cb_data->options[value].voice_id, false);
                }
            }
            if (cb_data->type == INT && !cb_data->options)
                *(int*)variable = cb_data->max -
                        gui_synclist_get_sel_pos(&lists)*cb_data->step;
            else if (cb_data->type == BOOL)
                *(bool*)variable = gui_synclist_get_sel_pos(&lists) ? true : false;
            else *(int*)variable = gui_synclist_get_sel_pos(&lists);
        }
        else if (action == ACTION_STD_CANCEL)
        {
            if (cb_data->type == INT)
            {
                if (*(int*)variable != oldvalue)
                {
                    gui_syncsplash(HZ/2, str(LANG_MENU_SETTING_CANCEL));
                    *(int*)variable = oldvalue;
                }
            }
            else
            {
                if (*(bool*)variable != (bool)oldvalue)
                {
                    gui_syncsplash(HZ/2, str(LANG_MENU_SETTING_CANCEL));
                    *(bool*)variable = (bool)oldvalue;
                }
            }
            done = true;
        }
        else if (action == ACTION_STD_OK)
        {
            done = true;
        }
        else if(default_event_handler(action) == SYS_USB_CONNECTED)
            return true;
        gui_syncstatusbar_draw(&statusbars, false);
        if ( function )
            function(type_fromvoidptr(cb_data->type,variable));
    }
    if (cb_data->type == INT)
    {
        if (oldvalue != *(int*)variable)
            settings_save();
    }
    else if (oldvalue != *(bool*)variable)
         settings_save();

    action_signalscreenchange();
    return false;
}
static const char *unit_strings[] = 
{   
    [UNIT_INT]
        = "",
    [UNIT_MS]
        = "ms",
    [UNIT_SEC]
        = "s", 
    [UNIT_MIN]
        = "min", 
    [UNIT_HOUR]
        = "hr", 
    [UNIT_KHZ]
        = "KHz", 
    [UNIT_DB]
        = "dB", 
    [UNIT_PERCENT]
        = "%",
    [UNIT_MAH]
        = "mAh",
    [UNIT_PIXEL]
        = "px",
    [UNIT_PER_SEC]
        = "per sec",
    [UNIT_HERTZ]
        = "Hz",
    [UNIT_MB]
        = "MB",
    [UNIT_KBIT]
        = "kb/s",
};
bool set_int_ex(const unsigned char* string,
             const char* unit,
             int voice_unit,
             int* variable,
             void (*function)(int),
             int step,
             int min,
             int max,
             void (*formatter)(char*, int, int, const char*),
             long (*get_talk_id)(int))
{
    int count = (max-min)/step + 1;
#if CONFIG_KEYPAD != PLAYER_PAD
    struct value_setting_data data = {
        INT,max, step, voice_unit,unit,formatter,get_talk_id,NULL };
    if (voice_unit < UNIT_LAST)
        data.unit = unit_strings[voice_unit];
    else 
        data.unit = str(voice_unit);
    return do_set_setting(string,variable,count,
                          (max-*variable)/step, &data,function);
#else
    struct value_setting_data data = {
        INT,min, -step, voice_unit,unit,formatter,get_talk_id,NULL };
    if (voice_unit < UNIT_LAST)
        data.unit = unit_strings[voice_unit];
    else 
        data.unit = str(voice_unit);
    return do_set_setting(string,variable,count,
                          (*variable-min)/step, &data,function);
#endif
}
bool set_int(const unsigned char* string,
             const char* unit,
             int voice_unit,
             int* variable,
             void (*function)(int),
             int step,
             int min,
             int max,
             void (*formatter)(char*, int, int, const char*) )
{
    return set_int_ex(string, unit, voice_unit, variable, function,
                      step, min, max, formatter, NULL);
}
/* NOTE: the 'type' parameter specifies the actual type of the variable
   that 'variable' points to. not the value within. Only variables with
   type 'bool' should use parameter BOOL.

   The type separation is necessary since int and bool are fundamentally
   different and bit-incompatible types and can not share the same access
   code. */
bool set_option(const char* string, void* variable, enum optiontype type,
                const struct opt_items* options, int numoptions, void (*function)(int))
{
    struct value_setting_data data = {
        type,0, 0, 0,NULL,NULL,NULL,(struct opt_items*)options };
    int selected;
    if (type == BOOL)
        selected = *(bool*)variable ? 1 : 0;
    else selected = *(int*)variable;
    return do_set_setting(string,variable,numoptions,
                          selected, &data,function);
}

/** extra stuff which is probably misplaced **/

void set_file(char* filename, char* setting, int maxlen)
{
    char* fptr = strrchr(filename,'/');
    int len;
    int extlen = 0;
    char* ptr;

    if (!fptr)
        return;

    *fptr = 0;
    fptr++;

    len = strlen(fptr);
    ptr = fptr + len;
    while ((*ptr != '.') && (ptr != fptr)) {
        extlen++;
        ptr--;
    }
    if(ptr == fptr) extlen = 0;

    if (strncasecmp(ROCKBOX_DIR, filename ,strlen(ROCKBOX_DIR)) ||
        (len-extlen > maxlen))
        return;

    strncpy(setting, fptr, len-extlen);
    setting[len-extlen]=0;

    settings_save();
}

#ifdef HAVE_RECORDING
/* This array holds the record timer interval lengths, in seconds */
static const unsigned long rec_timer_seconds[] =
{
    0,        /* 0 means OFF */
    5*60,     /* 00:05 */
    10*60,    /* 00:10 */
    15*60,    /* 00:15 */
    30*60,    /* 00:30 */
    60*60,    /* 01:00 */
    74*60,    /* 74:00 */
    80*60,    /* 80:00 */
    2*60*60,  /* 02:00 */
    4*60*60,  /* 04:00 */
    6*60*60,  /* 06:00 */
    8*60*60,  /* 08:00 */
    10L*60*60, /* 10:00 */
    12L*60*60, /* 12:00 */
    18L*60*60, /* 18:00 */
    24L*60*60  /* 24:00 */
};

unsigned int rec_timesplit_seconds(void)
{
    return rec_timer_seconds[global_settings.rec_timesplit];
}

/* This array holds the record size interval lengths, in bytes */
static const unsigned long rec_size_bytes[] =
{
    0,               /* 0 means OFF */
    5*1024*1024,     /* 5MB */
    10*1024*1024,    /* 10MB */
    15*1024*1024,    /* 15MB */
    32*1024*1024,    /* 32MB */
    64*1024*1024,    /* 64MB */
    75*1024*1024,    /* 75MB */
    100*1024*1024,   /* 100MB */
    128*1024*1024,   /* 128MB */
    256*1024*1024,   /* 256MB */
    512*1024*1024,   /* 512MB */
    650*1024*1024,   /* 650MB */
    700*1024*1024,   /* 700MB */
    1024*1024*1024,  /* 1GB */
    1536*1024*1024,  /* 1.5GB */
    1792*1024*1024,  /* 1.75GB  */
};

unsigned long rec_sizesplit_bytes(void)
{
    return rec_size_bytes[global_settings.rec_sizesplit];
}
/*
 * Time strings used for the trigger durations.
 * Keep synchronous to trigger_times in settings_apply_trigger
 */
const char * const trig_durations[TRIG_DURATION_COUNT] =
{
    "0s", "1s", "2s", "5s",
    "10s", "15s", "20s", "25s", "30s",
    "1min", "2min", "5min", "10min"
};

void settings_apply_trigger(void)
{
    /* Keep synchronous to trig_durations and trig_durations_conf*/
    static const long trigger_times[TRIG_DURATION_COUNT] = {
        0, HZ, 2*HZ, 5*HZ,
        10*HZ, 15*HZ, 20*HZ, 25*HZ, 30*HZ,
        60*HZ, 2*60*HZ, 5*60*HZ, 10*60*HZ
    };

    peak_meter_define_trigger(
        global_settings.rec_start_thres,
        trigger_times[global_settings.rec_start_duration],
        MIN(trigger_times[global_settings.rec_start_duration] / 2, 2*HZ),
        global_settings.rec_stop_thres,
        trigger_times[global_settings.rec_stop_postrec],
        trigger_times[global_settings.rec_stop_gap]
    );
}
#endif
