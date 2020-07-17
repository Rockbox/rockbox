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
#include "lib/playback_control.h"
#include "lib/display_text.h"

#define DEFAULT_FILES PLUGIN_APPS_DATA_DIR "/disktidy.config"
#define CUSTOM_FILES  PLUGIN_APPS_DATA_DIR "/disktidy_custom.config"
#define LAST_RUN_STATS_FILE PLUGIN_APPS_DATA_DIR "/disktidy.stats"
#define DIR_STACK_SIZE 25

struct dir_info {
    DIR *dir;
    int path_length;
    long size;
};

/* Store directory info when traversing file system */
struct dir_stack {
    struct dir_info dirs[DIR_STACK_SIZE];
    int size;
};

struct run_statistics {
    int files_removed; /* Number of files removed */
    int dirs_removed; /* Number of directories removed */
    int run_duration; /* Duration of last run in seconds */
    double removed_size; /* Size of items removed */
#if CONFIG_RTC
    struct tm last_run_time; /* Last time disktidy was run */
#endif
};

struct tidy_type {
    char filestring[64];
    int  pre;
    int  post;
    bool directory;
    bool remove;
} tidy_types[64];

static struct run_statistics run_stats;
static size_t tidy_type_count;
static bool user_abort;
static bool tidy_loaded_and_changed = false;
static bool stats_file_exists = false;

static void dir_stack_init(struct dir_stack *dstack)
{
    dstack->size = 0;
}

static inline int dir_stack_size(struct dir_stack *dstack)
{
    return dstack->size;
}

static inline bool dir_stack_push(struct dir_stack *dstack, struct dir_info dinfo)
{
    if (dstack->size == DIR_STACK_SIZE) {
        return false;
    }

    dstack->dirs[dstack->size++] = dinfo;
    return true;
}

static inline bool dir_stack_pop(struct dir_stack *dstack, struct dir_info *dinfo)
{
    if (dstack->size == 0) {
        return false;
    }

    *dinfo = dstack->dirs[--dstack->size];
    return true;
}

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

static bool save_run_stats(void)
{
    int fd = rb->open(LAST_RUN_STATS_FILE, O_WRONLY|O_CREAT, 0666);

    if (fd < 0) {
        return false;
    }

    bool save_success = rb->write(fd, &run_stats,
        sizeof(struct run_statistics)) > 0;

    rb->close(fd);

    return save_success;
}

static bool load_run_stats(void)
{
    int fd = rb->open(LAST_RUN_STATS_FILE, O_RDONLY);

    if (fd < 0) {
        return false;
    }

    bool load_success = rb->read(fd, &run_stats,
        sizeof(struct run_statistics)) == sizeof(struct run_statistics);

    rb->close(fd);

    return load_success;
}

static enum plugin_status display_run_stats(void)
{
    if (!load_run_stats()) {
        rb->splash(HZ * 2, "Unable to load last run stats");
        return PLUGIN_OK;
    }

#if CONFIG_RTC
    static const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
#endif
    static const char *size_units[] = {
        "B", "KB", "MB", "GB", "TB", "PB"
    };

    int magnitude = 0;
    double rm_size = run_stats.removed_size;

    while (rm_size >= 1000) {
        rm_size /= 1024;
        magnitude++;
    }

    char total_removed[8];
    rb->snprintf(total_removed, sizeof(total_removed), "%d",
                 run_stats.files_removed + run_stats.dirs_removed);

    char files_removed[8];
    rb->snprintf(files_removed, sizeof(files_removed), "%d",
                 run_stats.files_removed);

    char dirs_removed[8];
    rb->snprintf(dirs_removed, sizeof(dirs_removed), "%d",
                 run_stats.dirs_removed);

    char removed_size[9];
    rb->snprintf(removed_size, sizeof(removed_size), "%d.%d%s",
                 (int)rm_size, (int)((rm_size - (int)rm_size) * 100),
                 size_units[magnitude]);

    char run_time[9];
    rb->snprintf(run_time, sizeof(run_time), "%02d:%02d:%02d",
                 run_stats.run_duration / 3600, run_stats.run_duration / 60,
                 run_stats.run_duration % 60);

#if CONFIG_RTC
    char last_run[18];
    rb->snprintf(last_run, sizeof(last_run), "%02d:%02d %d/%s/%d",
                 run_stats.last_run_time.tm_hour,
                 run_stats.last_run_time.tm_min, run_stats.last_run_time.tm_mday,
                 months[run_stats.last_run_time.tm_mon],
                 2000 + (run_stats.last_run_time.tm_year % 100));
#endif

