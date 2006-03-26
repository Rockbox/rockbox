/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Miika Pekkarinen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/** 
 * Basic structure on this file was copied from dbtree.c and modified to
 * support the tag cache interface.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "system.h"
#include "kernel.h"
#include "splash.h"
#include "icons.h"
#include "tree.h"
#include "settings.h"
#include "tagcache.h"
#include "tagtree.h"
#include "lang.h"
#include "logf.h"
#include "playlist.h"
#include "keyboard.h"
#include "gui/list.h"

#define CHUNKED_NEXT   -2

static int tagtree_play_folder(struct tree_context* c);
static int tagtree_search(struct tree_context* c, char* string);

static char searchstring[32];
struct tagentry {
    char *name;
    int newtable;
    int extraseek;
};

static struct tagcache_search tcs;

int tagtree_load(struct tree_context* c)
{
    int i;
    int namebufused = 0;
    struct tagentry *dptr = (struct tagentry *)c->dircache;

    int table = c->currtable;
    int extra = c->currextra;
    int extra2 = c->currextra2;

    c->dentry_size = sizeof(struct tagentry);

    if (!table)
    {
        c->dirfull = false;
        table = root;
        c->currtable = table;
    }

    if (c->dirfull)
        table = chunked_next;

    switch (table) {
        case root: {
            static const int tables[] = {allartists, allalbums, allgenres, allsongs,
                                         search };
            unsigned char* labels[] = { str(LANG_ID3DB_ARTISTS),
                                        str(LANG_ID3DB_ALBUMS),
                                        str(LANG_ID3DB_GENRES),
                                        str(LANG_ID3DB_SONGS),
                                        str(LANG_ID3DB_SEARCH)};

            for (i = 0; i < 5; i++) {
                dptr->name = &c->name_buffer[namebufused];
                dptr->newtable = tables[i];
                strcpy(dptr->name, (char *)labels[i]);
                namebufused += strlen(dptr->name) + 1;
                dptr++;
            }
            c->dirlength = c->filesindir = i;
            return i;
        }

        case search: {
            static const int tables[] = {searchartists,
                                         searchalbums,
                                         searchsongs};
            unsigned char* labels[] = { str(LANG_ID3DB_SEARCH_ARTISTS),
                                        str(LANG_ID3DB_SEARCH_ALBUMS),
                                        str(LANG_ID3DB_SEARCH_SONGS)};
            
            for (i = 0; i < 3; i++) {
                dptr->name = &c->name_buffer[namebufused];
                dptr->newtable = tables[i];
                strcpy(dptr->name, (char *)labels[i]);
                namebufused += strlen(dptr->name) + 1;
                dptr++;
            }
            c->dirlength = c->filesindir = i;
            return i;
        }

        case searchartists:
        case searchalbums:
        case searchsongs:
            i = tagtree_search(c, searchstring);
            c->dirlength = c->filesindir = i;
            if (c->dirfull) {
                gui_syncsplash(HZ, true, (unsigned char *)"%s %s",
                               str(LANG_SHOWDIR_ERROR_BUFFER),
                               str(LANG_SHOWDIR_ERROR_FULL));
                c->dirfull = false;
            }
            else
                gui_syncsplash(HZ, true, str(LANG_ID3DB_MATCHES), i);
            return i;

        case allsongs:
            logf("songs..");
            tagcache_search(&tcs, tag_title);
            break;

        case allgenres:
            logf("genres..");
            tagcache_search(&tcs, tag_genre);
            break;

        case allalbums:
            logf("albums..");
            tagcache_search(&tcs, tag_album);
            break;

        case allartists:
            logf("artists..");
            tagcache_search(&tcs, tag_artist);
            break;

        case artist4genres:
            logf("artist4genres..");
            tagcache_search(&tcs, tag_artist);
            tagcache_search_add_filter(&tcs, tag_genre, extra);
            break;
    
        case albums4artist:
            logf("albums4artist..");
            tagcache_search(&tcs, tag_album);
            tagcache_search_add_filter(&tcs, tag_artist, extra);
            break;

        case songs4album:
            logf("songs4album..");
            tagcache_search(&tcs, tag_title);
            tagcache_search_add_filter(&tcs, tag_album, extra);
            if (extra2 > 0)
                tagcache_search_add_filter(&tcs, tag_artist, extra2);
            break;

        case songs4artist:
            logf("songs4artist..");
            tagcache_search(&tcs, tag_title);
            tagcache_search_add_filter(&tcs, tag_artist, extra);
            break;

        case chunked_next:
            logf("chunked next...");
            break;

        default:
            logf("Unsupported table %d\n", table);
            return -1;
    }
    
    i = 0;
    namebufused = 0;
    c->dirfull = false;
    while (tagcache_get_next(&tcs))
    {
        dptr->newtable = tcs.result_seek;
        if (!tcs.ramsearch)
        {
            dptr->name = &c->name_buffer[namebufused];
            namebufused += tcs.result_len;
            if (namebufused > c->name_buffer_size)
            {
                logf("buffer full, 1 entry missed.");
                c->dirfull = true;
                break ;
            }
            strcpy(dptr->name, tcs.result);
        }
        else
            dptr->name = tcs.result;
        dptr++;
        i++;

        /**
         * Estimate when we are running out of space so we can stop
         * and enabled chunked browsing without missing entries.
         */
        if (i >= global_settings.max_files_in_dir - 1
            || namebufused + 200 > c->name_buffer_size)
        {
            c->dirfull = true;
            break ;
        }
        
    }

    if (c->dirfull)
    {
        dptr->name = "===>";
        dptr->newtable = chunked_next;
        dptr++;
        i++;
    }
    else
        tagcache_search_finish(&tcs);
    
    c->dirlength = c->filesindir = i;
    
    return i;
}

