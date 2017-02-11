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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/**
 * Basic structure on this file was copied from dbtree.c and modified to
 * support the tag cache interface.
 */

/*#define LOGF_ENABLE*/

#include <stdio.h>
#include <stdlib.h>
#include "string-extra.h"
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "splash.h"
#include "icons.h"
#include "tree.h"
#include "action.h"
#include "settings.h"
#include "tagcache.h"
#include "tagtree.h"
#include "lang.h"
#include "logf.h"
#include "playlist.h"
#include "keyboard.h"
#include "gui/list.h"
#include "core_alloc.h"
#include "yesno.h"
#include "misc.h"
#include "filetypes.h"
#include "audio.h"
#include "appevents.h"
#include "storage.h"
#include "dir.h"
#include "playback.h"
#include "strnatcmp.h"
#include "panic.h"

#define str_or_empty(x) (x ? x : "(NULL)")

#define FILE_SEARCH_INSTRUCTIONS ROCKBOX_DIR "/tagnavi.config"

static int tagtree_play_folder(struct tree_context* c);

/* this needs to be same size as struct entry (tree.h) and name needs to be
 * the first; so that they're compatible enough to walk arrays of both
 * derefencing the name member*/
struct tagentry {
    char* name;
    int newtable;
    int extraseek;
};

static struct tagentry* tagtree_get_entry(struct tree_context *c, int id);

#define SEARCHSTR_SIZE 256

enum table {
    ROOT = 1,
    NAVIBROWSE,
    ALLSUBENTRIES,
    PLAYTRACK,
};

static const struct id3_to_search_mapping {
    char   *string;
    size_t id3_offset;
} id3_to_search_mapping[] = {
    { "",  0 },              /* offset n/a */
    { "#directory#",  0 },   /* offset n/a */
    { "#title#",  offsetof(struct mp3entry, title) },
    { "#artist#", offsetof(struct mp3entry, artist) },
    { "#album#",  offsetof(struct mp3entry, album) },
    { "#genre#",  offsetof(struct mp3entry, genre_string) },
    { "#composer#",  offsetof(struct mp3entry, composer) },
    { "#albumartist#",   offsetof(struct mp3entry, albumartist) },
};
enum variables {
    var_sorttype = 100,
    var_limit,
    var_strip,
    var_menu_start,
    var_include,
    var_rootmenu,
    var_format,
    menu_next,
    menu_load,
};

/* Capacity 10 000 entries (for example 10k different artists) */
#define UNIQBUF_SIZE (64*1024)
static long uniqbuf[UNIQBUF_SIZE / sizeof(long)];

#define MAX_TAGS 5
#define MAX_MENU_ID_SIZE 32

static bool sort_inverse;

/*
 * "%3d. %s" autoscore title %sort = "inverse" %limit = "100"
 *
 * valid = true
 * formatstr = "%-3d. %s"
 * tags[0] = tag_autoscore
 * tags[1] = tag_title
 * tag_count = 2
 *
 * limit = 100
 * sort_inverse = true
 */
struct display_format {
    char name[32];
    struct tagcache_search_clause *clause[TAGCACHE_MAX_CLAUSES];
    int clause_count;
    char *formatstr;
    int group_id;
    int tags[MAX_TAGS];
    int tag_count;

    int limit;
    int strip;
    bool sort_inverse;
};

static struct display_format *formats[TAGMENU_MAX_FMTS];
static int format_count;

struct menu_entry {
    char name[64];
    int type;
    struct search_instruction {
        char name[64];
        int tagorder[MAX_TAGS];
        int tagorder_count;
        struct tagcache_search_clause *clause[MAX_TAGS][TAGCACHE_MAX_CLAUSES];
        int format_id[MAX_TAGS];
        int clause_count[MAX_TAGS];
        int result_seek[MAX_TAGS];
    } si;
    int link;
};

struct menu_root {
    char title[64];
    char id[MAX_MENU_ID_SIZE];
    int itemcount;
    struct menu_entry *items[TAGMENU_MAX_ITEMS];
};

struct match
{
    const char* str;
    int symbol;
};

/* Statusbar text of the current view. */
static char current_title[MAX_TAGS][128];

static struct menu_root * menus[TAGMENU_MAX_MENUS];
static struct menu_root * menu;
static struct search_instruction *csi;
static const char *strp;
static int menu_count;
static int rootmenu;

static int current_offset;
static int current_entry_count;

static struct tree_context *tc;

/* a few memory alloc helper */
static int tagtree_handle, lock_count;
static size_t tagtree_bufsize, tagtree_buf_used;

#define UPDATE(x, y) { x = (typeof(x))((char*)(x) + (y)); }
static int move_callback(int handle, void* current, void* new)
{
    (void)handle; (void)current; (void)new;
    ptrdiff_t diff = new - current;

    if (lock_count > 0)
        return BUFLIB_CB_CANNOT_MOVE;

    if (menu)
        UPDATE(menu, diff);

    if (csi)
        UPDATE(csi, diff);

    /* loop over menus */
    for(int i = 0; i < menu_count; i++)
    {
        struct menu_root* menu = menus[i];
        /* then over the menu_entries of a menu */
        for(int j = 0; j < menu->itemcount; j++)
        {
            struct menu_entry* mentry = menu->items[j];
            /* then over the search_instructions of each menu_entry */
            for(int k = 0; k < mentry->si.tagorder_count; k++)
            {
                for(int l = 0; l < mentry->si.clause_count[k]; l++)
                {
                    UPDATE(mentry->si.clause[k][l]->str, diff);
                    UPDATE(mentry->si.clause[k][l], diff);
                }
            }
            UPDATE(menu->items[j], diff);
        }
        UPDATE(menus[i], diff);
    }

    /* now the same game for formats */
    for(int i = 0; i < format_count; i++)
    {
        for(int j = 0; j < formats[i]->clause_count; j++)
        {
            UPDATE(formats[i]->clause[j]->str, diff);
            UPDATE(formats[i]->clause[j], diff);
        }

        if (formats[i]->formatstr)
            UPDATE(formats[i]->formatstr, diff);

        UPDATE(formats[i], diff);
    }
    return BUFLIB_CB_OK;
}
#undef UPDATE

static inline void tagtree_lock(void)
{
    lock_count++;
}

static inline void tagtree_unlock(void)
{
    lock_count--;
}

static struct buflib_callbacks ops = {
    .move_callback = move_callback,
    .shrink_callback = NULL,
};

static void* tagtree_alloc(size_t size)
{
    char* buf = core_get_data(tagtree_handle) + tagtree_buf_used;
    size = ALIGN_UP(size, sizeof(void*));
    tagtree_buf_used += size;
    return buf;
}

