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

MEM_FUNCTION_WRAPPERS(rb);

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

int num_properties;

static char* filesize2string(long long size, char* pstr, int len)
{
    /* margin set at 10K boundary: 10239 B +1 => 10 KB
       routine below is 200 bytes smaller than cascaded if/else :)
       not using build-in function because of huge values (long long) */
    const char* kgb[4] = { "B", "KB", "MB", "GB" };
    int i = 0;
    while(true)
    {
        if((size < 10240) || (i > 2))
        {
            /* depends on the order in the above array */
            rb->snprintf(pstr, len, "%ld %s", (long)size, kgb[i]);
            break;
        }
        size >>= 10; /* div by 1024 */
        i++;
    }
    return pstr;
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
    rb->strncpy(tstr, selected_file, dirlen);
    tstr[dirlen] = 0;

    dir = rb->opendir(tstr);
    if (dir)
    {
        while(0 != (entry = rb->readdir(dir)))
        {
            if(!rb->strcmp(entry->d_name, selected_file+dirlen))
            {
                rb->snprintf(str_dirname, sizeof str_dirname, "Path: %s",
                             tstr);
                rb->snprintf(str_filename, sizeof str_filename, "Name: %s",
                             selected_file+dirlen);
                rb->snprintf(str_size, sizeof str_size, "Size: %s",
                             filesize2string(entry->size, tstr, sizeof tstr));
                rb->snprintf(str_date, sizeof str_date, "Date: %04d/%02d/%02d",
                    ((entry->wrtdate >> 9 ) & 0x7F) + 1980, /* year    */
                    ((entry->wrtdate >> 5 ) & 0x0F),        /* month   */
                    ((entry->wrtdate      ) & 0x1F));       /* day     */
                rb->snprintf(str_time, sizeof str_time, "Time: %02d:%02d",
                    ((entry->wrttime >> 11) & 0x1F),        /* hour    */
                    ((entry->wrttime >> 5 ) & 0x3F));       /* minutes */

                num_properties = 5;

#if (CONFIG_CODEC == SWCODEC)
                int fd = rb->open(selected_file, O_RDONLY);
                if (fd >= 0 &&
                    rb->get_metadata(&id3, fd, selected_file))
#else
                if (!rb->mp3info(&id3, selected_file))
#endif
                {
                    rb->snprintf(str_artist, sizeof str_artist,
                                 "Artist: %s", id3.artist ? id3.artist : "");
                    rb->snprintf(str_title, sizeof str_title,
                                 "Title: %s", id3.title ? id3.title : "");
                    rb->snprintf(str_album, sizeof str_album,
                                 "Album: %s", id3.album ? id3.album : "");
                    num_properties += 3;
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
    long long bc;
    char tstr[64];
    char tstr2[64];
} DPS;

static bool _dir_properties(DPS* dps)
{
    /* recursively scan directories in search of files
       and informs the user of the progress */
    bool result;
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
        /* append name to current directory */
        rb->snprintf(dps->dirname+dirlen, dps->len-dirlen, "/%s",
                     entry->d_name);

        if (entry->attribute & ATTR_DIRECTORY)
        {   
            if (!rb->strcmp((char *)entry->d_name, ".") ||
                !rb->strcmp((char *)entry->d_name, ".."))
                continue; /* skip these */

            dps->dc++; /* new directory */
            rb->lcd_clear_display();
            rb->lcd_puts(0,0,"SCANNING...");
            rb->lcd_puts(0,1,dps->dirname);
            rb->lcd_puts(0,2,entry->d_name);
            rb->snprintf(dps->tstr, 64, "Directories: %d", dps->dc);
            rb->lcd_puts(0,3,dps->tstr);
            rb->snprintf(dps->tstr, 64, "Files: %d", dps->fc);
            rb->lcd_puts(0,4,dps->tstr);
            rb->snprintf(dps->tstr, 64, "Size: %s",
                filesize2string(dps->bc, dps->tstr2, 64));
            rb->lcd_puts(0,5,dps->tstr);
            rb->lcd_update();

             /* recursion */
            result = _dir_properties(dps);
        }
        else
        {   
            dps->fc++; /* new file */
            dps->bc += entry->size;
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
    DPS dps;
    char tstr[64];
    rb->strncpy(dps.dirname, selected_file, MAX_PATH);
    dps.len = MAX_PATH;
    dps.dc = 0;
    dps.fc = 0;
    dps.bc = 0;
    if(false == _dir_properties(&dps))
        return false;

    rb->snprintf(str_dirname, MAX_PATH, selected_file);
    rb->snprintf(str_dircount, sizeof str_dircount, "Subdirs: %d", dps.dc);
    rb->snprintf(str_filecount, sizeof str_filecount, "Files: %d", dps.fc);
    rb->snprintf(str_size, sizeof str_size, "Size: %s",
                 filesize2string(dps.bc, tstr, sizeof tstr));
    num_properties = 4;
    return true;
}

char * get_props(int selected_item, void* data, char *buffer)
{
    (void)data;

    switch(selected_item)
    {
        case 0:
            rb->strcpy(buffer, str_dirname);
            break;
        case 1:
            rb->strcpy(buffer, its_a_dir ? str_dircount : str_filename);
            break;
        case 2:
            rb->strcpy(buffer, its_a_dir ? str_filecount : str_size);
            break;
        case 3:
            rb->strcpy(buffer, its_a_dir ? str_size : str_date);
            break;
        case 4:
            rb->strcpy(buffer, its_a_dir ? "" : str_time);
            break;
        case 5:
            rb->strcpy(buffer, its_a_dir ? "" : str_artist);
            break;
        case 6:
            rb->strcpy(buffer, its_a_dir ? "" : str_title);
            break;
        case 7:
            rb->strcpy(buffer, its_a_dir ? "" : str_album);
            break;
        default:
            rb->strcpy(buffer, "ERROR");
            break;
    }
    return buffer;
}

enum plugin_status plugin_start(struct plugin_api* api, void* file)
{
    rb = api;
    struct gui_synclist properties_lists;
    int button;
    bool prev_show_statusbar;
    bool quit = false;

    /* determine if it's a file or a directory */
    bool found = false;
    DIR* dir;
    struct dirent* entry;
    char* ptr = rb->strrchr((char*)file, '/') + 1;
    int dirlen = (ptr - (char*)file);
    rb->strncpy(str_dirname, (char*)file, dirlen);
    str_dirname[dirlen] = 0;

    dir = rb->opendir(str_dirname);
    if (dir)
    {
        while(0 != (entry = rb->readdir(dir)))
        {
            if(!rb->strcmp(entry->d_name, file+dirlen))
            {
                its_a_dir = entry->attribute & ATTR_DIRECTORY ? true : false;
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
        rb->splash(0, "File/Dir not found: %s", (char*)file);
        rb->action_userabort(TIMEOUT_BLOCK);
        return PLUGIN_OK;
    }

    /* get the info depending on its_a_dir */
    if(!(its_a_dir ? dir_properties((char*)file):file_properties((char*)file)))
    {
        /* something went wrong (to do: tell user what it was (nesting,...) */
        rb->splash(0, "Failed to gather information");
        rb->action_userabort(TIMEOUT_BLOCK);
        return PLUGIN_OK;
    }

    /* prepare the list for the user */
    prev_show_statusbar = rb->global_settings->statusbar;
    rb->global_settings->statusbar = false;

    rb->gui_synclist_init(&properties_lists, &get_props, file, false, 1, NULL);
    rb->gui_synclist_set_title(&properties_lists, its_a_dir ?
                                                  "Directory properties" :
                                                  "File properties", NOICON);
    rb->gui_synclist_set_icon_callback(&properties_lists, NULL);
    rb->gui_synclist_set_nb_items(&properties_lists, num_properties);
    rb->gui_synclist_limit_scroll(&properties_lists, true);
    rb->gui_synclist_select_item(&properties_lists, 0);
    rb->gui_synclist_draw(&properties_lists);

    while(!quit)
    {
        button = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
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
                    rb->global_settings->statusbar = prev_show_statusbar;
                    return PLUGIN_USB_CONNECTED;
                }
        }
    }
    rb->global_settings->statusbar = prev_show_statusbar;

    return PLUGIN_OK;
}
