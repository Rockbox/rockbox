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
#include "buffer.h"
#include "atoi.h"
#include "playback.h"
#include "yesno.h"
#include "misc.h"
#include "filetypes.h"

#define FILE_SEARCH_INSTRUCTIONS ROCKBOX_DIR "/tagnavi.config"

static int tagtree_play_folder(struct tree_context* c);

#define SEARCHSTR_SIZE 128
static char searchstring[SEARCHSTR_SIZE];

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
static long *uniqbuf;

#define MAX_TAGS 5
#define MAX_MENU_ID_SIZE 32

static struct tagcache_search tcs, tcs2;
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
    bool sort;
};

static struct display_format *formats[TAGMENU_MAX_FMTS];
static int format_count;

struct search_instruction {
    char name[64];
    int tagorder[MAX_TAGS];
    int tagorder_count;
    struct tagcache_search_clause *clause[MAX_TAGS][TAGCACHE_MAX_CLAUSES];
    int format_id[MAX_TAGS];
    int clause_count[MAX_TAGS];
    int result_seek[MAX_TAGS];
};

struct menu_entry {
    char name[64];
    int type;
    struct search_instruction *si;
    int link;
};

struct root_menu {
    char title[64];
    char id[MAX_MENU_ID_SIZE];
    int itemcount;
    struct menu_entry *items[TAGMENU_MAX_ITEMS];
};

/* Statusbar text of the current view. */
static char current_title[MAX_TAGS][128];

static struct root_menu *menus[TAGMENU_MAX_MENUS];
static struct root_menu *menu;
static struct search_instruction *csi;
static const char *strp;
static int menu_count;
static int root_menu;

static int current_offset;
static int current_entry_count;

static int format_count;
static struct tree_context *tc;

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
    char buf[128];
    int i;
    
    /* Find the start. */
    while ((*strp == ' ' || *strp == '>') && *strp != '\0')
        strp++;
    
    if (*strp == '\0' || *strp == '?')
        return 0;
    
    for (i = 0; i < (int)sizeof(buf)-1; i++)
    {
        if (*strp == '\0' || *strp == ' ')
            break ;
        buf[i] = *strp;
        strp++;
    }
    buf[i] = '\0';
    
    MATCH(tag, buf, "album", tag_album);
    MATCH(tag, buf, "artist", tag_artist);
    MATCH(tag, buf, "bitrate", tag_bitrate);
    MATCH(tag, buf, "composer", tag_composer);
    MATCH(tag, buf, "comment", tag_comment);
    MATCH(tag, buf, "albumartist", tag_albumartist);
    MATCH(tag, buf, "ensemble", tag_albumartist);
    MATCH(tag, buf, "grouping", tag_grouping);
    MATCH(tag, buf, "genre", tag_genre);
    MATCH(tag, buf, "length", tag_length);
    MATCH(tag, buf, "Lm", tag_virt_length_min);
    MATCH(tag, buf, "Ls", tag_virt_length_sec);
    MATCH(tag, buf, "Pm", tag_virt_playtime_min);
    MATCH(tag, buf, "Ps", tag_virt_playtime_sec);
    MATCH(tag, buf, "title", tag_title);
    MATCH(tag, buf, "filename", tag_filename);
    MATCH(tag, buf, "tracknum", tag_tracknumber);
    MATCH(tag, buf, "discnum", tag_discnumber);
    MATCH(tag, buf, "year", tag_year);
    MATCH(tag, buf, "playcount", tag_playcount);
    MATCH(tag, buf, "rating", tag_rating);
    MATCH(tag, buf, "lastplayed", tag_lastplayed);
    MATCH(tag, buf, "commitid", tag_commitid);
    MATCH(tag, buf, "entryage", tag_virt_entryage);
    MATCH(tag, buf, "autoscore", tag_virt_autoscore);
    MATCH(tag, buf, "%sort", var_sorttype);
    MATCH(tag, buf, "%limit", var_limit);
    MATCH(tag, buf, "%strip", var_strip);
    MATCH(tag, buf, "%menu_start", var_menu_start);
    MATCH(tag, buf, "%include", var_include);
    MATCH(tag, buf, "%root_menu", var_rootmenu);
    MATCH(tag, buf, "%format", var_format);
    MATCH(tag, buf, "->", menu_next);
    MATCH(tag, buf, "==>", menu_load);
    
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
    MATCH(condition, buf, "==", clause_is);
    MATCH(condition, buf, "!=", clause_is_not);
    MATCH(condition, buf, ">", clause_gt);
    MATCH(condition, buf, ">=", clause_gteq);
    MATCH(condition, buf, "<", clause_lt);
    MATCH(condition, buf, "<=", clause_lteq);
    MATCH(condition, buf, "~", clause_contains);
    MATCH(condition, buf, "!~", clause_not_contains);
    MATCH(condition, buf, "^", clause_begins_with);
    MATCH(condition, buf, "!^", clause_not_begins_with);
    MATCH(condition, buf, "$", clause_ends_with);
    MATCH(condition, buf, "!$", clause_not_ends_with);
    MATCH(condition, buf, "@", clause_oneof);
    
    return 0;
}

