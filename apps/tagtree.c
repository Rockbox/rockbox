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
#include "talk.h"
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
#include "onplay.h"
#include "plugin.h"

#define str_or_empty(x) (x ? x : "(NULL)")

#define TAGNAVI_DEFAULT_CONFIG  ROCKBOX_DIR "/tagnavi.config"
#define TAGNAVI_USER_CONFIG     ROCKBOX_DIR "/tagnavi_user.config"

static int tagtree_play_folder(struct tree_context* c);

/* reuse of tagtree data after tagtree_play_folder() */
static uint32_t loaded_entries_crc = 0;


/* this needs to be same size as struct entry (tree.h) and name needs to be
 * the first; so that they're compatible enough to walk arrays of both
 * derefencing the name member*/
struct tagentry {
    char* name;
    int newtable;
    int extraseek;
    int customaction;
};

static struct tagentry* tagtree_get_entry(struct tree_context *c, int id);

#define SEARCHSTR_SIZE 256

enum table {
    TABLE_ROOT = 1,
    TABLE_NAVIBROWSE,
    TABLE_ALLSUBENTRIES,
    TABLE_PLAYTRACK,
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
    menu_byfirstletter,
    menu_next,
    menu_load,
    menu_reload,
    menu_shuffle_songs,
};

/* Capacity 10 000 entries (for example 10k different artists) */
#define UNIQBUF_SIZE (64*1024)
static uint32_t uniqbuf[UNIQBUF_SIZE / sizeof(uint32_t)];

#define MAX_TAGS 5
#define MAX_MENU_ID_SIZE 32

#define RELOAD_TAGTREE (-1024)

static int(*qsort_fn)(const char*, const char*, size_t);
/* dummmy functions to allow compatibility strncasecmp */
static int strnatcasecmp_n(const char *a, const char *b, size_t n)
{
    (void)n;
     return strnatcasecmp(a, b);
}
static int strnatcasecmp_n_inv(const char *a, const char *b, size_t n)
{
    (void)n;
     return strnatcasecmp(b, a);
}
static int strncasecmp_inv(const char *a, const char *b, size_t n)
{
    return strncasecmp(b, a, n);
}

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

