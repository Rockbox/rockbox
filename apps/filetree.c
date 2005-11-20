/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdlib.h>
#include <file.h>
#include <dir.h>
#include <string.h>
#include <kernel.h>
#include <lcd.h>
#include <debug.h>
#include <font.h>
#include <limits.h>
#include "bookmark.h"
#include "tree.h"
#include "settings.h"
#include "filetypes.h"
#include "talk.h"
#include "playlist.h"
#include "gwps.h"
#include "lang.h"
#include "language.h"
#include "screens.h"
#include "plugin.h"
#include "rolo.h"
#include "sprintf.h"
#include "dircache.h"
#include "splash.h"

#ifndef SIMULATOR
static int boot_size = 0;
static int boot_cluster;
#endif
extern bool boot_changed;

int ft_build_playlist(struct tree_context* c, int start_index)
{
    int i;
    int start=start_index;

    struct entry *dircache = c->dircache;

    for(i = 0;i < c->filesindir;i++)
    {
        if((dircache[i].attr & TREE_ATTR_MASK) == TREE_ATTR_MPA)
        {
            DEBUGF("Adding %s\n", dircache[i].name);
            if (playlist_add(dircache[i].name) < 0)
                break;
        }
        else
        {
            /* Adjust the start index when se skip non-MP3 entries */
            if(i < start)
                start_index--;
        }
    }

    return start_index;
}

/* walk a directory and check all dircache entries if a .talk file exists */
static void check_file_thumbnails(struct tree_context* c)
{
    int i;
    struct dircache_entry *entry;
    struct entry* dircache = c->dircache;
    DIRCACHED *dir;

    dir = opendir_cached(c->currdir);
    if(!dir)
        return;
    /* mark all files as non talking, except the .talk ones */
    for (i=0; i < c->filesindir; i++)
    {
        if (dircache[i].attr & ATTR_DIRECTORY)
            continue; /* we're not touching directories */

        if (strcasecmp(file_thumbnail_ext,
            &dircache[i].name[strlen(dircache[i].name)
                              - strlen(file_thumbnail_ext)]))
        {   /* no .talk file */
            dircache[i].attr &= ~TREE_ATTR_THUMBNAIL; /* clear */
        }
        else
        {   /* .talk file, we later let them speak themselves */
            dircache[i].attr |= TREE_ATTR_THUMBNAIL; /* set */
        }
    }

    while((entry = readdir_cached(dir)) != 0) /* walk directory */
    {
        int ext_pos;

        ext_pos = strlen(entry->d_name) - strlen(file_thumbnail_ext);
        if (ext_pos <= 0 /* too short to carry ".talk" */
            || (entry->attribute & ATTR_DIRECTORY) /* no file */
            || strcasecmp(&entry->d_name[ext_pos], file_thumbnail_ext))
        {   /* or doesn't end with ".talk", no candidate */
            continue;
        }

        /* terminate the (disposable) name in dir buffer,
           this truncates off the ".talk" without needing an extra buffer */
        entry->d_name[ext_pos] = '\0';

        /* search corresponding file in dir cache */
        for (i=0; i < c->filesindir; i++)
        {
            if (!strcasecmp(dircache[i].name, entry->d_name))
            {   /* match */
                dircache[i].attr |= TREE_ATTR_THUMBNAIL; /* set the flag */
                break; /* exit search loop, because we found it */
            }
        }
    }
    closedir_cached(dir);
}