static bool read_clause(struct tagcache_search_clause *clause)
{
    char buf[256];
    
    if (get_tag(&clause->tag) <= 0)
        return false;
    
    if (get_clause(&clause->type) <= 0)
        return false;
    
    if (get_token_str(buf, sizeof buf) < 0)
        return false;
    
    clause->str = buffer_alloc(strlen(buf)+1);
    strcpy(clause->str, buf);
    
    logf("got clause: %d/%d [%s]", clause->tag, clause->type, clause->str);
    
    if (*(clause->str) == '\0')
        clause->source = source_input;
    else if (!strcasecmp(clause->str, SOURCE_CURRENT_ALBUM))
        clause->source = source_current_album;
    else if (!strcasecmp(clause->str, SOURCE_CURRENT_ARTIST))
        clause->source = source_current_artist;
    else
        clause->source = source_constant;
    
    if (tagcache_is_numeric_tag(clause->tag))
    {
        clause->numeric = true;
        clause->numeric_data = atoi(clause->str);
    }
    else
        clause->numeric = false;
    
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
    
    fmt->formatstr = buffer_alloc(strlen(buf) + 1);
    strcpy(fmt->formatstr, buf);
    
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

            fmt->sort = true;
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
    strp = buf;
    
    if (formats[format_count] == NULL)
        formats[format_count] = buffer_alloc(sizeof(struct display_format));
    
    memset(formats[format_count], 0, sizeof(struct display_format));
    if (get_format_str(formats[format_count]) < 0)
    {
        logf("get_format_str() parser failed!");
        return -4;
    }
    
    while (*strp != '\0' && *strp != '?')
        strp++;

    if (*strp == '?')
    {
        int clause_count = 0;
        strp++;
        
        while (1)
        {
            if (clause_count >= TAGCACHE_MAX_CLAUSES)
            {
                logf("too many clauses");
                break;
            }
            
            formats[format_count]->clause[clause_count] = 
                buffer_alloc(sizeof(struct tagcache_search_clause));
            
            if (!read_clause(formats[format_count]->clause[clause_count]))
                break;
            
            clause_count++;
        }
        
        formats[format_count]->clause_count = clause_count;
    }
    
    format_count++;
    
    return 1;
}

