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

static char searchstring[32];
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

static int current_offset;
static int current_entry_count;

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

bool show_search_progress(bool init, int count)
{
    static int last_tick = 0;
    
    if (init)
    {
        last_tick = current_tick;
        return true;
    }
    
    if (current_tick - last_tick > HZ/2)
    {
        gui_syncsplash(0, true, str(LANG_PLAYLIST_SEARCH_MSG), count,
#if CONFIG_KEYPAD == PLAYER_PAD
                       str(LANG_STOP_ABORT)
#else
                       str(LANG_OFF_ABORT)
#endif
                       );
        if (SETTINGS_CANCEL == button_get(false))
            return false;
        last_tick = current_tick;
    }
    
    return true;
}

int retrieve_entries(struct tree_context *c, struct tagcache_search *tcs, 
                     int offset, bool init)
{
    struct tagentry *dptr = (struct tagentry *)c->dircache;
    int i;
    int namebufused = 0;
    int total_count = 0;
    int extra = c->currextra;
    bool sort = false;
    
    if (init
#ifdef HAVE_TC_RAMCACHE
        && !tagcache_is_ramcache()
#endif
        )
    {
        show_search_progress(true, 0);
        gui_syncsplash(0, true, str(LANG_PLAYLIST_SEARCH_MSG),
                       0, csi->name);
    }
    
    if (!tagcache_search(tcs, csi->tagorder[extra]))
        return -1;
    
    for (i = 0; i < extra; i++)
    {
        tagcache_search_add_filter(tcs, csi->tagorder[i], csi->result_seek[i]);
        sort = true;
    }
    
    for (i = 0; i < csi->clause_count[extra]; i++)
    {
        tagcache_search_add_clause(tcs, &csi->clause[extra][i]);
        sort = true;
    }
    
    current_offset = offset;
    current_entry_count = 0;
    c->dirfull = false;
    while (tagcache_get_next(tcs))
    {
        if (total_count++ < offset)
            continue;
        
        dptr->newtable = tcs->result_seek;
        if (!tcs->ramsearch || csi->tagorder[extra] == tag_title)
        {
            int tracknum = -1;
            
            dptr->name = &c->name_buffer[namebufused];
            if (csi->tagorder[extra] == tag_title)
                tracknum = tagcache_get_numeric(tcs, tag_tracknumber);
            
            if (tracknum > 0)
            {
                snprintf(dptr->name, c->name_buffer_size - namebufused, "%02d. %s",
                         tracknum, tcs->result);
                namebufused += strlen(dptr->name) + 1;
                if (namebufused >= c->name_buffer_size)
                {
                    logf("chunk mode #1: %d", current_entry_count);
                    c->dirfull = true;
                    sort = false;
                    break ;
                }
            }
            else
            {
                namebufused += tcs->result_len;
                if (namebufused >= c->name_buffer_size)
                {
                    logf("chunk mode #2: %d", current_entry_count);
                    c->dirfull = true;
                    sort = false;
                    break ;
                }
                strcpy(dptr->name, tcs->result);
            }
        }
        else
            dptr->name = tcs->result;
        
        dptr++;
        current_entry_count++;

        if (current_entry_count >= global_settings.max_files_in_dir)
        {
            logf("chunk mode #3: %d", current_entry_count);
            c->dirfull = true;
            sort = false;
            break ;
        }
        
        if (init && !tcs->ramsearch)
        {
            if (!show_search_progress(false, i))
            {
                tagcache_search_finish(tcs);
                return current_entry_count;
            }
        }
    }
    
    if (sort)
        qsort(c->dircache, current_entry_count, c->dentry_size, compare);
    
    if (!init)
    {
        tagcache_search_finish(tcs);
        return current_entry_count;
    }
    
    while (tagcache_get_next(tcs))
    {
        if (!tcs->ramsearch)
        {
            if (!show_search_progress(false, total_count))
                break;
        }
        total_count++;
    }
    
    tagcache_search_finish(tcs);
    return total_count;
}