/* support function for qsort() */
static int compare(const void* p1, const void* p2)
{
    struct entry* e1 = (struct entry*)p1;
    struct entry* e2 = (struct entry*)p2;
    int criteria;

    if (e1->attr & ATTR_DIRECTORY && e2->attr & ATTR_DIRECTORY)
    {   /* two directories */
        criteria = global_settings.sort_dir;

        if (e1->attr & ATTR_VOLUME || e2->attr & ATTR_VOLUME)
        {   /* a volume identifier is involved */
            if (e1->attr & ATTR_VOLUME && e2->attr & ATTR_VOLUME)
                criteria = 0; /* two volumes: sort alphabetically */
            else /* only one is a volume: volume first */
                return (e2->attr & ATTR_VOLUME) - (e1->attr & ATTR_VOLUME);
        }
    }
    else if (!(e1->attr & ATTR_DIRECTORY) && !(e2->attr & ATTR_DIRECTORY))
    {   /* two files */
        criteria = global_settings.sort_file;
    }
    else /* dir and file, dir goes first */
        return ( e2->attr & ATTR_DIRECTORY ) - ( e1->attr & ATTR_DIRECTORY );

    switch(criteria)
    {
        case 3: /* sort type */
        {
            int t1 = e1->attr & TREE_ATTR_MASK;
            int t2 = e2->attr & TREE_ATTR_MASK;

            if (!t1) /* unknown type */
                t1 = INT_MAX; /* gets a high number, to sort after known */
            if (!t2) /* unknown type */
                t2 = INT_MAX; /* gets a high number, to sort after known */

            if (t1 - t2) /* if different */
                return t1 - t2;
            /* else fall through to alphabetical sorting */
        }
        case 0: /* sort alphabetically asc */
            if (global_settings.sort_case)
                return strncmp(e1->name, e2->name, MAX_PATH);
            else
                return strncasecmp(e1->name, e2->name, MAX_PATH);

        case 4: /* sort alphabetically desc */
            if (global_settings.sort_case)
                return strncmp(e2->name, e1->name, MAX_PATH);
            else
                return strncasecmp(e2->name, e1->name, MAX_PATH);

        case 1: /* sort date */
            return e1->time_write - e2->time_write;

        case 2: /* sort date, newest first */
            return e2->time_write - e1->time_write;
    }
    return 0; /* never reached */
}

