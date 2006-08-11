/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 David Dent
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

/* function return values */
enum tidy_return
{
    TIDY_RETURN_OK = 0,
    TIDY_RETURN_ERROR = 1,
    TIDY_RETURN_USB = 2,
    TIDY_RETURN_ABORT = 3,
};

/* Which systems junk are we removing */
enum tidy_system
{
    TIDY_MAC = 0,
    TIDY_WIN = 1,
    TIDY_BOTH = 2,
};

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define TIDY_DO BUTTON_ON
#define TIDY_STOP BUTTON_OFF
#define TIDY_LEFT BUTTON_LEFT
#define TIDY_RIGHT BUTTON_RIGHT

#elif CONFIG_KEYPAD == ONDIO_PAD
#define TIDY_DO BUTTON_MENU
#define TIDY_STOP BUTTON_OFF
#define TIDY_LEFT BUTTON_LEFT
#define TIDY_RIGHT BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define TIDY_DO BUTTON_SELECT
#define TIDY_STOP BUTTON_OFF
#define TIDY_LEFT BUTTON_LEFT
#define TIDY_RIGHT BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_4G_PAD)
#define TIDY_DO BUTTON_SELECT
#define TIDY_STOP BUTTON_MENU
#define TIDY_LEFT BUTTON_LEFT
#define TIDY_RIGHT BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define TIDY_DO BUTTON_PLAY
#define TIDY_STOP BUTTON_POWER
#define TIDY_LEFT BUTTON_LEFT
#define TIDY_RIGHT BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define TIDY_DO BUTTON_PLAY
#define TIDY_STOP BUTTON_POWER
#define TIDY_LEFT BUTTON_LEFT
#define TIDY_RIGHT BUTTON_RIGHT

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define TIDY_DO BUTTON_SELECT
#define TIDY_STOP BUTTON_A
#define TIDY_LEFT BUTTON_LEFT
#define TIDY_RIGHT BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define TIDY_DO BUTTON_PLAY
#define TIDY_STOP BUTTON_POWER
#define TIDY_LEFT BUTTON_LEFT
#define TIDY_RIGHT BUTTON_RIGHT

#else
  #error DISKTIDY: Unsupported keypad
#endif


void tidy_lcd_status(const char *name, int *removed)
{
    char text[24]; /* "Cleaned up nnnnn items" */

    /* display status text */
    rb->lcd_clear_display();
    rb->lcd_puts(0, 0, "Working ...");
    rb->lcd_puts(0, 1, name);
    rb->snprintf(text, 24, "Cleaned up %d items", *removed);
    rb->lcd_puts(0, 2, text);
    rb->lcd_update();
}

void tidy_get_absolute_path(struct dirent *entry, char *fullname, 
                            const char* name)
{
    /* gets absolute path using dirent and name */
    rb->strcpy(fullname, name);
    if (rb->strlen(name) > 1)
    {
        rb->strcat(fullname, "/");
    }
    rb->strcat(fullname, entry->d_name);
}

enum tidy_return tidy_removedir(const char *name, int *removed)
{
    /* delete directory */
    struct dirent *entry;
    enum tidy_return status = TIDY_RETURN_OK;
    int button;
    DIR *dir;
    char fullname[MAX_PATH];
    
    /* display status text */
    tidy_lcd_status(name, removed);
    
    rb->yield();
    
    dir = rb->opendir(name);
    if (dir)
    {
        while((status == TIDY_RETURN_OK) && ((entry = rb->readdir(dir)) != 0))
        /* walk directory */
        {
            /* check for user input and usb connect */
            button = rb->button_get(false);
            if (button == TIDY_STOP)
            {
                rb->closedir(dir);
                return TIDY_RETURN_ABORT;
            }
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
		        {
		            rb->closedir(dir);
    			      return TIDY_RETURN_USB;
            }
            
            rb->yield();
            
            /* get absolute path */
            tidy_get_absolute_path(entry, fullname, name);
            
            if (entry->attribute & ATTR_DIRECTORY)
            {
                /* dir ignore "." and ".." */
                if ((rb->strcmp(entry->d_name, ".") != 0) && \
                    (rb->strcmp(entry->d_name, "..") != 0))
                {
                    tidy_removedir(fullname, removed);
                }
            }
            else
            {
                /* file */
                *removed += 1;
                rb->remove(fullname);
            }
        }
        rb->closedir(dir);
        /* rmdir */
        *removed += 1;
        rb->rmdir(name);
    }
    else
    {
        status = TIDY_RETURN_ERROR;
    }
    return status;
}

