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
#include "errno.h"


static int removed = 0; /* number of items removed */

/* function return values */
enum tidy_return
{
    TIDY_RETURN_OK = 0,
    TIDY_RETURN_ERROR = 1,
    TIDY_RETURN_USB = 2,
    TIDY_RETURN_ABORT = 3,
};

#define MAX_TYPES 64
struct tidy_type {
    char filestring[64];
    int  pre;
    int  post;
    bool directory;
    bool remove;
} tidy_types[MAX_TYPES];
int tidy_type_count;
bool tidy_loaded_and_changed = false;

#define DEFAULT_FILES PLUGIN_APPS_DIR "/disktidy.config"
#define CUSTOM_FILES  PLUGIN_APPS_DIR "/disktidy_custom.config"

void add_item(const char* name, int index)
{
    char *a;
    struct tidy_type *entry = tidy_types + index;
    rb->strcpy(entry->filestring, name);
    if (name[rb->strlen(name)-1] == '/')
    {
        entry->directory = true;
        entry->filestring[rb->strlen(name)-1] = '\0';
    }
    else
        entry->directory = false;
    a = rb->strchr(entry->filestring, '*');
    if (a)
    {
        entry->pre = a - entry->filestring;
        entry->post = rb->strlen(a+1);
    }
    else
    {
        entry->pre = -1;
        entry->post = -1;
    }
}

static int find_file_string(const char *file, char *last_group)
{
    char temp[MAX_PATH];
    int i = 0, idx_last_group = -1;
    bool folder = false;
    rb->strcpy(temp, file);
    if (temp[rb->strlen(temp)-1] == '/')
    {
        folder = true;
        temp[rb->strlen(temp)-1] = '\0';
    }
    while (i<tidy_type_count)
    {
        if (!rb->strcmp(tidy_types[i].filestring, temp) &&
            folder == tidy_types[i].directory)
            return i;
        else if (!rb->strcmp(tidy_types[i].filestring, last_group))
            idx_last_group = i;
        i++;
    }
    /* not found, so insert it into its group */
    if (file[0] != '<' && idx_last_group != -1)
    {
        for (i=idx_last_group; i<tidy_type_count; i++)
        {
            if (tidy_types[i].filestring[0] == '<')
            {
                idx_last_group = i;
                break;
            }
        }
        /* shift items up one */
        for (i=tidy_type_count;i>idx_last_group;i--)
        {
            rb->memcpy(&tidy_types[i], &tidy_types[i-1], sizeof(struct tidy_type));
        }
        tidy_type_count++;
        add_item(file, idx_last_group+1);
        return idx_last_group+1;
    }
    return i;
}

bool tidy_load_file(const char* file)
{
    int fd = rb->open(file, O_RDONLY), i;
    char buf[MAX_PATH], *str, *remove;
    char last_group[MAX_PATH] = "";
    bool new;
    if (fd < 0)
        return false;
    while ((tidy_type_count < MAX_TYPES) && 
            rb->read_line(fd, buf, MAX_PATH))
    {
        if (rb->settings_parseline(buf, &str, &remove))
        {
            i = find_file_string(str, last_group);
            new = (i >= tidy_type_count);
            if (!rb->strcmp(remove, "yes"))
                tidy_types[i].remove = true;
            else tidy_types[i].remove = false;
            if (new)
            {
                i = tidy_type_count;
                add_item(str, i);
                tidy_type_count++;
            }
            if (str[0] == '<')
                rb->strcpy(last_group, str);
        }
    }
    rb->close(fd);
    return true;
}

static bool match(struct tidy_type *tidy_type, char *string, int len)
{
    char *pattern = tidy_type->filestring;
    if (tidy_type->pre < 0)
    {
        /* no '*', just compare. */
        return (rb->strcmp(pattern, string) == 0);
    }
    /* pattern is too long for the string. avoid 'ab*bc' matching 'abc'. */
    if (len < tidy_type->pre + tidy_type->post)
        return false;
    /* pattern has '*', compare former part of '*' to the begining of
       the string and compare next part of '*' to the end of string. */
    return (rb->strncmp(pattern, string, tidy_type->pre) == 0 &&
            rb->strcmp(pattern + tidy_type->pre + 1,
                        string + len - tidy_type->post) == 0);
}

bool tidy_remove_item(char *item, int attr)
{
    int i;
    int len;
    bool ret = false;
    len = rb->strlen(item);
    for (i=0; ret == false && i < tidy_type_count; i++)
    {
        if (match(&tidy_types[i], item, len))
        {
            if (!tidy_types[i].remove)
                return false;
            if (attr&ATTR_DIRECTORY)
                ret = tidy_types[i].directory;
            else
                ret = !tidy_types[i].directory;
        }
    }
    return ret;
}

void tidy_lcd_status(const char *name)
{
    /* display status text */
    rb->lcd_clear_display();
    rb->lcd_puts(0, 0, "Working ...");
    rb->lcd_puts(0, 1, name);
#ifdef HAVE_LCD_BITMAP
    rb->lcd_putsf(0, 2, "Cleaned up %d items", removed);
#endif
    rb->lcd_update();
}