static void* tagtree_alloc0(size_t size)
{
    void* ret = tagtree_alloc(size);
    memset(ret, 0, size);
    return ret;
}

static char* tagtree_strdup(const char* buf)
{
    size_t len = strlen(buf) + 1;
    char* dest = tagtree_alloc(len);
    strcpy(dest, buf);
    return dest;
}

/* save to call without locking */
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

static int get_tag(int *tag)
{
    static const struct match get_tag_match[] =
    {
        {"album", tag_album},
        {"artist", tag_artist},
        {"bitrate", tag_bitrate},
        {"composer", tag_composer},
        {"comment", tag_comment},
        {"albumartist", tag_albumartist},
        {"ensemble", tag_albumartist},
        {"grouping", tag_grouping},
        {"genre", tag_genre},
        {"length", tag_length},
        {"Lm", tag_virt_length_min},
        {"Ls", tag_virt_length_sec},
        {"Pm", tag_virt_playtime_min},
        {"Ps", tag_virt_playtime_sec},
        {"title", tag_title},
        {"filename", tag_filename},
        {"basename", tag_virt_basename},
        {"tracknum", tag_tracknumber},
        {"discnum", tag_discnumber},
        {"year", tag_year},
        {"playcount", tag_playcount},
        {"rating", tag_rating},
        {"lastplayed", tag_lastplayed},
        {"lastelapsed", tag_lastelapsed},
        {"lastoffset", tag_lastoffset},
        {"commitid", tag_commitid},
        {"entryage", tag_virt_entryage},
        {"autoscore", tag_virt_autoscore},
        {"%sort", var_sorttype},
        {"%limit", var_limit},
        {"%strip", var_strip},
        {"%menu_start", var_menu_start},
        {"%include", var_include},
        {"%root_menu", var_rootmenu},
        {"%format", var_format},
        {"->", menu_next},
        {"==>", menu_load}
    };
    char buf[128];
    unsigned int i;

    /* Find the start. */
    while ((*strp == ' ' || *strp == '>') && *strp != '\0')
        strp++;

    if (*strp == '\0' || *strp == '?')
        return 0;

    for (i = 0; i < sizeof(buf)-1; i++)
    {
        if (*strp == '\0' || *strp == ' ')
            break ;
        buf[i] = *strp;
        strp++;
    }
    buf[i] = '\0';

    for (i = 0; i < ARRAYLEN(get_tag_match); i++)
    {
        if (!strcasecmp(buf, get_tag_match[i].str))
        {
            *tag = get_tag_match[i].symbol;
            return 1;
        }
    }

    logf("NO MATCH: %s\n", buf);
    if (buf[0] == '?')
        return 0;

    return -1;
}

static int get_clause(int *condition)
{
    static const struct match get_clause_match[] =
    {
        {"=", clause_is},
        {"==", clause_is},
        {"!=", clause_is_not},
        {">", clause_gt},
        {">=", clause_gteq},
        {"<", clause_lt},
        {"<=", clause_lteq},
        {"~", clause_contains},
        {"!~", clause_not_contains},
        {"^", clause_begins_with},
        {"!^", clause_not_begins_with},
        {"$", clause_ends_with},
        {"!$", clause_not_ends_with},
        {"@", clause_oneof}
    };

    char buf[4];
    unsigned int i;

    /* Find the start. */
    while (*strp == ' ' && *strp != '\0')
        strp++;

    if (*strp == '\0')
        return 0;

    for (i = 0; i < sizeof(buf)-1; i++)
    {
        if (*strp == '\0' || *strp == ' ')
            break ;
        buf[i] = *strp;
        strp++;
    }
    buf[i] = '\0';

    for (i = 0; i < ARRAYLEN(get_clause_match); i++)
    {
        if (!strcasecmp(buf, get_clause_match[i].str))
        {
            *condition = get_clause_match[i].symbol;
            return 1;
        }
    }

    return 0;
}

static bool read_clause(struct tagcache_search_clause *clause)
{
    char buf[SEARCHSTR_SIZE];
    unsigned int i;

    if (get_tag(&clause->tag) <= 0)
        return false;

    if (get_clause(&clause->type) <= 0)
        return false;

    if (get_token_str(buf, sizeof buf) < 0)
        return false;

    for (i=0; i<ARRAYLEN(id3_to_search_mapping); i++)
    {
        if (!strcasecmp(buf, id3_to_search_mapping[i].string))
            break;
    }

    if (i<ARRAYLEN(id3_to_search_mapping)) /* runtime search operand found */
    {
        clause->source = source_runtime+i;
        clause->str = tagtree_alloc(SEARCHSTR_SIZE);
    }
    else
    {
        clause->source = source_constant;
        clause->str = tagtree_strdup(buf);
    }

    if (TAGCACHE_IS_NUMERIC(clause->tag))
    {
        clause->numeric = true;
        clause->numeric_data = atoi(clause->str);
    }
    else
        clause->numeric = false;

    logf("got clause: %d/%d [%s]", clause->tag, clause->type, clause->str);

    return true;
}

static bool read_variable(char *buf, int size)
{
    int condition;

    if (!get_clause(&condition))
        return false;

    if (condition != clause_is)
        return false;

    if (get_token_str(buf, size) < 0)
        return false;

    return true;
}

/* "%3d. %s" autoscore title %sort = "inverse" %limit = "100" */
static int get_format_str(struct display_format *fmt)
{
    int ret;
    char buf[128];
    int i;

    memset(fmt, 0, sizeof(struct display_format));

    if (get_token_str(fmt->name, sizeof fmt->name) < 0)
        return -10;

    /* Determine the group id */
    fmt->group_id = 0;
    for (i = 0; i < format_count; i++)
    {
        if (!strcasecmp(formats[i]->name, fmt->name))
        {
            fmt->group_id = formats[i]->group_id;
            break;
        }

        if (formats[i]->group_id > fmt->group_id)
            fmt->group_id = formats[i]->group_id;
    }

    if (i == format_count)
        fmt->group_id++;

    logf("format: (%d) %s", fmt->group_id, fmt->name);

    if (get_token_str(buf, sizeof buf) < 0)
        return -10;

    fmt->formatstr = tagtree_strdup(buf);

    while (fmt->tag_count < MAX_TAGS)
    {
        ret = get_tag(&fmt->tags[fmt->tag_count]);
        if (ret < 0)
            return -11;

        if (ret == 0)
            break;

        switch (fmt->tags[fmt->tag_count]) {
        case var_sorttype:
            if (!read_variable(buf, sizeof buf))
                return -12;
            if (!strcasecmp("inverse", buf))
                fmt->sort_inverse = true;
            break;

        case var_limit:
            if (!read_variable(buf, sizeof buf))
                return -13;
            fmt->limit = atoi(buf);
            break;

        case var_strip:
            if (!read_variable(buf, sizeof buf))
                return -14;
            fmt->strip = atoi(buf);
            break;

        default:
            fmt->tag_count++;
        }
    }

    return 1;
}

