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
#include "buffer.h"
#include "atoi.h"

#define FILE_SEARCH_INSTRUCTIONS ROCKBOX_DIR "/tagnavi.config"

static int tagtree_play_folder(struct tree_context* c);

static const int numeric_tags[] = { tag_year, tag_length, tag_bitrate, tag_tracknumber };
static const int string_tags[] = { tag_artist, tag_title, tag_album, tag_composer, tag_genre };

static char searchstring[32];
struct tagentry {
    char *name;
    int newtable;
    int extraseek;
};

#define MAX_TAGS 5

struct search_instruction {
    char name[64];
    int tagorder[MAX_TAGS];
    int tagorder_count;
    struct tagcache_search_clause clause[MAX_TAGS][TAGCACHE_MAX_CLAUSES];
    int clause_count[MAX_TAGS];
    int result_seek[MAX_TAGS];
};

static struct search_instruction *si, *csi;
static int si_count = 0;
static const char *strp;

static int get_token_str(char *buf, int size)
{
    /* Find the start. */
    while (*strp != '"' && *strp != '\0')
        strp++;

    if (*strp == '\0' || *(++strp) == '\0')
        return -1;
    
    /* Read the data. */
    while (*strp != '"' && *strp != '\0' && --size > 0)
        *(buf++) = *(strp++);
    
    *buf = '\0';
    if (*strp != '"')
        return -2;
    
    strp++;
    
    return 0;
}

#define MATCH(tag,str1,str2,settag) \
    if (!strcasecmp(str1, str2)) { \
        *tag = settag; \
        return 1; \
    }

static int get_tag(int *tag)
{
    char buf[32];
    int i;
    
    /* Find the start. */
    while (*strp == ' ' && *strp != '\0')
        strp++;
    
    if (*strp == '\0')
        return 0;
    
    for (i = 0; i < (int)sizeof(buf)-1; i++)
    {
        if (*strp == '\0' || *strp == ' ')
            break ;
        buf[i] = *strp;
        strp++;
    }
    buf[i] = '\0';
    
    MATCH(tag, buf, "artist", tag_artist);
    MATCH(tag, buf, "song", tag_title);
    MATCH(tag, buf, "album", tag_album);
    MATCH(tag, buf, "genre", tag_genre);
    MATCH(tag, buf, "composer", tag_composer);
    MATCH(tag, buf, "year", tag_year);
    MATCH(tag, buf, "length", tag_length);
    MATCH(tag, buf, "tracknum", tag_tracknumber);
    MATCH(tag, buf, "bitrate", tag_bitrate);
    
    logf("NO MATCH: %s\n", buf);
    if (buf[0] == '?')
        return 0;
    
    return -1;
}

static int get_clause(int *condition)
{
    char buf[4];
    int i;
    
    /* Find the start. */
    while (*strp == ' ' && *strp != '\0')
        strp++;
    
    if (*strp == '\0')
        return 0;
    
    for (i = 0; i < (int)sizeof(buf)-1; i++)
    {
        if (*strp == '\0' || *strp == ' ')
            break ;
        buf[i] = *strp;
        strp++;
    }
    buf[i] = '\0';
    
    MATCH(condition, buf, "=", clause_is);
    MATCH(condition, buf, ">", clause_gt);
    MATCH(condition, buf, ">=", clause_gteq);
    MATCH(condition, buf, "<", clause_lt);
    MATCH(condition, buf, "<=", clause_lteq);
    MATCH(condition, buf, "~", clause_contains);
    MATCH(condition, buf, "^", clause_begins_with);
    MATCH(condition, buf, "$", clause_ends_with);
    
    return 0;
}

static bool add_clause(struct search_instruction *inst,
                       int tag, int type, const char *str)
{
    int len = strlen(str);
    struct tagcache_search_clause *clause;
    
    if (inst->clause_count[inst->tagorder_count] >= TAGCACHE_MAX_CLAUSES)
    {
        logf("Too many clauses");
        return false;
    }
    
    clause = &inst->clause[inst->tagorder_count]
        [inst->clause_count[inst->tagorder_count]];
    if (len >= (int)sizeof(clause->str) - 1)
    {
        logf("Too long str in condition");
        return false;
    }
    
    clause->tag = tag;
    clause->type = type;
    if (len == 0)
        clause->input = true;
    else
        clause->input = false;
    
    if (tagcache_is_numeric_tag(tag))
    {
        clause->numeric = true;
        clause->numeric_data = atoi(str);
    }
    else
    {
        clause->numeric = false;
        strcpy(clause->str, str);
    }
    
    inst->clause_count[inst->tagorder_count]++;
    
    return true;
}

static int get_condition(struct search_instruction *inst)
{
    int tag;
    int condition;
    char buf[32];
    
    switch (*strp)
    {
        case '?':
        case ' ':
        case '&':
            strp++;
            return 1;
        case ':':
            strp++;
        case '\0':
            return 0;
    }

    if (get_tag(&tag) <= 0)
        return -1;
    
    if (get_clause(&condition) <= 0)
        return -2;
    
    if (get_token_str(buf, sizeof buf) < 0)
        return -3;
    
    logf("got clause: %d/%d [%s]", tag, condition, buf);
    add_clause(inst, tag, condition, buf);
    
    return 1;
}

/* example search:
 * "Best" artist ? year >= "2000" & title !^ "crap" & genre = "good genre" \
 *      : album  ? year >= "2000" : songs
 * ^  begins with
 * *  contains
 * $  ends with
 */