int tidy_path_append_entry(char *path, struct dirent *entry, int *path_length)
{
    int name_len = rb->strlen(entry->d_name);
    /* for the special case of path="/" this is one bigger but it's not a problem */
    int new_length = *path_length + name_len + 1;
    
    /* check overflow (keep space for trailing zero) */
    if(new_length >= MAX_PATH)
        return 0;
    
    /* special case for path <> "/" */
    if(rb->strcmp(path, "/") != 0)
    {
        rb->strcat(path + *path_length, "/");
        (*path_length)++;
    }
    /* strcat is unsafe but the previous check normally avoid any problem */
    /* use path_length to optimise */
    
    rb->strcat(path + *path_length, entry->d_name);
    *path_length += name_len;
    
    return 1;
}

void tidy_path_remove_entry(char *path, int old_path_length, int *path_length)
{
    path[old_path_length] = '\0';
    *path_length = old_path_length;
}

/* Removes the directory specified by 'path'. This includes recursively
   removing all files and directories in that directory.
   path is assumed to be array of size MAX_PATH.
*/
enum tidy_return tidy_removedir(char *path, int *path_length)
{
    /* delete directory */
    struct dirent *entry;
    enum tidy_return status = TIDY_RETURN_OK;
    int button;
    DIR *dir;
    int old_path_length = *path_length;

    /* display status text */
    tidy_lcd_status(path);

    rb->yield();

    dir = rb->opendir(path);
    if (dir)
    {
        while((status == TIDY_RETURN_OK) && ((entry = rb->readdir(dir)) != 0))
        /* walk directory */
        {
            /* check for user input and usb connect */
            button = rb->get_action(CONTEXT_STD, TIMEOUT_NOBLOCK);
            if (button == ACTION_STD_CANCEL)
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
            /* returns an error if path is too long */
            if(!tidy_path_append_entry(path, entry, path_length))
                /* silent error */
                continue;

            struct dirinfo info = rb->dir_get_info(dir, entry);
            if (info.attribute & ATTR_DIRECTORY)
            {
                /* dir ignore "." and ".." */
                if ((rb->strcmp(entry->d_name, ".") != 0) && \
                    (rb->strcmp(entry->d_name, "..") != 0))
                {
                    status = tidy_removedir(path, path_length);
                }
            }
            else
            {
                /* file */
                removed++;
                rb->remove(path);
            }

            /* restore path */
            tidy_path_remove_entry(path, old_path_length, path_length);
        }
        rb->closedir(dir);
        /* rmdir */
        removed++;
        rb->rmdir(path);
    }
    else
    {
        status = TIDY_RETURN_ERROR;
    }
    return status;
}

/* path is assumed to be array of size MAX_PATH */
enum tidy_return tidy_clean(char *path, int *path_length)
{
    /* deletes junk files and dirs left by system */
    struct dirent *entry;
    enum tidy_return status = TIDY_RETURN_OK;
    int button;
    DIR *dir;
    int old_path_length = *path_length;

    /* display status text */
    tidy_lcd_status(path);

    rb->yield();