enum tidy_return tidy_clean(const char *name, int *removed, \
                            enum tidy_system system)
{
    /* deletes junk files and dirs left by system */
    struct dirent *entry;
    enum tidy_return status = TIDY_RETURN_OK;
    int button;
    int del; /* has the item been deleted */
    DIR *dir;
    char fullname[MAX_PATH];
        
    /* display status text */
    tidy_lcd_status(name, removed);
    
    rb->yield();
    
    dir = rb->opendir(name);
    if (dir)
    {
        while((status == TIDY_RETURN_OK) && ((entry = rb->readdir(dir)) != 0))
        /* walk directory */
        {
            /* check for user input and usb connect */
            button = rb->button_get(false);
            if (button == TIDY_STOP)
            {
                rb->closedir(dir);
                return TIDY_RETURN_ABORT;
            }
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
		        {
		            rb->closedir(dir);
    			      return TIDY_RETURN_USB;
            }
            
            rb->yield();
            
            if (entry->attribute & ATTR_DIRECTORY)
            {
                /* directory ignore "." and ".." */
                if ((rb->strcmp(entry->d_name, ".") != 0) && \
                    (rb->strcmp(entry->d_name, "..") != 0))
                {
                    del = 0;
                    
                    /* get absolute path */
                    tidy_get_absolute_path(entry, fullname, name);
                    
                    /* check if we are in root directory "/" */
                    if (rb->strcmp(name, "/") == 0)
                    {
                        if ((system == TIDY_MAC) || (system == TIDY_BOTH))
                        {
                            /* mac directories */
                            if (rb->strcmp(entry->d_name, ".Trashes") == 0)
                            {
                                /* delete dir */
                                tidy_removedir(fullname, removed);
                                del = 1;
                            }
                        }
                        
                        if (del == 0)
                        {
                            if ((system == TIDY_WIN) || (system == TIDY_BOTH))
                            {
                                /* windows directories */
                                if (rb->strcmp(entry->d_name, "Recycled") == 0 \
                || rb->strcmp(entry->d_name, "System Volume Information") == 0)
                                {
                                    /* delete dir */
                                    tidy_removedir(fullname, removed);
                                    del = 1;
                                }
                            }
                        }
                    }
                    
                    if (del == 0)
                    {
                        /* dir not deleted so clean it */
                        status = tidy_clean(fullname, removed, system);
                    }
                }
            }
            else
            {
                /* file */
                del = 0;
                
                if ((system == TIDY_MAC) || (system == TIDY_BOTH))
                {
                    /* remove mac files */
                    if ((rb->strcmp(entry->d_name, ".DS_Store") == 0) || \
                        (rb->strncmp(entry->d_name, "._", 2) == 0))
                    {
                        *removed += 1; /* increment removed files counter */
                        
                        /* get absolute path */
                        char fullname[MAX_PATH];
                        tidy_get_absolute_path(entry, fullname, name);
                        
                        /* delete file */
                        rb->remove(fullname);
                        del = 1;
                    }
                }
                
                if (del == 0)
                {
                    if ((system == TIDY_WIN) || (system == TIDY_BOTH))
                    {
                        /* remove windows files*/
                        if ((rb->strcmp(entry->d_name, "Thumbs.db") == 0))
                        {
                            *removed += 1; /* increment removed files counter */
                            
                            /* get absolute path */
                            char fullname[MAX_PATH];
                            tidy_get_absolute_path(entry, fullname, name);
                            
                            /* delete file */
                            rb->remove(fullname);
                            del = 1;
                        }
                    }                    
                }
            }  
        }
        rb->closedir(dir);
        return status;
    }
    else
    {
        return TIDY_RETURN_ERROR;
    }
}

