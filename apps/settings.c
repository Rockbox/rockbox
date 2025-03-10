/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Stuart Martin
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
/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
/*Define DEBUG_AVAIL_SETTINGS to get a list of all available settings and flags */
/*#define DEBUG_AVAIL_SETTINGS*/ /* Needs (LOGF_ENABLE) */
#include "logf.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include "inttypes.h"
#include "config.h"
#include "rbpaths.h"
#include "action.h"
#include "crc32.h"
#include "sound.h"
#include "settings.h"
#include "debug.h"
#include "usb.h"
#include "backlight.h"
#include "audio.h"
#include "talk.h"
#include "string-extra.h"
#include "rtc.h"
#include "power.h"
#include "ata_idle_notify.h"
#include "storage.h"
#include "screens.h"
#include "ctype.h"
#include "file.h"
#include "system.h"
#include "general.h"
#include "misc.h"
#include "icons.h"
#include "font.h"
#include "peakmeter.h"
#include "lang.h"
#include "language.h"
#include "powermgmt.h"
#include "keyboard.h"
#include "version.h"
#include "rbunicode.h"
#include "dircache.h"
#include "splash.h"
#include "list.h"
#include "settings_list.h"
#include "filetypes.h"
#include "option_select.h"
#if CONFIG_TUNER
#include "radio.h"
#endif
#include "wps.h"
#include "skin_engine/skin_engine.h"
#include "viewport.h"
#include "statusbar-skinned.h"
#include "bootchart.h"
#include "scroll_engine.h"

struct user_settings global_settings;
struct system_status global_status;
static uint32_t user_settings_crc;
static long next_status_update_tick;
static long lasttime = 0;

/* flush system_status more often for spinning harddisks as we may not be able
 * to spin up at shutdown in order to save so keep the gap minimal */
#if (CONFIG_STORAGE & STORAGE_ATA)
#define SYSTEM_STATUS_UPDATE_TICKS (HZ * 60 * 5) /* flush every 5 minutes */
#else
#define SYSTEM_STATUS_UPDATE_TICKS (HZ * 60 * 15) /* flush every 15 minutes */
#endif

#include "dsp_proc_settings.h"
#include "playback.h"
#ifdef HAVE_RECORDING
#include "enc_config.h"
#endif
#include "pcm_sampr.h"

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

#if defined(DX50) || defined(DX90)
#include "governor-ibasso.h"
#include "usb-ibasso.h"
#endif

#ifdef ROCKBOX_NO_TEMP_SETTINGS_FILE /* Overwrites same file each time */
#define CONFIGFILE_TEMP CONFIGFILE
#define RESUMEFILE_TEMP RESUMEFILE
#define rename_temp_file(a,b,c)
#else /* creates temp files on save, renames next load, saves old file if desired */
#define CONFIGFILE_TEMP CONFIGFILE".new"
#define RESUMEFILE_TEMP RESUMEFILE".new"

#ifdef LOGF_ENABLE
static char *debug_get_flags(uint32_t flags);
#undef DEBUGF /* allow DEBUGF or logf not both */
#define DEBUGF(...) do { } while(0)
#endif
static void debug_available_settings(void);

static void rename_temp_file(const char *tempfile,
                            const char *file,
                            const char *oldfile)
{
    /* if tempfile does not exist -- Return
     * if oldfile is supplied     -- Rename file to oldfile
     * if tempfile does exist     -- Rename tempfile to file
    */
    if (file_exists(tempfile))
    {
        if (oldfile != NULL && file_exists(file))
            rename(file, oldfile);
        rename(tempfile, file);
    }
}
#endif /* ndef ROCKBOX_NO_TEMP_SETTINGS_FILE */

const char* setting_get_cfgvals(const struct settings_list *setting)
{
    if ((setting->flags & F_TABLE_SETTING) == F_TABLE_SETTING)
        return setting->table_setting->cfg_vals;
    else if ((setting->flags & F_CHOICE_SETTING) == F_CHOICE_SETTING)
        return setting->choice_setting->cfg_vals;
    else if ((setting->flags & F_BOOL_SETTING) == F_BOOL_SETTING)
    {
        logf("Setting: %s", setting->cfg_name);
        return setting->bool_setting->cfg_vals;
    }
    else if ((setting->flags & F_HAS_CFGVALS) == F_HAS_CFGVALS)
        return setting->cfg_vals;
    return NULL;
}

/* calculates and stores crc of settings, returns true if settings have changed */
static bool settings_crc_changed(void)
{
    char value[MAX_PATH];
    uint32_t custom_crc = 0xFFFFFFFF;
    for(int i=0; i<nb_settings; i++)
    {
        const struct settings_list *setting = &settings[i];
        if (!(setting->flags & F_CUSTOM_SETTING))
            continue;
        cfg_to_string(setting, value, sizeof(value));
        custom_crc = crc_32(value, strlen(value), custom_crc);
    }

    uint32_t crc = crc_32(&global_settings, sizeof(global_settings), custom_crc);
    if (crc != user_settings_crc)
    {
        user_settings_crc = crc;
        return true;
    }

    return false;
}

/** Reading from a config file **/
/*
 * load settings from disk
 */