static int add_format(const char *buf)
{
    if (format_count >= TAGMENU_MAX_FMTS)
    {
        logf("too many formats");
        return -1;
    }

    strp = buf;

    if (formats[format_count] == NULL)
        formats[format_count] = tagtree_alloc0(sizeof(struct display_format));

    if (get_format_str(formats[format_count]) < 0)
    {
        logf("get_format_str() parser failed!");
        memset(formats[format_count], 0, sizeof(struct display_format));
        return -4;
    }

    while (*strp != '\0' && *strp != '?')
        strp++;

    if (*strp == '?')
    {
        int clause_count = 0;
        strp++;

        tagtree_lock();
        while (1)
        {
            struct tagcache_search_clause *newclause;

            if (clause_count >= TAGCACHE_MAX_CLAUSES)
            {
                logf("too many clauses");
                break;
            }

            newclause = tagtree_alloc(sizeof(struct tagcache_search_clause));

            formats[format_count]->clause[clause_count] = newclause;
            if (!read_clause(newclause))
                break;

            clause_count++;
        }
        tagtree_unlock();

        formats[format_count]->clause_count = clause_count;
    }

    format_count++;

    return 1;
}

static int get_condition(struct search_instruction *inst)
{
    struct tagcache_search_clause *new_clause;
    int clause_count;
    char buf[128];

    switch (*strp)
    {
        case '=':
        {
            int i;

            if (get_token_str(buf, sizeof buf) < 0)
                return -1;

            for (i = 0; i < format_count; i++)
            {
                if (!strcasecmp(formats[i]->name, buf))
                    break;
            }

            if (i == format_count)
            {
                logf("format not found: %s", buf);
                return -2;
            }

            inst->format_id[inst->tagorder_count] = formats[i]->group_id;
            return 1;
        }
        case '?':
        case ' ':
        case '&':
            strp++;
            return 1;
        case '-':
        case '\0':
            return 0;
    }

    clause_count = inst->clause_count[inst->tagorder_count];
    if (clause_count >= TAGCACHE_MAX_CLAUSES)
    {
        logf("Too many clauses");
        return false;
    }

    new_clause = tagtree_alloc(sizeof(struct tagcache_search_clause));
    inst->clause[inst->tagorder_count][clause_count] = new_clause;

    if (*strp == '|')
    {
        strp++;
        new_clause->type = clause_logical_or;
    }
    else
    {
        tagtree_lock();
        bool ret = read_clause(new_clause);
        tagtree_unlock();
        if (!ret)
            return -1;
    }
    inst->clause_count[inst->tagorder_count]++;

    return 1;
}

/* example search:
 * "Best" artist ? year >= "2000" & title !^ "crap" & genre = "good genre" \
 *      : album  ? year >= "2000" : songs
 * ^  begins with
 * *  contains
 * $  ends with
 */

static bool parse_search(struct menu_entry *entry, const char *str)
{
    int ret;
    int type;
    struct search_instruction *inst = &entry->si;
    char buf[MAX_PATH];
    int i;

    strp = str;

    /* Parse entry name */
    if (get_token_str(entry->name, sizeof entry->name) < 0)
    {
        logf("No name found.");
        return false;
    }

    /* Parse entry type */
    if (get_tag(&entry->type) <= 0)
        return false;

    if (entry->type == menu_load)
    {
        if (get_token_str(buf, sizeof buf) < 0)
            return false;

        /* Find the matching root menu or "create" it */
        for (i = 0; i < menu_count; i++)
        {
            if (!strcasecmp(menus[i]->id, buf))
            {
                entry->link = i;
                return true;
            }
        }

        if (menu_count >= TAGMENU_MAX_MENUS)
        {
            logf("max menucount reached");
            return false;
        }

        /* Allocate a new menu unless link is found. */
        menus[menu_count] = tagtree_alloc0(sizeof(struct menu_root));
        strlcpy(menus[menu_count]->id, buf, MAX_MENU_ID_SIZE);
        entry->link = menu_count;
        ++menu_count;

        return true;
    }

    if (entry->type != menu_next)
        return false;

    while (inst->tagorder_count < MAX_TAGS)
    {
        ret = get_tag(&inst->tagorder[inst->tagorder_count]);
        if (ret < 0)
        {
            logf("Parse error #1");
            logf("%s", strp);
            return false;
        }

        if (ret == 0)
            break ;

        logf("tag: %d", inst->tagorder[inst->tagorder_count]);

        tagtree_lock();
        while ( (ret = get_condition(inst)) > 0 ) ;
        tagtree_unlock();

        if (ret < 0)
            return false;

        inst->tagorder_count++;

        if (get_tag(&type) <= 0 || type != menu_next)
            break;
    }

    return true;
}

static int compare(const void *p1, const void *p2)
{
    struct tagentry *e1 = (struct tagentry *)p1;
    struct tagentry *e2 = (struct tagentry *)p2;

    if (sort_inverse)
        return strncasecmp(e2->name, e1->name, MAX_PATH);

    return strncasecmp(e1->name, e2->name, MAX_PATH);
}

static int nat_compare(const void *p1, const void *p2)
{
    struct tagentry *e1 = (struct tagentry *)p1;
    struct tagentry *e2 = (struct tagentry *)p2;

    if (sort_inverse)
        return strnatcasecmp(e2->name, e1->name);

    return strnatcasecmp(e1->name, e2->name);
}