    dir = rb->opendir(path);
    if (dir)
    {
        while((status == TIDY_RETURN_OK) && ((entry = rb->readdir(dir)) != 0))
        /* walk directory */
        {
            struct dirinfo info = rb->dir_get_info(dir, entry);
            /* check for user input and usb connect */
            button = rb->get_action(CONTEXT_STD, TIMEOUT_NOBLOCK);
            if (button == ACTION_STD_CANCEL)
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

            if (info.attribute & ATTR_DIRECTORY)
            {
                /* directory ignore "." and ".." */
                if ((rb->strcmp(entry->d_name, ".") != 0) && \
                    (rb->strcmp(entry->d_name, "..") != 0))
                {
                    /* get absolute path */
                    /* returns an error if path is too long */
                    if(!tidy_path_append_entry(path, entry, path_length))
                        /* silent error */
                        continue;

                    if (tidy_remove_item(entry->d_name, info.attribute))
                    {
                        /* delete dir */
                        status = tidy_removedir(path, path_length);
                    }
                    else
                    {
                        /* dir not deleted so clean it */
                        status = tidy_clean(path, path_length);
                    }
                    
                    /* restore path */
                    tidy_path_remove_entry(path, old_path_length, path_length);
                }
            }
            else
            {
                /* file */
                if (tidy_remove_item(entry->d_name, info.attribute))
                {
                    /* get absolute path */
                    /* returns an error if path is too long */
                    if(!tidy_path_append_entry(path, entry, path_length))
                        /* silent error */
                        continue;

                    removed++; /* increment removed files counter */
                    /* delete file */
                    if (rb->remove(path) != 0)
                        DEBUGF("Could not delete file %s\n", path);

                    /* restore path */
                    tidy_path_remove_entry(path, old_path_length, path_length);
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

enum tidy_return tidy_do(void)
{
    /* clean disk and display num of items removed */
    enum tidy_return status;
    char path[MAX_PATH];
    int path_length;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    rb->strcpy(path, "/");
    path_length = rb->strlen(path);
    status = tidy_clean(path, &path_length);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    if ((status == TIDY_RETURN_OK) || (status == TIDY_RETURN_ABORT))
    {
        rb->lcd_clear_display();
        if (status == TIDY_RETURN_ABORT)
        {
            rb->splash(HZ, "User aborted");
            rb->lcd_clear_display();
        }
        rb->splashf(HZ*2, "Cleaned up %d items", removed);
    }
    return status;
}

enum themable_icons get_icon(int item, void * data)
{
    (void)data;
    if (tidy_types[item].filestring[0] == '<') /* special type */
        return Icon_Folder;
    else if (tidy_types[item].remove)
        return Icon_Cursor;
    else
        return Icon_NOICON;
}

static const char* get_name(int selected_item, void * data,
                            char * buffer, size_t buffer_len)
{
    (void)data;
    if (tidy_types[selected_item].directory)
    {
        rb->snprintf(buffer, buffer_len, "%s/",
                     tidy_types[selected_item].filestring);
        return buffer;
    }
    return tidy_types[selected_item].filestring;
}

int list_action_callback(int action, struct gui_synclist *lists)
{
    if (action == ACTION_STD_OK)
    {
        int selection = rb->gui_synclist_get_sel_pos(lists);
        if (tidy_types[selection].filestring[0] == '<')
        {
            int i;
            if (!rb->strcmp(tidy_types[selection].filestring, "< ALL >"))
            {
                for (i=0; i<tidy_type_count; i++)
                {
                    if (tidy_types[i].filestring[0] != '<')
                        tidy_types[i].remove = true;
                }
            }
            else if (!rb->strcmp(tidy_types[selection].filestring, "< NONE >"))
            {
                for (i=0; i<tidy_type_count; i++)
                {
                    if (tidy_types[i].filestring[0] != '<')
                        tidy_types[i].remove = false;
                }
            }
            else /* toggle all untill the next <> */
            {
                selection++;
                while (selection < tidy_type_count &&
                       tidy_types[selection].filestring[0] != '<')
                {
                    tidy_types[selection].remove = !tidy_types[selection].remove;
                    selection++;
                }
            }
        }
        else
            tidy_types[selection].remove = !tidy_types[selection].remove;
        tidy_loaded_and_changed = true;
        return ACTION_REDRAW;
    }
    return action;
}

enum tidy_return tidy_lcd_menu(void)
{
    int selection = 0;
    enum tidy_return status = TIDY_RETURN_OK;
    bool menu_quit = false;

    MENUITEM_STRINGLIST(menu, "Disktidy Menu", NULL,
                        "Start Cleaning", "Files to Clean",
                        "Quit");

    while (!menu_quit)
    {
        switch(rb->do_menu(&menu, &selection, NULL, false))
        {
            case 0:
                menu_quit = true;   /* start cleaning */
                break;

            case 1:
            {
                struct simplelist_info list;
                rb->simplelist_info_init(&list, "Files to Clean",
                                         tidy_type_count, NULL);
                list.get_icon = get_icon;
                list.get_name = get_name;
                list.action_callback = list_action_callback;
                rb->simplelist_show_list(&list);
            }
            break;

            default:
                status = TIDY_RETURN_ABORT; /* exit plugin */
                menu_quit = true;
                break;
        }
    }
    return status;
}

/* Creates a file and writes information about what files to
   delete and what to keep to it.
   Returns true iff the file was successfully created.
*/
static bool save_config(const char *file_name)
{
    int fd, i;
    bool result;

    fd = rb->creat(file_name, 0666);
    result = (fd >= 0);

    if (result)
    {
        for (i=0; i<tidy_type_count; i++)
        {
	    rb->fdprintf(fd, "%s%s: %s\n", tidy_types[i].filestring,
	                 tidy_types[i].directory ? "/" : "",
	                 tidy_types[i].remove ? "yes" : "no");
        }
        rb->close(fd);
    }
    else
    {
        DEBUGF("Could not create file %s\n", file_name);
    }

    return result;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    enum tidy_return status;
    (void)parameter;

    tidy_type_count = 0;
    tidy_load_file(DEFAULT_FILES);
    tidy_load_file(CUSTOM_FILES);
    if (tidy_type_count == 0)
    {
        rb->splash(3*HZ, "Missing disktidy.config file");
        return PLUGIN_ERROR;
    }
    status = tidy_lcd_menu();
    if (tidy_loaded_and_changed)
    {
        save_config(CUSTOM_FILES);
    }
    if (status == TIDY_RETURN_ABORT)
        return PLUGIN_OK;
    while (true)
    {
            status = tidy_do();

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

    if (rb->default_event_handler(rb->button_get(false)) == SYS_USB_CONNECTED)
        return PLUGIN_USB_CONNECTED;

    rb->yield();

    return PLUGIN_OK;
}