void settings_load(void)
{
    logf("\r\n%s()\r\n", __func__);
    debug_available_settings();

    /* make temp files current make current files .old */
    rename_temp_file(RESUMEFILE_TEMP, RESUMEFILE, RESUMEFILE".old");
    rename_temp_file(CONFIGFILE_TEMP, CONFIGFILE, CONFIGFILE".old");

    settings_load_config(CONFIGFILE, false); /* load user_settings items */
    settings_load_config(RESUMEFILE, false); /* load system_status items */

    /* fixed settings file has final say on user_settings AND system_status items */
    settings_load_config(FIXEDSETTINGSFILE, false);

    /* set initial CRC value - settings_save checks, if changed writes to disk */
    settings_crc_changed();
}

bool cfg_string_to_int(const struct settings_list *setting, int* out, const char* str)
{
    const char* ptr = setting_get_cfgvals(setting);
    size_t len = strlen(str);
    int index = 0;

    while (ptr)
    {
        if (!strncmp(ptr, str, len))
        {
            ptr += len;
            /* if the next character is not a comma or end of string,
             * it means the comparison was only a partial match. */
            if (*ptr == ',' || *ptr == '\0')
            {
                *out = index;
                return true;
            }
        }

        while (*ptr != ',')
        {
            if (!*ptr)
                return false;
            ptr++;
        }

        ptr++;
        index++;
    }
    logf("%s() bad setting\n", __func__);
    return false;
}

/**
 * Copy an input string to an output buffer, stripping the prefix and
 * suffix listed in the filename setting. Returns false if the output
 * string does not fit in the buffer or is longer than the setting's
 * max_len, and the output buffer will not be modified.
 *
 * Returns true if the setting was copied successfully. The input and
 * output buffers are allowed to alias.
 */
bool copy_filename_setting(char *buf, size_t buflen, const char *input,
                           const struct filename_setting *fs)
{
    size_t input_len = strlen(input);
    size_t len;

    if (fs->prefix)
    {
        len = strlen(fs->prefix);
        if (len <= input_len && !strncasecmp(input, fs->prefix, len))
        {
            input += len;
            input_len -= len;
        }
    }

    if (fs->suffix)
    {
        len = strlen(fs->suffix);
        if (len <= input_len &&
            !strcasecmp(input + input_len - len, fs->suffix))
        {
            input_len -= len;
        }
    }

    /* Make sure it fits the output buffer and repsects the setting's max_len.
     * Note that max_len is a buffer size and thus includes a null terminator */
    if (input_len >= (size_t)fs->max_len || input_len >= buflen)
        return false;

    /* Copy what remains into buf - use memmove in case of aliasing */
    memmove(buf, input, input_len);
    buf[input_len] = '\0';
    return true;
}

void string_to_cfg(const char *name, char* value, bool *theme_changed)
{
    const struct settings_list *setting = find_setting_by_cfgname(name);
    if (!setting)
        return;

    uint32_t flags = setting->flags;

    if (flags & F_THEMESETTING)
        *theme_changed = true;

    switch (flags & F_T_MASK)
    {
    case F_T_CUSTOM:
        setting->custom_setting->load_from_cfg(setting->setting, value);
        logf("Val: %s\r\n",value);
        break;
    case F_T_INT:
    case F_T_UINT:
#ifdef HAVE_LCD_COLOR
        if (flags & F_RGB)
        {
            hex_to_rgb(value, (int*)setting->setting);
            logf("Val: %s\r\n", value);
        }
        else
#endif
            if (setting_get_cfgvals(setting) == NULL)
            {
                *(int*)setting->setting = atoi(value);
                logf("Val: %s\r\n",value);
            }
            else
            {
                int temp, *v = (int*)setting->setting;
                bool found = cfg_string_to_int(setting, &temp, value);
                if (found)
                {
                    if (flags & F_TABLE_SETTING)
                        *v = setting->table_setting->values[temp];
                    else
                        *v = temp;
                    logf("Val: %d\r\n", *v);
                }
                else if (flags & F_ALLOW_ARBITRARY_VALS)
                {
                    *v = atoi(value);
                    logf("Val: %s = %d\r\n", value, *v);
                }
                else if (flags & F_TABLE_SETTING)
                {
                    const struct table_setting *info = setting->table_setting;
                    temp = atoi(value);
                    *v = setting->default_val.int_;
                    if (info->values)
                    {
                        for(int i = 0; i < info->count; i++)
                        {
                            if (info->values[i] == temp)
                            {
                                *v = temp;
                                break;
                            }
                        }
                    }
                    logf("Val: %s", *v == temp ? "Found":"Error Not Found");
                    logf("Val: %s = %d\r\n", value, *v);
                }

                else
                {
                    logf("Error: %s: Not Found! [%s]\r\n",
                                      setting->cfg_name, value);
                }
            }
        break;
    case F_T_BOOL:
    {
        int temp;
        if (cfg_string_to_int(setting, &temp, value))
        {
            *(bool*)setting->setting = !!temp;
            logf("Val: %s\r\n", value);
        }
        if (setting->bool_setting->option_callback)
        {
            setting->bool_setting->option_callback(!!temp);
        }
        break;
    }
    /* these can be plain text, filenames, or dirnames */
    case F_T_CHARPTR:
    case F_T_UCHARPTR:
    {
        const struct filename_setting *fs = setting->filename_setting;
        copy_filename_setting((char*)setting->setting,
                              fs->max_len, value, fs);
        logf("Val: %s\r\n", value);
        break;
    }
    }
}