/* load and sort directory into dircache. returns NULL on failure. */
int ft_load(struct tree_context* c, const char* tempdir)
{
    int i;
    int name_buffer_used = 0;
    DIRCACHED *dir;

    if (tempdir)
        dir = opendir_cached(tempdir);
    else
        dir = opendir_cached(c->currdir);
    if(!dir)
        return -1; /* not a directory */

    c->dirsindir = 0;
    c->dirfull = false;

    for ( i=0; i < global_settings.max_files_in_dir; i++ ) {
        int len;
        struct dircache_entry *entry = readdir_cached(dir);
        struct entry* dptr =
            (struct entry*)(c->dircache + i * sizeof(struct entry));
        if (!entry)
            break;

        len = strlen(entry->d_name);

        /* skip directories . and .. */
        if ((entry->attribute & ATTR_DIRECTORY) &&
            (((len == 1) &&
            (!strncmp(entry->d_name, ".", 1))) ||
            ((len == 2) &&
            (!strncmp(entry->d_name, "..", 2))))) {
            i--;
            continue;
        }

        /* Skip FAT volume ID */
        if (entry->attribute & ATTR_VOLUME_ID) {
            i--;
            continue;
        }

        /* filter out dotfiles and hidden files */
        if (*c->dirfilter != SHOW_ALL &&
            ((entry->d_name[0]=='.') ||
            (entry->attribute & ATTR_HIDDEN))) {
            i--;
            continue;
        }

        dptr->attr = entry->attribute;

        /* check for known file types */
        if ( !(dptr->attr & ATTR_DIRECTORY) )
           dptr->attr |= filetype_get_attr(entry->d_name);

#ifdef BOOTFILE
        /* memorize/compare details about the boot file */
        if ((c->currdir[1] == 0) && !strcasecmp(entry->d_name, BOOTFILE)) {
            if (boot_size) {
                if ((entry->size != boot_size) ||
                    (entry->startcluster != boot_cluster))
                    boot_changed = true;
            }
            boot_size = entry->size;
            boot_cluster = entry->startcluster;
        }
#endif

        /* filter out non-visible files */
        if ((!(dptr->attr & ATTR_DIRECTORY) && (
            (*c->dirfilter == SHOW_PLAYLIST &&
             (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_M3U) ||
            ((*c->dirfilter == SHOW_MUSIC &&
             (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_MPA) &&
             (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_M3U) ||
            (*c->dirfilter == SHOW_SUPPORTED && !filetype_supported(dptr->attr)))) ||
            (*c->dirfilter == SHOW_WPS && (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_WPS) ||
#ifdef HAVE_REMOTE_LCD
            (*c->dirfilter == SHOW_RWPS && (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_RWPS) ||
#endif
            (*c->dirfilter == SHOW_CFG && (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_CFG) ||
            (*c->dirfilter == SHOW_LNG && (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_LNG) ||
            (*c->dirfilter == SHOW_MOD && (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_MOD) ||
            (*c->dirfilter == SHOW_FONT && (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_FONT) ||
            (*c->dirfilter == SHOW_PLUGINS && (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_ROCK))
        {
            i--;
            continue;
        }

        if (len > c->name_buffer_size - name_buffer_used - 1) {
            /* Tell the world that we ran out of buffer space */
            c->dirfull = true;
            break;
        }
        dptr->name = &c->name_buffer[name_buffer_used];
        dptr->time_write =
            (long)entry->wrtdate<<16 |
            (long)entry->wrttime; /* in one # */
        strcpy(dptr->name,entry->d_name);
        name_buffer_used += len + 1;

        if (dptr->attr & ATTR_DIRECTORY) /* count the remaining dirs */
            c->dirsindir++;
    }
    c->filesindir = i;
    c->dirlength = i;
    closedir_cached(dir);

    qsort(c->dircache,i,sizeof(struct entry),compare);

    /* If thumbnail talking is enabled, make an extra run to mark files with
       associated thumbnails, so we don't do unsuccessful spinups later. */
    if (global_settings.talk_file == 3)
        check_file_thumbnails(c); /* map .talk to ours */

    return 0;
}

int ft_enter(struct tree_context* c)
{
    int rc = 0;
    char buf[MAX_PATH];
    struct entry *dircache = c->dircache;
    struct entry* file = &dircache[c->selected_item];
    bool reload_dir = false;
    bool start_wps = false;
    bool exit_func = false;

    if (c->currdir[1])
        snprintf(buf,sizeof(buf),"%s/%s",c->currdir, file->name);
    else
        snprintf(buf,sizeof(buf),"/%s",file->name);

    if (file->attr & ATTR_DIRECTORY) {
        memcpy(c->currdir, buf, sizeof(c->currdir));
        if ( c->dirlevel < MAX_DIR_LEVELS )
            c->selected_item_history[c->dirlevel] = c->selected_item;
        c->dirlevel++;
        c->selected_item=0;
    }
    else {
        int seed = current_tick;
        bool play = false;
        int start_index=0;

        gui_syncsplash(0, true, str(LANG_WAIT));
        switch ( file->attr & TREE_ATTR_MASK ) {
            case TREE_ATTR_M3U:
                if (bookmark_autoload(buf))
                    break;

                if (playlist_create(c->currdir, file->name) != -1)
                {
                    if (global_settings.playlist_shuffle)
                        playlist_shuffle(seed, -1);
                    start_index = 0;
                    playlist_start(start_index,0);
                    play = true;
                }
                break;

            case TREE_ATTR_MPA:
                if (bookmark_autoload(c->currdir))
                    break;

                if (playlist_create(c->currdir, NULL) != -1)
                {
                    start_index = ft_build_playlist(c, c->selected_item);
                    if (global_settings.playlist_shuffle)
                    {
                        start_index = playlist_shuffle(seed, start_index);

                        /* when shuffling dir.: play all files
                           even if the file selected by user is
                           not the first one */
                        if (!global_settings.play_selected)
                            start_index = 0;
                    }

                    playlist_start(start_index, 0);
                    play = true;
                }
                break;

                /* wps config file */
            case TREE_ATTR_WPS:
                wps_data_load(gui_wps[0].data, buf, true, true);
                set_file(buf, global_settings.wps_file,
                         MAX_FILENAME);
                break;

#ifdef HAVE_REMOTE_LCD
                /* remote-wps config file */
            case TREE_ATTR_RWPS:
                wps_data_load(gui_wps[1].data, buf, true, true);
                set_file(buf, global_settings.rwps_file,
                         MAX_FILENAME);
                break;
#endif

            case TREE_ATTR_CFG:
                if (!settings_load_config(buf))
                    break;
                lcd_clear_display();
                lcd_puts(0,0,str(LANG_SETTINGS_LOADED1));
                lcd_puts(0,1,str(LANG_SETTINGS_LOADED2));
#ifdef HAVE_LCD_BITMAP
                lcd_update();
#endif
                sleep(HZ/2);
                break;

            case TREE_ATTR_BMARK:
                bookmark_load(buf, false);
                reload_dir = true;
                break;

            case TREE_ATTR_LNG:
                if(!lang_load(buf)) {
                    set_file(buf, global_settings.lang_file,
                             MAX_FILENAME);
                    talk_init(); /* use voice of same language */
                    gui_syncsplash(HZ, true, str(LANG_LANGUAGE_LOADED));
                }
                break;

#ifdef HAVE_LCD_BITMAP
            case TREE_ATTR_FONT:
                font_load(buf);
                set_file(buf, global_settings.font_file, MAX_FILENAME);
                break;
#endif

#ifndef SIMULATOR
                /* firmware file */
            case TREE_ATTR_MOD:
                rolo_load(buf);
                break;
#endif

                /* plugin file */
            case TREE_ATTR_ROCK:
                if (plugin_load(buf,NULL) == PLUGIN_USB_CONNECTED)
                {
                    if(*c->dirfilter > NUM_FILTER_MODES)
                        /* leave sub-browsers after usb, doing
                           otherwise might be confusing to the user */
                        exit_func = true;
                    else
                        reload_dir = true;
                }
                break;

            default:
            {
                char* plugin = filetype_get_plugin(file);
                if (plugin)
                {
                    if (plugin_load(plugin,buf) == PLUGIN_USB_CONNECTED)
                        reload_dir = true;
                }
                break;
            }
        }

        if ( play ) {
            /* the resume_index must always be the index in the
               shuffled list in case shuffle is enabled */
            global_settings.resume_index = start_index;
            global_settings.resume_offset = 0;
            settings_save();

            start_wps = true;
        }
        else {
            if (*c->dirfilter > NUM_FILTER_MODES &&
                *c->dirfilter != SHOW_FONT &&
                *c->dirfilter != SHOW_PLUGINS)
            {
                exit_func = true;
            }
        }
    }

    if (reload_dir)
        rc = 1;
    if (start_wps)
        rc = 2;
    if (exit_func)
        rc = 3;

    return rc;
}

int ft_exit(struct tree_context* c)
{
    extern char lastfile[]; /* from tree.c */
    char buf[MAX_PATH];
    int rc = 0;
    bool exit_func = false;

    int i = strlen(c->currdir);
    if (i>1) {
        while (c->currdir[i-1]!='/')
            i--;
        strcpy(buf,&c->currdir[i]);
        if (i==1)
            c->currdir[i]=0;
        else
            c->currdir[i-1]=0;

        if (*c->dirfilter > NUM_FILTER_MODES && c->dirlevel < 1)
            exit_func = true;

        c->dirlevel--;
        if ( c->dirlevel < MAX_DIR_LEVELS )
            c->selected_item=c->selected_item_history[c->dirlevel];
        else
            c->selected_item=0;

        /* if undefined position */
        if (c->selected_item == -1)
            strcpy(lastfile, buf);
    }
    else
    {
        if (*c->dirfilter > NUM_FILTER_MODES && c->dirlevel < 1)
            exit_func = true;
    }

    if (exit_func)
        rc = 3;

    return rc;
}