    char* last_run_text[] = {
        "Last Run Stats" , ""           , "",
        "Total Removed: ", total_removed, "",
        "Files Removed: ", files_removed, "",
        "Dirs Removed: " , dirs_removed , "",
        "Removed Size: " , removed_size , "",
        "Run Time: "     , run_time     , "",
#if CONFIG_RTC
        "Run: "          , last_run
#endif
    };

    static struct style_text display_style[] = {
        { 0, C_ORANGE | TEXT_CENTER },
        { 3, C_BLUE },
        { 6, C_BLUE },
        { 9, C_BLUE },
        { 12, C_BLUE },
        { 15, C_BLUE },
#if CONFIG_RTC
        { 18, C_BLUE },
#endif
        LAST_STYLE_ITEM
    };

    if (display_text(ARRAYLEN(last_run_text), last_run_text,
                     display_style, NULL, true)) {
        return PLUGIN_USB_CONNECTED;
    }

    return PLUGIN_OK;
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
    rb->lcd_putsf(0, 2, "Cleaned up %d items",
        run_stats.files_removed + run_stats.dirs_removed);
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

/* Cleanup when user abort or USB event during tidy_clean */
static void tidy_clean_cleanup(struct dir_stack *dstack, DIR *dir) {
    struct dir_info dinfo;

    rb->closedir(dir);

    while (dir_stack_pop(dstack, &dinfo)) {
        rb->closedir(dinfo.dir);
    }
}

/* Perform iterative depth-first search for files to clean */
static enum plugin_status tidy_clean(char *path, int *path_length) {
    struct dir_stack dstack;
    struct dir_info dinfo;
    struct dirent *entry;
    struct dirinfo info;
    DIR *dir, *dir_test;
    /* Set to true when directory and its contents are to be deleted */
    bool rm_all = false;
    /* Used to mark where rm_all starts and ends */
    int rm_all_start_depth = 0;
    int button;
    bool remove;
    int old_path_length;

    dir_stack_init(&dstack);
    dir = rb->opendir(path);

    if (!dir) {
        /* If can't open / then immediately stop */
        return PLUGIN_ERROR;
    }

    dinfo.dir = dir;
    dinfo.path_length = *path_length;
    /* Size only used when deleting directory so value here doesn't matter */
    dinfo.size = 0;

    dir_stack_push(&dstack, dinfo);

    while (dir_stack_pop(&dstack, &dinfo)) {
        /* Restore path to poped dir */
        tidy_path_remove_entry(path, dinfo.path_length, path_length);
        dir = dinfo.dir;
        tidy_lcd_status(path);

        while ((entry = rb->readdir(dir))) {
            /* Check for user input and usb connect */
            button = rb->get_action(CONTEXT_STD, TIMEOUT_NOBLOCK);

            if (button == ACTION_STD_CANCEL) {
                tidy_clean_cleanup(&dstack, dir);
                user_abort = true;
                return PLUGIN_OK;
            }
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
                tidy_clean_cleanup(&dstack, dir);
                return PLUGIN_USB_CONNECTED;
            }

            rb->yield();

            old_path_length = *path_length;
            info = rb->dir_get_info(dir, entry);

            remove = rm_all || tidy_remove_item(entry->d_name, info.attribute);

            if (info.attribute & ATTR_DIRECTORY) {
                if (rb->strcmp(entry->d_name, ".") == 0 ||
                    rb->strcmp(entry->d_name, "..") == 0) {
                    continue;
                }

                if (!remove) {
                    /* Get absolute path, returns an error if path is too long */
                    if (!tidy_path_append_entry(path, entry, path_length)) {
                        continue; /* Silent error */
                    }

                    dinfo.dir = dir;
                    dinfo.path_length = old_path_length;
                    dinfo.size = 0;

                    /* This directory doesn't need to be deleted, so try to add
                       the current directory we're in to the stack and search
                       this one for files/directories to delete. If we can't
                       add the current directory to the stack or open the new
                       directory to search then continue on in the current
                       directory. */
                    if (dir_stack_push(&dstack, dinfo)) {
                        dir_test = rb->opendir(path);

                        if (dir_test) {
                            dir = dir_test;
                            tidy_lcd_status(path);
                        }
                    }
                }
            }

            if (!remove) {
                continue;
            }

            /* Get absolute path, returns an error if path is too long */
            if (!tidy_path_append_entry(path, entry, path_length)) {
                continue; /* Silent error */
            }

            if (info.attribute & ATTR_DIRECTORY) {
                /* Remove this directory and all files/directories it contains */
                dinfo.dir = dir;
                dinfo.path_length = old_path_length;
                dinfo.size = info.size;

                if (dir_stack_push(&dstack, dinfo)) {
                    dir_test = rb->opendir(path);

                    if (dir_test) {
                        dir = dir_test;

                        if (!rm_all) {
                            rm_all = true;
                            rm_all_start_depth = dir_stack_size(&dstack);
                        }

                        tidy_lcd_status(path);
                    }
                }
            } else {
                run_stats.files_removed++;
                run_stats.removed_size += info.size;
                rb->remove(path);

                /* Restore path */
                tidy_path_remove_entry(path, old_path_length, path_length);
            }
        }

