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
#include "filetypes.h"

#if (LCD_DEPTH > 1) || (defined(HAVE_LCD_REMOTE) && (LCD_REMOTE_DEPTH > 1))
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

#if CONFIG_CODEC == SWCODEC
#include "pcmbuf.h"
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
    for(i=0; i<nb_settings; i++)
    {
        int nvram_bytes = (settings[i].flags&F_NVRAM_BYTES_MASK)
                                >>F_NVRAM_MASK_SHIFT;
        if (nvram_bytes)
        {
            if ((var_count>0) && (buf_pos<max_len))
            {
                memcpy(settings[i].setting,&buf[buf_pos],nvram_bytes);
                buf_pos += nvram_bytes;
                var_count--;
            }
            else /* should only happen when new items are added to the end */
            {
                memcpy(settings[i].setting, &settings[i].default_val, nvram_bytes);
            }
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
#ifdef HAVE_RECORDING
        else if ((options == SETTINGS_SAVE_RECPRESETS) &&
                 ((settings[i].flags&F_RECSETTING) == 0))
            continue;
#endif
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
            screens[i].puts(0, 0, str(LANG_SETTINGS_SAVE_FAILED));
            screens[i].puts(0, 1, str(LANG_SETTINGS_PARTITION));
#else
            screens[i].puts(4, 2, str(LANG_SETTINGS_SAVE_FAILED));
            screens[i].puts(2, 4, str(LANG_SETTINGS_PARTITION));
            screens[i].update();
#endif
        }
        cond_talk_ids_fq(LANG_SETTINGS_SAVE_FAILED);
        sleep(HZ*2);
        return -1;
    }
    return 0;
}
bool settings_save_config(int options)
{
    char filename[MAX_PATH];
    char *folder;
    switch (options)
    {
        case SETTINGS_SAVE_THEME:
            folder = THEME_DIR;
            break;
#ifdef HAVE_RECORDING
        case SETTINGS_SAVE_RECPRESETS:
            folder = RECPRESETS_DIR;
            break;
#endif
        default:
            folder = ROCKBOX_DIR;
    }
    create_numbered_filename(filename, folder, "config", ".cfg", 2
                             IF_CNFN_NUM_(, NULL));

    /* allow user to modify filename */
    while (true) {
        if (!kbd_input(filename, sizeof filename)) {
            break;
        }
        else {
            gui_syncsplash(HZ, ID2P(LANG_CANCEL));
            return false;
        }
    }

    if (settings_write_config(filename, options))
        gui_syncsplash(HZ, ID2P(LANG_SETTINGS_SAVED));
    else
        gui_syncsplash(HZ, ID2P(LANG_FAILED));
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

#ifdef HAVE_WM8758
    sound_set(SOUND_BASS_CUTOFF, global_settings.bass_cutoff);
    sound_set(SOUND_TREBLE_CUTOFF, global_settings.treble_cutoff);
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

#ifndef HAVE_FLASH_STORAGE
    audio_set_buffer_margin(global_settings.buffer_margin);
#endif

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
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    backlight_set_brightness(global_settings.brightness);
#endif
#ifdef HAVE_BACKLIGHT
    backlight_set_timeout(global_settings.backlight_timeout);
#if CONFIG_CHARGING
    backlight_set_timeout_plugged(global_settings.backlight_timeout_plugged);
#endif
#if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
    backlight_set_fade_in(global_settings.backlight_fade_in);
    backlight_set_fade_out(global_settings.backlight_fade_out);
#endif
#endif
#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
    buttonlight_set_brightness(global_settings.buttonlight_brightness);
#endif
#ifdef HAVE_BUTTON_LIGHT
    buttonlight_set_timeout(global_settings.buttonlight_timeout);
#endif
#ifndef HAVE_FLASH_STORAGE
    ata_spindown(global_settings.disk_spindown);
#endif
#if (CONFIG_CODEC == MAS3507D) && !defined(SIMULATOR)
    dac_line_in(global_settings.line_in);
#endif
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
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    unload_remote_wps_backdrop();
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
#ifdef HAVE_REMOTE_LCD
        gui_wps[0].data->remote_wps = false;
#endif
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
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    show_remote_main_backdrop();
#endif

#ifdef HAVE_LCD_COLOR
    screens[SCREEN_MAIN].set_foreground(global_settings.fg_color);
    screens[SCREEN_MAIN].set_background(global_settings.bg_color);
    screens[SCREEN_MAIN].set_selector_start(global_settings.lss_color);
    screens[SCREEN_MAIN].set_selector_end(global_settings.lse_color);
    screens[SCREEN_MAIN].set_selector_text(global_settings.lst_color);
#endif

#if defined(HAVE_REMOTE_LCD) && (NB_SCREENS > 1)
    if ( global_settings.rwps_file[0]) {
        snprintf(buf, sizeof buf, WPS_DIR "/%s.rwps",
                 global_settings.rwps_file);
        wps_data_load(gui_wps[1].data, buf, true);
    }
    else
    {
        wps_data_init(gui_wps[1].data);
        gui_wps[1].data->remote_wps = true;
    }
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

#ifdef HAVE_BACKLIGHT
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
#endif /* HAVE_BACKLIGHT */

    /* This should stay last */
#if defined(HAVE_RECORDING) && CONFIG_CODEC == SWCODEC
    enc_global_settings_apply();
#endif
    /* load the icon set */
    icons_init();

#ifdef HAVE_LCD_COLOR
    if (global_settings.colors_file)
        read_color_theme_file();
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
    if (!global_settings.talk_menu)
        return;
    setting = find_setting(global_settings_variable, NULL);
    if (setting == NULL)
        return;
    if (setting->lang_id)
        talk_id(setting->lang_id,false);
}

bool set_bool(const char* string, bool* variable )
{
    return set_bool_options(string, variable,
                            (char *)STR(LANG_SET_BOOL_YES),
                            (char *)STR(LANG_SET_BOOL_NO),
                            NULL);
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

    result = set_option(string, variable, BOOL, names, 2,
                        (void (*)(int))function);
    return result;
}

bool set_int(const unsigned char* string,
             const char* unit,
             int voice_unit,
             int* variable,
             void (*function)(int),
             int step,
             int min,
             int max,
             void (*formatter)(char*, size_t, int, const char*) )
{
    return set_int_ex(string, unit, voice_unit, variable, function,
                      step, min, max, formatter, NULL);
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

