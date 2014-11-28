/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Peter D'Hoye
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



bool its_a_dir = false;

char str_filename[MAX_PATH];
char str_dirname[MAX_PATH];
char str_size[64];
char str_dircount[64];
char str_filecount[64];
char str_date[64];
char str_time[64];

char str_title[MAX_PATH];
char str_artist[MAX_PATH];
char str_album[MAX_PATH];
char str_duration[32];

int num_properties;

static const char* props_file[] =
{
    "[Path]",       str_dirname,
    "[Filename]",   str_filename,
    "[Size]",       str_size,
    "[Date]",       str_date,
    "[Time]",       str_time,
    "[Artist]",     str_artist,
    "[Title]",      str_title,
    "[Album]",      str_album,
    "[Duration]",   str_duration,
};
static const char* props_dir[] =
{
    "[Path]",       str_dirname,
    "[Subdirs]",    str_dircount,
    "[Files]",      str_filecount,
    "[Size]",       str_size,
};

static const char human_size_prefix[4] = { '\0', 'K', 'M', 'G' };
static unsigned human_size_log(unsigned long long size)
{
    const size_t n = sizeof(human_size_prefix)/sizeof(human_size_prefix[0]);

    unsigned i;
    /* margin set at 10K boundary: 10239 B +1 => 10 KB */
    for(i=0; i < n-1 && size >= 10*1024; i++)
        size >>= 10; /* div by 1024 */

    return i;
}

static bool file_properties(char* selected_file)
{
    bool found = false;
    char tstr[MAX_PATH];
    DIR* dir;
    struct dirent* entry;
    struct mp3entry id3;

    char* ptr = rb->strrchr(selected_file, '/') + 1;
    int dirlen = (ptr - selected_file);
    rb->strlcpy(tstr, selected_file, dirlen + 1);

    dir = rb->opendir(tstr);
    if (dir)
    {
        while(0 != (entry = rb->readdir(dir)))
        {
            struct dirinfo info = rb->dir_get_info(dir, entry);
            if(!rb->strcmp(entry->d_name, selected_file+dirlen))
            {
                unsigned log;
                rb->snprintf(str_dirname, sizeof str_dirname, "%s", tstr);
                rb->snprintf(str_filename, sizeof str_filename, "%s",
                             selected_file+dirlen);
                log = human_size_log((unsigned long)info.size);
                rb->snprintf(str_size, sizeof str_size, "%lu %cB",
                             ((unsigned long)info.size) >> (log*10), human_size_prefix[log]);
                rb->snprintf(str_date, sizeof str_date, "%04d/%02d/%02d",
                    ((info.wrtdate >> 9 ) & 0x7F) + 1980, /* year    */
                    ((info.wrtdate >> 5 ) & 0x0F),        /* month   */
                    ((info.wrtdate      ) & 0x1F));       /* day     */
                rb->snprintf(str_time, sizeof str_time, "%02d:%02d",
                    ((info.wrttime >> 11) & 0x1F),        /* hour    */
                    ((info.wrttime >> 5 ) & 0x3F));       /* minutes */

                num_properties = 5;

#if (CONFIG_CODEC == SWCODEC)
                int fd = rb->open(selected_file, O_RDONLY);
                if (fd >= 0 &&
                    rb->get_metadata(&id3, fd, selected_file))
#else
                if (!rb->mp3info(&id3, selected_file))
#endif
                {
                    long dur = id3.length / 1000;           /* seconds */
                    rb->snprintf(str_artist, sizeof str_artist,
                                 "%s", id3.artist ? id3.artist : "");
                    rb->snprintf(str_title, sizeof str_title,
                                 "%s", id3.title ? id3.title : "");
                    rb->snprintf(str_album, sizeof str_album,
                                 "%s", id3.album ? id3.album : "");
                    num_properties += 3;

                    if (dur > 0)
                    {
                        if (dur < 3600)
                            rb->snprintf(str_duration, sizeof str_duration,
                                         "%d:%02d", (int)(dur / 60),
                                         (int)(dur % 60));
                        else
                            rb->snprintf(str_duration, sizeof str_duration,
                                         "%d:%02d:%02d",
                                         (int)(dur / 3600),
                                         (int)(dur % 3600 / 60),
                                         (int)(dur % 60));
                        num_properties++;
                    }
                }
#if (CONFIG_CODEC == SWCODEC)
                rb->close(fd);
#endif
                found = true;
                break;
            }
        }
        rb->closedir(dir);
    }
    return found;
}

typedef struct {
    char dirname[MAX_PATH];
    int len;
    unsigned int dc;
    unsigned int fc;
    unsigned long long bc;
} DPS;

static bool _dir_properties(DPS* dps)
{
    /* recursively scan directories in search of files
       and informs the user of the progress */
    bool result;
    static long lasttick=0;
    int dirlen;
    DIR* dir;
    struct dirent* entry;

    result = true;
    dirlen = rb->strlen(dps->dirname);
    dir = rb->opendir(dps->dirname);
    if (!dir)
        return false; /* open error */

    /* walk through the directory content */
    while(result && (0 != (entry = rb->readdir(dir))))
    {
        struct dirinfo info = rb->dir_get_info(dir, entry);
        /* append name to current directory */
        rb->snprintf(dps->dirname+dirlen, dps->len-dirlen, "/%s",
                     entry->d_name);

        if (info.attribute & ATTR_DIRECTORY)
        {
            unsigned log;

            if (!rb->strcmp((char *)entry->d_name, ".") ||
                !rb->strcmp((char *)entry->d_name, ".."))
                continue; /* skip these */

            dps->dc++; /* new directory */
            if (*rb->current_tick - lasttick > (HZ/8))
            {
                lasttick = *rb->current_tick;
                rb->lcd_clear_display();
                rb->lcd_puts(0,0,"SCANNING...");
                rb->lcd_puts(0,1,dps->dirname);
                rb->lcd_puts(0,2,entry->d_name);
                rb->lcd_putsf(0,3,"Directories: %d", dps->dc);
                rb->lcd_putsf(0,4,"Files: %d", dps->fc);
                log = human_size_log(dps->bc);
                rb->lcd_putsf(0,5,"Size: %lu %cB", (unsigned long)(dps->bc >> (10*log)),
                                                human_size_prefix[log]);
                rb->lcd_update();
            }

             /* recursion */
            result = _dir_properties(dps);
        }
        else
        {
            dps->fc++; /* new file */
            dps->bc += info.size;
        }
        if(ACTION_STD_CANCEL == rb->get_action(CONTEXT_STD,TIMEOUT_NOBLOCK))
            result = false;
        rb->yield();
    }
    rb->closedir(dir);
    return result;
}