bool settings_load_config(const char* file, bool apply)
{
    logf("%s()\r\n", __func__);
    int fd;
    char line[128];
    bool theme_changed = false;

    fd = open_utf8(file, O_RDONLY);
    if (fd < 0)
        return false;

    while (read_line(fd, line, sizeof line) > 0)
    {
        char *name, *value;
        if (!settings_parseline(line, &name, &value))
            continue;
        string_to_cfg(name, value, &theme_changed);
    } /* while(...) */

    close(fd);
    if (apply)
    {
        settings_save();
        settings_apply(true);
        if (theme_changed)
            settings_apply_skins();
    }
    return true;
}

/** Writing to a config file and saving settings **/

bool cfg_int_to_string(const struct settings_list *setting, int val, char* buf, int buf_len)
{
    const char* ptr = setting_get_cfgvals(setting);
    const int *values = NULL;
    int index = 0;

    if (setting->flags & F_TABLE_SETTING)
        values = setting->table_setting->values;

    while (ptr)
    {
        if ((values && values[index] == val) ||
            (!values && index == val))
        {
            char *buf_end = buf + buf_len - 1;
            while (*ptr && *ptr != ',' && buf != buf_end)
                *buf++ = *ptr++;

            *buf++ = '\0';
            return true;
        }

        while (*ptr != ',')
        {
            if (!*ptr)
                return false;
            ptr++;
        }

        ptr++;
        index++;
    }
    logf("%s() bad setting\n", __func__);
    return false;
}

void cfg_to_string(const struct settings_list *setting, char* buf, int buf_len)
{
    switch (setting->flags & F_T_MASK)
    {
        case F_T_CUSTOM:
            setting->custom_setting->write_to_cfg(setting->setting, buf, buf_len);
            break;
        case F_T_INT:
        case F_T_UINT:
#ifdef HAVE_LCD_COLOR
            if (setting->flags & F_RGB)
            {
                int colour = *(int*)setting->setting;
                snprintf(buf,buf_len,"%02x%02x%02x",
                            (int)RGB_UNPACK_RED(colour),
                            (int)RGB_UNPACK_GREEN(colour),
                            (int)RGB_UNPACK_BLUE(colour));
                break; /* we got a value */
            }
            else
#endif
            if (setting_get_cfgvals(setting) != NULL && cfg_int_to_string(
                setting, *(int*)setting->setting, buf, buf_len))
            {
                break; /* we got a value */
            }

            itoa_buf(buf, buf_len, *(int*)setting->setting);
            break;
        case F_T_BOOL:
            cfg_int_to_string(setting, *(bool*)setting->setting, buf, buf_len);
            break;
        case F_T_CHARPTR:
        case F_T_UCHARPTR:
        {
            char *value = setting->setting;
            const struct filename_setting *fs = setting->filename_setting;
            if (value[0] && fs->prefix)
            {
                if (value[0] == '-')
                {
                    buf[0] = '-';
                    buf[1] = '\0';
                }
                else
                {
                    snprintf(buf, buf_len, "%s%s%s",
                             fs->prefix, value, fs->suffix);
                }
            }
            else
            {
                strmemccpy(buf, value, buf_len);
            }
            break;
        }
    } /* switch () */
}


