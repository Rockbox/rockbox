/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jonathan Gordon
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
#include "config.h"
#include "plugin.h"
#include "file.h"

#define RFA_LIST_DAT ROCKBOX_DIR "/folder_advance_list.dat" /* Folder data  */
#define RFA_LIST_TXT ROCKBOX_DIR "/folder_advance_list.txt" /* Exported txt */
#define RFA_CSTM_DIR ROCKBOX_DIR "/folder_advance_dir.txt"  /* Custom dirs  */
#define MAX_IGNORED_DIRS 10 /* Maximum number of folders that are skipped   */

struct rfa_dirs /* File format for folder data */
{
    int count;
    char dir[][MAX_PATH];
};

struct rfa_scan /* Context for single run */
{
    char ignored_dirs[MAX_IGNORED_DIRS][MAX_PATH];
    char path[MAX_PATH];
    struct rfa_dirs *dirs;
    char *err_dir;
    unsigned long last_update;
    int num_ignored_dirs;
    int num_dirs;
    int num_err;
    int fd; /* writes to RFA_LIST_DAT */
    bool cancel;
    bool dirty;
};

static void update_screen(struct rfa_scan *scan)
{
    char buf[15];
    struct screen *display;
    struct viewport vp, *last_vp;

    rb->snprintf(buf, sizeof buf, "Folders: %d    ", scan->num_dirs);

    FOR_NB_SCREENS(i)
    {
        display = rb->screens[i];
        rb->viewport_set_defaults(&vp, i);
        last_vp = display->set_viewport(&vp);
        if(!scan->path[0])
            display->clear_viewport();

        display->puts(0, 0, buf);
        if (scan->num_err && scan->err_dir)
        {
            display->puts(0, 1, "Not found:");
            display->puts(0, 1 + scan->num_err, scan->err_dir);
        }

        display->update_viewport();
        display->set_viewport(last_vp);
    }
    scan->last_update = *rb->current_tick;
}

static void traverse_path(struct rfa_scan *scan)
{
    DIR* dir;
    struct dirent *entry;
    char *p;
    int i, dirlen;
    bool skip;

    dir = rb->opendir(scan->path);

    /* remove slash from root dir; will be prepended later */
    if (scan->path[0] == '/' && scan->path[1] == '\0')
        scan->path[0] = dirlen = 0;
    else
        dirlen = rb->strlen(scan->path);

    if (!dir)
        return;

    while ((entry = rb->readdir(dir)))
    {
        if (scan->cancel)
            break;
        /* Don't check special dirs */
        if (entry->d_name[0] == '.')
        {
            if (!rb->strcmp(entry->d_name, ".") ||
                !rb->strcmp(entry->d_name, "..") ||
                !rb->strcmp(entry->d_name, ".rockbox"))
                continue;
        }
        struct dirinfo info = rb->dir_get_info(dir, entry);
        if (info.attribute & ATTR_DIRECTORY)
        {
            /* Append name to full path */
            rb->snprintf(scan->path + dirlen, sizeof(scan->path) - dirlen,
                         "/%s", entry->d_name);

            /* Skip ignored dirs */
            skip = false;
            for(i = 0; i < scan->num_ignored_dirs; i++)
                if(!rb->strcmp(scan->path, scan->ignored_dirs[i]))
                {
                    skip = true;
                    break;
                }
            if (skip)
                continue;

            scan->num_dirs++;

            /* Write full path to file (erase end of buffer) */
            p = &scan->path[rb->strlen(scan->path)];
            rb->memset(p, 0, &scan->path[sizeof(scan->path) - 1] - p);
            rb->write(scan->fd, scan->path, sizeof scan->path);

            /* Recursion */
            traverse_path(scan);
        }

        if (*rb->current_tick - scan->last_update > (HZ/2))
        {
            update_screen(scan);
            if (rb->action_userabort(TIMEOUT_NOBLOCK))
            {
                scan->cancel = true;
                break;
            }
        }
        rb->yield();
    }
    rb->closedir(dir);
}