static bool dir_properties(char* selected_file)
{
    unsigned log;
    DPS dps = {
        .len = MAX_PATH,
        .dc  = 0,
        .fc  = 0,
        .bc  = 0,
    };
    rb->strlcpy(dps.dirname, selected_file, MAX_PATH);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    if(false == _dir_properties(&dps))
        return false;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    rb->strlcpy(str_dirname, selected_file, MAX_PATH);
    rb->snprintf(str_dircount, sizeof str_dircount, "%d", dps.dc);
    rb->snprintf(str_filecount, sizeof str_filecount, "%d", dps.fc);
    log = human_size_log(dps.bc);
    rb->snprintf(str_size, sizeof str_size, "%ld %cB",
                 (long) (dps.bc >> (log*10)), human_size_prefix[log]);
    num_properties = 4;
    return true;
}

static const char * get_props(int selected_item, void* data,
                              char *buffer, size_t buffer_len)
{
    (void)data;
    if(its_a_dir)
    {
        if(selected_item >= (int)(sizeof(props_dir) / sizeof(props_dir[0])))
        {
            rb->strlcpy(buffer, "ERROR", buffer_len);
        }
        else
        {
            rb->strlcpy(buffer, props_dir[selected_item], buffer_len);
        }
    }
    else
    {
        if(selected_item >= (int)(sizeof(props_file) / sizeof(props_file[0])))
        {
            rb->strlcpy(buffer, "ERROR", buffer_len);
        }
        else
        {
            rb->strlcpy(buffer, props_file[selected_item], buffer_len);
        }
    }
    return buffer;
}

enum plugin_status plugin_start(const void* parameter)
{
    struct gui_synclist properties_lists;
    int button;
    bool quit = false, usb = false;
    char file[MAX_PATH];
    if(!parameter) return PLUGIN_ERROR;
    rb->strcpy(file, (const char *) parameter);
#ifdef HAVE_TOUCHSCREEN
    rb->touchscreen_set_mode(rb->global_settings->touch_mode);
#endif

    /* determine if it's a file or a directory */
    bool found = false;
    DIR* dir;
    struct dirent* entry;
    char* ptr = rb->strrchr(file, '/') + 1;
    int dirlen = (ptr - file);
    rb->strlcpy(str_dirname, file, dirlen + 1);

    dir = rb->opendir(str_dirname);
    if (dir)
    {
        while(0 != (entry = rb->readdir(dir)))
        {
            if(!rb->strcmp(entry->d_name, file+dirlen))
            {
                struct dirinfo info = rb->dir_get_info(dir, entry);
                its_a_dir = info.attribute & ATTR_DIRECTORY ? true : false;
                found = true;
                break;
            }
        }
        rb->closedir(dir);
    }
    /* now we know if it's a file or a dir or maybe something failed */

    if(!found)
    {
        /* weird: we couldn't find the entry. This Should Never Happen (TM) */
        rb->splashf(0, "File/Dir not found: %s", file);
        rb->action_userabort(TIMEOUT_BLOCK);
        return PLUGIN_OK;
    }

    /* get the info depending on its_a_dir */
    if(!(its_a_dir ? dir_properties(file) : file_properties(file)))
    {
        /* something went wrong (to do: tell user what it was (nesting,...) */
        rb->splash(0, "Failed to gather information");
        rb->action_userabort(TIMEOUT_BLOCK);
        return PLUGIN_OK;
    }

#ifdef HAVE_LCD_BITMAP
    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_enable(i, true, NULL);
#endif

    rb->gui_synclist_init(&properties_lists, &get_props, file, false, 2, NULL);
    rb->gui_synclist_set_title(&properties_lists, its_a_dir ?
                                                  "Directory properties" :
                                                  "File properties", NOICON);
    rb->gui_synclist_set_icon_callback(&properties_lists, NULL);
    rb->gui_synclist_set_nb_items(&properties_lists, num_properties * 2);
    rb->gui_synclist_limit_scroll(&properties_lists, true);
    rb->gui_synclist_select_item(&properties_lists, 0);
    rb->gui_synclist_draw(&properties_lists);

    while(!quit)
    {
        button = rb->get_action(CONTEXT_LIST, HZ);
        /* HZ so the status bar redraws corectly */
        if (rb->gui_synclist_do_button(&properties_lists,&button,LIST_WRAP_ON))
            continue;
        switch(button)
        {
            case ACTION_STD_CANCEL:
                quit = true;
                break;
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    quit = true;
                    usb = true;
                }
                break;
        }
    }

#ifdef HAVE_LCD_BITMAP
    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_undo(i, false);
#endif

    return usb? PLUGIN_USB_CONNECTED: PLUGIN_OK;
}