enum plugin_status tidy_do(enum tidy_system system)
{
    /* clean disk and display num of items removed */
    int removed = 0;
    enum tidy_return status;
    char text[24]; /* "Cleaned up nnnnn items" */
    
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    
    status = tidy_clean("/", &removed, system);
    
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    
    if ((status == TIDY_RETURN_OK) || (status == TIDY_RETURN_ABORT))
    {
        rb->lcd_clear_display();
        rb->snprintf(text, 24, "Cleaned up %d items", removed);
        if (status == TIDY_RETURN_ABORT)
        {
            rb->splash(HZ, true, "User aborted");
            rb->lcd_clear_display();
        }
        rb->splash(HZ*2, true, text);
    }
    return status;
}

void tidy_lcd_menu(enum tidy_system system)
{
    /* show menu text */
    rb->lcd_clear_display();
    
   /* show keys */
#if CONFIG_KEYPAD == RECORDER_PAD
    rb->lcd_puts(0, 0, "[On] to clean up");
    rb->lcd_puts(0, 1, "[Off] to exit/abort");
#elif CONFIG_KEYPAD == ONDIO_PAD
    rb->lcd_puts(0, 0, "[Menu] to clean up");
    rb->lcd_puts(0, 1, "[Off] to exit/abort");
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
    rb->lcd_puts(0, 0, "[Navi] to clean up");
    rb->lcd_puts(0, 1, "[Off] to exit/abort");
#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_4G_PAD)
    rb->lcd_puts(0, 0, "[Select] to clean up");
    rb->lcd_puts(0, 1, "[Menu] to exit/abort");
#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
    rb->lcd_puts(0, 0, "[Play] to clean up");
    rb->lcd_puts(0, 1, "[Power] to exit/abort");
#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
    rb->lcd_puts(0, 0, "[Select] to clean up");
    rb->lcd_puts(0, 1, "[A] to exit/abort");
#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
    rb->lcd_puts(0, 0, "[Play] to clean up");
    rb->lcd_puts(0, 1, "[Power] to exit/abort");
#else
    #error DISKTIDY: Unsupported model
#endif
    
    /* show selcted system */
    switch (system)
    {
        case TIDY_MAC:
            rb->lcd_puts(0, 2, "< Mac >");
            rb->lcd_puts(0, 3, "Deletes");
            rb->lcd_puts(0, 4, "._*  .DS_Store");
            rb->lcd_puts(0, 5, "/.Trashes/");
            break;
        case TIDY_WIN:
            rb->lcd_puts(0, 2, "< Win >");
            rb->lcd_puts(0, 3, "Deletes");
            rb->lcd_puts(0, 4, "Thumbs.db  /Recycled/");
            rb->lcd_puts(0, 5, "/System Volume Information/");
            break;
        case TIDY_BOTH:
            rb->lcd_puts(0, 2, "< Both >");
            break;
       
    }
    rb->lcd_update();
}

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    enum tidy_system system = TIDY_MAC;
    enum tidy_return status;
    int button;

    (void)parameter;

    rb = api;

    tidy_lcd_menu(system);
    
    while (1)
    {
        button = rb->button_get(false);
        
        if (button == TIDY_LEFT)
        {
            if (system == 0)
            {
                system = 2;
            }
            else
            {
                system --;
            }
            tidy_lcd_menu(system);
        }
        
        if (button == TIDY_RIGHT)
        {
            if (system == 2)
            {
                system = 0;
            }
            else
            {
                system ++;
            }
            tidy_lcd_menu(system);
        }
        
        if (button == TIDY_DO)
        {
        
            status = tidy_do(system);

            switch (status)
            {
                case TIDY_RETURN_OK:
                    return PLUGIN_OK;
                case TIDY_RETURN_ERROR:
                    return PLUGIN_ERROR;
                case TIDY_RETURN_USB:
                   return PLUGIN_USB_CONNECTED;
                case TIDY_RETURN_ABORT:
                    return PLUGIN_OK;                
            }
        }
        
        if (button == TIDY_STOP)
        {
            return PLUGIN_OK;
        }
        
        if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
        {
            return PLUGIN_USB_CONNECTED;
        }
        
        rb->yield();
    }
    
    return PLUGIN_OK;
}