static int load_root(struct tree_context *c)
{
    struct tagentry *dptr = (struct tagentry *)c->dircache;
    int i;
    
    c->currtable = root;
    for (i = 0; i < si_count; i++)
    {
        dptr->name = (si+i)->name;
        dptr->newtable = navibrowse;
        dptr->extraseek = i;
        dptr++;
    }
    
    current_offset = 0;
    current_entry_count = i;
    
    return i;
}

int tagtree_load(struct tree_context* c)
{
    int count;
    int table = c->currtable;
    
    c->dentry_size = sizeof(struct tagentry);

    if (!table)
    {
        c->dirfull = false;
        table = root;
        c->currtable = table;
    }

    switch (table) 
    {
        case root:
            count = load_root(c);
            break;

        case navibrowse:
            logf("navibrowse...");
            count = retrieve_entries(c, &tcs, 0, true);
            break;
        
        default:
            logf("Unsupported table %d\n", table);
            return -1;
    }
    
    if (count < 0)
    {
        c->dirlevel = 0;
        count = load_root(c);
        gui_syncsplash(HZ, true, str(LANG_TAGCACHE_BUSY));
    }

    /* The _total_ numer of entries available. */
    c->dirlength = c->filesindir = count;
    
    return count;
}

int tagtree_enter(struct tree_context* c)
{
    int rc = 0;
    struct tagentry *dptr;
    int newextra;

    dptr = tagtree_get_entry(c, c->selected_item);
    
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
                            return 0;
                        }
                        
                        if (csi->clause[i][j].numeric)
                            csi->clause[i][j].numeric_data = atoi(searchstring);
                        else
                            strncpy(csi->clause[i][j].str, searchstring,
                                    sizeof(csi->clause[i][j].str)-1);
                    }
                }
            }
        
            c->currtable = newextra;
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
    struct tagentry *entry;
    
    entry = tagtree_get_entry(c, c->selected_item);

    if (!tagcache_search(&tcs, tag_filename))
        return -1;

    tagcache_search_add_filter(&tcs, tag_title, entry->newtable);
    
    if (!tagcache_get_next(&tcs))
    {
        tagcache_search_finish(&tcs);
        return -2;
    }

    strncpy(buf, tcs.result, buflen-1);
    tagcache_search_finish(&tcs);
    
    return 0;
}
   
static int tagtree_play_folder(struct tree_context* c)
{
    int i;
    char buf[MAX_PATH];

    if (playlist_create(NULL, NULL) < 0)
    {
        logf("Failed creating playlist\n");
        return -1;
    }

    cpu_boost(true);
    if (!tagcache_search(&tcs, tag_filename))
    {
        gui_syncsplash(HZ, true, str(LANG_TAGCACHE_BUSY));
        return -1;
    }
    
    for (i=0; i < c->filesindir; i++)
    {
        if (!show_search_progress(false, i))
            break;
        
        if (!tagcache_retrieve(&tcs, tagtree_get_entry(c, i)->newtable, 
                               buf, sizeof buf))
        {
            continue;
        }

        playlist_insert_track(NULL, buf, PLAYLIST_INSERT, false);
    }
    tagcache_search_finish(&tcs);
    cpu_boost(false);
    
    if (global_settings.playlist_shuffle)
        c->selected_item = playlist_shuffle(current_tick, c->selected_item);
    if (!global_settings.play_selected)
        c->selected_item = 0;
    gui_synclist_select_item(&tree_lists, c->selected_item);

    playlist_start(c->selected_item,0);

    return 0;
}

struct tagentry* tagtree_get_entry(struct tree_context *c, int id)
{
    struct tagentry *entry = (struct tagentry *)c->dircache;
    int realid = id - current_offset;
    
    /* Load the next chunk if necessary. */
    if (realid >= current_entry_count || realid < 0)
    {
        if (retrieve_entries(c, &tcs, MAX(0, id - (current_entry_count / 2)), 
                             false) < 0)
        {
            return NULL;
        }
        realid = id - current_offset;
    }
    
    return &entry[realid];
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