static bool is_changed(const struct settings_list *setting)
{
    switch (setting->flags&F_T_MASK)
    {
    case F_T_CUSTOM:
        return setting->custom_setting->is_changed(setting->setting,
                                            setting->default_val.custom);
        break;
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
        else if (setting->flags & F_RESUMESETTING)
        {
            /* exclude resume settings they will get saved to '.resume.cfg' */
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

static bool settings_write_config(const char* filename, int options)
{
    logf("%s\r\n", __func__);
    int i;
    int fd;
    char value[MAX_PATH];
    fd = open(filename,O_CREAT|O_TRUNC|O_WRONLY, 0666);
    if (fd < 0)
        return false;

    if (options != SETTINGS_SAVE_RESUMEINFO)
    {
        fdprintf(fd, "# .cfg file created by rockbox %s - "
                 "http://www.rockbox.org\r\n\r\n", rbversion);
    }

    for(i=0; i<nb_settings; i++)
    {
        const struct settings_list *setting = &settings[i];
        if (!setting->cfg_name || (setting->flags & F_DEPRECATED))
            continue;

        switch (options)
        {
            case SETTINGS_SAVE_CHANGED:
                if (!is_changed(setting))
                    continue;
                break;
            case SETTINGS_SAVE_SOUND:
                if (!(setting->flags & F_SOUNDSETTING))
                    continue;
                break;
            case SETTINGS_SAVE_THEME:
                if (!(setting->flags & F_THEMESETTING))
                    continue;
                break;
#ifdef HAVE_RECORDING
            case SETTINGS_SAVE_RECPRESETS:
                if (!(setting->flags & F_RECSETTING))
                    continue;
                break;
#endif
            case SETTINGS_SAVE_EQPRESET:
                if (!(setting->flags & F_EQSETTING))
                    continue;
                break;
            case SETTINGS_SAVE_RESUMEINFO:
                if (!(setting->flags & F_RESUMESETTING))
                    continue;
                break;
            case SETTINGS_SAVE_ALL:
            {
                /*only save sound settings (volume) from F_RESUMESETTING items */
                uint32_t exclude_flag = (F_RESUMESETTING|F_SOUNDSETTING);
                if ((setting->flags & exclude_flag) == F_RESUMESETTING)
                    continue;
                break;
            }
        }
        cfg_to_string(setting, value, MAX_PATH);
        logf("Written: '%s: %s'\r\n",setting->cfg_name, value);

        fdprintf(fd,"%s: %s\r\n",setting->cfg_name,value);
    } /* for(...) */
    close(fd);
    return true;
}

static void flush_global_status_callback(void)
{
    if (TIME_AFTER(current_tick, next_status_update_tick))
    {
        next_status_update_tick = current_tick + SYSTEM_STATUS_UPDATE_TICKS;
        update_runtime();

        DEBUGF("Writing system_status to disk\n");
        logf("Writing system_status to disk");

        settings_write_config(RESUMEFILE_TEMP, SETTINGS_SAVE_RESUMEINFO);
    }
}

static void flush_config_block_callback(void)
{
    if (settings_crc_changed())
    {
        DEBUGF("Writing changed user_settings to disk\n");
        logf("Writing changed user_settings to disk");

        if (!settings_write_config(CONFIGFILE_TEMP, SETTINGS_SAVE_CHANGED))
        {
            user_settings_crc = 0;
            DEBUGF("Error failed to write settings to disk\n");
            logf("Error failed to write settings to disk");
        }
    }
#ifdef LOGF_ENABLE
    else
        logf("No changes to user_settings");
#endif
    /* remove any outstanding status_cb and call it unconditionally */
    next_status_update_tick = current_tick - 1;
    unregister_storage_idle_func(flush_global_status_callback, true);
}

void reset_runtime(void)
{
    update_runtime(); /* in case this is > topruntimetime */
    lasttime = current_tick;
    global_status.runtime = 0;
}

/*
 * update runtime and if greater topruntime as well
 */
void update_runtime(void)
{
    int elapsed_secs;

    elapsed_secs = (current_tick - lasttime) / HZ;
    global_status.runtime += elapsed_secs;
    lasttime += (elapsed_secs * HZ);

    if ( global_status.runtime > global_status.topruntime )
        global_status.topruntime = global_status.runtime;
}

void status_save(bool force)
{
    if(force)
    {
        settings_save(); /* will force a status flush */
    }
    else
        register_storage_idle_func(flush_global_status_callback);
}

int settings_save(void)
{
    logf("%s", __func__);
    /* remove any oustanding status_cb we will call it in the config_block_cb */
    unregister_storage_idle_func(flush_global_status_callback, false);
    register_storage_idle_func(flush_config_block_callback);
    return 0;
}

bool settings_save_config(int options)
{
    char filename[MAX_PATH];
    const char *folder, *namebase;
    switch (options)
    {
        case SETTINGS_SAVE_THEME:
            folder = THEME_DIR;
            namebase = "theme";
            break;
#ifdef HAVE_RECORDING
        case SETTINGS_SAVE_RECPRESETS:
            folder = RECPRESETS_DIR;
            namebase = "recording";
            break;
#endif
        case SETTINGS_SAVE_EQPRESET:
            folder = EQS_DIR;
            namebase = "eq";
            break;
        case SETTINGS_SAVE_SOUND:
            folder = ROCKBOX_DIR;
            namebase = "sound";
            break;
        default:
            folder = ROCKBOX_DIR;
            namebase = "config";
            break;
    }
    create_numbered_filename(filename, folder, namebase, ".cfg", 2
                             IF_CNFN_NUM_(, NULL));

    /* allow user to modify filename */
    while (true) {
        if (!kbd_input(filename, sizeof(filename), NULL)) {
            break;
        }
        else {
            return false;
        }
    }

    if (settings_write_config(filename, options))
        splash(HZ, ID2P(LANG_SETTINGS_SAVED));
    else
        splash(HZ, ID2P(LANG_FAILED));
    return true;
}

/** Apply and Reset settings **/

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

void sound_settings_apply(void)
{
#ifdef AUDIOHW_HAVE_BASS
    sound_set(SOUND_BASS, global_settings.bass);
#endif
#ifdef AUDIOHW_HAVE_TREBLE
    sound_set(SOUND_TREBLE, global_settings.treble);
#endif
    sound_set(SOUND_BALANCE, global_settings.balance);
#ifndef PLATFORM_HAS_VOLUME_CHANGE
    sound_set(SOUND_VOLUME, global_status.volume);
#endif
    sound_set(SOUND_CHANNELS, global_settings.channel_config);
    sound_set(SOUND_STEREO_WIDTH, global_settings.stereo_width);
#ifdef AUDIOHW_HAVE_BASS_CUTOFF
    sound_set(SOUND_BASS_CUTOFF, global_settings.bass_cutoff);
#endif
#ifdef AUDIOHW_HAVE_TREBLE_CUTOFF
    sound_set(SOUND_TREBLE_CUTOFF, global_settings.treble_cutoff);
#endif
#ifdef AUDIOHW_HAVE_DEPTH_3D
    sound_set(SOUND_DEPTH_3D, global_settings.depth_3d);
#endif
#ifdef AUDIOHW_HAVE_FILTER_ROLL_OFF
    sound_set(SOUND_FILTER_ROLL_OFF, global_settings.roll_off);
#endif
#ifdef AUDIOHW_HAVE_POWER_MODE
    sound_set(SOUND_POWER_MODE, global_settings.power_mode);
#endif
#ifdef AUDIOHW_HAVE_EQ
    int b;

    for (b = 0; b < AUDIOHW_EQ_BAND_NUM; b++)
    {
        int setting = sound_enum_hw_eq_band_setting(b, AUDIOHW_EQ_GAIN);
        sound_set(setting, global_settings.hw_eq_bands[b].gain);

#ifdef AUDIOHW_HAVE_EQ_FREQUENCY
        setting = sound_enum_hw_eq_band_setting(b, AUDIOHW_EQ_FREQUENCY);
        if (setting != -1)
            sound_set(setting, global_settings.hw_eq_bands[b].frequency);
#endif /* AUDIOHW_HAVE_EQ_FREQUENCY */
#ifdef AUDIOHW_HAVE_EQ_WIDTH
        setting = sound_enum_hw_eq_band_setting(b, AUDIOHW_EQ_WIDTH);
        if (setting != -1)
            sound_set(setting, global_settings.hw_eq_bands[b].width);
#endif /* AUDIOHW_HAVE_EQ_WIDTH */
    }
#endif
}

void settings_apply(bool read_disk)
{
    logf("%s", __func__);
    int rc;
    CHART(">set_codepage");
    set_codepage(global_settings.default_codepage);
    CHART("<set_codepage");

    sound_settings_apply();

#ifdef HAVE_DISK_STORAGE
    audio_set_buffer_margin(global_settings.buffer_margin);
#endif

#ifdef HAVE_LCD_CONTRAST
    lcd_set_contrast(global_settings.contrast);
#endif
    lcd_scroll_speed(global_settings.scroll_speed);
#ifdef HAVE_REMOTE_LCD
    lcd_remote_set_contrast(global_settings.remote_contrast);
    lcd_remote_set_invert_display(global_settings.remote_invert);

#ifdef HAVE_LCD_FLIP
    lcd_remote_set_flip(global_settings.remote_flip_display);
#endif

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
#if    defined(HAVE_BACKLIGHT_FADING_INT_SETTING) \
    || defined(HAVE_BACKLIGHT_FADING_BOOL_SETTING)
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
#ifdef HAVE_DISK_STORAGE
    storage_spindown(global_settings.disk_spindown);
#endif
    set_poweroff_timeout(global_settings.poweroff);
    if (global_settings.sleeptimer_on_startup)
        set_sleeptimer_duration(global_settings.sleeptimer_duration);
    set_keypress_restarts_sleep_timer(
        global_settings.keypress_restarts_sleeptimer);

#if BATTERY_CAPACITY_INC > 0
    /* only call if it's really exchangable */
    set_battery_capacity(global_settings.battery_capacity);
#endif

#ifdef HAVE_LCD_INVERT
    lcd_set_invert_display(global_settings.invert);
#endif
#ifdef HAVE_LCD_FLIP
    lcd_set_flip(global_settings.flip_display);
    button_set_flip(global_settings.flip_display);
#endif
    lcd_update(); /* refresh after flipping the screen */
    settings_apply_pm_range();
    peak_meter_init_times(
        global_settings.peak_meter_release, global_settings.peak_meter_hold,
        global_settings.peak_meter_clip_hold);

#ifdef HAVE_SPEAKER
    audio_enable_speaker(global_settings.speaker_mode);
#endif

    if (read_disk)
    {
        char buf[MAX_PATH];
        /* fonts need to be loaded before the WPS */
        if (global_settings.font_file[0]
            && global_settings.font_file[0] != '-') {
            int font_ui = screens[SCREEN_MAIN].getuifont();
            snprintf(buf, sizeof buf, FONT_DIR "/%s.fnt",
                     global_settings.font_file);
            if (!font_filename_matches_loaded_id(font_ui, buf))
            {
                CHART2(">font_load ", global_settings.font_file);
                if (font_ui >= 0)
                    font_unload(font_ui);
                rc = font_load_ex(buf, 0, global_settings.glyphs_to_cache);
                CHART2("<font_load ", global_settings.font_file);
                screens[SCREEN_MAIN].setuifont(rc);
                screens[SCREEN_MAIN].setfont(rc);
            }
        }
#ifdef HAVE_REMOTE_LCD
        if ( global_settings.remote_font_file[0]
            && global_settings.remote_font_file[0] != '-') {
            int font_ui = screens[SCREEN_REMOTE].getuifont();
            snprintf(buf, sizeof buf, FONT_DIR "/%s.fnt",
                     global_settings.remote_font_file);
            if (!font_filename_matches_loaded_id(font_ui, buf))
            {
                CHART2(">font_load_remoteui ", global_settings.remote_font_file);
                if (font_ui >= 0)
                    font_unload(font_ui);
                rc = font_load(buf);
                CHART2("<font_load_remoteui ", global_settings.remote_font_file);
                screens[SCREEN_REMOTE].setuifont(rc);
                screens[SCREEN_REMOTE].setfont(rc);
            }
        }
#endif
        if ( global_settings.kbd_file[0]
             && global_settings.kbd_file[0] != '-') {
            snprintf(buf, sizeof buf, ROCKBOX_DIR "/%s.kbd",
                     global_settings.kbd_file);
            CHART(">load_kbd");
            load_kbd(buf);
            CHART("<load_kbd");
        }
        else
            load_kbd(NULL);
        if ( global_settings.lang_file[0]) {
            snprintf(buf, sizeof buf, LANG_DIR "/%s.lng",
                     global_settings.lang_file);
            CHART(">lang_core_load");
            lang_core_load(buf);
            CHART("<lang_core_load");
        }
        CHART(">talk_init");
        talk_init(); /* use voice of same language */
        CHART("<talk_init");

        /* load the icon set */
        CHART(">icons_init");
        icons_init();
        CHART("<icons_init");

#ifdef HAVE_LCD_COLOR
        CHART(">read_color_theme_file");
        read_color_theme_file();
        CHART("<read_color_theme_file");
#endif
    }
#ifdef HAVE_LCD_COLOR
    screens[SCREEN_MAIN].set_foreground(global_settings.fg_color);
    screens[SCREEN_MAIN].set_background(global_settings.bg_color);
#endif

    lcd_scroll_step(global_settings.scroll_step);
    lcd_bidir_scroll(global_settings.bidir_limit);
    lcd_scroll_delay(global_settings.scroll_delay);

#ifdef HAVE_ALBUMART
    set_albumart_mode(global_settings.album_art);
#endif

#ifdef HAVE_PLAY_FREQ
    /* before crossfade */
    audio_set_playback_frequency(global_settings.play_frequency);
#endif
#ifdef HAVE_CROSSFADE
    audio_set_crossfade(global_settings.crossfade);
#endif
    replaygain_update();
    dsp_set_crossfeed_type(global_settings.crossfeed);
    dsp_set_crossfeed_direct_gain(global_settings.crossfeed_direct_gain);
    dsp_set_crossfeed_cross_params(global_settings.crossfeed_cross_gain,
                                   global_settings.crossfeed_hf_attenuation,
                                   global_settings.crossfeed_hf_cutoff);

    /* Configure software equalizer, hardware eq is handled in audio_init() */
    dsp_eq_enable(global_settings.eq_enabled);
    dsp_set_eq_precut(global_settings.eq_precut);
    for(int i = 0; i < EQ_NUM_BANDS; i++) {
        dsp_set_eq_coefs(i, &global_settings.eq_band_settings[i]);
    }

    dsp_dither_enable(global_settings.dithering_enabled);
    dsp_surround_set_balance(global_settings.surround_balance);
    dsp_surround_set_cutoff(global_settings.surround_fx1, global_settings.surround_fx2);
    dsp_surround_mix(global_settings.surround_mix);
    dsp_surround_enable(global_settings.surround_enabled);
    dsp_afr_enable(global_settings.afr_enabled);
    dsp_pbe_precut(global_settings.pbe_precut);
    dsp_pbe_enable(global_settings.pbe);
#ifdef HAVE_PITCHCONTROL
    dsp_timestretch_enable(global_settings.timestretch_enabled);
#endif
    dsp_set_compressor(&global_settings.compressor_settings);

#ifdef HAVE_SPDIF_POWER
    spdif_power_enable(global_settings.spdif_enable);
#endif

#ifdef HAVE_BACKLIGHT
    set_backlight_filter_keypress(global_settings.bl_filter_first_keypress);
    set_selective_backlight_actions(global_settings.bl_selective_actions,
                                    global_settings.bl_selective_actions_mask,
                                    global_settings.bl_filter_first_keypress);
#ifdef HAVE_REMOTE_LCD
    set_remote_backlight_filter_keypress(global_settings.remote_bl_filter_first_keypress);
#endif
    backlight_set_on_button_hold(global_settings.backlight_on_button_hold);

#ifdef HAVE_LCD_SLEEP_SETTING
    lcd_set_sleep_after_backlight_off(global_settings.lcd_sleep_after_backlight_off);
#endif
#endif /* HAVE_BACKLIGHT */

#ifndef HAS_BUTTON_HOLD
    set_selective_softlock_actions(
                            global_settings.bt_selective_softlock_actions,
                            global_settings.bt_selective_softlock_actions_mask);
    action_autosoftlock_init();
#endif

#ifdef HAVE_TOUCHPAD_SENSITIVITY_SETTING
    touchpad_set_sensitivity(global_settings.touchpad_sensitivity);
#endif

#ifdef HAVE_TOUCHPAD_DEADZONE
    touchpad_set_deadzone(global_settings.touchpad_deadzone);
#endif

#ifdef HAVE_USB_CHARGING_ENABLE
    usb_charging_enable(global_settings.usb_charging);
#endif

#ifdef HAVE_TOUCHSCREEN
    touchscreen_set_mode(global_settings.touch_mode);
    memcpy(&calibration_parameters, &global_settings.ts_calibration_data, sizeof(struct touchscreen_parameter));
#endif

#if defined(DX50) || defined(DX90)
    ibasso_set_usb_mode(global_settings.usb_mode);
#elif defined(HAVE_USB_POWER) && !defined(USB_NONE) && !defined(SIMULATOR)
    usb_set_mode(global_settings.usb_mode);
#endif

#if defined(DX50) || defined(DX90)
    ibasso_set_governor(global_settings.governor);
#endif

    /* This should stay last */
#if defined(HAVE_RECORDING)
    enc_global_settings_apply();
#endif
    /* already called with THEME_STATUSBAR in settings_apply_skins() */
    CHART(">viewportmanager_theme_changed");
    viewportmanager_theme_changed(THEME_UI_VIEWPORT|THEME_LANGUAGE|THEME_BUTTONBAR);
    CHART("<viewportmanager_theme_changed");
}

/*
 * reset all settings to their default value
 */
void reset_setting(const struct settings_list *setting, void *var)
{
    switch (setting->flags&F_T_MASK)
    {
    case F_T_CUSTOM:
        setting->custom_setting->set_default(setting->setting,
                                             setting->default_val.custom);
        break;
    case F_T_INT:
    case F_T_UINT:
        if (setting->flags&F_DEF_ISFUNC)
            *(int*)var = setting->default_val.func();
        else if (setting->flags&F_T_SOUND)
            *(int*)var = sound_default(setting->sound_setting->setting);
        else *(int*)var = setting->default_val.int_;
        break;
    case F_T_BOOL:
        *(bool*)var = setting->default_val.bool_;
        break;
    case F_T_CHARPTR:
    case F_T_UCHARPTR:
        strmemccpy((char*)var, setting->default_val.charptr,
                   setting->filename_setting->max_len);
        break;
    }
}

void settings_reset(void)
{
    for(int i=0; i<nb_settings; i++)
        reset_setting(&settings[i], settings[i].setting);
#if defined (HAVE_RECORDING)
    enc_global_settings_reset();
#endif
    FOR_NB_SCREENS(i)
    {
        if (screens[i].getuifont() > FONT_SYSFIXED)
        {
            font_unload(screens[i].getuifont());
            screens[i].setuifont(FONT_SYSFIXED);
            screens[i].setfont(FONT_SYSFIXED);
        }
    }
}

/** Changing setting values **/
const struct settings_list* find_setting(const void* variable)
{
    for(int i = 0; i < nb_settings; i++)
    {
        const struct settings_list *setting = &settings[i];
        if (setting->setting == variable)
            return setting;
    }

    return NULL;
}

const struct settings_list* find_setting_by_cfgname(const char* name)
{
    logf("Searching for Setting: '%s'",name);
    for(int i = 0; i < nb_settings; i++)
    {
        const struct settings_list *setting = &settings[i];
        if (setting->cfg_name && !strcasecmp(setting->cfg_name, name))
        {
#ifdef LOGF_ENABLE
            name = debug_get_flags(settings[i].flags);
            logf("Found, %s", name);
#endif
            return setting;
        }
    }
    logf("Setting: '%s' Not Found!",name);

    return NULL;
}

bool set_bool(const char* string, const bool* variable )
{
    return set_bool_options(string, variable,
                            (char *)STR(LANG_SET_BOOL_YES),
                            (char *)STR(LANG_SET_BOOL_NO),
                            NULL);
}


bool set_bool_options(const char* string, const bool* variable,
                      const char* yes_str, int yes_voice,
                      const char* no_str, int no_voice,
                      void (*function)(bool))
{
    struct opt_items names[] = {
        {(unsigned const char *)no_str, no_voice},
        {(unsigned const char *)yes_str, yes_voice}
    };
    bool result;

    result = set_option(string, variable, RB_BOOL, names, 2,
                        (void (*)(int))(void (*)(void))function);
    return result;
}

bool set_int(const unsigned char* string,
             const char* unit,
             int voice_unit,
             const int* variable,
             void (*function)(int),
             int step,
             int min,
             int max,
             const char* (*formatter)(char*, size_t, int, const char*) )
{
    return set_int_ex(string, unit, voice_unit, variable, function,
                      step, min, max, formatter, NULL);
}

bool set_int_ex(const unsigned char* string,
                const char* unit,
                int voice_unit,
                const int* variable,
                void (*function)(int),
                int step,
                int min,
                int max,
                const char* (*formatter)(char*, size_t, int, const char*),
                int32_t (*get_talk_id)(int, int))
{
    (void)unit;
    struct settings_list item;
    const struct int_setting data = {
        .option_callback = function,
        .unit = voice_unit,
        .step = step,
        .min = min,
        .max = max,
        .formatter = formatter,
        .get_talk_id = get_talk_id,
    };
    item.int_setting = &data;
    item.flags = F_INT_SETTING|F_T_INT;
    item.lang_id = -1;
    item.setting = (void *)variable;
    return option_screen(&item, NULL, false, string);
}


static const struct opt_items *set_option_options;
static const char* set_option_formatter(char* buf, size_t size, int item, const char* unit)
{
    (void)buf, (void)unit, (void)size;
    return P2STR(set_option_options[item].string);
}

static int32_t set_option_get_talk_id(int value, int unit)
{
    (void)unit;
    return set_option_options[value].voice_id;
}

bool set_option(const char* string, const void* variable, enum optiontype type,
                const struct opt_items* options,
                int numoptions, void (*function)(int))
{
    int temp;
    struct settings_list item;
    const struct int_setting data = {
        .option_callback = function,
        .unit = UNIT_INT,
        .step = 1,
        .min = 0,
        .max = numoptions-1,
        .formatter = set_option_formatter,
        .get_talk_id = set_option_get_talk_id
    };
    memset(&item, 0, sizeof(struct settings_list));
    set_option_options = options;
    item.int_setting = &data;
    item.flags = F_INT_SETTING|F_T_INT;
    item.lang_id = -1;
    item.setting = &temp;
    if (type == RB_BOOL)
        temp = *(bool*)variable? 1: 0;
    else
        temp = *(int*)variable;
    if (!option_screen(&item, NULL, false, string))
    {
        if (type == RB_BOOL)

            *(bool*)variable = (temp == 1);
        else
            *(int*)variable = temp;
        return false;
    }
    return true;
}

/*
 * Takes filename, removes the directory and the extension,
 * and then copies the basename into setting, unless the basename exceeds maxlen
 **/
void set_file(const char* filename, char* setting)
{
    const int maxlen = MAX_FILENAME;
    const char* fptr = strrchr(filename,'/');
    const char* extptr;
    int len;
    int extlen = 0;

    if (!fptr)
        return;

    fptr++;

    extptr = strrchr(fptr, '.');

    if (!extptr || extptr < fptr)
        extlen = 0;
    else
        extlen = strlen(extptr);

    len = strlen(fptr) - extlen + 1;

    /* error later if filename isn't in ROCKBOX_DIR */
    if (len > maxlen || !file_exists(filename))
    {
        DEBUGF("%s Error %s\n", __func__, filename);
        return;
    }

    strmemccpy(setting, fptr, len);
    settings_save();
}

#ifdef LOGF_ENABLE
static char *debug_get_flags(uint32_t flags)
{
    static char buf[256];
    uint32_t ftype = flags & F_T_MASK; /* the variable type for the setting */
    flags &= ~F_T_MASK;
    uint32_t flags_rem = flags;
    switch (ftype)
    {
        case F_T_CUSTOM:
            strlcpy(buf, "Type: [CUSTOM]   Flags: ", sizeof(buf));
            break;
        case F_T_INT:
            strlcpy(buf, "Type: [INT]      Flags: ", sizeof(buf));
            break;
        case F_T_UINT:
            strlcpy(buf, "Type: [UINT]     Flags: ", sizeof(buf));
            break;
        case F_T_BOOL:
            strlcpy(buf, "Type: [BOOL]     Flags: ", sizeof(buf));
            break;
        case F_T_CHARPTR:
            strlcpy(buf, "Type: [CHARPTR]  Flags: ", sizeof(buf));
            break;
        case F_T_UCHARPTR:
            strlcpy(buf, "Type: [UCHARPTR] Flags: ", sizeof(buf));
            break;
        default:
            snprintf(buf, sizeof(buf),
                     "Type: [!UNKNOWN TYPE! (0x%lx)] Flags: ", (long)ftype);
            break;
    }

#define SETTINGFLAGS(n)                 \
        if(flags_rem & n) {             \
           flags_rem &= ~n;             \
    strlcat(buf, "["#n"]", sizeof(buf));}

    SETTINGFLAGS(F_RESUMESETTING);
    SETTINGFLAGS(F_THEMESETTING);
    SETTINGFLAGS(F_RECSETTING);
    SETTINGFLAGS(F_EQSETTING);
    SETTINGFLAGS(F_SOUNDSETTING);
    SETTINGFLAGS(F_NEEDAPPLY);
    SETTINGFLAGS(F_CHOICE_SETTING);
    SETTINGFLAGS(F_CHOICETALKS);
    SETTINGFLAGS(F_TABLE_SETTING);
    SETTINGFLAGS(F_CUSTOM_SETTING);
    SETTINGFLAGS(F_TIME_SETTING);

    SETTINGFLAGS(F_FILENAME);
    SETTINGFLAGS(F_INT_SETTING);
    SETTINGFLAGS(F_T_SOUND);
    SETTINGFLAGS(F_RGB);
    SETTINGFLAGS(F_BOOL_SETTING);

    SETTINGFLAGS(F_MIN_ISFUNC);
    SETTINGFLAGS(F_MAX_ISFUNC);
    SETTINGFLAGS(F_DEF_ISFUNC);
    SETTINGFLAGS(F_ALLOW_ARBITRARY_VALS);
    SETTINGFLAGS(F_CB_ON_SELECT_ONLY);
    SETTINGFLAGS(F_CB_ONLY_IF_CHANGED);
    SETTINGFLAGS(F_TEMPVAR);
    SETTINGFLAGS(F_PADTITLE);
    SETTINGFLAGS(F_NO_WRAP);
    SETTINGFLAGS(F_BANFROMQS);
    SETTINGFLAGS(F_DEPRECATED);

    SETTINGFLAGS(F_HAS_CFGVALS);
#undef SETTINGFLAGS

    /* anything left is unknown */
    if (flags_rem)
    {
        strlcat(buf, "[!UNKNOWN FLAGS!]", sizeof(buf));
        size_t len = strlen(buf);
        if (len < sizeof(buf))
            snprintf(buf + len, sizeof(buf) - len - 1, "[0x%lx]", flags_rem);
    }
    /* no flags set */
    if (flags == 0)
        strlcat(buf, "[0x0]", sizeof(buf));
    return buf;
}
#endif
static void debug_available_settings(void)
{
#if defined(DEBUG_AVAIL_SETTINGS) && defined(LOGF_ENABLE)
    static char namebuf[128];

    logf("\r\nAvailable Settings:");
    for (int i=0; i<nb_settings; i++)
    {
        uint32_t flags = settings[i].flags;
        const char *name;
        if (settings[i].cfg_name)
        {
            snprintf(namebuf, sizeof(namebuf), "'%s'", settings[i].cfg_name);
            name = namebuf;
        }
        else if (settings[i].RESERVED == NULL)
        {
            name = "SYS";
        }
        else
        {
            name = "?? UNKNOWN NAME ?? ";
        }

        logf("%-45s %s",name, debug_get_flags(flags));
    }
    logf("End Available Settings\r\n");
#endif
}