static bool custom_paths(struct rfa_scan *scan)
{
    char line[MAX_PATH], *p;
    int i, fd;
    bool write_line;

    fd = rb->open(RFA_CSTM_DIR, O_RDONLY);
    if (fd < 0)
        return false;

    /* Populate ignored dirs array */
    while ((rb->read_line(fd, line, sizeof line)) > 0)
        if ((line[0] == '-') && (line[1] == '/') &&
            (scan->num_ignored_dirs < MAX_IGNORED_DIRS))
        {
            rb->strlcpy(scan->ignored_dirs[scan->num_ignored_dirs],
                        line + 1, sizeof(*scan->ignored_dirs));
            scan->num_ignored_dirs++;
        }

    /* Check which paths to traverse */
    rb->lseek(fd, 0, SEEK_SET);
    while ((rb->read_line(fd, line, sizeof line)) > 0)
    {
        /* Skip blank lines or dirs to ignore */
        if (line[0] == '\0' || ((line[0] == '-') && (line[1] == '/')))
            continue;

        /* Set path to traverse */
        p = line;
        while(*p == '/')
            p++;
        rb->snprintf(scan->path, sizeof scan->path, "/%s", p);
        if (rb->dir_exists(scan->path))
        {
            /* Erase end of path buffer */
            p = &scan->path[rb->strlen(scan->path)];
            rb->memset(p, 0, &scan->path[sizeof(scan->path) - 1] - p);

            /* Don't write ignored dirs to file  */
            write_line = true;
            for(i = 0; i < scan->num_ignored_dirs; i++)
                if(!rb->strcmp(scan->path, scan->ignored_dirs[i]))
                {
                     write_line = false;
                     break;
                }

            if(write_line)
            {
                scan->num_dirs++;
                rb->write(scan->fd, scan->path, sizeof scan->path);
            }

            traverse_path(scan);
            update_screen(scan);
        }
        else
        {
            scan->num_err++;
            scan->err_dir = scan->path;
            update_screen(scan);
            scan->err_dir = NULL;
        }
    }

    rb->close(fd);

    if(scan->num_err)
        rb->get_action(CONTEXT_STD, TIMEOUT_BLOCK);

    return true;
}

static void generate(struct rfa_scan *scan)
{
    bool update_title = false;

    /* Reset run context */
    rb->memset(scan, 0, sizeof(struct rfa_scan));

    FOR_NB_SCREENS(i)
        update_title = rb->sb_set_title_text("Scanning...", Icon_NOICON, i) ||
                       update_title;
    if (update_title)
        rb->send_event(GUI_EVENT_ACTIONUPDATE, (void*)1);

#ifdef HAVE_DIRCACHE
    rb->splash(0, ID2P(LANG_WAIT));
    rb->dircache_wait();
#endif

    scan->fd = rb->open(RFA_LIST_DAT, O_CREAT|O_WRONLY, 0666);
    if (scan->fd < 0)
    {
        rb->splashf(HZ, "Couldnt open %s", RFA_LIST_DAT);
        return;
    }

    /* Write placeholder for number of folders */
    rb->write(scan->fd, &scan->num_dirs, sizeof(int));

    update_screen(scan);
    if(!custom_paths(scan))
    {
        scan->path[0] = '/';
        scan->path[1] = '\0';
        traverse_path(scan);
        update_screen(scan);
    }
    rb->lseek(scan->fd, 0, SEEK_SET);
    rb->write(scan->fd, &scan->num_dirs, sizeof(int));
    rb->close(scan->fd);
    rb->splash(HZ, "Done");
}

static const char* list_get_name_cb(int selected_item, void* data,
                                    char* buf, size_t buf_len)
{
    struct rfa_scan *scan = (struct rfa_scan *)data;
    rb->strlcpy(buf, scan->dirs->dir[selected_item], buf_len);
    return buf;
}

static int load_dirs(struct rfa_scan *scan)
{
    char *buf;
    size_t buf_sz;

    int fd = rb->open(RFA_LIST_DAT , O_RDONLY);
    if (fd < 0)
        return -1;

    buf = rb->plugin_get_audio_buffer(&buf_sz);
    if (!buf)
        return -2;

    rb->read(fd, buf, buf_sz);
    rb->close(fd);
    scan->dirs = (struct rfa_dirs *) buf;

    return 0;
}