static int get_condition(struct search_instruction *inst)
{
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
    
    inst->clause[inst->tagorder_count][clause_count] = 
        buffer_alloc(sizeof(struct tagcache_search_clause));
    
    if (!read_clause(inst->clause[inst->tagorder_count][clause_count]))
        return -1;
    
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
    struct search_instruction *inst = entry->si;
    char buf[MAX_PATH];
    int i;
    struct root_menu *new_menu;
    
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
        menus[menu_count] = buffer_alloc(sizeof(struct root_menu));
        new_menu = menus[menu_count];
        memset(new_menu, 0, sizeof(struct root_menu));
        strncpy(new_menu->id, buf, MAX_MENU_ID_SIZE-1);
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
        
        while ( (ret = get_condition(inst)) > 0 ) ;
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

static void tagtree_buffer_event(struct mp3entry *id3, bool last_track)
{
    (void)id3;
    (void)last_track;
    
    /* Do not gather data unless proper setting has been enabled. */
    if (!global_settings.runtimedb)
        return;

    logf("be:%d%s", last_track, id3->path);
    
    if (!tagcache_find_index(&tcs, id3->path))
    {
        logf("tc stat: not found: %s", id3->path);
        return;
    }
    
    id3->playcount  = tagcache_get_numeric(&tcs, tag_playcount);
    if (!id3->rating)
        id3->rating = tagcache_get_numeric(&tcs, tag_rating);
    id3->lastplayed = tagcache_get_numeric(&tcs, tag_lastplayed);
    id3->score      = tagcache_get_numeric(&tcs, tag_virt_autoscore) / 10;
    id3->playtime   = tagcache_get_numeric(&tcs, tag_playtime);
    
    /* Store our tagcache index pointer. */
    id3->tagcache_idx = tcs.idx_id;
    
    tagcache_search_finish(&tcs);
}

static void tagtree_unbuffer_event(struct mp3entry *id3, bool last_track)
{
    (void)last_track;
    long playcount;
    long playtime;
    long lastplayed;
    
    /* Do not gather data unless proper setting has been enabled. */
    if (!global_settings.runtimedb)
    {
        logf("runtimedb gathering not enabled");
        return;
    }
    
    if (!id3->tagcache_idx)
    {
        logf("No tagcache index pointer found");
        return;
    }
    
    /* Don't process unplayed tracks. */
    if (id3->elapsed == 0)
    {
        logf("not logging unplayed track");
        return;
    }
    
    playcount = id3->playcount + 1;
    lastplayed = tagcache_increase_serial();
    if (lastplayed < 0)
    {
        logf("incorrect tc serial:%ld", lastplayed);
        return;
    }
    
    /* Ignore the last 15s (crossfade etc.) */
    playtime = id3->playtime + MIN(id3->length, id3->elapsed + 15 * 1000);
    
    logf("ube:%s", id3->path);
    logf("-> %d/%ld/%ld", last_track, playcount, playtime);
    logf("-> %ld/%ld/%ld", id3->elapsed, id3->length, MIN(id3->length, id3->elapsed + 15 * 1000));
    
    /* Queue the updates to the tagcache system. */
    tagcache_update_numeric(id3->tagcache_idx, tag_playcount, playcount);
    tagcache_update_numeric(id3->tagcache_idx, tag_playtime, playtime);
    tagcache_update_numeric(id3->tagcache_idx, tag_lastplayed, lastplayed);
}

bool tagtree_export(void)
{
    gui_syncsplash(0, str(LANG_CREATING));
    if (!tagcache_create_changelog(&tcs))
    {
        gui_syncsplash(HZ*2, ID2P(LANG_FAILED));
    }
    
    return false;
}

bool tagtree_import(void)
{
    gui_syncsplash(0, ID2P(LANG_WAIT));
    if (!tagcache_import_changelog())
    {
        gui_syncsplash(HZ*2, ID2P(LANG_FAILED));
    }
    
    return false;
}

static bool parse_menu(const char *filename);

static int parse_line(int n, const char *buf, void *parameters)
{
    char data[256];
    int variable;
    static bool read_menu;
    int i;
    
    (void)parameters;
    
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
                    menus[menu_count] = buffer_alloc(sizeof(struct root_menu));
                    menu = menus[menu_count];
                    ++menu_count;
                    memset(menu, 0, sizeof(struct root_menu));
                    strncpy(menu->id, data, MAX_MENU_ID_SIZE-1);
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
                if (root_menu >= 0)
                    break;
                
                if (get_token_str(data, sizeof(data)) < 0)
                {
                    logf("%%root_menu empty");
                    return 0;
                }
                
                for (i = 0; i < menu_count; i++)
                {
                    if (!strcasecmp(menus[i]->id, data))
                    {
                        root_menu = i;
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
    {
        menu->items[menu->itemcount] = buffer_alloc(sizeof(struct menu_entry));
        memset(menu->items[menu->itemcount], 0, sizeof(struct menu_entry));
        menu->items[menu->itemcount]->si = buffer_alloc(sizeof(struct search_instruction));
    }
    
    memset(menu->items[menu->itemcount]->si, 0, sizeof(struct search_instruction));
    if (!parse_search(menu->items[menu->itemcount], buf))
        return 0;
    
    menu->itemcount++;
    
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
    root_menu = -1;
    parse_menu(FILE_SEARCH_INSTRUCTIONS);
    
    /* If no root menu is set, assume it's the first single menu
     * we have. That shouldn't normally happen. */
    if (root_menu < 0)
        root_menu = 0;
    
    uniqbuf = buffer_alloc(UNIQBUF_SIZE);
    audio_set_track_buffer_event(tagtree_buffer_event);
    audio_set_track_unbuffer_event(tagtree_unbuffer_event);
}

static bool show_search_progress(bool init, int count)
{
    static int last_tick = 0;
    
    if (init)
    {
        last_tick = current_tick;
        return true;
    }
    
    if (current_tick - last_tick > HZ/4)
    {
        gui_syncsplash(0, str(LANG_PLAYLIST_SEARCH_MSG), count,
                          str(LANG_OFF_ABORT));
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
    char fmtbuf[8];
    bool read_format = false;
    int fmtbuf_pos = 0;
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
        
        if (read_format)
        {
            fmtbuf[fmtbuf_pos++] = fmt->formatstr[i];
            if (fmtbuf_pos >= buf_size)
            {
                logf("format parse error");
                return -2;
            }
            
            if (fmt->formatstr[i] == 's')
            {
                fmtbuf[fmtbuf_pos] = '\0';
                read_format = false;
                if (fmt->tags[parpos] == tcs->type)
                {
                    snprintf(&buf[buf_pos], buf_size - buf_pos, fmtbuf, tcs->result);
                }
                else
                {
                    /* Need to fetch the tag data. */
                    if (!tagcache_retrieve(tcs, tcs->idx_id, fmt->tags[parpos],
                                           &buf[buf_pos], buf_size - buf_pos))
                    {
                        logf("retrieve failed");
                        return -3;
                    }
                }
                buf_pos += strlen(&buf[buf_pos]);
                parpos++;
            }
            else if (fmt->formatstr[i] == 'd')
            {
                fmtbuf[fmtbuf_pos] = '\0';
                read_format = false;
                snprintf(&buf[buf_pos], buf_size - buf_pos, fmtbuf,
                         tagcache_get_numeric(tcs, fmt->tags[parpos]));
                buf_pos += strlen(&buf[buf_pos]);
                parpos++;
            }
            continue;
        }
        
        buf[buf_pos++] = fmt->formatstr[i];
        
        if (buf_pos - 1 >= buf_size)
        {
            logf("buffer overflow");
            return -4;
        }
    }
    
    buf[buf_pos++] = '\0';
    
    return 0;
}

static int retrieve_entries(struct tree_context *c, struct tagcache_search *tcs,
                            int offset, bool init)
{
    struct tagentry *dptr = (struct tagentry *)c->dircache;
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
    
    if (init
#ifdef HAVE_TC_RAMCACHE
        && !tagcache_is_ramcache()
#endif
        )
    {
        show_search_progress(true, 0);
        gui_syncsplash(0, str(LANG_PLAYLIST_SEARCH_MSG),
                       0, csi->name);
    }
    
    if (c->currtable == allsubentries)
    {
        tag = tag_title;
        level--;
    }
    else
        tag = csi->tagorder[level];

    if (!tagcache_search(tcs, tag))
        return -1;
    
    /* Prevent duplicate entries in the search list. */
    tagcache_search_set_uniqbuf(tcs, uniqbuf, UNIQBUF_SIZE);
    
    if (level || csi->clause_count[0] || tagcache_is_numeric_tag(tag))
        sort = true;
    
    for (i = 0; i < level; i++)
    {
        if (tagcache_is_numeric_tag(csi->tagorder[i]))
        {
            static struct tagcache_search_clause cc;
            
            memset(&cc, 0, sizeof(struct tagcache_search_clause));
            cc.tag = csi->tagorder[i];
            cc.type = clause_is;
            cc.numeric = true;
            cc.numeric_data = csi->result_seek[i];
            tagcache_search_add_clause(tcs, &cc);
        }
        else
        {
            tagcache_search_add_filter(tcs, csi->tagorder[i], 
                                       csi->result_seek[i]);
        }
    }
   
    for (i = 0; i <= level; i++)
    {
        int j;
        
        for (j = 0; j < csi->clause_count[i]; j++)
            tagcache_search_add_clause(tcs, csi->clause[i][j]);
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
        
        /* Check if sorting is forced. */
        if (fmt->sort)
            sort = true;
    }
    else
    {
        sort_inverse = false;
        sort_limit = 0;
        strip = 0;
    }
    
    if (tag != tag_title && tag != tag_filename)
    {
        if (offset == 0)
        {
            dptr->newtable = allsubentries;
            dptr->name = str(LANG_TAGNAVI_ALL_TRACKS);
            dptr++;
            current_entry_count++;
        }
        special_entry_count++;
    }
    
    total_count += special_entry_count;
    
    while (tagcache_get_next(tcs))
    {
        if (total_count++ < offset)
            continue;
        
        dptr->newtable = navibrowse;
        if (tag == tag_title || tag == tag_filename)
        {
            dptr->newtable = playtrack;
            dptr->extraseek = tcs->idx_id;
        }
        else
            dptr->extraseek = tcs->result_seek;
        
        fmt = NULL;
        /* Check the format */
        for (i = 0; i < format_count; i++)
        {
            if (formats[i]->group_id != csi->format_id[level])
                continue;
            
            if (tagcache_check_clauses(tcs, formats[i]->clause,
                                       formats[i]->clause_count))
            {
                fmt = formats[i];
                break;
            }
        }

        if (!tcs->ramresult || fmt)
        {
            char buf[MAX_PATH];
            
            if (fmt)
            {
                if (format_str(tcs, fmt, buf, sizeof buf) < 0)
                {
                    logf("format_str() failed");
                    return 0;
                }
            }
            
            dptr->name = &c->name_buffer[namebufused];
            if (fmt)
                namebufused += strlen(buf)+1;
            else
                namebufused += tcs->result_len;
            
            if (namebufused >= c->name_buffer_size)
            {
                logf("chunk mode #2: %d", current_entry_count);
                c->dirfull = true;
                sort = false;
                break ;
            }
            if (fmt)
                strcpy(dptr->name, buf);
            else
                strcpy(dptr->name, tcs->result);
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
        qsort(c->dircache + special_entry_count * c->dentry_size,
              current_entry_count - special_entry_count,
              c->dentry_size, compare);
    
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
    
    if (!sort && (sort_inverse || sort_limit))
    {
        gui_syncsplash(HZ*4, ID2P(LANG_SHOWDIR_BUFFER_FULL), total_count);
        logf("Too small dir buffer");
        return 0;
    }
    
    if (sort_limit)
        total_count = MIN(total_count, sort_limit);
    
    if (strip)
    {
        dptr = c->dircache;
        for (i = 0; i < total_count; i++, dptr++)
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
    struct tagentry *dptr = (struct tagentry *)c->dircache;
    int i;
    
    tc = c;
    c->currtable = root;
    if (c->dirlevel == 0)
        c->currextra = root_menu;
    
    menu = menus[c->currextra];
    if (menu == NULL)
        return 0;
    
    for (i = 0; i < menu->itemcount; i++)
    {
        dptr->name = menu->items[i]->name;
        switch (menu->items[i]->type)
        {
            case menu_next:
                dptr->newtable = navibrowse;
                dptr->extraseek = i;
                break;
            
            case menu_load:
                dptr->newtable = root;
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
    
    c->dentry_size = sizeof(struct tagentry);
    c->dirsindir = 0;

    if (!table)
    {
        c->dirfull = false;
        table = root;
        c->currtable = table;
        c->currextra = root_menu;
    }

    switch (table) 
    {
        case root:
            count = load_root(c);
            break;

        case allsubentries:
        case navibrowse:
            logf("navibrowse...");
            cpu_boost(true);
            count = retrieve_entries(c, &tcs, 0, true);
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
        gui_syncsplash(HZ, str(LANG_TAGCACHE_BUSY));
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
    newextra = dptr->newtable;
    seek = dptr->extraseek;

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
        
            if (newextra == root)
            {
                menu = menus[seek];
                c->currextra = seek;
            }
        
            else if (newextra == navibrowse)
            {
                int i, j;
                
                csi = menu->items[seek]->si;
                c->currextra = 0;
                
                strncpy(current_title[c->currextra], dptr->name, 
                        sizeof(current_title[0]) - 1);
    
                /* Read input as necessary. */
                for (i = 0; i < csi->tagorder_count; i++)
                {
                    for (j = 0; j < csi->clause_count[i]; j++)
                    {
                        *searchstring='\0';
                        source = csi->clause[i][j]->source;
                        
                        if (source == source_constant)
                            continue;
                        
                        id3 = audio_current_track();

                        if ((source == source_current_artist) && 
                            (id3) && (id3->artist)) 
                        {
                            strncpy(searchstring, id3->artist, SEARCHSTR_SIZE);
                            searchstring[SEARCHSTR_SIZE-1] = '\0';
                        }           

                        if ((source == source_current_album) && 
                            (id3) && (id3->album)) 
                        {
                            strncpy(searchstring, id3->album, SEARCHSTR_SIZE);
                            searchstring[SEARCHSTR_SIZE-1] = '\0';
                        }           

                        if((source == source_input) || (*searchstring=='\0'))
                        {
                            rc = kbd_input(searchstring, SEARCHSTR_SIZE);
                            if (rc == -1 || !searchstring[0])
                            {
                                tagtree_exit(c);
                                return 0;
                            }
                        }   
                                             
                        if (csi->clause[i][j]->numeric)
                            csi->clause[i][j]->numeric_data = atoi(searchstring);
                            
                        /* existing bug: only one dynamic string per clause! */    
                        csi->clause[i][j]->str = searchstring;
                    }
                }
            }
            c->currtable = newextra;
        
            break;

        case navibrowse:
        case allsubentries:
            if (newextra == playtrack)
            {
                c->dirlevel--;
                /* about to create a new current playlist...
                 allow user to cancel the operation */
                if (global_settings.warnon_erase_dynplaylist &&
                    !global_settings.party_mode &&
                    playlist_modified(NULL))
                {
                    char *lines[]={ID2P(LANG_WARN_ERASEDYNPLAYLIST_PROMPT)};
                    struct text_message message={lines, 1};
                    
                    if (gui_syncyesno_run(&message, NULL, NULL) != YESNO_YES)
                        break;
                }

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
            strncpy(current_title[c->currextra], dptr->name, 
                    sizeof(current_title[0]) - 1);
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
    struct tagentry *entry;
    
    entry = tagtree_get_entry(c, c->selected_item);

    if (!tagcache_search(&tcs, tag_filename))
        return -1;

    if (!tagcache_retrieve(&tcs, entry->extraseek, tcs.type, buf, buflen))
    {
        tagcache_search_finish(&tcs);
        return -2;
    }

    tagcache_search_finish(&tcs);
    
    return 0;
}

static bool insert_all_playlist(struct tree_context *c, int position, bool queue)
{
    int i;
    char buf[MAX_PATH];
    int from, to, direction;
    int files_left = c->filesindir;

    cpu_boost(true);
    if (!tagcache_search(&tcs, tag_filename))
    {
        gui_syncsplash(HZ, ID2P(LANG_TAGCACHE_BUSY));
        cpu_boost(false);
        return false;
    }
    
    if (position == PLAYLIST_REPLACE)
    {
        if (remove_all_tracks(NULL) == 0)
            position = PLAYLIST_INSERT_LAST;
        else return -1;    }

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
    struct tagentry *dptr;
    char buf[MAX_PATH];
    int dirlevel = tc->dirlevel;

    /* We need to set the table to allsubentries. */
    show_search_progress(true, 0);
    
    dptr = tagtree_get_entry(tc, tc->selected_item);
    
    /* Insert a single track? */
    if (dptr->newtable == playtrack)
    {
        if (tagtree_get_filename(tc, buf, sizeof buf) < 0)
        {
            logf("tagtree_get_filename failed");
            return false;
        }
        playlist_insert_track(NULL, buf, position, queue, true);
        
        return true;
    }
    
    if (dptr->newtable == navibrowse)
    {
        tagtree_enter(tc);
        tagtree_load(tc);
        dptr = tagtree_get_entry(tc, tc->selected_item);
    }
    else if (dptr->newtable != allsubentries)
    {
        logf("unsupported table: %d", dptr->newtable);
        return false;
    }
    
    /* Now the current table should be allsubentries. */
    if (dptr->newtable != playtrack)
    {
        tagtree_enter(tc);
        tagtree_load(tc);
        dptr = tagtree_get_entry(tc, tc->selected_item);
    
        /* And now the newtable should be playtrack. */
        if (dptr->newtable != playtrack)
        {
            logf("newtable: %d !!", dptr->newtable);
            tc->dirlevel = dirlevel;
            return false;
        }
    }

    if (tc->filesindir <= 0)
        gui_syncsplash(HZ, ID2P(LANG_END_PLAYLIST));
    else
    {
        logf("insert_all_playlist");
        if (!insert_all_playlist(tc, position, queue))
            gui_syncsplash(HZ*2, ID2P(LANG_FAILED));
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
        cpu_boost(true);
        if (retrieve_entries(c, &tcs2, MAX(0, id - (current_entry_count / 2)),
                             false) < 0)
        {
            logf("retrieve failed");
            cpu_boost(false);
            return NULL;
        }
        realid = id - current_offset;
        cpu_boost(false);
    }
    
    return &entry[realid];
}

char *tagtree_get_title(struct tree_context* c)
{
    switch (c->currtable)
    {
        case root:
            return menu->title;
        
        case navibrowse:
        case allsubentries:
            return current_title[c->currextra];
    }
    
    return "?";
}

int tagtree_get_attr(struct tree_context* c)
{
    int attr = -1;
    switch (c->currtable)
    {
        case navibrowse:
            if (csi->tagorder[c->currextra] == tag_title)
                attr = FILE_ATTR_AUDIO;
            else
                attr = ATTR_DIRECTORY;
            break;

        case allsubentries:
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