static void tagtree_buffer_event(unsigned short id, void *ev_data)
{
    (void)id;
    struct tagcache_search tcs;
    struct mp3entry *id3 = ((struct track_event *)ev_data)->id3;

    bool runtimedb = global_settings.runtimedb;
    bool autoresume = global_settings.autoresume_enable;

    /* Do not gather data unless proper setting has been enabled. */
    if (!runtimedb && !autoresume)
        return;

    logf("be:%s", id3->path);

    while (! tagcache_is_fully_initialized())
        yield();

    if (!tagcache_find_index(&tcs, id3->path))
    {
        logf("tc stat: not found: %s", id3->path);
        return;
    }

    if (runtimedb)
    {
        id3->playcount  = tagcache_get_numeric(&tcs, tag_playcount);
        if (!id3->rating)
            id3->rating = tagcache_get_numeric(&tcs, tag_rating);
        id3->lastplayed = tagcache_get_numeric(&tcs, tag_lastplayed);
        id3->score      = tagcache_get_numeric(&tcs, tag_virt_autoscore) / 10;
        id3->playtime   = tagcache_get_numeric(&tcs, tag_playtime);

        logf("-> %ld/%ld", id3->playcount, id3->playtime);
    }

 #if CONFIG_CODEC == SWCODEC
    if (autoresume)
    {
        /* Load current file resume info if not already defined (by
           another resume mechanism) */
        if (id3->elapsed == 0)
        {
            id3->elapsed = tagcache_get_numeric(&tcs, tag_lastelapsed);

            logf("tagtree_buffer_event: Set elapsed for %s to %lX\n",
                 str_or_empty(id3->title), id3->elapsed);
        }

        if (id3->offset == 0)
        {
            id3->offset = tagcache_get_numeric(&tcs, tag_lastoffset);

            logf("tagtree_buffer_event: Set offset for %s to %lX\n",
                 str_or_empty(id3->title), id3->offset);
        }
    }
 #endif

    /* Store our tagcache index pointer. */
    id3->tagcache_idx = tcs.idx_id+1;

    tagcache_search_finish(&tcs);
}

static void tagtree_track_finish_event(unsigned short id, void *ev_data)
{
    (void)id;
    struct track_event *te = (struct track_event *)ev_data;
    struct mp3entry *id3 = te->id3;

    long tagcache_idx = id3->tagcache_idx;
    if (!tagcache_idx)
    {
        logf("No tagcache index pointer found");
        return;
    }
    tagcache_idx--;

#if CONFIG_CODEC == SWCODEC /* HWCODEC doesn't have automatic_skip */
    bool auto_skip = te->flags & TEF_AUTO_SKIP;
#endif
    bool runtimedb = global_settings.runtimedb;
    bool autoresume = global_settings.autoresume_enable;

    /* Don't process unplayed tracks, or tracks interrupted within the
       first 15 seconds but always process autoresume point */
    if (runtimedb && (id3->elapsed == 0
#if CONFIG_CODEC == SWCODEC
        || (id3->elapsed < 15 * 1000 && !auto_skip)
#endif
        ))
    {
        logf("not db logging unplayed or skipped track");
        runtimedb = false;
    }

#if CONFIG_CODEC == SWCODEC
    /* 3s because that is the threshold the WPS uses to rewind instead
       of skip backwards */
    if (autoresume && (id3->elapsed == 0
        || (id3->elapsed < 3 * 1000 && !auto_skip)))
    {
        logf("not logging autoresume");
        autoresume = false;
    }
#endif

    /* Do not gather data unless proper setting has been enabled and at least
       one is still slated to be recorded */
    if (!(runtimedb || autoresume))
    {
        logf("runtimedb gathering and autoresume not enabled/ignored");
        return;
    }

    long lastplayed = tagcache_increase_serial();
    if (lastplayed < 0)
    {
        logf("incorrect tc serial:%ld", lastplayed);
        return;
    }

    if (runtimedb)
    {
        long playcount;
        long playtime;

        playcount = id3->playcount + 1;

        /* Ignore the last 15s (crossfade etc.) */
        playtime = id3->playtime + MIN(id3->length, id3->elapsed + 15 * 1000);

        logf("ube:%s", id3->path);
        logf("-> %ld/%ld", playcount, playtime);
        logf("-> %ld/%ld/%ld", id3->elapsed, id3->length,
             MIN(id3->length, id3->elapsed + 15 * 1000));

        /* Queue the updates to the tagcache system. */
        tagcache_update_numeric(tagcache_idx, tag_playcount, playcount);
        tagcache_update_numeric(tagcache_idx, tag_playtime, playtime);
        tagcache_update_numeric(tagcache_idx, tag_lastplayed, lastplayed);
    }

#if CONFIG_CODEC == SWCODEC
    if (autoresume)
    {
        unsigned long elapsed = auto_skip ? 0 : id3->elapsed;
        unsigned long offset = auto_skip ? 0 : id3->offset;
        tagcache_update_numeric(tagcache_idx, tag_lastelapsed, elapsed);
        tagcache_update_numeric(tagcache_idx, tag_lastoffset, offset);

        logf("tagtree_track_finish_event: Save resume for %s: %lX %lX",
             str_or_empty(id3->title), elapsed, offset);
    }
#endif
}

bool tagtree_export(void)
{
    struct tagcache_search tcs;

    splash(0, str(LANG_CREATING));
    if (!tagcache_create_changelog(&tcs))
    {
        splash(HZ*2, ID2P(LANG_FAILED));
    }

    return false;
}

bool tagtree_import(void)
{
    splash(0, ID2P(LANG_WAIT));
    if (!tagcache_import_changelog())
    {
        splash(HZ*2, ID2P(LANG_FAILED));
    }

    return false;
}

static bool parse_menu(const char *filename);

