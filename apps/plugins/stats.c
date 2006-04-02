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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

PLUGIN_HEADER

static struct plugin_api* rb;
static int files, dirs, musicfiles;
static int lasttick;
static bool abort;
#ifdef HAVE_LCD_BITMAP
static int fontwidth, fontheight;
#endif

#if CONFIG_KEYPAD == PLAYER_PAD 
#define STATS_STOP BUTTON_STOP
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD)
#define STATS_STOP BUTTON_MENU
#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define STATS_STOP BUTTON_PLAY
#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
#define STATS_STOP BUTTON_POWER
#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define STATS_STOP BUTTON_A
#else
#define STATS_STOP BUTTON_OFF
#endif
#ifdef HAVE_REMOTE_LCD
#define STATS_STOP_REMOTE BUTTON_RC_STOP
#endif

/* TODO: Better get the exts from the filetypes var in tree.c */
const char *music_exts[] = {"mp3","mp2","mp1","mpa","ogg",
        "wav","flac","ac3","a52","mpc","wv","m4a","mp4","shn",
        "aif","aiff"};

void prn(const char *str, int y)
{
#ifdef HAVE_LCD_BITMAP
    rb->lcd_putsxy(0,y,str);
#ifdef HAVE_REMOTE_LCD
    rb->lcd_remote_putsxy(0,y,str);    
#endif
#else
    rb->lcd_puts(0,y,str);   
#endif
}

void update_screen(void)
{
    char buf[15];
#ifdef HAVE_LCD_BITMAP
    rb->lcd_clear_display();
#ifdef HAVE_REMOTE_LCD
    rb->lcd_remote_clear_display();
#endif
    
    rb->snprintf(buf, sizeof(buf), "Files: %d", files);
    prn(buf,0);

    rb->snprintf(buf, sizeof(buf), "Music: %d", musicfiles);
    prn(buf,fontheight);
    
    rb->snprintf(buf, sizeof(buf), "Dirs: %d", dirs);
    prn(buf,fontheight*2);
    rb->lcd_update();
#ifdef HAVE_REMOTE_LCD
    rb->lcd_remote_update();
#endif
#else
    rb->snprintf(buf, sizeof(buf), "Files:%5d", files);
    prn(buf,0);
    rb->snprintf(buf, sizeof(buf), "Dirs: %5d", dirs);
    prn(buf,1);
#endif
}

void traversedir(char* location, char* name)
{
    int button;
    struct dirent *entry;
    DIR* dir;
    char fullpath[MAX_PATH];

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
                    files++;
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
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button;

    (void)parameter;

    rb = api;
    files = 0;
    dirs = 0;
    musicfiles = 0;
    abort = false;

#ifdef HAVE_LCD_BITMAP
    rb->lcd_getstringsize("Files: ", &fontwidth, &fontheight);
#endif
    rb->splash(HZ, true, "Counting...");
    update_screen();
    lasttick = *rb->current_tick;

    traversedir("", "");
    if (abort == true) {
        rb->splash(HZ, true, "Aborted");
        return PLUGIN_OK;
    }
    update_screen();
#ifdef HAVE_REMOTE_LCD
    rb->remote_backlight_on();
#endif
    rb->backlight_on();
    rb->splash(HZ, true, "Done");
    update_screen();
    button = rb->button_get(true);
    while (1) {
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
        return PLUGIN_OK;
    }
}