#define MENUENTRY_MAX_NAME 64
struct menu_entry {
    char name[MENUENTRY_MAX_NAME];
    int type;
    struct search_instruction {
        char name[MENUENTRY_MAX_NAME];
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
    char title[MENUENTRY_MAX_NAME];
    char id[MAX_MENU_ID_SIZE];
    int itemcount;
    struct menu_entry *items[TAGMENU_MAX_ITEMS];
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

static int max_history_level; /* depth of menu levels with applicable history */
static int selected_item_history[MAX_DIR_LEVELS];
static int table_history[MAX_DIR_LEVELS];
static int extra_history[MAX_DIR_LEVELS];

/* a few memory alloc helper */
static int tagtree_handle;
static size_t tagtree_bufsize, tagtree_buf_used;

#define UPDATE(x, y) { x = (typeof(x))((char*)(x) + (y)); }
static int move_callback(int handle, void* current, void* new)
{
    (void)handle; (void)current; (void)new;
    ptrdiff_t diff = new - current;

    if (menu)
        UPDATE(menu, diff);

    if (csi)
        UPDATE(csi, diff);

    /* loop over menus */
    for(int i = 0; i < menu_count; i++)
    {
        struct menu_root* menuroot = menus[i];
        /* then over the menu_entries of a menu */
        for(int j = 0; j < menuroot->itemcount; j++)
        {
            struct menu_entry* mentry = menuroot->items[j];
            /* then over the search_instructions of each menu_entry */
            for(int k = 0; k < mentry->si.tagorder_count; k++)
            {
                for(int l = 0; l < mentry->si.clause_count[k]; l++)
                {
                    if(mentry->si.clause[k][l]->str)
                        UPDATE(mentry->si.clause[k][l]->str, diff);
                    UPDATE(mentry->si.clause[k][l], diff);
                }
            }
            UPDATE(menuroot->items[j], diff);
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

static struct buflib_callbacks ops = {
    .move_callback = move_callback,
    .shrink_callback = NULL,
};

static uint32_t tagtree_data_crc(struct tree_context* c)
{
    char* buf;
    uint32_t crc;
    buf = core_get_data(tagtree_handle); /* data for the search clauses etc */
    crc = crc_32(buf, tagtree_buf_used, c->dirlength);
    buf = core_get_data(c->cache.name_buffer_handle); /* names */
    crc = crc_32(buf, c->cache.name_buffer_size, crc);
    buf = core_get_data(c->cache.entries_handle); /* tagentries */
    crc = crc_32(buf, c->cache.max_entries * sizeof(struct tagentry), crc);
    logf("%s 0x%x", __func__, crc);
    return crc;
}

static void* tagtree_alloc(size_t size)
{
    size = ALIGN_UP(size, sizeof(void*));
    if (size > (tagtree_bufsize - tagtree_buf_used))
        return NULL;

    char* buf = core_get_data(tagtree_handle) + tagtree_buf_used;

    tagtree_buf_used += size;
    return buf;
}

static void* tagtree_alloc0(size_t size)
{
    void* ret = tagtree_alloc(size);
    if (ret)
        memset(ret, 0, size);
    return ret;
}

static char* tagtree_strdup(const char* buf)
{
    size_t len = strlen(buf) + 1;
    char* dest = tagtree_alloc(len);
    if (dest)
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
    #define TAG_MATCH(str, tag) {str, sizeof(str) - 1, tag}
    struct match {const char* str; uint16_t len; uint16_t symbol;};
    static const struct match get_tag_match[] =
    {
        TAG_MATCH("lm", tag_virt_length_min),
        TAG_MATCH("ls", tag_virt_length_sec),
        TAG_MATCH("pm", tag_virt_playtime_min),
        TAG_MATCH("ps", tag_virt_playtime_sec),
        TAG_MATCH("->", menu_next),
        TAG_MATCH("~>", menu_shuffle_songs),

        TAG_MATCH("==>", menu_load),

        TAG_MATCH("year", tag_year),

        TAG_MATCH("album", tag_album),
        TAG_MATCH("genre", tag_genre),
        TAG_MATCH("title", tag_title),
        TAG_MATCH("%sort", var_sorttype),

        TAG_MATCH("artist", tag_artist),
        TAG_MATCH("length", tag_length),
        TAG_MATCH("rating", tag_rating),
        TAG_MATCH("%limit", var_limit),
        TAG_MATCH("%strip", var_strip),

        TAG_MATCH("bitrate", tag_bitrate),
        TAG_MATCH("comment", tag_comment),
        TAG_MATCH("discnum", tag_discnumber),
        TAG_MATCH("%format", var_format),
        TAG_MATCH("%reload", menu_reload),

        TAG_MATCH("filename", tag_filename),
        TAG_MATCH("basename", tag_virt_basename),
        TAG_MATCH("tracknum", tag_tracknumber),
        TAG_MATCH("composer", tag_composer),
        TAG_MATCH("ensemble", tag_albumartist),
        TAG_MATCH("grouping", tag_grouping),
        TAG_MATCH("entryage", tag_virt_entryage),
        TAG_MATCH("commitid", tag_commitid),
        TAG_MATCH("%include", var_include),

        TAG_MATCH("playcount", tag_playcount),
        TAG_MATCH("autoscore", tag_virt_autoscore),

        TAG_MATCH("lastplayed", tag_lastplayed),
        TAG_MATCH("lastoffset", tag_lastoffset),
        TAG_MATCH("%root_menu", var_rootmenu),

        TAG_MATCH("albumartist", tag_albumartist),
        TAG_MATCH("lastelapsed", tag_lastelapsed),
        TAG_MATCH("%menu_start", var_menu_start),

        TAG_MATCH("%byfirstletter", menu_byfirstletter),
        TAG_MATCH("canonicalartist", tag_virt_canonicalartist),

        TAG_MATCH("", 0) /* sentinel */
    };
    #undef TAG_MATCH
    const size_t max_cmd_sz = 32; /* needs to be >= to len of longest tagstr */
    const char *tagstr;
    uint16_t tagstr_len;
    const struct match *match;

    /* Find the start. */
    while (*strp == ' ' || *strp == '>')
        strp++;

    if (*strp == '\0' || *strp == '?')
        return 0;

    tagstr = strp;
    for (tagstr_len = 0; tagstr_len < max_cmd_sz; tagstr_len++)
    {
        if (*strp == '\0' || *strp == ' ')
            break ;
        strp++;
    }

    for (match = get_tag_match; match->len != 0; match++)
    {
        if (tagstr_len != match->len)
            continue;
        else if (strncasecmp(tagstr, match->str, match->len) == 0)
        {
            *tag = match->symbol;
            return 1;
        }
    }

    logf("NO MATCH: %.*s\n", tagstr_len, tagstr);

    return -1;
}

static int get_clause(int *condition)
{
    /* one or two operator conditionals */
    #define OPS2VAL(op1, op2) ((int)op1 << 8 | (int)op2)
    #define CLAUSE(op1, op2, symbol) {OPS2VAL(op1, op2), symbol }

    struct clause_symbol {int value;int symbol;};
    const struct clause_symbol *match;
    static const struct clause_symbol get_clause_match[] =
    {
        CLAUSE('=', ' ', clause_is),
        CLAUSE('=', '=', clause_is),
        CLAUSE('!', '=', clause_is_not),
        CLAUSE('>', ' ', clause_gt),
        CLAUSE('>', '=', clause_gteq),
        CLAUSE('<', ' ', clause_lt),
        CLAUSE('<', '=', clause_lteq),
        CLAUSE('~', ' ', clause_contains),
        CLAUSE('!', '~', clause_not_contains),
        CLAUSE('^', ' ', clause_begins_with),
        CLAUSE('!', '^', clause_not_begins_with),
        CLAUSE('$', ' ', clause_ends_with),
        CLAUSE('!', '$', clause_not_ends_with),
        CLAUSE('@', '^', clause_begins_oneof),
        CLAUSE('@', '$', clause_ends_oneof),
        CLAUSE('@', ' ', clause_oneof),
        CLAUSE(0, 0, 0) /* sentinel */
    };

    /* Find the start. */
    while (*strp == ' ' && *strp != '\0')
        strp++;

    if (*strp == '\0')
        return 0;

    char op1 = strp[0];
    char op2 = strp[1];
    if (op2 == '"') /*allow " to end a single op conditional */
        op2 = ' ';

    int value = OPS2VAL(op1, op2);

    for (match = get_clause_match; match->value != 0; match++)
    {
        if (value == match->value)
        {
            *condition = match->symbol;
            return 1;
        }
    }

    return 0;
#undef OPS2VAL
#undef CLAUSE
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

    if (!clause->str)
    {
        logf("tagtree failed to allocate %s", "clause string");
        return false;
    }
    else if (TAGCACHE_IS_NUMERIC(clause->tag))
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
    if (!formats[format_count])
    {
        logf("tagtree failed to allocate %s", "format string");
        return -2;
    }
    if (get_format_str(formats[format_count]) < 0)
    {
        logf("get_format_str() parser failed!");
        if (formats[format_count])
            memset(formats[format_count], 0, sizeof(struct display_format));
        return -4;
    }

    while (*strp != '\0' && *strp != '?')
        strp++;

    if (*strp == '?')
    {
        int clause_count = 0;
        strp++;

        core_pin(tagtree_handle);
        while (1)
        {
            struct tagcache_search_clause *new_clause;

            if (clause_count >= TAGCACHE_MAX_CLAUSES)
            {
                logf("too many clauses");
                break;
            }

            new_clause = tagtree_alloc(sizeof(struct tagcache_search_clause));
            if (!new_clause)
            {
                logf("tagtree failed to allocate %s", "search clause");
                return -3;
            }
            formats[format_count]->clause[clause_count] = new_clause;
            if (!read_clause(new_clause))
                break;

            clause_count++;
        }
        core_unpin(tagtree_handle);

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
        return -2;
    }

    new_clause = tagtree_alloc0(sizeof(struct tagcache_search_clause));
    if (!new_clause)
    {
        logf("tagtree failed to allocate %s", "search clause");
        return -3;
    }

    inst->clause[inst->tagorder_count][clause_count] = new_clause;

    if (*strp == '|')
    {
        strp++;
        new_clause->type = clause_logical_or;
    }
    else
    {
        core_pin(tagtree_handle);
        bool ret = read_clause(new_clause);
        core_unpin(tagtree_handle);
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
        if (!menus[menu_count])
        {
            logf("tagtree failed to allocate %s", "menu");
            return false;
        }
        strmemccpy(menus[menu_count]->id, buf, MAX_MENU_ID_SIZE);
        entry->link = menu_count;
        ++menu_count;

        return true;
    }

    if (entry->type != menu_next && entry->type != menu_shuffle_songs)
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

        core_pin(tagtree_handle);
        while ( (ret = get_condition(inst)) > 0 ) ;
        core_unpin(tagtree_handle);

        if (ret < 0)
            return false;

        inst->tagorder_count++;

        if (get_tag(&type) <= 0 || (type != menu_next && type != menu_shuffle_songs))
            break;
    }

    return true;
}

static int compare(const void *p1, const void *p2)
{
    struct tagentry *e1 = (struct tagentry *)p1;
    struct tagentry *e2 = (struct tagentry *)p2;
    return qsort_fn(e1->name, e2->name, MAX_PATH);
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

    bool auto_skip = te->flags & TEF_AUTO_SKIP;
    bool runtimedb = global_settings.runtimedb;
    bool autoresume = global_settings.autoresume_enable;

    /* Don't process unplayed tracks, or tracks interrupted within the
       first 15 seconds but always process autoresume point */
    if (runtimedb && (id3->elapsed == 0
        || (id3->elapsed < 15 * 1000 && !auto_skip)
        ))
    {
        logf("not db logging unplayed or skipped track");
        runtimedb = false;
    }

    /* 3s because that is the threshold the WPS uses to rewind instead
       of skip backwards */
    if (autoresume && (id3->elapsed == 0
        || (id3->elapsed < 3 * 1000 && !auto_skip)))
    {
        logf("not logging autoresume");
        autoresume = false;
    }

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

    if (autoresume)
    {
        unsigned long elapsed = auto_skip ? 0 : id3->elapsed;
        unsigned long offset = auto_skip ? 0 : id3->offset;
        tagcache_update_numeric(tagcache_idx, tag_lastelapsed, elapsed);
        tagcache_update_numeric(tagcache_idx, tag_lastoffset, offset);

        logf("tagtree_track_finish_event: Save resume for %s: %lX %lX",
             str_or_empty(id3->title), elapsed, offset);
    }
}

int tagtree_export(void)
{
    struct tagcache_search tcs;

    splash(0, str(LANG_CREATING));
    if (!tagcache_create_changelog(&tcs))
    {
        splash(HZ*2, ID2P(LANG_FAILED));
    }

    return 0;
}

int tagtree_import(void)
{
    splash(0, ID2P(LANG_WAIT));
    if (!tagcache_import_changelog())
    {
        splash(HZ*2, ID2P(LANG_FAILED));
    }

    return 0;
}

static bool alloc_menu_parse_buf(char *buf, int type)
{
    /* allocate a new menu item (if needed) initialize it with data parsed
       from buf Note: allows setting menu type, type ignored when < 0
    */
    /* Allocate */
    if (menu->items[menu->itemcount] == NULL)
        menu->items[menu->itemcount] = tagtree_alloc0(sizeof(struct menu_entry));
    if (!menu->items[menu->itemcount])
    {
        logf("tagtree failed to allocate %s", "menu items");
        return false;
    }

    /* Initialize */
    core_pin(tagtree_handle);
    if (parse_search(menu->items[menu->itemcount], buf))
    {
        if (type >= 0)
            menu->items[menu->itemcount]->type = type;
        menu->itemcount++;
    }
    core_unpin(tagtree_handle);
    return true;
}

static void build_firstletter_menu(char *buf, size_t bufsz)
{
    const char *subitem = buf;
    size_t l = strlen(buf) + 1;
    buf+=l;
    bufsz-=l;

    const char * const fmt ="\"%s\"-> %s ? %s %c\"%c\"-> %s =\"fmt_title\"";
    const char * const showsub = /* album subitem for canonicalartist */
        ((strcasestr(subitem, "artist") == NULL) ? "title" : "album -> title");

    /* Numeric ex: "Numeric" -> album ? album < "A" -> title = "fmt_title" */
    snprintf(buf, bufsz, fmt,
            str(LANG_DISPLAY_NUMERIC), subitem, subitem,'<', 'A', showsub);

    if (!alloc_menu_parse_buf(buf, menu_byfirstletter))
    {
        return;
    }

    for (int i = 0; i < 26; i++)
    {
        snprintf(buf, bufsz, fmt, "#", subitem, subitem,'^', 'A' + i, showsub);
        buf[1] = 'A' + i; /* overwrite the placeholder # with the current letter */
        /* ex: "A" -> title ? title ^ "A" -> title = "fmt_title" */
        if (!alloc_menu_parse_buf(buf, menu_byfirstletter))
        {
            return;
        }
    }
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
            case menu_byfirstletter: /* Fallthrough */
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
                    if (!menus[menu_count])
                    {
                        logf("tagtree failed to allocate %s", "menu");
                        return -2;
                    }
                    menu = menus[menu_count];
                    ++menu_count;
                    strmemccpy(menu->id, data, MAX_MENU_ID_SIZE);
                }

                if (get_token_str(menu->title, sizeof(menu->title)) < 0)
                {
                    logf("%%menu_start title empty");
                    return 0;
                }
                logf("menu: %s", menu->title);

                if (variable == menu_byfirstletter)
                {
                    if (get_token_str(data, sizeof(data)) < 0)
                    {
                        logf("%%firstletter_menu has no subitem"); /*artist,album*/
                        return 0;
                    }
                    logf("A-Z Menu subitem: %s", data);
                    read_menu = false;
                    build_firstletter_menu(data, sizeof(data));
                    break;
                }
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

    if (!alloc_menu_parse_buf(buf, -1))
    {
        return -2;
    }

    return 0;
}

static bool parse_menu(const char *filename)
{
    int fd;
    char buf[1024];
    int rc;

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
    rc = fast_readline(fd, buf, sizeof buf, NULL, parse_line);
    close(fd);

    return (rc >= 0);
}

static void tagtree_unload(struct tree_context *c)
{
    /* may be spurious... */
    core_pin(tagtree_handle);

    remove_event(PLAYBACK_EVENT_TRACK_BUFFER, tagtree_buffer_event);
    remove_event(PLAYBACK_EVENT_TRACK_FINISH, tagtree_track_finish_event);

    if (c)
    {
        tree_lock_cache(c);
        struct tagentry *dptr = core_get_data(c->cache.entries_handle);
        menu = menus[c->currextra];
        if (!menu)
        {
            logf("tagtree menu doesn't exist");
            return;
        }

        for (int i = 0; i < menu->itemcount; i++)
        {
            dptr->name = NULL;
            dptr->newtable = 0;
            dptr->extraseek = 0;
            dptr->customaction = ONPLAY_NO_CUSTOMACTION;
            dptr++;
        }
    }

    for (int i = 0; i < menu_count; i++)
        menus[i] = NULL;
    menu_count = 0;

    for (int i = 0; i < format_count; i++)
        formats[i] = NULL;
    format_count = 0;

    core_free(tagtree_handle);
    tagtree_handle   = 0;
    tagtree_buf_used = 0;
    tagtree_bufsize  = 0;

    if (c)
        tree_unlock_cache(c);
}

static bool initialize_tagtree(void) /* also used when user selects 'Reload' in 'custom view'*/
{
    max_history_level = 0;
    format_count = 0;
    menu_count = 0;
    menu = NULL;
    rootmenu = -1;
    tagtree_handle = core_alloc_maximum(&tagtree_bufsize, &ops);
    if (tagtree_handle < 0)
        panicf("tagtree OOM");

    /* Use the user tagnavi config if present, otherwise use the default. */
    const char* tagnavi_file;
    if(file_exists(TAGNAVI_USER_CONFIG))
        tagnavi_file = TAGNAVI_USER_CONFIG;
    else
        tagnavi_file = TAGNAVI_DEFAULT_CONFIG;

    if (!parse_menu(tagnavi_file))
    {
        tagtree_unload(NULL);
        return false;
    }

    /* safety check since tree.c needs to cast tagentry to entry */
    if (sizeof(struct tagentry) != sizeof(struct entry))
        panicf("tagentry(%zu) and entry mismatch(%zu)",
                sizeof(struct tagentry), sizeof(struct entry));

    /* If no root menu is set, assume it's the first single menu
     * we have. That shouldn't normally happen. */
    if (rootmenu < 0)
        rootmenu = 0;

    add_event(PLAYBACK_EVENT_TRACK_BUFFER, tagtree_buffer_event);
    add_event(PLAYBACK_EVENT_TRACK_FINISH, tagtree_track_finish_event);

    core_shrink(tagtree_handle, NULL, tagtree_buf_used);
    return true;
}

void tagtree_init(void)
{
    initialize_tagtree();
}

static int format_str(struct tagcache_search *tcs, struct display_format *fmt,
                      char *buf, int buf_size)
{
    static char fmtbuf[20];
    bool read_format = false;
    unsigned fmtbuf_pos = 0;
    int parpos = 0;
    int buf_pos = 0;
    int i;

    /* memset(buf, 0, buf_size); probably uneeded */
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
                                               tag, tmpbuf, sizeof tmpbuf))
                        {
                            logf("retrieve failed");
                            return -3;
                        }

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

static void tcs_get_basename(struct tagcache_search *tcs, bool is_basename)
{
    if (is_basename)
    {
        char* basename = strrchr(tcs->result, '/');
        if (basename != NULL)
        {
            tcs->result = basename + 1;
            tcs->result_len = strlen(tcs->result) + 1;
        }
    }
}

static int retrieve_entries(struct tree_context *c, int offset, bool init)
{
    logf( "%s", __func__);
    char tcs_buf[TAGCACHE_BUFSZ];
    const long tcs_bufsz = sizeof(tcs_buf);
    struct tagcache_search tcs;
    struct display_format *fmt;
    int i;
    int namebufused = 0;
    int total_count = 0;
    int special_entry_count = 0;
    int level = c->currextra;
    int tag;
    bool sort = false;
    bool sort_inverse;
    bool is_basename = false;
    int sort_limit;
    int strip;

    /* Show search progress straight away if the disk needs to spin up,
       otherwise show it after the normal 1/2 second delay */
    show_search_progress(
#ifdef HAVE_DISK_STORAGE
#ifdef HAVE_TC_RAMCACHE
        tagcache_is_in_ram() ? true :
#endif
        storage_disk_is_active()
#else
        true
#endif
        , 0, 0, 0);

    if (c->currtable == TABLE_ALLSUBENTRIES)
    {
        tag = tag_title;
        level--;
    }
    else
        tag = csi->tagorder[level];

    if (tag == menu_reload)
        return RELOAD_TAGTREE;

    if (tag == tag_virt_basename) /* basename shortcut */
    {
        is_basename = true;
        tag = tag_filename;
    }

    if (!tagcache_search(&tcs, tag))
        return -1;

    /* Prevent duplicate entries in the search list. */
    tagcache_search_set_uniqbuf(&tcs, uniqbuf, UNIQBUF_SIZE);

    if (level || is_basename|| csi->clause_count[0] || TAGCACHE_IS_NUMERIC(tag))
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
    core_pin(tagtree_handle);
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
            dptr->newtable = TABLE_ALLSUBENTRIES;
            dptr->name = str(LANG_TAGNAVI_ALL_TRACKS);
            dptr->customaction = ONPLAY_NO_CUSTOMACTION;
            dptr++;
            current_entry_count++;
            special_entry_count++;
        }
        if (offset <= 1)
        {
            dptr->newtable = TABLE_NAVIBROWSE;
            dptr->name = str(LANG_TAGNAVI_RANDOM);
            dptr->extraseek = -1;
            dptr->customaction = ONPLAY_NO_CUSTOMACTION;
            dptr++;
            current_entry_count++;
            special_entry_count++;
        }