static int parse_line(int n, char *buf, void *parameters)
{
    char data[256];
    int variable;
    static bool read_menu;
    int i;
    char *p;

    (void)parameters;

    /* Strip possible <CR> at end of line. */
    p = strchr(buf, '\r');
    if (p != NULL)
        *p = '\0';

    logf("parse:%d/%s", n, buf);

    /* First line, do initialisation. */
    if (n == 0)
    {
        if (strcasecmp(TAGNAVI_VERSION, buf))
        {
            logf("Version mismatch");
            return -1;
        }

        read_menu = false;
    }

    if (buf[0] == '#')
        return 0;

    if (buf[0] == '\0')
    {
        if (read_menu)
        {
            /* End the menu */
            read_menu = false;
        }
        return 0;
    }

    if (!read_menu)
    {
        strp = buf;
        if (get_tag(&variable) <= 0)
            return 0;

        switch (variable)
        {
            case var_format:
                if (add_format(strp) < 0)
                {
                    logf("Format add fail: %s", data);
                }
                break;

            case var_include:
                if (get_token_str(data, sizeof(data)) < 0)
                {
                    logf("%%include empty");
                    return 0;
                }

                if (!parse_menu(data))
                {
                    logf("Load menu fail: %s", data);
                }
                break;

            case var_menu_start:
                if (menu_count >= TAGMENU_MAX_MENUS)
                {
                    logf("max menucount reached");
                    return 0;
                }

                if (get_token_str(data, sizeof data) < 0)
                {
                    logf("%%menu_start id empty");
                    return 0;
                }

                menu = NULL;
                for (i = 0; i < menu_count; i++)
                {
                    if (!strcasecmp(menus[i]->id, data))
                    {
                        menu = menus[i];
                    }
                }

                if (menu == NULL)
                {
                    menus[menu_count] = tagtree_alloc0(sizeof(struct menu_root));
                    menu = menus[menu_count];
                    ++menu_count;
                    strlcpy(menu->id, data, MAX_MENU_ID_SIZE);
                }

                if (get_token_str(menu->title, sizeof(menu->title)) < 0)
                {
                    logf("%%menu_start title empty");
                    return 0;
                }
                logf("menu: %s", menu->title);
                read_menu = true;
                break;

            case var_rootmenu:
                /* Only set root menu once. */
                if (rootmenu >= 0)
                    break;

                if (get_token_str(data, sizeof(data)) < 0)
                {
                    logf("%%rootmenu empty");
                    return 0;
                }

                for (i = 0; i < menu_count; i++)
                {
                    if (!strcasecmp(menus[i]->id, data))
                    {
                        rootmenu = i;
                    }
                }
                break;
        }

        return 0;
    }

    if (menu->itemcount >= TAGMENU_MAX_ITEMS)
    {
        logf("max itemcount reached");
        return 0;
    }

    /* Allocate */
    if (menu->items[menu->itemcount] == NULL)
        menu->items[menu->itemcount] = tagtree_alloc0(sizeof(struct menu_entry));

    tagtree_lock();
    if (parse_search(menu->items[menu->itemcount], buf))
        menu->itemcount++;
    tagtree_unlock();

    return 0;
}

static bool parse_menu(const char *filename)
{
    int fd;
    char buf[1024];

    if (menu_count >= TAGMENU_MAX_MENUS)
    {
        logf("max menucount reached");
        return false;
    }

    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        logf("Search instruction file not found.");
        return false;
    }

    /* Now read file for real, parsing into si */
    fast_readline(fd, buf, sizeof buf, NULL, parse_line);
    close(fd);

    return true;
}

void tagtree_init(void)
{
    format_count = 0;
    menu_count = 0;
    menu = NULL;
    rootmenu = -1;
    tagtree_handle = core_alloc_maximum("tagtree", &tagtree_bufsize, &ops);
    parse_menu(FILE_SEARCH_INSTRUCTIONS);

    /* safety check since tree.c needs to cast tagentry to entry */
    if (sizeof(struct tagentry) != sizeof(struct entry))
        panicf("tagentry(%zu) and entry mismatch(%zu)",
                sizeof(struct tagentry), sizeof(struct entry));
    if (lock_count > 0)
        panicf("tagtree locked after parsing");

    /* If no root menu is set, assume it's the first single menu
     * we have. That shouldn't normally happen. */
    if (rootmenu < 0)
        rootmenu = 0;

    add_event(PLAYBACK_EVENT_TRACK_BUFFER, tagtree_buffer_event);
    add_event(PLAYBACK_EVENT_TRACK_FINISH, tagtree_track_finish_event);

    core_shrink(tagtree_handle, core_get_data(tagtree_handle), tagtree_buf_used);
}

static bool show_search_progress(bool init, int count)
{
    static int last_tick = 0;

    /* Don't show splashes for 1/2 second after starting search */
    if (init)
    {
        last_tick = current_tick + HZ/2;
        return true;
    }

    /* Update progress every 1/10 of a second */
    if (TIME_AFTER(current_tick, last_tick + HZ/10))
    {
        splashf(0, str(LANG_PLAYLIST_SEARCH_MSG), count, str(LANG_OFF_ABORT));
        if (action_userabort(TIMEOUT_NOBLOCK))
            return false;
        last_tick = current_tick;
        yield();
    }

    return true;
}

static int format_str(struct tagcache_search *tcs, struct display_format *fmt,
                      char *buf, int buf_size)
{
    char fmtbuf[20];
    bool read_format = false;
    unsigned fmtbuf_pos = 0;
    int parpos = 0;
    int buf_pos = 0;
    int i;

    memset(buf, 0, buf_size);
    for (i = 0; fmt->formatstr[i] != '\0'; i++)
    {
        if (fmt->formatstr[i] == '%')
        {
            read_format = true;
            fmtbuf_pos = 0;
            if (parpos >= fmt->tag_count)
            {
                logf("too many format tags");
                return -1;
            }
        }

        char formatchar = fmt->formatstr[i];

        if (read_format)
        {
            fmtbuf[fmtbuf_pos++] = formatchar;
            if (fmtbuf_pos >= sizeof fmtbuf)
            {
                logf("format parse error");
                return -2;
            }

            if (formatchar == 's' || formatchar == 'd')
            {
                unsigned space_left = buf_size - buf_pos;
                char tmpbuf[MAX_PATH];
                char *result;

                fmtbuf[fmtbuf_pos] = '\0';
                read_format = false;

                switch (formatchar)
                {
                case 's':
                    if (fmt->tags[parpos] == tcs->type)
                    {
                        result = tcs->result;
                    }
                    else
                    {
                        /* Need to fetch the tag data. */
                        int tag = fmt->tags[parpos];

                        if (!tagcache_retrieve(tcs, tcs->idx_id,
                                               (tag == tag_virt_basename ?
                                                tag_filename : tag),
                                               tmpbuf, sizeof tmpbuf))
                        {
                            logf("retrieve failed");
                            return -3;
                        }

                        if (tag == tag_virt_basename
                            && (result = strrchr(tmpbuf, '/')) != NULL)
                        {
                            result++;
                        }
                        else
                            result = tmpbuf;
                    }
                    buf_pos +=
                        snprintf(&buf[buf_pos], space_left, fmtbuf, result);
                    break;

                case 'd':
                    buf_pos +=
                        snprintf(&buf[buf_pos], space_left, fmtbuf,
                                 tagcache_get_numeric(tcs, fmt->tags[parpos]));
                }

                parpos++;
            }
        }
        else
            buf[buf_pos++] = formatchar;

        if (buf_pos >= buf_size - 1)    /* need at least one more byte for \0 */
        {
            logf("buffer overflow");
            return -4;
        }
    }

    buf[buf_pos++] = '\0';

    return 0;
}

static struct tagentry* get_entries(struct tree_context *tc)
{
    return core_get_data(tc->cache.entries_handle);
}

