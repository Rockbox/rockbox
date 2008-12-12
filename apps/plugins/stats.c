/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Jonas Haggqvist
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

PLUGIN_HEADER

static const struct plugin_api* rb;
static int files, dirs, musicfiles, largestdir;
static int lasttick;
static bool abort;

#if CONFIG_KEYPAD == PLAYER_PAD 
#define STATS_STOP BUTTON_STOP

#elif (CONFIG_KEYPAD == RECORDER_PAD) \
   || (CONFIG_KEYPAD == ONDIO_PAD) \
   || (CONFIG_KEYPAD == ARCHOS_AV300_PAD)
#define STATS_STOP BUTTON_OFF

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) \
   || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define STATS_STOP BUTTON_OFF
#define STATS_STOP_REMOTE BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define STATS_STOP BUTTON_MENU

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define STATS_STOP BUTTON_PLAY

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define STATS_STOP BUTTON_POWER
#define STATS_STOP_REMOTE BUTTON_RC_PLAY

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define STATS_STOP BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
(CONFIG_KEYPAD == SANSA_C200_PAD) || \
(CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
(CONFIG_KEYPAD == SANSA_M200_PAD)
#define STATS_STOP BUTTON_POWER

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define STATS_STOP BUTTON_POWER

#elif CONFIG_KEYPAD == MROBE500_PAD
#define STATS_STOP BUTTON_POWER

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define STATS_STOP BUTTON_BACK

#elif CONFIG_KEYPAD == MROBE100_PAD
#define STATS_STOP BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define STATS_STOP BUTTON_REC
#define STATS_STOP_REMOTE BUTTON_RC_REC

#elif CONFIG_KEYPAD == COWOND2_PAD
#define STATS_STOP BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define STATS_STOP BUTTON_POWER

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define STATS_STOP BUTTON_BACK

#else
#error No keymap defined!
#endif

/* TODO: Better get the exts from the filetypes var in tree.c */
const char *music_exts[] = {"mp3","mp2","mp1","mpa","ogg",
        "wav","flac","ac3","a52","mpc","wv","m4a","m4b","mp4",
        "shn","aif","aiff","wma","wmv","spx","ape","adx",
        "sid","mod","nsf","nsfe","spc","asf","mac"};

void prn(const char *str, int y)
{
    rb->lcd_puts(0,y,str);
#ifdef HAVE_REMOTE_LCD
    rb->lcd_remote_puts(0,y,str);
#endif
}

void update_screen(void)
{
    char buf[32];

    rb->lcd_clear_display();
#ifdef HAVE_REMOTE_LCD
    rb->lcd_remote_clear_display();
#endif

#ifdef HAVE_LCD_BITMAP
    rb->snprintf(buf, sizeof(buf), "Files: %d", files);
    prn(buf,0);
    rb->snprintf(buf, sizeof(buf), "Music: %d", musicfiles);
    prn(buf,1);
    rb->snprintf(buf, sizeof(buf), "Dirs: %d", dirs);
    prn(buf,2);
    rb->snprintf(buf, sizeof(buf), "Max files in Dir: %d", largestdir);
    prn(buf,3);
#else
    rb->snprintf(buf, sizeof(buf), "Files:%5d", files);
    prn(buf,0);
    rb->snprintf(buf, sizeof(buf), "Dirs: %5d", dirs);
    prn(buf,1);
#endif

    rb->lcd_update();
#ifdef HAVE_REMOTE_LCD
    rb->lcd_remote_update();
#endif
}

void traversedir(char* location, char* name)
{
    int button;
    struct dirent *entry;
    DIR* dir;
    char fullpath[MAX_PATH];
    int files_in_dir = 0;

    rb->snprintf(fullpath, sizeof(fullpath), "%s/%s", location, name);
    dir = rb->opendir(fullpath);
    if (dir) {
        entry = rb->readdir(dir);
        while (entry) {
            if (abort == true)
                break;
            /* Skip .. and . */
            if (rb->strcmp(entry->d_name, ".") && rb->strcmp(entry->d_name, ".."))
            {
                if (entry->attribute & ATTR_DIRECTORY) {
                    traversedir(fullpath, entry->d_name);
                    dirs++;
                }
                else {
                    char *ptr = rb->strrchr(entry->d_name,'.'); 
                    files++; files_in_dir++;
                    /* Might want to only count .mp3, .ogg etc. */
                    if(ptr){
                        unsigned i;
                        ptr++;
                        for(i=0;i<sizeof(music_exts)/sizeof(char*);i++)
                            if(!rb->strcasecmp(ptr,music_exts[i])){
                                musicfiles++; break;
                            }
                        
                    }
                }
            }
            if (*rb->current_tick - lasttick > (HZ/2)) {
                update_screen();
                lasttick = *rb->current_tick;
                button = rb->button_get(false);
                if (button == STATS_STOP
#ifdef HAVE_REMOTE_LCD
                    || button == STATS_STOP_REMOTE
#endif
                    ) {
                    abort = true;
                    break;
                }
            }

            entry = rb->readdir(dir);
        }
        rb->closedir(dir);
    }
    if (largestdir < files_in_dir)
        largestdir = files_in_dir;
}

enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    int button;

    (void)parameter;

    rb = api;
    files = 0;
    dirs = 0;
    musicfiles = 0;
    largestdir = 0;
    abort = false;

    rb->splash(HZ, "Counting...");
    update_screen();
    lasttick = *rb->current_tick;

    traversedir("", "");
    if (abort == true) {
        rb->splash(HZ, "Aborted");
        return PLUGIN_OK;
    }
    update_screen();
#ifdef HAVE_REMOTE_LCD
    rb->remote_backlight_on();
#endif
    rb->backlight_on();
    rb->splash(HZ, "Done");
    update_screen();
    while (1) {
        button = rb->button_get(true);
        switch (button) {
#ifdef HAVE_REMOTE_LCD
            case STATS_STOP_REMOTE:
#endif
            case STATS_STOP:
                return PLUGIN_OK;
                break;
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
                    return PLUGIN_USB_CONNECTED;
                }
                break;
        }
    }
    return PLUGIN_OK;
}