        rb->closedir(dir);

        if (rm_all) {
            /* Check if returned to the directory we started rm_all from */
            if (rm_all_start_depth == dir_stack_size(&dstack)) {
                rm_all = false;
            }

            rb->rmdir(path);
            run_stats.dirs_removed++;
            run_stats.removed_size += dinfo.size;
        }
    }

    return PLUGIN_OK;
}

static enum plugin_status tidy_do(void)
{
    /* clean disk and display num of items removed */
    char path[MAX_PATH];

    run_stats.files_removed = 0;
    run_stats.dirs_removed = 0;
    run_stats.removed_size = 0;
    long start_time = *rb->current_tick;

#if CONFIG_RTC
    run_stats.last_run_time = *rb->get_time();
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    rb->strcpy(path, "/");
    int path_length = rb->strlen(path);
    enum plugin_status status = tidy_clean(path, &path_length);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    run_stats.run_duration = (*rb->current_tick - start_time) / HZ;
    stats_file_exists = save_run_stats();

    if (status == PLUGIN_OK)
    {
        rb->lcd_clear_display();
        if (user_abort)
        {
            rb->splash(HZ, "User aborted");
            rb->lcd_clear_display();
        }
        rb->lcd_update();
        rb->splashf(HZ*2, "Cleaned up %d items",
            run_stats.files_removed + run_stats.dirs_removed);
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

static bool tidy_types_selected(void)
{
    for (unsigned int i = 0; i < tidy_type_count; i++) {
        if (tidy_types[i].filestring[0] != '<' && tidy_types[i].remove) {
            return true;
        }
    }

    return false;
}

static int disktidy_menu_cb(int action,
                            const struct menu_item_ex *this_item,
                            struct gui_synclist *this_list)
{
    (void)this_list;
    int item = ((intptr_t)this_item);

    if (action == ACTION_REQUEST_MENUITEM &&
        !stats_file_exists && item == 2) {

        return ACTION_EXIT_MENUITEM;
    }

    return action;
}

static enum plugin_status tidy_lcd_menu(void)
{
    enum plugin_status disktidy_status = PLUGIN_OK;
    bool exit = false;
    int selection = 0;
    struct simplelist_info list;

    MENUITEM_STRINGLIST(menu, "Disktidy Menu", disktidy_menu_cb,
                        "Start Cleaning", "Files to Clean", "Last Run Stats",
                        "Playback Control", "Quit");

    while (!exit && disktidy_status == PLUGIN_OK) {
        switch(rb->do_menu(&menu, &selection, NULL, false)) {
            case 0:
                if (tidy_types_selected()) {
                    disktidy_status = tidy_do();
                } else {
                    rb->splash(HZ * 2, "Select at least one file type to clean");
                }

                break;
            case 1:
                rb->simplelist_info_init(&list, "Files to Clean",
                    tidy_type_count, NULL);
                list.get_icon = get_icon;
                list.get_name = get_name;
                list.action_callback = list_action_callback;
                rb->simplelist_show_list(&list);
                break;
            case 2:
                disktidy_status = display_run_stats();
                break;
            case 3:
                if (playback_control(NULL)) {
                    disktidy_status = PLUGIN_USB_CONNECTED;
                }

                break;
            default:
                exit = true;
                break;
        }
    }

    return disktidy_status;
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

    stats_file_exists = rb->file_exists(LAST_RUN_STATS_FILE);

    enum plugin_status disktidy_status = tidy_lcd_menu();

    if (tidy_loaded_and_changed) {
        save_config();
    }

    return disktidy_status;
}
