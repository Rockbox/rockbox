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
static bool abort;

struct tidy_type {
    char filestring[64];
    int  pre;
    int  post;
    bool directory;
    bool remove;
} tidy_types[64];

static size_t tidy_type_count;

bool tidy_loaded_and_changed = false;

#define DEFAULT_FILES PLUGIN_APPS_DATA_DIR "/disktidy.config"
#define CUSTOM_FILES  PLUGIN_APPS_DATA_DIR "/disktidy_custom.config"

static void add_item(const char* name, int index)
{
    struct tidy_type *entry = &tidy_types[index];
    rb->strcpy(entry->filestring, name);
    if (name[rb->strlen(name)-1] == '/')
    {
        entry->directory = true;
        entry->filestring[rb->strlen(name)-1] = '\0';
    }
    else
        entry->directory = false;

    char *a = rb->strchr(entry->filestring, '*');
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
    int idx_last_group = -1;
    bool folder = false;
    rb->strcpy(temp, file);
    if (temp[rb->strlen(temp)-1] == '/')
    {
        folder = true;
        temp[rb->strlen(temp)-1] = '\0';
    }

    for (unsigned i = 0; i < tidy_type_count; i++)
        if (!rb->strcmp(tidy_types[i].filestring, temp) && folder == tidy_types[i].directory)
            return i;
        else if (!rb->strcmp(tidy_types[i].filestring, last_group))
            idx_last_group = i;

    if (file[0] == '<' || idx_last_group == -1)
        return tidy_type_count;


    /* not found, so insert it into its group */
    for (unsigned i=idx_last_group; i<tidy_type_count; i++)
        if (tidy_types[i].filestring[0] == '<')
        {
            idx_last_group = i;
            break;
        }

    /* shift items up one */
    for (int i=tidy_type_count;i>idx_last_group;i--)
        rb->memcpy(&tidy_types[i], &tidy_types[i-1], sizeof(struct tidy_type));

    tidy_type_count++;
    add_item(file, idx_last_group+1);
    return idx_last_group+1;
}

static void tidy_load_file(const char* file)
{
    int fd = rb->open(file, O_RDONLY);
    char buf[MAX_PATH], *str, *remove;
    char last_group[MAX_PATH] = "";
    if (fd < 0)
        return;

    while ((tidy_type_count < sizeof(tidy_types) / sizeof(tidy_types[0])) && rb->read_line(fd, buf, MAX_PATH))
    {
        if (!rb->settings_parseline(buf, &str, &remove))
            continue;

        if (*str == '\\') /* escape first character ? */
            str++;
        unsigned i = find_file_string(str, last_group);

        tidy_types[i].remove = rb->strcmp(remove, "yes");

        if (i >= tidy_type_count)
        {
            i = tidy_type_count;
            add_item(str, i);
            tidy_type_count++;
        }
        if (str[0] == '<')
            rb->strcpy(last_group, str);
    }
    rb->close(fd);
}

static bool match(struct tidy_type *tidy_type, const char *string, int len)
{
    char *pattern = tidy_type->filestring;

    if (tidy_type->pre < 0) /* no '*', just compare. */
        return !rb->strcmp(pattern, string);

    /* pattern is too long for the string. avoid 'ab*bc' matching 'abc'. */
    if (len < tidy_type->pre + tidy_type->post)
        return false;

    /* pattern has '*', compare former part of '*' to the begining of
       the string and compare next part of '*' to the end of string. */
    return !rb->strncmp(pattern, string, tidy_type->pre) &&
           !rb->strcmp(pattern + tidy_type->pre + 1, string + len - tidy_type->post);
}

static bool tidy_remove_item(const char *item, int attr)
{
    for (struct tidy_type *t = &tidy_types[0]; t < &tidy_types[tidy_type_count]; t++)
        if (match(t, item, rb->strlen(item)))
            return t->remove && ((!!(attr&ATTR_DIRECTORY)) == t->directory);

    return false;
}

static void tidy_lcd_status(const char *name)
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

static int tidy_path_append_entry(char *path, struct dirent *entry, int *path_length)
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

static void tidy_path_remove_entry(char *path, int old_path_length, int *path_length)
{
    path[old_path_length] = '\0';
    *path_length = old_path_length;
}

/* path is assumed to be array of size MAX_PATH. */
static enum plugin_status tidy_clean(char *path, int *path_length, bool rmdir)
{
    int old_path_length = *path_length;

    tidy_lcd_status(path);

    DIR *dir = rb->opendir(path);
    if (!dir)
        return PLUGIN_ERROR;