static int retrieve_entries(struct tree_context *c, int offset, bool init)
{
    struct tagcache_search tcs;
    struct display_format *fmt;
    int i;
    int namebufused = 0;
    int total_count = 0;
    int special_entry_count = 0;
    int level = c->currextra;
    int tag;
    bool sort = false;
    int sort_limit;
    int strip;

    /* Show search progress straight away if the disk needs to spin up,
       otherwise show it after the normal 1/2 second delay */
    show_search_progress(
#ifdef HAVE_DISK_STORAGE
        storage_disk_is_active()
#else
        true
#endif
        , 0);

    if (c->currtable == ALLSUBENTRIES)
    {
        tag = tag_title;
        level--;
    }
    else
        tag = csi->tagorder[level];

    if (!tagcache_search(&tcs, tag))
        return -1;

    /* Prevent duplicate entries in the search list. */
    tagcache_search_set_uniqbuf(&tcs, uniqbuf, UNIQBUF_SIZE);

    if (level || csi->clause_count[0] || TAGCACHE_IS_NUMERIC(tag))
        sort = true;

    for (i = 0; i < level; i++)
    {
        if (TAGCACHE_IS_NUMERIC(csi->tagorder[i]))
        {
            static struct tagcache_search_clause cc;

            memset(&cc, 0, sizeof(struct tagcache_search_clause));
            cc.tag = csi->tagorder[i];
            cc.type = clause_is;
            cc.numeric = true;
            cc.numeric_data = csi->result_seek[i];
            tagcache_search_add_clause(&tcs, &cc);
        }
        else
        {
            tagcache_search_add_filter(&tcs, csi->tagorder[i],
                                       csi->result_seek[i]);
        }
    }

    /* because tagcache saves the clauses, we need to lock the buffer
     * for the entire duration of the search */
    tagtree_lock();
    for (i = 0; i <= level; i++)
    {
        int j;

        for (j = 0; j < csi->clause_count[i]; j++)
            tagcache_search_add_clause(&tcs, csi->clause[i][j]);
    }

    current_offset = offset;
    current_entry_count = 0;
    c->dirfull = false;

    fmt = NULL;
    for (i = 0; i < format_count; i++)
    {
        if (formats[i]->group_id == csi->format_id[level])
            fmt = formats[i];
    }

    if (fmt)
    {
        sort_inverse = fmt->sort_inverse;
        sort_limit = fmt->limit;
        strip = fmt->strip;
        sort = true;
    }
    else
    {
        sort_inverse = false;
        sort_limit = 0;
        strip = 0;
    }

    /* lock buflib out due to possible yields */
    tree_lock_cache(c);
    struct tagentry *dptr = core_get_data(c->cache.entries_handle);

    if (tag != tag_title && tag != tag_filename)
    {
        if (offset == 0)
        {
            dptr->newtable = ALLSUBENTRIES;
            dptr->name = str(LANG_TAGNAVI_ALL_TRACKS);
            dptr++;
            current_entry_count++;
            special_entry_count++;
        }
        if (offset <= 1)
        {
            dptr->newtable = NAVIBROWSE;
            dptr->name = str(LANG_TAGNAVI_RANDOM);
            dptr->extraseek = -1;
            dptr++;
            current_entry_count++;
            special_entry_count++;
        }

        total_count += 2;
    }

    while (tagcache_get_next(&tcs))
    {
        if (total_count++ < offset)
            continue;

        dptr->newtable = NAVIBROWSE;
        if (tag == tag_title || tag == tag_filename)
        {
            dptr->newtable = PLAYTRACK;
            dptr->extraseek = tcs.idx_id;
        }
        else
            dptr->extraseek = tcs.result_seek;

        fmt = NULL;
        /* Check the format */
        tagtree_lock();
        for (i = 0; i < format_count; i++)
        {
            if (formats[i]->group_id != csi->format_id[level])
                continue;

            if (tagcache_check_clauses(&tcs, formats[i]->clause,
                                       formats[i]->clause_count))
            {
                fmt = formats[i];
                break;
            }
        }
        tagtree_unlock();

        if (strcmp(tcs.result, UNTAGGED) == 0)
        {
            tcs.result = str(LANG_TAGNAVI_UNTAGGED);
            tcs.result_len = strlen(tcs.result);
            tcs.ramresult = true;
        }

        if (!tcs.ramresult || fmt)
        {
            dptr->name = core_get_data(c->cache.name_buffer_handle)+namebufused;

            if (fmt)
            {
                int ret = format_str(&tcs, fmt, dptr->name,
                                     c->cache.name_buffer_size - namebufused);
                if (ret == -4)          /* buffer full */
                {
                    logf("chunk mode #2: %d", current_entry_count);
                    c->dirfull = true;
                    sort = false;
                    break ;
                }
                else if (ret < 0)
                {
                    logf("format_str() failed");
                    tagcache_search_finish(&tcs);
                    tree_unlock_cache(c);
                    tagtree_unlock();
                    return 0;
                }
                else
                    namebufused += strlen(dptr->name)+1;
            }
            else
            {
                namebufused += tcs.result_len;
                if (namebufused < c->cache.name_buffer_size)
                    strcpy(dptr->name, tcs.result);
                else
                {
                    logf("chunk mode #2a: %d", current_entry_count);
                    c->dirfull = true;
                    sort = false;
                    break ;
                }
            }
        }
        else
            dptr->name = tcs.result;

        dptr++;
        current_entry_count++;

        if (current_entry_count >= c->cache.max_entries)
        {
            logf("chunk mode #3: %d", current_entry_count);
            c->dirfull = true;
            sort = false;
            break ;
        }

        if (init && !tcs.ramsearch)
        {
            if (!show_search_progress(false, total_count))
            {   /* user aborted */
                tagcache_search_finish(&tcs);
                tree_unlock_cache(c);
                tagtree_unlock();
                return current_entry_count;
            }
        }
    }

    if (sort)
    {
        struct tagentry *entries = get_entries(c);
        qsort(&entries[special_entry_count],
              current_entry_count - special_entry_count,
              sizeof(struct tagentry),
              global_settings.interpret_numbers ? nat_compare : compare);
    }

    if (!init)
    {
        tagcache_search_finish(&tcs);
        tree_unlock_cache(c);
        tagtree_unlock();
        return current_entry_count;
    }

    while (tagcache_get_next(&tcs))
    {
        if (!tcs.ramsearch)
        {
            if (!show_search_progress(false, total_count))
                break;
        }
        total_count++;
    }

    tagcache_search_finish(&tcs);
    tree_unlock_cache(c);
    tagtree_unlock();

    if (!sort && (sort_inverse || sort_limit))
    {
        splashf(HZ*4, ID2P(LANG_SHOWDIR_BUFFER_FULL), total_count);
        logf("Too small dir buffer");
        return 0;
    }

    if (sort_limit)
        total_count = MIN(total_count, sort_limit);

    if (strip)
    {
        dptr = get_entries(c);
        for (i = special_entry_count; i < current_entry_count; i++, dptr++)
        {
            int len = strlen(dptr->name);

            if (len < strip)
                continue;

            dptr->name = &dptr->name[strip];
        }
    }

    return total_count;

}