static int tagtree_search(struct tree_context* c, char* string)
{
    struct tagentry *dptr = (struct tagentry *)c->dircache;
    int hits = 0;
    int namebufused = 0;

    switch (c->currtable) {
        case searchartists:
            tagcache_search(&tcs, tag_artist);
            break;

        case searchalbums:
            tagcache_search(&tcs, tag_album);
            break;

        case searchsongs:
            tagcache_search(&tcs, tag_title);
            break;

        default:
            logf("Invalid table %d\n", c->currtable);
            return 0;
    }

    while (tagcache_get_next(&tcs))
    {
        if (!strcasestr(tcs.result, string))
            continue ;

        if (!tcs.ramsearch)
        {
            dptr->name = &c->name_buffer[namebufused];
            namebufused += tcs.result_len;
            strcpy(dptr->name, tcs.result);
        }
        else
            dptr->name = tcs.result;
        
        dptr->newtable = tcs.result_seek;
        dptr++;
        hits++;
    }

    tagcache_search_finish(&tcs);

    return hits;
}

int tagtree_enter(struct tree_context* c)
{
    int rc = 0;
    struct tagentry *dptr = (struct tagentry *)c->dircache;
    int newextra;

    dptr += c->selected_item;
    
    if (dptr->newtable == chunked_next)
    {
        c->selected_item=0;
        gui_synclist_select_item(&tree_lists, c->selected_item);
        return 0;
    }
    
    c->dirfull = false;
    newextra = dptr->newtable;

    if (c->dirlevel >= MAX_DIR_LEVELS)
        return 0;

    c->selected_item_history[c->dirlevel]=c->selected_item;
    c->table_history[c->dirlevel] = c->currtable;
    c->extra_history[c->dirlevel] = c->currextra;
    c->pos_history[c->dirlevel] = c->firstpos;
    c->dirlevel++;

    switch (c->currtable) {
        case root:
            c->currtable = newextra;
            c->currextra = newextra;
            break;

        case allartists:
        case searchartists:
            c->currtable = albums4artist;
            c->currextra = newextra;
            break;

        case allgenres:
            c->currtable = artist4genres;
            c->currextra = newextra;
            break;
        
        case artist4genres:
            c->currtable = albums4artist;
            c->currextra = newextra;
            break;
            
        case allalbums:
            c->currtable = songs4album;
            c->currextra = newextra;
            c->currextra2 = -1;
            break;
        case albums4artist:
        case searchalbums:
            c->currtable = songs4album;
            c->currextra2 = c->currextra;
            c->currextra = newextra;
            break;

        case allsongs:
        case songs4album:
        case songs4artist:
        case searchsongs:
            c->dirlevel--;
            if (tagtree_play_folder(c) >= 0)
                rc = 2;
            break;

        case search:
            rc = kbd_input(searchstring, sizeof(searchstring));
            if (rc == -1 || !searchstring[0])
                c->dirlevel--;
            else
                c->currtable = newextra;
            break;

        default:
            c->dirlevel--;
            break;
    }
    c->selected_item=0;
    gui_synclist_select_item(&tree_lists, c->selected_item);

    return rc;
}