    struct dirent *entry;
    while ((entry = rb->readdir(dir)))
    {
        /* check for user input and usb connect */
        int button = rb->get_action(CONTEXT_STD, TIMEOUT_NOBLOCK);
        if (button == ACTION_STD_CANCEL)
        {
            rb->closedir(dir);
            abort = true;
            return PLUGIN_OK;
        }
        if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
        {
            rb->closedir(dir);
            return PLUGIN_USB_CONNECTED;
        }

        rb->yield();

        struct dirinfo info = rb->dir_get_info(dir, entry);
        if (!rmdir && !tidy_remove_item(entry->d_name, info.attribute))
            continue;

        /* get absolute path, returns an error if path is too long */
        if(!tidy_path_append_entry(path, entry, path_length))
            continue; /* silent error */

        if (info.attribute & ATTR_DIRECTORY)
        {
            /* dir ignore "." and ".." */
            if (rb->strcmp(entry->d_name, ".") && rb->strcmp(entry->d_name, ".."))
                tidy_clean(path, path_length, true);
        }
        else
        {
            removed++;
            rb->remove(path);
        }

        /* restore path */
        tidy_path_remove_entry(path, old_path_length, path_length);
    }
    rb->closedir(dir);

    if (rmdir)
    {
        removed++;
        rb->rmdir(path);
    }

    return PLUGIN_OK;
}

static enum plugin_status tidy_do(void)
{
    /* clean disk and display num of items removed */
    char path[MAX_PATH];

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    rb->strcpy(path, "/");
    int path_length = rb->strlen(path);
    enum plugin_status status = tidy_clean(path, &path_length, false);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    if (status == PLUGIN_OK)
    {
        rb->lcd_clear_display();
        if (abort)
        {
            rb->splash(HZ, "User aborted");
            rb->lcd_clear_display();
        }
        rb->splashf(HZ*2, "Cleaned up %d items", removed);
    }
    return status;
}

static enum themable_icons get_icon(int item, void * data)
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

static int list_action_callback(int action, struct gui_synclist *lists)
{
    if (action != ACTION_STD_OK)
        return action;

    unsigned selection = rb->gui_synclist_get_sel_pos(lists);
    if (tidy_types[selection].filestring[0] == '<')
    {
        bool all = !rb->strcmp(tidy_types[selection].filestring, "< ALL >");
        bool none= !rb->strcmp(tidy_types[selection].filestring, "< NONE >");

        if (all || none)
        {
            for (unsigned i=0; i<tidy_type_count; i++)
                if (tidy_types[i].filestring[0] != '<')
                    tidy_types[i].remove = all;
        }
        else /* toggle all untill the next <> */
            while (++selection < tidy_type_count && tidy_types[selection].filestring[0] != '<')
                tidy_types[selection].remove = !tidy_types[selection].remove;
    }
    else
        tidy_types[selection].remove = !tidy_types[selection].remove;
    tidy_loaded_and_changed = true;
    return ACTION_REDRAW;
}

static void tidy_lcd_menu(void)
{
    int selection = 0;
    struct simplelist_info list;

    MENUITEM_STRINGLIST(menu, "Disktidy Menu", NULL, "Start Cleaning",
                        "Files to Clean", "Quit");

    for(;;)
        switch(rb->do_menu(&menu, &selection, NULL, false))
        {
        default:
            abort = true;
        case 0:
            return; /* start cleaning */

        case 1:
            rb->simplelist_info_init(&list, "Files to Clean", tidy_type_count, NULL);
            list.get_icon = get_icon;
            list.get_name = get_name;
            list.action_callback = list_action_callback;
            rb->simplelist_show_list(&list);
            break;
        }
}

/* Creates a file and writes information about what files to
   delete and what to keep to it.
*/
static void save_config(void)
{
    int fd = rb->creat(CUSTOM_FILES, 0666);
    if (fd < 0)
        return;

    for (unsigned i=0; i<tidy_type_count; i++)
        rb->fdprintf(fd, "%s%s%s: %s\n", 
                     tidy_types[i].filestring[0] == '#' ? "\\" : "",
                     tidy_types[i].filestring,
                     tidy_types[i].directory ? "/" : "",
                     tidy_types[i].remove ? "yes" : "no");
    rb->close(fd);
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

    tidy_load_file(DEFAULT_FILES);
    tidy_load_file(CUSTOM_FILES);
    if (tidy_type_count == 0)
    {
        rb->splash(3*HZ, "Missing disktidy.config file");
        return PLUGIN_ERROR;
    }
    tidy_lcd_menu();
    if (tidy_loaded_and_changed)
        save_config();

    return abort ? PLUGIN_OK : tidy_do();
}