static int load_root(struct tree_context *c)
{
    struct tagentry *dptr = core_get_data(c->cache.entries_handle);
    int i;

    tc = c;
    c->currtable = ROOT;
    if (c->dirlevel == 0)
        c->currextra = rootmenu;

    menu = menus[c->currextra];
    if (menu == NULL)
        return 0;

    for (i = 0; i < menu->itemcount; i++)
    {
        dptr->name = menu->items[i]->name;
        switch (menu->items[i]->type)
        {
            case menu_next:
                dptr->newtable = NAVIBROWSE;
                dptr->extraseek = i;
                break;

            case menu_load:
                dptr->newtable = ROOT;
                dptr->extraseek = menu->items[i]->link;
                break;
        }

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

    c->dirsindir = 0;

    if (!table)
    {
        c->dirfull = false;
        table = ROOT;
        c->currtable = table;
        c->currextra = rootmenu;
    }

    switch (table)
    {
        case ROOT:
            count = load_root(c);
            break;

        case ALLSUBENTRIES:
        case NAVIBROWSE:
            logf("navibrowse...");
            cpu_boost(true);
            count = retrieve_entries(c, 0, true);
            cpu_boost(false);
            break;

        default:
            logf("Unsupported table %d\n", table);
            return -1;
    }

    if (count < 0)
    {
        c->dirlevel = 0;
        count = load_root(c);
        splash(HZ, str(LANG_TAGCACHE_BUSY));
    }

    /* The _total_ numer of entries available. */
    c->dirlength = c->filesindir = count;

    return count;
}

int tagtree_enter(struct tree_context* c)
{
    int rc = 0;
    struct tagentry *dptr;
    struct mp3entry *id3;
    int newextra;
    int seek;
    int source;

    dptr = tagtree_get_entry(c, c->selected_item);

    c->dirfull = false;
    seek = dptr->extraseek;
    if (seek == -1)
    {
        if(c->filesindir<=2)
            return 0;
        srand(current_tick);
        dptr = (tagtree_get_entry(c, 2+(rand() % (c->filesindir-2))));
        seek = dptr->extraseek;
    }
    newextra = dptr->newtable;

    if (c->dirlevel >= MAX_DIR_LEVELS)
        return 0;

    c->selected_item_history[c->dirlevel]=c->selected_item;
    c->table_history[c->dirlevel] = c->currtable;
    c->extra_history[c->dirlevel] = c->currextra;
    c->pos_history[c->dirlevel] = c->firstpos;
    c->dirlevel++;

    /* lock buflib for possible I/O to protect dptr */
    tree_lock_cache(c);
    tagtree_lock();

    switch (c->currtable) {
        case ROOT:
            c->currextra = newextra;

            if (newextra == ROOT)
            {
                menu = menus[seek];
                c->currextra = seek;
            }

            else if (newextra == NAVIBROWSE)
            {
                int i, j;

                csi = &menu->items[seek]->si;
                c->currextra = 0;

                strlcpy(current_title[c->currextra], dptr->name,
                        sizeof(current_title[0]));

                /* Read input as necessary. */
                for (i = 0; i < csi->tagorder_count; i++)
                {
                    for (j = 0; j < csi->clause_count[i]; j++)
                    {
                        char* searchstring;

                        if (csi->clause[i][j]->type == clause_logical_or)
                            continue;

                        source = csi->clause[i][j]->source;

                        if (source == source_constant)
                            continue;

                        searchstring=csi->clause[i][j]->str;
                        *searchstring = '\0';

                        id3 = audio_current_track();

                        if (source == source_current_path && id3)
                        {
                            char *e;
                            strlcpy(searchstring, id3->path, SEARCHSTR_SIZE);
                            e = strrchr(searchstring, '/');
                            if (e)
                                *e = '\0';
                        }
                        else if (source > source_runtime && id3)
                        {

                            int k = source-source_runtime;
                            int offset = id3_to_search_mapping[k].id3_offset;
                            char **src = (char**)((char*)id3 + offset);
                            if (*src)
                            {
                                strlcpy(searchstring, *src, SEARCHSTR_SIZE);
                            }
                        }
                        else
                        {
                            rc = kbd_input(searchstring, SEARCHSTR_SIZE);
                            if (rc < 0 || !searchstring[0])
                            {
                                tagtree_exit(c);
                                tree_unlock_cache(c);
                                tagtree_unlock();
                                return 0;
                            }
                            if (csi->clause[i][j]->numeric)
                                csi->clause[i][j]->numeric_data = atoi(searchstring);
                        }


                    }
                }
            }
            c->currtable = newextra;

            break;

        case NAVIBROWSE:
        case ALLSUBENTRIES:
            if (newextra == PLAYTRACK)
            {
                if (global_settings.party_mode && audio_status()) {
                    splash(HZ, ID2P(LANG_PARTY_MODE));
                    break;
                }
                c->dirlevel--;
                /* about to create a new current playlist...
                 allow user to cancel the operation */
                if (!warn_on_pl_erase())
                    break;
                if (tagtree_play_folder(c) >= 0)
                    rc = 2;
                break;
            }

            c->currtable = newextra;
            csi->result_seek[c->currextra] = seek;
            if (c->currextra < csi->tagorder_count-1)
                c->currextra++;
            else
                c->dirlevel--;

            /* Update the statusbar title */
            strlcpy(current_title[c->currextra], dptr->name,
                    sizeof(current_title[0]));
            break;

        default:
            c->dirlevel--;
            break;
    }


    c->selected_item=0;
    gui_synclist_select_item(&tree_lists, c->selected_item);
    tree_unlock_cache(c);
    tagtree_unlock();

    return rc;
}

void tagtree_exit(struct tree_context* c)
{
    c->dirfull = false;
    if (c->dirlevel > 0)
        c->dirlevel--;
    c->selected_item=c->selected_item_history[c->dirlevel];
    gui_synclist_select_item(&tree_lists, c->selected_item);
    c->currtable = c->table_history[c->dirlevel];
    c->currextra = c->extra_history[c->dirlevel];
    c->firstpos  = c->pos_history[c->dirlevel];
}

int tagtree_get_filename(struct tree_context* c, char *buf, int buflen)
{
    struct tagcache_search tcs;
    int extraseek = tagtree_get_entry(c, c->selected_item)->extraseek;


    if (!tagcache_search(&tcs, tag_filename))
        return -1;

    if (!tagcache_retrieve(&tcs, extraseek, tcs.type, buf, buflen))
    {
        tagcache_search_finish(&tcs);
        return -2;
    }

    tagcache_search_finish(&tcs);

    return 0;
}

static bool insert_all_playlist(struct tree_context *c, int position, bool queue)
{
    struct tagcache_search tcs;
    int i;
    char buf[MAX_PATH];
    int from, to, direction;
    int files_left = c->filesindir;

    cpu_boost(true);
    if (!tagcache_search(&tcs, tag_filename))
    {
        splash(HZ, ID2P(LANG_TAGCACHE_BUSY));
        cpu_boost(false);
        return false;
    }

    if (position == PLAYLIST_REPLACE)
    {
        if (playlist_remove_all_tracks(NULL) == 0)
            position = PLAYLIST_INSERT_LAST;
        else
        {
            cpu_boost(false);
            return false;
        }
    }

    if (position == PLAYLIST_INSERT_FIRST)
    {
        from = c->filesindir - 1;
        to = -1;
        direction = -1;
    }
    else
    {
        from = 0;
        to = c->filesindir;
        direction = 1;
    }

    for (i = from; i != to; i += direction)
    {
        /* Count back to zero */
        if (!show_search_progress(false, files_left--))
            break;

        if (!tagcache_retrieve(&tcs, tagtree_get_entry(c, i)->extraseek,
                               tcs.type, buf, sizeof buf))
        {
            continue;
        }

        if (playlist_insert_track(NULL, buf, position, queue, false) < 0)
        {
            logf("playlist_insert_track failed");
            break;
        }
        yield();
    }
    playlist_sync(NULL);
    tagcache_search_finish(&tcs);
    cpu_boost(false);

    return true;
}

bool tagtree_insert_selection_playlist(int position, bool queue)
{
    char buf[MAX_PATH];
    int dirlevel = tc->dirlevel;
    int newtable;

    show_search_progress(
#ifdef HAVE_DISK_STORAGE
        storage_disk_is_active()
#else
        true
#endif
        , 0);


    /* We need to set the table to allsubentries. */
    newtable = tagtree_get_entry(tc, tc->selected_item)->newtable;

    /* Insert a single track? */
    if (newtable == PLAYTRACK)
    {
        if (tagtree_get_filename(tc, buf, sizeof buf) < 0)
        {
            logf("tagtree_get_filename failed");
            return false;
        }
        playlist_insert_track(NULL, buf, position, queue, true);

        return true;
    }

    if (newtable == NAVIBROWSE)
    {
        tagtree_enter(tc);
        tagtree_load(tc);
        newtable = tagtree_get_entry(tc, tc->selected_item)->newtable;
    }
    else if (newtable != ALLSUBENTRIES)
    {
        logf("unsupported table: %d", newtable);
        return false;
    }

    /* Now the current table should be allsubentries. */
    if (newtable != PLAYTRACK)
    {
        tagtree_enter(tc);
        tagtree_load(tc);
        newtable = tagtree_get_entry(tc, tc->selected_item)->newtable;

        /* And now the newtable should be playtrack. */
        if (newtable != PLAYTRACK)
        {
            logf("newtable: %d !!", newtable);
            tc->dirlevel = dirlevel;
            return false;
        }
    }

    if (tc->filesindir <= 0)
        splash(HZ, ID2P(LANG_END_PLAYLIST));
    else
    {
        logf("insert_all_playlist");
        if (!insert_all_playlist(tc, position, queue))
            splash(HZ*2, ID2P(LANG_FAILED));
    }

    /* Finally return the dirlevel to its original value. */
    while (tc->dirlevel > dirlevel)
        tagtree_exit(tc);
    tagtree_load(tc);

    return true;
}

static int tagtree_play_folder(struct tree_context* c)
{
    if (playlist_create(NULL, NULL) < 0)
    {
        logf("Failed creating playlist\n");
        return -1;
    }

    if (!insert_all_playlist(c, PLAYLIST_INSERT_LAST, false))
        return -2;

    if (global_settings.playlist_shuffle)
        c->selected_item = playlist_shuffle(current_tick, c->selected_item);
    if (!global_settings.play_selected)
        c->selected_item = 0;
    gui_synclist_select_item(&tree_lists, c->selected_item);

    playlist_start(c->selected_item, 0, 0);
    playlist_get_current()->num_inserted_tracks = 0; /* make warn on playlist erase work */
    return 0;
}

static struct tagentry* tagtree_get_entry(struct tree_context *c, int id)
{
    struct tagentry *entry;
    int realid = id - current_offset;

    /* Load the next chunk if necessary. */
    if (realid >= current_entry_count || realid < 0)
    {
        cpu_boost(true);
        if (retrieve_entries(c, MAX(0, id - (current_entry_count / 2)),
                             false) < 0)
        {
            logf("retrieve failed");
            cpu_boost(false);
            return NULL;
        }
        realid = id - current_offset;
        cpu_boost(false);
    }

    entry = get_entries(c);
    return &entry[realid];
}

char* tagtree_get_entry_name(struct tree_context *c, int id,
                                    char* buf, size_t bufsize)
{
    struct tagentry *entry = tagtree_get_entry(c, id);
    if (!entry)
        return NULL;
    strlcpy(buf, entry->name, bufsize);
    return buf;
}


char *tagtree_get_title(struct tree_context* c)
{
    switch (c->currtable)
    {
        case ROOT:
            return menu->title;

        case NAVIBROWSE:
        case ALLSUBENTRIES:
            return current_title[c->currextra];
    }

    return "?";
}

int tagtree_get_attr(struct tree_context* c)
{
    int attr = -1;
    switch (c->currtable)
    {
        case NAVIBROWSE:
            if (csi->tagorder[c->currextra] == tag_title)
                attr = FILE_ATTR_AUDIO;
            else
                attr = ATTR_DIRECTORY;
            break;

        case ALLSUBENTRIES:
            attr = FILE_ATTR_AUDIO;
            break;

        default:
            attr = ATTR_DIRECTORY;
            break;
    }

    return attr;
}

int tagtree_get_icon(struct tree_context* c)
{
    int icon = Icon_Folder;

    if (tagtree_get_attr(c) == FILE_ATTR_AUDIO)
        icon = Icon_Audio;

    return icon;
}