void tagtree_exit(struct tree_context* c)
{
    c->dirfull = false;
    c->dirlevel--;
    c->selected_item=c->selected_item_history[c->dirlevel];
    gui_synclist_select_item(&tree_lists, c->selected_item);
    c->currtable = c->table_history[c->dirlevel];
    c->currextra = c->extra_history[c->dirlevel];
    c->firstpos  = c->pos_history[c->dirlevel];

    /* Just to be sure when chunked browsing is used. */
    tagcache_search_finish(&tcs);
}

int tagtree_get_filename(struct tree_context* c, char *buf, int buflen)
{
    struct tagentry *entry = (struct tagentry *)c->dircache;
    
    entry += c->selected_item;

    tagcache_search(&tcs, tag_filename);
    tagcache_search_add_filter(&tcs, tag_title, entry->newtable);
    
    if (!tagcache_get_next(&tcs))
    {
        tagcache_search_finish(&tcs);
        return -1;
    }

    strncpy(buf, tcs.result, buflen-1);
    tagcache_search_finish(&tcs);
    
    return 0;
}

#if 0
bool tagtree_rename_tag(struct tree_context *c, const char *newtext)
{
    struct tagentry *dptr = (struct tagentry *)c->dircache;
    int extra, extra2;
    int type;
    
    dptr += c->selected_item;
    extra = dptr->newtable;
    extra2 = dptr->extraseek;

    switch (c->currtable) {
        case allgenres:
            tagcache_search(&tcs, tag_title);
            tagcache_search_add_filter(&tcs, tag_genre, extra);
            type = tag_genre;
            break;

        case allalbums:
            tagcache_search(&tcs, tag_title);
            tagcache_search_add_filter(&tcs, tag_album, extra);
            type = tag_album;
            break;

        case allartists:
            tagcache_search(&tcs, tag_title);
            tagcache_search_add_filter(&tcs, tag_artist, extra);
            type = tag_artist;
            break;

        case artist4genres:
            tagcache_search(&tcs, tag_title);
            tagcache_search_add_filter(&tcs, tag_genre, extra);
            type = tag_artist;
            break;
    
        case albums4artist:
            tagcache_search(&tcs, tag_title);
            tagcache_search_add_filter(&tcs, tag_album, extra);
            tagcache_search_add_filter(&tcs, tag_artist, extra2);
            type = tag_album;
            break;

        default:
            logf("wrong table");
            return false;
    }

    while (tagcache_get_next(&tcs))
    {
        // tagcache_modify(&tcs, type, newtext);
    }
    
    tagcache_search_finish(&tcs);
    return true;
}
#endif
   
static int tagtree_play_folder(struct tree_context* c)
{
    struct tagentry *entry = (struct tagentry *)c->dircache;
    int i;

    if (playlist_create(NULL, NULL) < 0) {
        logf("Failed creating playlist\n");
        return -1;
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    cpu_boost(true);
#endif

    for (i=0; i < c->filesindir; i++) {
        tagcache_search(&tcs, tag_filename);
        tagcache_search_add_filter(&tcs, tag_title, entry[i].newtable);

        if (!tagcache_get_next(&tcs))
        {
            tagcache_search_finish(&tcs);
            continue ;
        }
        playlist_insert_track(NULL, tcs.result, PLAYLIST_INSERT, false);
        tagcache_search_finish(&tcs);
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    cpu_boost(false);
#endif
    
    if (global_settings.playlist_shuffle)
        c->selected_item = playlist_shuffle(current_tick, c->selected_item);
    if (!global_settings.play_selected)
        c->selected_item = 0;
    gui_synclist_select_item(&tree_lists, c->selected_item);

    playlist_start(c->selected_item,0);

    return 0;
}

#ifdef HAVE_LCD_BITMAP
const char* tagtree_get_icon(struct tree_context* c)
#else
int   tagtree_get_icon(struct tree_context* c)
#endif
{
    int icon;

    switch (c->currtable)
    {
        case allsongs:
        case songs4album:
        case songs4artist:
        case searchsongs:
            icon = Icon_Audio;
            break;

        default:
            icon = Icon_Folder;
            break;
    }

#ifdef HAVE_LCD_BITMAP
    return (char *)bitmap_icons_6x8[icon];
#else
    return icon;
#endif
}