static int save_dirs(struct rfa_scan *scan)
{
    int num_dirs = 0, i = 0;
    int fd = rb->creat(RFA_LIST_DAT, 0666);
    if (fd < 0)
    {
        rb->splash(HZ, "Could Not Open " RFA_LIST_DAT);
        return -1;
    }

    rb->splash_progress_set_delay(HZ/2);
    rb->write(fd, &num_dirs, sizeof(int));
    for ( ; i < scan->dirs->count; i++)
    {
        /* (voiced) */
        rb->splash_progress(i, scan->dirs->count, "%s", rb->str(LANG_WAIT));
        if (scan->dirs->dir[i][0] != ' ')
        {
            num_dirs++;
            rb->write(fd, scan->dirs->dir[i], sizeof(*scan->dirs->dir));
        }
    }
    rb->lseek(fd, 0, SEEK_SET);
    rb->write(fd, &num_dirs, sizeof(int));
    rb->close(fd);

    return 1;
}

static bool disable_dir(int i, struct rfa_scan *scan)
{
    if (scan->dirs->dir[i][0] != ' ')
    {
        scan->dirty = true;
        scan->dirs->dir[i][0] = ' ';
        scan->dirs->dir[i][1] = '\0';
        return true;
    }
    return false;
}

static void set_title(struct gui_synclist *lists, bool update, int num_dirs)
{
    static int title_count;
    static char title[15];

    if (update)
        title_count += num_dirs;
    else if (num_dirs >= 0)
        title_count = num_dirs;

    rb->snprintf(title, sizeof title, title_count == 1 ?
                 "%d Folder" :  "%d Folders", title_count);
    rb->gui_synclist_set_title(lists, title, Icon_NOICON);
}

static int edit(struct rfa_scan *scan)
{
    struct gui_synclist lists;
    int button, i, j, selection;

    /* load the dat file if not already done */
    if (!scan->dirs || !scan->dirs->count)
    {
        if (!rb->file_exists(RFA_LIST_DAT))
            generate(scan);

        if ((i = load_dirs(scan)) != 0)
        {
            rb->splashf(HZ*2, "Could not load %s, err %d", RFA_LIST_DAT, i);
            return -1;
        }
    }
    scan->dirty = false;
    rb->gui_synclist_init(&lists, list_get_name_cb, (void *) scan,
                          false, 1, NULL);
    rb->gui_synclist_set_nb_items(&lists, scan->dirs->count);
    rb->gui_synclist_select_item(&lists, 0);
    set_title(&lists, false, scan->dirs->count);
    while (true)
    {
        rb->gui_synclist_draw(&lists);
        button = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
        if (rb->gui_synclist_do_button(&lists, &button))
            continue;
        selection = rb->gui_synclist_get_sel_pos(&lists);
        switch (button)
        {
            case ACTION_STD_OK:
                if (disable_dir(selection, scan))
                    set_title(&lists, true, -1);
                break;
            case ACTION_STD_CONTEXT:
            {
                int len;
                MENUITEM_STRINGLIST(menu, "Remove", NULL,
                                    "Remove Folder", "Remove Folder Tree");

                switch (rb->do_menu(&menu, NULL, NULL, false))
                {
                    case 0:
                        if (disable_dir(selection, scan))
                            set_title(&lists, true, -1);
                        break;
                    case 1:
                        rb->strcpy(scan->path, scan->dirs->dir[selection]);
                        len = rb->strlen(scan->path);
                        j = 0;
                        for (i = 0; i < scan->dirs->count; i++)
                            if (!rb->strncmp(scan->dirs->dir[i], scan->path, len))
                                if (disable_dir(i, scan))
                                    j++;
                        set_title(&lists, true, -j);
                        break;
                    default:
                        set_title(&lists, false, -1);
                        break;
                }
            }
            break;
            case ACTION_STD_CANCEL:
            {
                MENUITEM_STRINGLIST(menu, "Save Changes?", NULL,
                                    "Save", "Ignore Changes");

                if (!scan->dirty)
                    return 0;
                switch (rb->do_menu(&menu, NULL, NULL, false))
                {
                    case 0:
                        save_dirs(scan);
                        /* fallthrough */
                    case 1:
                        scan->dirs = NULL;
                        return 0;
                    default:
                        set_title(&lists, false, -1);
                        break;
                }
            }
            break;
        }
    }
    return 0;
}