        total_count += 2;
    }

    while (tagcache_get_next(&tcs, tcs_buf, tcs_bufsz))
    {
        if (total_count++ < offset)
            continue;

        dptr->newtable = TABLE_NAVIBROWSE;
        if (tag == tag_title || tag == tag_filename)
        {
            dptr->newtable = TABLE_PLAYTRACK;
            dptr->extraseek = tcs.idx_id;
        }
        else
            dptr->extraseek = tcs.result_seek;
        dptr->customaction = ONPLAY_NO_CUSTOMACTION;

        fmt = NULL;
        /* Check the format */
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

        if (strcmp(tcs.result, UNTAGGED) == 0)
        {
            if (tag == tag_title && tcs.type == tag_title && tcs.filter_count <= 1)
            { /* Fallback to basename */
                char *lastname = dptr->name;
                dptr->name = core_get_data(c->cache.name_buffer_handle)+namebufused;
                if (tagcache_retrieve(&tcs, tcs.idx_id, tag_virt_basename, dptr->name,
                                      c->cache.name_buffer_size - namebufused))
                {
                    namebufused += strlen(dptr->name)+1;
                    goto entry_skip_formatter;
                }
                dptr->name = lastname; /* restore last entry if filename failed */
            }

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
                if (ret >= 0)
                {
                    namebufused += strlen(dptr->name)+1; /* include NULL */
                }
                else
                {
                    dptr->name[0] = '\0';
                    if (ret == -4)          /* buffer full */
                    {
                        logf("chunk mode #2: %d", current_entry_count);
                        c->dirfull = true;
                        sort = false;
                        break ;
                    }

                    logf("format_str() failed");
                    tagcache_search_finish(&tcs);
                    tree_unlock_cache(c);
                    core_unpin(tagtree_handle);
                    return 0;
                }
            }
            else
            {
                tcs_get_basename(&tcs, is_basename);
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
        {
            tcs_get_basename(&tcs, is_basename);
            dptr->name = tcs.result;
        }
entry_skip_formatter:
        dptr++;
        current_entry_count++;

        if (current_entry_count >= c->cache.max_entries)
        {
            logf("chunk mode #3: %d", current_entry_count);
            c->dirfull = true;
            sort = false;
            break ;
        }

        if (init)
        {
            if (!show_search_progress(false, total_count, 0, 0))
            {   /* user aborted */
                tagcache_search_finish(&tcs);
                tree_unlock_cache(c);
                core_unpin(tagtree_handle);
                return current_entry_count;
            }
        }
    }

    if (sort)
    {
        if (global_settings.interpret_numbers)
            qsort_fn = sort_inverse ? strnatcasecmp_n_inv : strnatcasecmp_n;
        else
            qsort_fn = sort_inverse ? strncasecmp_inv : strncasecmp;

        struct tagentry *entries = get_entries(c);
        qsort(&entries[special_entry_count],
              current_entry_count - special_entry_count,
              sizeof(struct tagentry),
              compare);
    }

    if (!init)
    {
        tagcache_search_finish(&tcs);
        tree_unlock_cache(c);
        core_unpin(tagtree_handle);
        return current_entry_count;
    }

    while (tagcache_get_next(&tcs, tcs_buf, tcs_bufsz))
    {
        if (!show_search_progress(false, total_count, 0, 0))
            break;
        total_count++;
    }

    tagcache_search_finish(&tcs);
    tree_unlock_cache(c);
    core_unpin(tagtree_handle);

    if (!sort && (sort_inverse || sort_limit))
    {
        if (global_settings.talk_menu) {
            talk_id(LANG_SHOWDIR_BUFFER_FULL, true);
            talk_value(total_count, UNIT_INT, true);
        }

        splashf(HZ*4, str(LANG_SHOWDIR_BUFFER_FULL), total_count);
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
    c->currtable = TABLE_ROOT;
    if (c->dirlevel == 0)
        c->currextra = rootmenu;

    menu = menus[c->currextra];
    if (menu == NULL)
        return 0;

    if (menu->itemcount > c->cache.max_entries)
            panicf("%s tree_cache too small", __func__);

    for (i = 0; i < menu->itemcount; i++)
    {
        dptr->name = menu->items[i]->name;
        switch (menu->items[i]->type)
        {
            case menu_next:
                dptr->newtable = TABLE_NAVIBROWSE;
                dptr->extraseek = i;
                dptr->customaction = ONPLAY_NO_CUSTOMACTION;
                break;

            case menu_load:
                dptr->newtable = TABLE_ROOT;
                dptr->extraseek = menu->items[i]->link;
                dptr->customaction = ONPLAY_NO_CUSTOMACTION;
                break;

            case menu_shuffle_songs:
                dptr->newtable = TABLE_NAVIBROWSE;
                dptr->extraseek = i;
                dptr->customaction = ONPLAY_CUSTOMACTION_SHUFFLE_SONGS;
                break;

            case menu_byfirstletter:
                dptr->newtable = TABLE_NAVIBROWSE;
                dptr->extraseek = i;
                dptr->customaction = ONPLAY_CUSTOMACTION_FIRSTLETTER;
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
    logf( "%s", __func__);

    int count;
    int table = c->currtable;

    c->dirsindir = 0;

    if (!table)
    {
        c->dirfull = false;
        table = TABLE_ROOT;
        c->currtable = table;
        c->currextra = rootmenu;
    }

    switch (table)
    {
        case TABLE_ROOT:
            logf( "root...");
            count = load_root(c);
            break;

        case TABLE_ALLSUBENTRIES:
        case TABLE_NAVIBROWSE:
            logf("navibrowse...");

            if (loaded_entries_crc != 0)
            {
                if (loaded_entries_crc == tagtree_data_crc(c))
                {
                    count = c->dirlength;
                    logf("Reusing %d entries", count);
                    break;
                }
            }

            cpu_boost(true);
            count = retrieve_entries(c, 0, true);
            cpu_boost(false);
            break;

        default:
            logf("Unsupported table %d\n", table);
            return -1;
    }

    loaded_entries_crc = 0;

    if (count < 0)
    {
        if (count != RELOAD_TAGTREE)
            splash(HZ, str(LANG_TAGCACHE_BUSY));
        else /* unload and re-init tagtree */
        {
            splash(HZ, str(LANG_WAIT));
            tagtree_unload(c);
            if (!initialize_tagtree())
                return 0;
        }
        c->dirlevel = 0;
        count = load_root(c);
    }

    /* The _total_ numer of entries available. */
    c->dirlength = c->filesindir = count;

    return count;
}

/* Enters menu or table for selected item in the database.
 *
 * Call this with the is_visible parameter set to false to
 * prevent selected_item_history from being updated or applied, in
 * case the menus aren't displayed to the user.
 * Before calling tagtree_enter again with the parameter set to
 * true, make sure that you are back at the previous dirlevel, by
 * calling tagtree_exit as needed, with is_visible set to false.
 */
int tagtree_enter(struct tree_context* c, bool is_visible)
{
    logf( "%s", __func__);

    int rc = 0;
    struct tagentry *dptr;
    struct mp3entry *id3;
    int newextra;
    int seek;
    int source;
    bool is_random_item = false;
    bool adjust_selection = true;

    dptr = tagtree_get_entry(c, c->selected_item);

    c->dirfull = false;
    seek = dptr->extraseek;
    if (seek == -1) /* <Random> menu item was selected */
    {
        is_random_item = true;
        if(c->filesindir<=2) /* Menu contains only <All> and <Random> menu items */
            return 0;
        srand(current_tick);
        dptr = (tagtree_get_entry(c, 2+(rand() % (c->filesindir-2))));
        seek = dptr->extraseek;
    }
    newextra = dptr->newtable;

    if (c->dirlevel >= MAX_DIR_LEVELS)
        return 0;

    if (is_visible) /* update selection history only for user-selected items */
    {
        /* We need to discard selected item history for levels
        descending from current one if selection has changed */
        if (max_history_level < c->dirlevel + 1
            || (max_history_level > c->dirlevel
                && selected_item_history[c->dirlevel] != c->selected_item)
            || is_random_item)
        {
            max_history_level = c->dirlevel + 1;
            if (max_history_level < MAX_DIR_LEVELS)
                selected_item_history[max_history_level] = 0;
        }

        selected_item_history[c->dirlevel]=c->selected_item;
    }
    table_history[c->dirlevel] = c->currtable;
    extra_history[c->dirlevel] = c->currextra;

    if (c->dirlevel + 1 < MAX_DIR_LEVELS)
    {
        c->dirlevel++;
        /*DEBUGF("Tagtree depth %d\n", c->dirlevel);*/
    }
    else
    {
        DEBUGF("Tagtree depth exceeded\n");
    }

    /* lock buflib for possible I/O to protect dptr */
    tree_lock_cache(c);
    core_pin(tagtree_handle);

    switch (c->currtable) {
        case TABLE_ROOT:
            c->currextra = newextra;

            if (newextra == TABLE_ROOT)
            {
                menu = menus[seek];
                c->currextra = seek;
            }

            else if (newextra == TABLE_NAVIBROWSE)
            {
                int i, j;

                csi = &menu->items[seek]->si;
                c->currextra = 0;

                strmemccpy(current_title[c->currextra], dptr->name,
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

                        /* discard history for lower levels when doing runtime searches */
                        if (is_visible)
                            max_history_level = c->dirlevel - 1;

                        searchstring=csi->clause[i][j]->str;
                        *searchstring = '\0';

                        id3 = audio_current_track();

                        if (source == source_current_path && id3)
                        {
                            char *e;
                            strmemccpy(searchstring, id3->path, SEARCHSTR_SIZE);
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
                                strmemccpy(searchstring, *src, SEARCHSTR_SIZE);
                            }
                        }
                        else
                        {
                            rc = kbd_input(searchstring, SEARCHSTR_SIZE, NULL);
                            if (rc < 0 || !searchstring[0])
                            {
                                tagtree_exit(c, is_visible);
                                tree_unlock_cache(c);
                                core_unpin(tagtree_handle);
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

        case TABLE_NAVIBROWSE:
        case TABLE_ALLSUBENTRIES:
            if (newextra == TABLE_PLAYTRACK)
            {
                adjust_selection = false;

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
            strmemccpy(current_title[c->currextra], dptr->name,
                       sizeof(current_title[0]));
            break;

        default:
            c->dirlevel--;
            break;
    }

    if (adjust_selection)
    {
        if (is_visible && c->dirlevel <= max_history_level)
            c->selected_item = selected_item_history[c->dirlevel];
        else
            c->selected_item = 0;
    }

    tree_unlock_cache(c);
    core_unpin(tagtree_handle);

    return rc;
}

/* Exits current database menu or table */
void tagtree_exit(struct tree_context* c, bool is_visible)
{
    logf( "%s", __func__);
    if (is_visible) /* update selection history only for user-selected items */
    {
        if (c->selected_item != selected_item_history[c->dirlevel])
            max_history_level = c->dirlevel; /* discard descending item history */
        selected_item_history[c->dirlevel] = c->selected_item;
    }
    c->dirfull = false;
    if (c->dirlevel > 0)
    {
        c->dirlevel--;
        /*DEBUGF("Tagtree depth %d\n", c->dirlevel);*/
    }
    else
    {
        DEBUGF("Tagtree nothing to exit\n");
    }

    if (is_visible)
        c->selected_item = selected_item_history[c->dirlevel];
    c->currtable = table_history[c->dirlevel];
    c->currextra = extra_history[c->dirlevel];
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

int tagtree_get_custom_action(struct tree_context* c)
{
    return tagtree_get_entry(c, c->selected_item)->customaction;
}

static void swap_array_bool(bool *a, bool *b)
{
    bool temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * Randomly shuffle an array using the Fisher-Yates algorithm :
 *  https://en.wikipedia.org/wiki/Random_permutation
 *  This algorithm has a linear complexity.
 *  Don't forget to srand before call to use it with a relevant seed.
 */
static bool* fill_random_playlist_indexes(bool *bool_array, size_t arr_sz,
                                         size_t track_count, size_t max_slots)
{
    size_t i;
    if (track_count * sizeof(bool) > arr_sz || max_slots > track_count)
        return NULL;

    for (i = 0; i < arr_sz; i++) /* fill max_slots with TRUE */
        bool_array[i] = i < max_slots;

    /* shuffle bool array */
    for (i = track_count - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        swap_array_bool(&bool_array[i], &bool_array[j]);
    }
    return bool_array;
}

static bool insert_all_playlist(struct tree_context *c,
                                const char* playlist, bool new_playlist,
                                int position, bool queue)
{
    struct tagcache_search tcs;
    int n;
    int fd = -1;
    unsigned long last_tick;
    int slots_remaining = 0;
    bool fill_randomly = false;
    bool *rand_bool_array = NULL;
    char buf[MAX_PATH];
    struct playlist_insert_context context;

    cpu_boost(true);

    if (!tagcache_search(&tcs, tag_filename))
    {
        splash(HZ, ID2P(LANG_TAGCACHE_BUSY));
        cpu_boost(false);
        return false;
    } /* NOTE: you need to close this search before returning */

    if (playlist == NULL)
    {
        if (playlist_insert_context_create(NULL, &context, position, queue, false) < 0)
        {
            tagcache_search_finish(&tcs);
            cpu_boost(false);
            return false;
        }
    }
    else
    {
        if (new_playlist)
            fd = open_utf8(playlist, O_CREAT|O_WRONLY|O_TRUNC);
        else
            fd = open(playlist, O_CREAT|O_WRONLY|O_APPEND, 0666);
        if(fd < 0)
        {
            tagcache_search_finish(&tcs);
            cpu_boost(false);
            return false;
        }
    }

    last_tick = current_tick + HZ/2; /* Show splash after 0.5 seconds have passed */
    splash_progress_set_delay(HZ / 2); /* wait 1/2 sec before progress */
    n = c->filesindir;

    if (playlist == NULL)
    {
        int max_playlist_size = playlist_get_current()->max_playlist_size;
        slots_remaining = max_playlist_size - playlist_get_current()->amount;
        if (slots_remaining <= 0)
        {
            logf("Playlist has no space remaining");
            tagcache_search_finish(&tcs);
            cpu_boost(false);
            return false;
        }

        fill_randomly = n > slots_remaining;
    
        if (fill_randomly)
        {
            srand(current_tick);
            size_t bufsize = 0;
            bool *buffer = (bool *) plugin_get_buffer(&bufsize);
            rand_bool_array = fill_random_playlist_indexes(buffer, bufsize,
                                                           n, slots_remaining);

            talk_id(LANG_RANDOM_SHUFFLE_RANDOM_SELECTIVE_SONGS_SUMMARY, true);
            splashf(HZ * 2, str(LANG_RANDOM_SHUFFLE_RANDOM_SELECTIVE_SONGS_SUMMARY),
                    slots_remaining);
        }
    }

    bool exit_loop_now = false;
    for (int i = 0; i < n; i++)
    {
        if (TIME_AFTER(current_tick, last_tick + HZ/4))
        {
            splash_progress(i, n, "%s (%s)", str(LANG_WAIT), str(LANG_OFF_ABORT));
            if (action_userabort(TIMEOUT_NOBLOCK))
            {
                exit_loop_now = true;
                break;
            }
            last_tick = current_tick;
        }

        if (playlist == NULL)
        {
            if (fill_randomly)
            {
                int remaining_tracks = n - i;
                if (remaining_tracks > slots_remaining)
                {
                    if (rand_bool_array)
                    {
                        /* Skip the track if rand_bool_array[i] is FALSE */
                        if (!rand_bool_array[i])
                            continue;
                    }
                    else
                    {
                        /* Generate random value between 0 and remaining_tracks - 1 */
                        int selrange = RAND_MAX / remaining_tracks; /* Improve distribution */
                        int random;

                        for (int r = 0; r < 0x0FFF; r++) /* limit loops */
                        {
                            random = rand() / selrange;
                            if (random < remaining_tracks)
                                break;
                            else
                                random = 0;
                        }
                        /* Skip the track if random >= slots_remaining */
                        if (random >= slots_remaining)
                            continue;
                    }
                }
            }
        }

        if (!tagcache_retrieve(&tcs, tagtree_get_entry(c, i)->extraseek, tcs.type, buf, sizeof buf))
            continue;

        if (playlist == NULL)
        {
            if (fill_randomly)
            {
                if (--slots_remaining <= 0)
                {
                    exit_loop_now = true;
                    break;
                }
            }

            if (playlist_insert_context_add(&context, buf) < 0) {
                logf("playlist_insert_track failed");
                exit_loop_now = true;
                break;
            }
        }
        else if (fdprintf(fd, "%s\n", buf) <= 0)
        {
            exit_loop_now = true;
            break;
        }
        yield();

        if (exit_loop_now)
            break;
    }

    if (playlist == NULL)
        playlist_insert_context_release(&context);
    else
        close(fd);

    tagcache_search_finish(&tcs);
    cpu_boost(false);

    return true;
}

static bool goto_allsubentries(int newtable)
{
    int i = 0;
    while (i < 2 && (newtable == TABLE_NAVIBROWSE || newtable == TABLE_ALLSUBENTRIES))
    {
        tagtree_enter(tc, false);
        tagtree_load(tc);
        newtable = tagtree_get_entry(tc, tc->selected_item)->newtable;
        i++;
    }
    return (newtable == TABLE_PLAYTRACK);
}

static void reset_tc_to_prev(int dirlevel, int selected_item)
{
    while (tc->dirlevel > dirlevel)
        tagtree_exit(tc, false);
    tc->selected_item = selected_item;
    tagtree_load(tc);
}

static bool tagtree_insert_selection(int position, bool queue,
                                     const char* playlist, bool new_playlist)
{
    char buf[MAX_PATH];
    int dirlevel = tc->dirlevel;
    int selected_item = tc->selected_item;
    int newtable;
    int ret;

    show_search_progress(
#ifdef HAVE_DISK_STORAGE
        storage_disk_is_active()
#else
        true
#endif
        , 0, 0, 0);

    newtable = tagtree_get_entry(tc, tc->selected_item)->newtable;

    if (newtable == TABLE_PLAYTRACK) /* Insert a single track? */
    {
        if (tagtree_get_filename(tc, buf, sizeof buf) < 0)
            return false;

        playlist_insert_track(NULL, buf, position, queue, true);

        return true;
    }

    ret = goto_allsubentries(newtable);
    if (ret)
    {
        if (tc->filesindir <= 0)
            splash(HZ, ID2P(LANG_END_PLAYLIST));
        else if (!insert_all_playlist(tc, playlist, new_playlist, position, queue))
            splash(HZ*2, ID2P(LANG_FAILED));
    }

    reset_tc_to_prev(dirlevel, selected_item);
    return ret;
}

/* Execute action_cb for all subentries of the current table's
 * selected item, handing over each entry's filename in the
 * callback function parameter. Parameter will be NULL for
 * entries whose filename couldn't be retrieved.
 */
bool tagtree_subentries_do_action(bool (*action_cb)(const char *file_name))
{
    struct tagcache_search tcs;
    int i, n;
    unsigned long last_tick;
    char buf[MAX_PATH];
    int ret = true;
    int dirlevel = tc->dirlevel;
    int selected_item = tc->selected_item;
    int newtable = tagtree_get_entry(tc, tc->selected_item)->newtable;

    cpu_boost(true);
    if (!goto_allsubentries(newtable))
        ret = false;
    else if (tagcache_search(&tcs, tag_filename))
    {
        last_tick = current_tick + HZ/2;
        splash_progress_set_delay(HZ / 2); /* wait 1/2 sec before progress */
        n = tc->filesindir;
        for (i = 0; i < n; i++)
        {
            splash_progress(i, n, "%s (%s)", str(LANG_WAIT), str(LANG_OFF_ABORT));
            if (TIME_AFTER(current_tick, last_tick + HZ/4))
            {
                if (action_userabort(TIMEOUT_NOBLOCK))
                    break;
                last_tick = current_tick;
            }

            if (!action_cb(tagcache_retrieve(&tcs, tagtree_get_entry(tc, i)->extraseek,
                                             tcs.type, buf, sizeof buf) ? buf : NULL))
            {
                ret = false;
                break;
            }
            yield();
        }

        tagcache_search_finish(&tcs);
    }
    else
    {
        splash(HZ, ID2P(LANG_TAGCACHE_BUSY));
        ret = false;
    }
    reset_tc_to_prev(dirlevel, selected_item);
    cpu_boost(false);
    return ret;
}

/* Try to return first subentry's filename for current selection
 */
bool tagtree_get_subentry_filename(char *buf, size_t bufsize)
{
    int ret = true;
    int dirlevel = tc->dirlevel;
    int selected_item = tc->selected_item;
    int newtable = tagtree_get_entry(tc, tc->selected_item)->newtable;

    if (!goto_allsubentries(newtable) || tagtree_get_filename(tc, buf, bufsize) < 0)
        ret = false;

    reset_tc_to_prev(dirlevel, selected_item);
    return ret;
}

bool tagtree_current_playlist_insert(int position, bool queue)
{
    return tagtree_insert_selection(position, queue, NULL, false);
}


int tagtree_add_to_playlist(const char* playlist, bool new_playlist)
{
    if (!new_playlist)
        tagtree_load(tc); /* because display_playlists was called */
    return tagtree_insert_selection(0, false, playlist, new_playlist) ? 0 : -1;
}

static int tagtree_play_folder(struct tree_context* c)
{
    logf( "%s", __func__);
    int start_index = c->selected_item;

    if (playlist_create(NULL, NULL) < 0)
    {
        logf("Failed creating playlist\n");
        return -1;
    }

    if (!insert_all_playlist(c, NULL, false, PLAYLIST_INSERT_LAST, false))
        return -2;

    int n = c->filesindir;
    bool has_playlist_been_randomized = n > playlist_get_current()->max_playlist_size;
    if (has_playlist_been_randomized)
    {
        /* We need to recalculate the start index based on a percentage to put the user
        around its desired start position and avoid out of bounds */

        int percentage_start_index = 100 * start_index / n;
        start_index = percentage_start_index * playlist_get_current()->amount / 100;
    }

    if (global_settings.playlist_shuffle)
    {
        start_index = playlist_shuffle(current_tick, start_index);
        if (!global_settings.play_selected)
            start_index = 0;
    }

    playlist_start(start_index, 0, 0);
    loaded_entries_crc = tagtree_data_crc(c); /* save crc in case we return */
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
    strmemccpy(buf, entry->name, bufsize);
    return buf;
}


char *tagtree_get_title(struct tree_context* c)
{
    switch (c->currtable)
    {
        case TABLE_ROOT:
            return menu->title;

        case TABLE_NAVIBROWSE:
        case TABLE_ALLSUBENTRIES:
            return current_title[c->currextra];
    }

    return "?";
}

int tagtree_get_attr(struct tree_context* c)
{
    int attr = -1;
    switch (c->currtable)
    {
        case TABLE_NAVIBROWSE:
            if (csi->tagorder[c->currextra] == tag_title
                || csi->tagorder[c->currextra] == tag_virt_basename)
                attr = FILE_ATTR_AUDIO;
            else
                attr = ATTR_DIRECTORY;
            break;

        case TABLE_ALLSUBENTRIES:
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