static bool parse_search(struct search_instruction *inst, const char *str)
{
    int ret;
    
    memset(inst, 0, sizeof(struct search_instruction));
    strp = str;
    
    if (get_token_str(inst->name, sizeof inst->name) < 0)
    {
        logf("No name found.");
        return false;
    }
    
    while (inst->tagorder_count < MAX_TAGS)
    {
        ret = get_tag(&inst->tagorder[inst->tagorder_count]);
        if (ret < 0) 
        {
            logf("Parse error #1");
            return false;
        }
        
        if (ret == 0)
            break ;
        
        logf("tag: %d", inst->tagorder[inst->tagorder_count]);
        
        while (get_condition(inst) > 0) ;

        inst->tagorder_count++;
    }
    
    return true;
}


static struct tagcache_search tcs;

static int compare(const void *p1, const void *p2)
{
    struct tagentry *e1 = (struct tagentry *)p1;
    struct tagentry *e2 = (struct tagentry *)p2;
    
    return strncasecmp(e1->name, e2->name, MAX_PATH);
}

void tagtree_init(void)
{
    int fd;
    char buf[256];
    int pos = 0;
    
    si_count = 0;
    
    fd = open(FILE_SEARCH_INSTRUCTIONS, O_RDONLY);
    if (fd < 0)
    {
        logf("Search instruction file not found.");
        return ;
    }
    
    si = (struct search_instruction *)(((long)audiobuf & ~0x03) + 0x04);
    
    while ( 1 )
    {
        char *p;
        char *next = NULL;
        int rc;
        
        rc = read(fd, &buf[pos], sizeof(buf)-pos-1);
        if (rc >= 0)
            buf[pos+rc] = '\0';
        
        if ( (p = strchr(buf, '\r')) != NULL)
        {
            *p = '\0';
            next = ++p;
        }
        else
            p = buf;
        
        if ( (p = strchr(p, '\n')) != NULL)
        {
            *p = '\0';
            next = ++p;
        }
        
        if (!parse_search(si + si_count, buf))
            break;
        si_count++;
        
        if (next)
        {
            pos = sizeof(buf) - ((long)next - (long)buf) - 1;
            memmove(buf, next, pos);
        }
        else
            break ;
    }
    close(fd);
    
    audiobuf += sizeof(struct search_instruction) * si_count + 4;
}

int tagtree_load(struct tree_context* c)
{
    int i;
    int namebufused = 0;
    struct tagentry *dptr = (struct tagentry *)c->dircache;
    bool sort = false;

    int table = c->currtable;
    int extra = c->currextra;
    
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
            for (i = 0; i < si_count; i++)
            {
                dptr->name = (si+i)->name;
                dptr->newtable = navibrowse;
                dptr->extraseek = i;
                dptr++;
            }
            c->dirlength = c->filesindir = i;
            
            return c->dirlength;
        }

        case navibrowse:
            logf("navibrowse...");
            tagcache_search(&tcs, csi->tagorder[extra]);
            for (i = 0; i < extra; i++)
            {
                tagcache_search_add_filter(&tcs, csi->tagorder[i], csi->result_seek[i]);
                sort = true;
            }
        
            for (i = 0; i < csi->clause_count[extra]; i++)
            {
                tagcache_search_add_clause(&tcs, &csi->clause[extra][i]);
                sort = true;
            }
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
        if (!tcs.ramsearch || csi->tagorder[extra] == tag_title)
        {
            dptr->name = &c->name_buffer[namebufused];
            if (csi->tagorder[extra] == tag_title)
            {
                snprintf(dptr->name, c->name_buffer_size - namebufused, "%02d. %s",
                         tagcache_get_numeric(&tcs, tag_tracknumber), 
                         tcs.result);
                namebufused += strlen(dptr->name) + 1;
                if (namebufused >= c->name_buffer_size)
                {
                    logf("buffer full, 1 entry missed.");
                    c->dirfull = true;
                    break ;
                }
            }
            else
            {
                namebufused += tcs.result_len;
                if (namebufused >= c->name_buffer_size)
                {
                    logf("buffer full, 1 entry missed.");
                    c->dirfull = true;
                    break ;
                }
                strcpy(dptr->name, tcs.result);
            }
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

    if (sort)
        qsort(c->dircache, i, c->dentry_size, compare);
    
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
            if (newextra == navibrowse)
            {
                int i, j;
                
                csi = si+dptr->extraseek;
                c->currextra = 0;
                
                /* Read input as necessary. */
                for (i = 0; i < csi->tagorder_count; i++)
                {
                    for (j = 0; j < csi->clause_count[i]; j++)
                    {
                        if (!csi->clause[i][j].input)
                            continue;
                        
                        rc = kbd_input(searchstring, sizeof(searchstring));
                        if (rc == -1 || !searchstring[0])
                        {
                            c->dirlevel--;
                            break;
                        }
                        
                        if (csi->clause[i][j].numeric)
                            csi->clause[i][j].numeric_data = atoi(searchstring);
                        else
                            strncpy(csi->clause[i][j].str, searchstring,
                                    sizeof(csi->clause[i][j].str)-1);
                    }
                }
                
            }
            break;

        case navibrowse:
            csi->result_seek[c->currextra] = newextra;
            if (c->currextra < csi->tagorder_count-1)
            {
                c->currextra++;
                break;
            }
        
            c->dirlevel--;
            if (csi->tagorder[c->currextra] == tag_title)
            {
                if (tagtree_play_folder(c) >= 0)
                    rc = 2;
            }
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
        case navibrowse:
            if (csi->tagorder[c->currextra] == tag_title)
                icon = Icon_Audio;
            else
                icon = Icon_Folder;
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