static int export(struct rfa_scan *scan)
{
    int i, fd;

    /* load the dat file if not already done */
    if (!scan->dirs || !scan->dirs->count)
        if ((i = load_dirs(scan)) != 0)
        {
            rb->splashf(HZ*2, "Could not load %s, err %d", RFA_LIST_DAT, i);
            return 0;
        }

    if (scan->dirs->count <= 0)
    {
        rb->splashf(HZ*2, "no dirs in list file: %s", RFA_LIST_DAT);
        return 0;
    }

    /* create and open the file */
    fd = rb->creat(RFA_LIST_TXT, 0666);
    if (fd < 0)
    {
        rb->splashf(HZ*4, "failed to open: fd %d, file %s",
                    fd, RFA_LIST_TXT);
        return -1;
    }

    /* write each directory to file */
    rb->splash_progress_set_delay(HZ/2);
    for (i = 0; i < scan->dirs->count; i++)
    {
        /* (voiced) */
        rb->splash_progress(i, scan->dirs->count, "%s", rb->str(LANG_WAIT));

        if (scan->dirs->dir[i][0] != ' ')
            rb->fdprintf(fd, "%s\n", scan->dirs->dir[i]);
    }

    rb->close(fd);
    rb->splash(HZ, "Done");
    return 1;
}

static int import(struct rfa_scan *scan)
{
    char *buf;
    size_t buf_sz;
    int fd;

    buf = rb->plugin_get_audio_buffer(&buf_sz);
    if (!buf || buf_sz < sizeof(int))
    {
        rb->splash(HZ*2, "failed to get audio buffer");
        return -1;
    }

    fd = rb->open(RFA_LIST_TXT, O_RDONLY);
    if (fd < 0)
    {
        rb->splashf(HZ*2, "failed to open: %s", RFA_LIST_TXT);
        return -1;
    }

    rb->splash(0, ID2P(LANG_WAIT));

    /* set the list structure, and initialize count */
    scan->dirs = (struct rfa_dirs *)buf;
    scan->dirs->count = 0;
    buf_sz -= sizeof(int);

    while (buf_sz >= sizeof(*scan->dirs->dir))
    {
        int numread = rb->read_line(fd, scan->dirs->dir[scan->dirs->count],
                                    sizeof(*scan->dirs->dir));
        if ((numread) <= 0)
            break;

        /* if the line we read fills up buffer, the next call to read_line
           will return an empty string for a remaining newline character. */
        if (scan->dirs->dir[scan->dirs->count][0])
        {
            scan->dirs->count++;
            buf_sz -= sizeof(*scan->dirs->dir);
        }
    }

    rb->close(fd);

    if (!scan->dirs->count)
        load_dirs(scan);
    else
        save_dirs(scan);

    rb->splash(HZ, "Done");
    return scan->dirs->count;
}

static int play_shuffled(struct rfa_scan *scan)
{
    struct playlist_insert_context pl_context;
    int *order, i = 0, ret = -1;
    size_t max_shuffle_size;

    /* load the dat file if not already done */
    if (!scan->dirs || !scan->dirs->count)
    {
        if (!rb->file_exists(RFA_LIST_DAT))
            generate(scan);

        if ((i = load_dirs(scan)) != 0)
        {
            rb->splashf(HZ*2, "Could not load %s, err %d", RFA_LIST_DAT, i);
            return ret;
        }
    }

    if (scan->dirs->count <= 0)
    {
        rb->splashf(HZ*2, "No dirs in list file: %s", RFA_LIST_DAT);
        return ret;
    }

     /* get memory for shuffling */
    order = rb->plugin_get_buffer(&max_shuffle_size);
    max_shuffle_size /= sizeof(int);
    if (!order || !max_shuffle_size)
    {
        rb->splashf(HZ*2, "Not enough memory for shuffling");
        return ret;
    }

    /* shuffle the thing */
    rb->srand(*rb->current_tick);
    if(scan->dirs->count > (int) max_shuffle_size)
    {
        rb->splashf(HZ*2, "Too many folders: %d (room for %d)",
                    scan->dirs->count, (int) max_shuffle_size);
        return ret;
    }
    for(i = 0; i < scan->dirs->count; i++)
        order[i] = i;

    for(i = scan->dirs->count - 1; i >= 0; i--)
    {
        /* the rand is from 0 to RAND_MAX, so adjust to our value range */
        int candidate = rb->rand() % (i + 1);

        /* now swap the values at the 'i' and 'candidate' positions */
        int store = order[candidate];
        order[candidate] = order[i];
        order[i] = store;
    }

    if (!(rb->playlist_remove_all_tracks(NULL) == 0
          && rb->playlist_create(NULL, NULL) == 0))
    {
        rb->splashf(HZ*2, "Could not clear playlist");
        return ret;
    }

    rb->splash_progress_set_delay(HZ/2);

    /* Create insert context, disable progress for individual dirs */
    if (rb->playlist_insert_context_create(NULL, &pl_context, PLAYLIST_INSERT_LAST,
                                           false, false) < 0)
    {
        rb->playlist_insert_context_release(&pl_context);
        rb->splashf(HZ*2, "Could not insert directories");
        return ret;
    }

    for (i = 0; i < scan->dirs->count; i++)
    {
        /* (voiced) */
        rb->splash_progress(i, scan->dirs->count, "%s (%s)",
                            rb->str(LANG_WAIT), rb->str(LANG_OFF_ABORT));
        if (rb->action_userabort(TIMEOUT_NOBLOCK))
        {
            ret = 0;
            break;
        }

        if (scan->dirs->dir[order[i]][0] != ' ')
        {
            ret = rb->playlist_insert_directory(NULL, scan->dirs->dir[order[i]],
                                                PLAYLIST_INSERT_LAST, false,
                                                false, &pl_context);
            if (ret < 0)
            {
                if (ret == -2) /* User canceled */
                    break;
                else if (rb->playlist_amount() >=
                         rb->global_settings->max_files_in_playlist)
                {
                    rb->splashf(HZ, "Reached maximum number of files in playlist (%d)",
                                rb->global_settings->max_files_in_playlist);
                    ret = 1;
                    break;
                }
                else
                    rb->splashf(HZ, "Error inserting: %s", scan->dirs->dir[order[i]]);
            }
            ret = 1;
        }
    }
    rb->playlist_insert_context_release(&pl_context);

    if (ret > 0)
    {
        /* the core needs the audio buffer back in order to start playback. */
        scan->dirs = NULL;
        rb->plugin_release_audio_buffer();
        rb->playlist_set_modified(NULL, true);
        rb->playlist_start(0, 0, 0);
    }
    return ret;
}

static enum plugin_status main_menu(void)
{
    static struct rfa_scan scan;
    int selected = 0;

    MENUITEM_STRINGLIST(menu, "Random Folder Advance", NULL,
                        "Play Shuffled",
                        "Scan Folders",
                        "Edit",
                        "Export to Text File",
                        "Import from Text File",
                        "Quit");

    while (true)
    {
        switch (rb->do_menu(&menu, &selected, NULL, false))
        {
            case GO_TO_PREVIOUS_MUSIC:
                return PLUGIN_GOTO_WPS;
            case GO_TO_ROOT:
                return PLUGIN_GOTO_ROOT;
            case GO_TO_PREVIOUS:
                return PLUGIN_OK;
            case 0: /* Play Shuffled */
                if (!rb->warn_on_pl_erase())
                    break;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                rb->cpu_boost(true);
#endif
                int ret = play_shuffled(&scan);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                rb->cpu_boost(false);
#endif
                if (ret > 0)
                    return PLUGIN_GOTO_WPS;
                break;
            case 1: /* Scan Folders */
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                rb->cpu_boost(true);
#endif
                generate(&scan);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                rb->cpu_boost(false);
#endif
#ifdef HAVE_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
                rb->remote_backlight_on();
#endif
                rb->backlight_on();
#endif
                break;
            case 2: /* Edit */
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                rb->cpu_boost(true);
#endif
                edit(&scan);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                rb->cpu_boost(false);
#endif
#ifdef HAVE_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
                rb->remote_backlight_on();
#endif
                rb->backlight_on();
#endif
                break;
            case 3: /* Export to Text File */
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                rb->cpu_boost(true);
#endif
                export(&scan);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                rb->cpu_boost(false);
#endif
#ifdef HAVE_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
                rb->remote_backlight_on();
#endif
                rb->backlight_on();
#endif
                break;
            case 4: /* Import from Text File */
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                rb->cpu_boost(true);
#endif
                import(&scan);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                rb->cpu_boost(false);
#endif
#ifdef HAVE_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
                rb->remote_backlight_on();
#endif
                rb->backlight_on();
#endif
                break;
            case 5: /* Quit */
                return PLUGIN_OK;
        }
    }
    return PLUGIN_OK;
}

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    enum plugin_status ret;

#ifdef HAVE_TOUCHSCREEN
    rb->touchscreen_set_mode(rb->global_settings->touch_mode);
#endif

    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_enable(i, true, NULL);
    ret = main_menu();
    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_undo(i, false);

    return ret;
}
