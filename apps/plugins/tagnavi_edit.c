/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael Giacomelli
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

/* Database menu editor.
 *
 * Edits the Database browser's menus on the player and writes
 * /.rockbox/tagnavi_user.config, which tagtree.c loads in place of the stock
 * tagnavi.config.
 *
 * The file is loaded into a small model:
 *   menus (%menu_start, %byfirstletter) -> items -> screens (one per "->")
 * Each item has one filter list (the first screen's clauses).  Anything the
 * model does not understand (comments, formats, includes, lines that fail to
 * parse) is kept as raw text, so loading and saving never loses content.
 * Saving checks that the written text parses back to the same text.
 *
 * The UI only uses the standard list, keyboard and yes/no screens, so it
 * follows the user's theme, font and keymap on every target.
 */

#include "plugin.h"

/* ----------------------------------------------------------------------- */
/* Limits (tagtree.c / tagtree.h / tagcache.h)                             */
/* ----------------------------------------------------------------------- */

#define MAX_SCREENS     5     /* MAX_TAGS in tagtree.c */
#define NAME_LEN        64    /* MENUENTRY_MAX_NAME */
#define ID_LEN          32    /* MAX_MENU_ID_SIZE */
#define VALUE_LEN       256   /* SEARCHSTR_SIZE */
#define MAX_TOKENS      200

#define STOCK_FILE  ROCKBOX_DIR "/tagnavi.config"
#define USER_FILE   ROCKBOX_DIR "/tagnavi_user.config"
#define TMP_FILE    ROCKBOX_DIR "/tagnavi_user.tmp"
#define BAK_FILE    ROCKBOX_DIR "/tagnavi_user.bak"

#define TEXT_MAX    (40 * 1024)  /* largest config file handled */

/* ----------------------------------------------------------------------- */
/* Fields and tests                                                        */
/* ----------------------------------------------------------------------- */

enum fld {
    FLD_CARTIST, FLD_AARTIST, FLD_ARTIST, FLD_ALBUM, FLD_GENRE, FLD_YEAR,
    FLD_COMPOSER, FLD_GROUPING, FLD_COMMENT, FLD_TITLE, FLD_BASENAME,
    FLD_FILENAME, FLD_TRACKNUM, FLD_DISCNUM, FLD_RATING, FLD_PLAYCOUNT,
    FLD_AUTOSCORE, FLD_LASTPLAYED, FLD_ENTRYAGE, FLD_COMMITID, FLD_LENGTH,
    FLD_BITRATE, FLD_LASTELAPSED, FLD_LASTOFFSET, FLD_LM, FLD_LS, FLD_PM,
    FLD_PS, FLD_COUNT
};

#define F_NUM   1   /* numeric tag */
#define F_PICK  2   /* values can be listed from the database */

struct tn_field {
    const char *word;    /* name in tagnavi files */
    const char *label;   /* name shown to the user */
    int tag;             /* tagcache tag */
    unsigned char flags;
    int lang;            /* translated name, or -1 to use label */
};

static const struct tn_field fields[FLD_COUNT] = {
    [FLD_CARTIST]     = { "canonicalartist", "Artist",        tag_virt_canonicalartist, F_PICK, LANG_ID3_ARTIST },
    [FLD_AARTIST]     = { "albumartist",     "Album Artist",  tag_albumartist,     F_PICK, LANG_ID3_ALBUMARTIST },
    [FLD_ARTIST]      = { "artist",          "Track artist",  tag_artist,          F_PICK, -1 },
    [FLD_ALBUM]       = { "album",           "Album",         tag_album,           F_PICK, LANG_ID3_ALBUM },
    [FLD_GENRE]       = { "genre",           "Genre",         tag_genre,           F_PICK, LANG_ID3_GENRE },
    [FLD_YEAR]        = { "year",            "Year",          tag_year,            F_NUM|F_PICK, LANG_ID3_YEAR },
    [FLD_COMPOSER]    = { "composer",        "Composer",      tag_composer,        F_PICK, LANG_ID3_COMPOSER },
    [FLD_GROUPING]    = { "grouping",        "Grouping",      tag_grouping,        F_PICK, -1 },
    [FLD_COMMENT]     = { "comment",         "Comment",       tag_comment,         F_PICK, LANG_ID3_COMMENT },
    [FLD_TITLE]       = { "title",           "Title",         tag_title,           F_PICK, LANG_ID3_TITLE },
    [FLD_BASENAME]    = { "basename",        "File name",     tag_virt_basename,   0, LANG_FILENAME },
    [FLD_FILENAME]    = { "filename",        "Folder",        tag_filename,        0, LANG_DIRECTORY },
    [FLD_TRACKNUM]    = { "tracknum",        "Track number",  tag_tracknumber,     F_NUM, LANG_ID3_TRACKNUM },
    [FLD_DISCNUM]     = { "discnum",         "Disc number",   tag_discnumber,      F_NUM|F_PICK, LANG_DISC },
    [FLD_RATING]      = { "rating",          "Rating",        tag_rating,          F_NUM|F_PICK, LANG_USER_RATING },
    [FLD_PLAYCOUNT]   = { "playcount",       "Play count",    tag_playcount,       F_NUM, -1 },
    [FLD_AUTOSCORE]   = { "autoscore",       "Score",         tag_virt_autoscore,  F_NUM, -1 },
    [FLD_LASTPLAYED]  = { "lastplayed",      "Last played",   tag_lastplayed,      F_NUM, -1 },
    [FLD_ENTRYAGE]    = { "entryage",        "Age in database", tag_virt_entryage, F_NUM, -1 },
    [FLD_COMMITID]    = { "commitid",        "Database update", tag_commitid,      F_NUM, -1 },
    [FLD_LENGTH]      = { "length",          "Length (ms)",   tag_length,          F_NUM, -1 },
    [FLD_BITRATE]     = { "bitrate",         "Bitrate",       tag_bitrate,         F_NUM|F_PICK, LANG_ID3_BITRATE },
    [FLD_LASTELAPSED] = { "lastelapsed",     "Last position", tag_lastelapsed,     F_NUM, -1 },
    [FLD_LASTOFFSET]  = { "lastoffset",      "Last offset",   tag_lastoffset,      F_NUM, -1 },
    [FLD_LM]          = { "Lm",              "Length (min)",  tag_virt_length_min, F_NUM, -1 },
    [FLD_LS]          = { "Ls",              "Length (sec)",  tag_virt_length_sec, F_NUM, -1 },
    [FLD_PM]          = { "Pm",              "Play time (min)", tag_virt_playtime_min, F_NUM, -1 },
    [FLD_PS]          = { "Ps",              "Play time (sec)", tag_virt_playtime_sec, F_NUM, -1 },
};

/* a core language string */
#define LSTR(id) ((const char *)rb->str(id))

/* a field's name, translated if the core has it */
static const char *fld_label(int fld)
{
    return fields[fld].lang >= 0 ? LSTR(fields[fld].lang) : fields[fld].label;
}

/* the English name, for item and menu names saved in the file: the
   Database translates the stock names itself */
static const char *screen_name(int fld)
{
    return fld == FLD_TITLE ? "Tracks" : fields[fld].label;
}

static const char *screen_label(int fld)
{
    return fld == FLD_TITLE ? LSTR(LANG_TRACKS) : fld_label(fld);
}

/* what a screen can list, most useful first */
static const unsigned char screen_fields[] = {
    FLD_CARTIST, FLD_AARTIST, FLD_ALBUM, FLD_GENRE, FLD_YEAR, FLD_COMPOSER,
    FLD_TITLE, FLD_ARTIST, FLD_GROUPING, FLD_RATING, FLD_COMMENT, FLD_BASENAME,
};

/* what a filter can test, most useful first */
static const unsigned char filter_fields[] = {
    FLD_CARTIST, FLD_AARTIST, FLD_ALBUM, FLD_GENRE, FLD_YEAR, FLD_COMPOSER,
    FLD_FILENAME, FLD_TITLE, FLD_PLAYCOUNT, FLD_AUTOSCORE, FLD_RATING,
    FLD_LASTPLAYED, FLD_ENTRYAGE, FLD_ARTIST, FLD_GROUPING, FLD_COMMENT,
    FLD_TRACKNUM, FLD_DISCNUM, FLD_LENGTH, FLD_BITRATE, FLD_BASENAME,
};

#define IS_NUM(f)    (fields[f].flags & F_NUM)
#define IS_TRACKS(f) ((f) == FLD_TITLE || (f) == FLD_BASENAME || (f) == FLD_FILENAME)

enum op {
    OP_IS, OP_ISNOT, OP_GT, OP_GTEQ, OP_LT, OP_LTEQ, OP_CONTAINS, OP_NCONTAINS,
    OP_BEGINS, OP_NBEGINS, OP_ENDS, OP_NENDS, OP_ONEOF, OP_NONEOF,
    OP_CONTAINS_ONEOF, OP_BEGINS_ONEOF, OP_ENDS_ONEOF, OP_NCONTAINS_ONEOF,
    OP_NBEGINS_ONEOF, OP_NENDS_ONEOF, OP_COUNT,
    OP_BETWEEN = 0xfe,   /* editor only: written as >= and <= */
    OP_OR = 0xff,        /* "|" between clauses */
};

static const char *const op_sym[OP_COUNT] = {
    "=", "!=", ">", ">=", "<", "<=", "~", "!~", "^", "!^", "$", "!$",
    "@", "!@", "@~", "@^", "@$", "*~", "*^", "*$",
};

static const char *const op_label[OP_COUNT] = {
    "is", "is not", "greater than", "at least", "less than", "at most",
    "contains", "doesn't contain", "starts with", "doesn't start with",
    "ends with", "doesn't end with", "is one of", "is none of",
    "contains one of", "starts with one of", "ends with one of",
    "contains none of", "starts with none of", "ends with none of",
};

static const unsigned char str_ops[] = {
    OP_IS, OP_ONEOF, OP_ISNOT, OP_CONTAINS, OP_NCONTAINS, OP_BEGINS,
    OP_NBEGINS, OP_ENDS, OP_NENDS,
};
static const unsigned char num_ops[] = {
    OP_IS, OP_BETWEEN, OP_ISNOT, OP_GT, OP_GTEQ, OP_LT, OP_LTEQ,
};
/* Folder: the track's path starts (or doesn't start) with the folder */
static const unsigned char folder_ops[] = { OP_BEGINS, OP_NBEGINS };

static const char *test_label(int fld, int op)
{
    if (fld == FLD_FILENAME && op == OP_BEGINS)
        return "is in";
    if (fld == FLD_FILENAME && op == OP_NBEGINS)
        return "is not in";
    return op == OP_BETWEEN ? "is between" : op_label[op];
}

/* the tests offered for a field */
static const unsigned char *ops_for(int fld, int *n)
{
    if (fld == FLD_FILENAME)
    {
        *n = ARRAYLEN(folder_ops);
        return folder_ops;
    }
    if (fields[fld].flags & F_NUM)
    {
        *n = ARRAYLEN(num_ops);
        return num_ops;
    }
    *n = ARRAYLEN(str_ops);
    return str_ops;
}

/* values tagtree.c fills in from the playing track */
static const struct { unsigned char fld; const char *value; } runtime_values[] = {
    { FLD_TITLE, "#title#" }, { FLD_ARTIST, "#artist#" },
    { FLD_CARTIST, "#artist#" }, { FLD_ALBUM, "#album#" },
    { FLD_GENRE, "#genre#" }, { FLD_COMPOSER, "#composer#" },
    { FLD_AARTIST, "#albumartist#" }, { FLD_FILENAME, "#directory#" },
};

/* friendly names for the stock display formats */
static const struct { const char *name, *label; } format_labels[] = {
    { "fmt_title",          "Track# Title - Length" },
    { "fmt_alphanum_title", "Title - Length" },
    { "fmt_mostplayed",     "Most played first" },
    { "fmt_lastplayed",     "Last played first" },
    { "fmt_forgotten",      "Least recently played" },
    { "fmt_best_tracks",    "Track# Title (Score)" },
    { "fmt_score",          "(Score) Title - Artist" },
    { "fmt_rating",         "(Rating) Title - Artist" },
};

/* ----------------------------------------------------------------------- */
/* Model                                                                   */
/* ----------------------------------------------------------------------- */

struct tn_clause {
    struct tn_clause *next;
    unsigned char fld, op;     /* op == OP_OR: an "or" marker */
    char *value;
};

struct tn_screen {
    struct tn_screen *next;
    unsigned char fld;
    char *fmt;                 /* display format name or NULL */
    struct tn_clause *clauses;    /* screen 1: the item's filters */
};

enum { E_COMMENT, E_RAW, E_BROWSE, E_SHUFFLE, E_LINK, E_RELOAD, E_EDIT };

struct tn_entry {
    struct tn_entry *next;
    unsigned char kind;
    bool hide_special;         /* %hide_special: no [All tracks]/[Random] rows */
    char *name;
    char *text;                /* raw line, comment, or link target id */
    struct tn_screen *screens;
};

enum { K_COMMENT, K_BLANK, K_RAW, K_FORMAT, K_INCLUDE, K_MENU, K_AZ, K_ROOT };

struct tn_stmt {
    struct tn_stmt *next;
    unsigned char kind;
    unsigned char fld;         /* K_AZ */
    char *text;                /* raw text for comment/raw/format/include */
    char *id;                  /* menu id; root target; format name */
    char *title;
    struct tn_entry *entries;
};

struct tn_model {
    char *header;
    struct tn_stmt *stmts;
};

static struct tn_model model;
static bool dirty;
static bool usb;

/* ----------------------------------------------------------------------- */
/* Memory                                                                  */
/* ----------------------------------------------------------------------- */

static char *arena;
static size_t arena_size, arena_used, arena_base;
static bool oom;

static void *alloc(size_t n)
{
    n = (n + sizeof(void *) - 1) & ~(sizeof(void *) - 1);   /* keep pointers aligned */
    if (arena_used + n > arena_size)
    {
        oom = true;
        return NULL;
    }
    void *p = arena + arena_used;
    arena_used += n;
    rb->memset(p, 0, n);
    return p;
}

static char *sdup_n(const char *s, size_t len)
{
    char *p = alloc(len + 1);
    if (p)
    {
        rb->memcpy(p, s, len);
        p[len] = '\0';
    }
    return p;
}

static char *sdup(const char *s)
{
    return sdup_n(s, rb->strlen(s));
}

static char *text_buf;   /* file text read in, and scratch for checks */
static char *ser_buf;    /* serialised model */

/* ----------------------------------------------------------------------- */
/* Tokenizer (mirrors the matching in tagtree.c get_tag / get_clause)      */
/* ----------------------------------------------------------------------- */

static const char *const syms[] = {
    "==>", "->", "~>", "==", "!=", ">=", "<=", "!~", "!^", "!$", "@~", "@^",
    "@$", "*~", "*^", "*$", "!@", "=", ">", "<", "~", "^", "$", "@", "&",
    "|", "?",
};
enum { SY_LOAD = 0, SY_NEXT = 1, SY_SHUFFLE = 2, SY_EQEQ = 3, SY_EQ = 17,
       SY_AND = 24, SY_OR = 25, SY_QUERY = 26 };

enum { TK_STR, TK_WORD, TK_SYM };

struct tn_token {
    unsigned char type, sym;
    unsigned short len;
    const char *s;
};

static struct tn_token toks[MAX_TOKENS];
static int ntoks, tpos;

static int match_sym(const char *p)
{
    for (unsigned i = 0; i < ARRAYLEN(syms); i++)
    {
        size_t l = rb->strlen(syms[i]);
        if (!rb->strncmp(p, syms[i], l))
            return i;
    }
    return -1;
}

static bool tokenize(const char *line)
{
    const char *p = line;
    ntoks = tpos = 0;
    while (*p)
    {
        if (*p == ' ' || *p == '\t')
        {
            p++;
            continue;
        }
        if (ntoks >= MAX_TOKENS)
            return false;
        struct tn_token *t = &toks[ntoks++];
        if (*p == '"')
        {
            const char *q = rb->strchr(p + 1, '"');
            if (!q)
                return false;
            t->type = TK_STR;
            t->s = p + 1;
            t->len = q - p - 1;
            p = q + 1;
            continue;
        }
        int s = match_sym(p);
        if (s >= 0)
        {
            t->type = TK_SYM;
            t->sym = s;
            t->s = p;
            t->len = rb->strlen(syms[s]);
            p += t->len;
            continue;
        }
        const char *q = p;
        while (*q && *q != ' ' && *q != '\t' && *q != '"' && match_sym(q) < 0)
            q++;
        if (q == p)
            q++;
        t->type = TK_WORD;
        t->s = p;
        t->len = q - p;
        p = q;
    }
    return true;
}

static struct tn_token *peek(void)
{
    return tpos < ntoks ? &toks[tpos] : NULL;
}

static bool is_sym(struct tn_token *t, int sym)
{
    return t && t->type == TK_SYM && t->sym == sym;
}

static bool word_is(struct tn_token *t, const char *w)
{
    return t && t->type == TK_WORD && t->len == rb->strlen(w)
           && !rb->strncasecmp(t->s, w, t->len);
}

static char *need_str(void)
{
    struct tn_token *t = peek();
    if (!t || t->type != TK_STR)
        return NULL;
    tpos++;
    return sdup_n(t->s, t->len);
}

static int field_of(struct tn_token *t)
{
    if (!t || t->type != TK_WORD)
        return -1;
    if (word_is(t, "ensemble"))
        return FLD_AARTIST;
    for (int i = 0; i < FLD_COUNT; i++)
        if (word_is(t, fields[i].word))
            return i;
    return -1;
}

static int field_of_word(const char *w)
{
    for (int i = 0; i < FLD_COUNT; i++)
        if (!rb->strcasecmp(w, fields[i].word))
            return i;
    if (!rb->strcasecmp(w, "ensemble"))
        return FLD_AARTIST;
    return -1;
}

static int op_of(struct tn_token *t)
{
    if (!t || t->type != TK_SYM)
        return -1;
    if (t->sym == SY_EQEQ || t->sym == SY_EQ)
        return OP_IS;
    for (int i = 0; i < OP_COUNT; i++)
        if (t->len == rb->strlen(op_sym[i]) && !rb->strncmp(t->s, op_sym[i], t->len))
            return i;
    return -1;
}

/* ----------------------------------------------------------------------- */
/* Parser                                                                  */
/* ----------------------------------------------------------------------- */

static struct tn_clause *new_clause(int fld, int op, const char *value)
{
    struct tn_clause *c = alloc(sizeof *c);
    if (c)
    {
        c->fld = fld;
        c->op = op;
        c->value = value ? sdup(value) : NULL;
        if (value && !c->value)
            return NULL;
    }
    return c;
}

/* clauses after "?": tag op "value", joined by & or |; also "= fmt" */
static bool read_conds(struct tn_screen *sc, struct tn_entry *e)
{
    struct tn_clause **tail = &sc->clauses;
    while (*tail)
        tail = &(*tail)->next;

    struct tn_token *t;
    while ((t = peek()))
    {
        if (is_sym(t, SY_NEXT) || is_sym(t, SY_SHUFFLE))
            return true;
        if (is_sym(t, SY_AND) || is_sym(t, SY_QUERY))
        {
            tpos++;
            continue;
        }
        if (is_sym(t, SY_OR))
        {
            tpos++;
            if (!(*tail = new_clause(0, OP_OR, NULL)))
                return false;
            tail = &(*tail)->next;
            continue;
        }
        if (is_sym(t, SY_EQ) && tpos + 1 < ntoks && toks[tpos + 1].type == TK_STR)
        {
            tpos++;
            if (!(sc->fmt = need_str()))
                return false;
            continue;
        }
        if (word_is(t, "%hide_special"))
        {
            tpos++;
            e->hide_special = true;
            continue;
        }
        int fld = field_of(t);
        if (fld < 0)
            return false;
        tpos++;
        int op = op_of(peek());
        if (op < 0)
            return false;
        tpos++;
        char *v = need_str();
        if (!v)
            return false;
        struct tn_clause *c = alloc(sizeof *c);
        if (!c)
            return false;
        c->fld = fld;
        c->op = op;
        c->value = v;
        *tail = c;
        tail = &c->next;
    }
    return true;
}

static struct tn_entry *parse_entry(const char *line)
{
    if (!tokenize(line))
        return NULL;
    struct tn_entry *e = alloc(sizeof *e);
    if (!e || !(e->name = need_str()))
        return NULL;

    struct tn_token *t = peek();
    if (is_sym(t, SY_LOAD))
    {
        tpos++;
        e->kind = E_LINK;
        if (!(e->text = need_str()) || peek())
            return NULL;
        return e;
    }
    if (!is_sym(t, SY_NEXT) && !is_sym(t, SY_SHUFFLE))
        return NULL;
    e->kind = is_sym(t, SY_SHUFFLE) ? E_SHUFFLE : E_BROWSE;
    tpos++;

    struct tn_screen **tail = &e->screens;
    int count = 0;
    for (;;)
    {
        t = peek();
        if ((word_is(t, "%reload") || word_is(t, "%edit_menus")) && count == 0)
        {
            int kind = word_is(t, "%reload") ? E_RELOAD : E_EDIT;
            tpos++;
            if (peek())
                return NULL;
            e->kind = kind;
            return e;
        }
        int fld = field_of(t);
        if (fld < 0)
            return NULL;
        tpos++;
        struct tn_screen *sc = alloc(sizeof *sc);
        if (!sc)
            return NULL;
        sc->fld = fld;
        if (!read_conds(sc, e))
            return NULL;
        *tail = sc;
        tail = &sc->next;
        if (++count > MAX_SCREENS)
            return NULL;
        if (!peek())
            break;
        tpos++; /* -> or ~> */
    }
    return e;
}

static struct tn_stmt *parse_stmt(const char *line)
{
    if (!tokenize(line))
        return NULL;
    struct tn_token *t = peek();
    struct tn_stmt *s = alloc(sizeof *s);
    if (!s || !t)
        return NULL;
    tpos++;

    if (word_is(t, "%format"))
    {
        s->kind = K_FORMAT;
        s->id = need_str();
        s->text = sdup(line);
        return (s->id && s->text) ? s : NULL;
    }
    if (word_is(t, "%include"))
    {
        s->kind = K_INCLUDE;
        s->text = sdup(line);
        return s->text ? s : NULL;
    }
    if (word_is(t, "%menu_start"))
    {
        s->kind = K_MENU;
        s->id = need_str();
        s->title = need_str();
        return (s->id && s->title) ? s : NULL;
    }
    if (word_is(t, "%byfirstletter"))
    {
        s->kind = K_AZ;
        s->id = need_str();
        s->title = need_str();
        char *w = need_str();
        int fld = w ? field_of_word(w) : -1;
        if (!s->id || !s->title || fld < 0)
            return NULL;
        s->fld = fld;
        return s;
    }
    if (word_is(t, "%root_menu"))
    {
        s->kind = K_ROOT;
        s->id = need_str();
        return s->id ? s : NULL;
    }
    return NULL;
}

/* Parses text (modified in place).  Returns NULL or an error message. */
static const char *parse_text(char *text, struct tn_model *m)
{
    rb->memset(m, 0, sizeof *m);
    struct tn_stmt **stail = &m->stmts;
    struct tn_stmt *menu = NULL;
    struct tn_entry **etail = NULL;
    char *p = text;
    bool first = true;

    while (*p)
    {
        char *line = p;
        char *nl = rb->strchr(p, '\n');
        if (nl)
        {
            *nl = '\0';
            p = nl + 1;
        }
        else
            p += rb->strlen(p);
        size_t len = rb->strlen(line);
        if (len && line[len - 1] == '\r')
            line[--len] = '\0';

        if (first)
        {
            if (rb->strcmp(line, TAGNAVI_VERSION))
                return "Not a tagnavi file (wrong first line)";
            if (!(m->header = sdup(line)))
                return "Out of memory";
            first = false;
            continue;
        }

        if (menu)
        {
            if (!*line)
            {
                menu = NULL;
                continue;
            }
            struct tn_entry *e = NULL;
            if (*line != '#')
                e = parse_entry(line);
            if (!e)
            {
                if (oom || !(e = alloc(sizeof *e)) || !(e->text = sdup(line)))
                    return "Out of memory";
                e->kind = (*line == '#') ? E_COMMENT : E_RAW;
                e->screens = NULL;
                e->next = NULL;
                if (e->kind == E_RAW)
                {
                    /* show the quoted name if there is one */
                    const char *q = rb->strchr(line, '"');
                    const char *q2 = q ? rb->strchr(q + 1, '"') : NULL;
                    e->name = (q && q2) ? sdup_n(q + 1, q2 - q - 1) : sdup("Unreadable line");
                }
            }
            *etail = e;
            etail = &e->next;
            continue;
        }

        struct tn_stmt *s = NULL;
        if (*line && *line != '#')
            s = parse_stmt(line);
        if (!s)
        {
            if (oom || !(s = alloc(sizeof *s)))
                return "Out of memory";
            s->kind = !*line ? K_BLANK : (*line == '#') ? K_COMMENT : K_RAW;
            if (*line && !(s->text = sdup(line)))
                return "Out of memory";
        }
        *stail = s;
        stail = &s->next;
        if (s->kind == K_MENU)
        {
            menu = s;
            etail = &s->entries;
        }
    }
    if (first)
        return "The file is empty";
    return oom ? "Out of memory" : NULL;
}

/* ----------------------------------------------------------------------- */
/* Writer                                                                  */
/* ----------------------------------------------------------------------- */

static char *out_buf;
static size_t out_len, out_cap;
static bool out_full;

static void out(const char *s)
{
    size_t l = rb->strlen(s);
    if (out_len + l + 1 > out_cap)
    {
        out_full = true;
        return;
    }
    rb->memcpy(out_buf + out_len, s, l + 1);
    out_len += l;
}

static void outq(const char *s)
{
    out("\"");
    out(s);
    out("\"");
}

static void out_clauses(const struct tn_clause *c)
{
    bool need_and = false;
    for (; c; c = c->next)
    {
        if (c->op == OP_OR)
        {
            out(" |");
            need_and = false;
            continue;
        }
        out(need_and ? " & " : " ");
        out(fields[c->fld].word);
        out(" ");
        out(op_sym[c->op]);
        out(" ");
        outq(c->value);
        need_and = true;
    }
}

static void out_entry(const struct tn_entry *e)
{
    switch (e->kind)
    {
        case E_COMMENT:
        case E_RAW:
            out(e->text);
            break;
        case E_LINK:
            outq(e->name);
            out(" ==> ");
            outq(e->text);
            break;
        case E_RELOAD:
            outq(e->name);
            out(" -> %reload");
            break;
        case E_EDIT:
            outq(e->name);
            out(" -> %edit_menus");
            break;
        default:
            outq(e->name);
            out(e->kind == E_SHUFFLE ? " ~> " : " -> ");
            for (const struct tn_screen *sc = e->screens; sc; sc = sc->next)
            {
                out(fields[sc->fld].word);
                if (sc->fmt)
                {
                    out(" = ");
                    outq(sc->fmt);
                }
                if (sc->clauses)
                {
                    out(" ?");
                    out_clauses(sc->clauses);
                }
                if (sc->next)
                    out(" -> ");
            }
            if (e->hide_special)
                out(" %hide_special");
            break;
    }
    out("\n");
}

/* Returns the text length, or -1 if it did not fit. */
static int write_model(const struct tn_model *m, char *buf, size_t cap)
{
    out_buf = buf;
    out_cap = cap;
    out_len = 0;
    out_full = false;
    buf[0] = '\0';

    out(m->header);
    out("\n");
    for (const struct tn_stmt *s = m->stmts; s; s = s->next)
    {
        switch (s->kind)
        {
            case K_BLANK:
                break;
            case K_MENU:
                out("%menu_start ");
                outq(s->id);
                out(" ");
                outq(s->title);
                out("\n");
                for (const struct tn_entry *e = s->entries; e; e = e->next)
                    out_entry(e);
                break;   /* the blank line below ends the menu */
            case K_AZ:
                out("%byfirstletter ");
                outq(s->id);
                out(" ");
                outq(s->title);
                out(" ");
                outq(fields[s->fld].word);
                break;
            case K_ROOT:
                out("%root_menu ");
                outq(s->id);
                break;
            default:
                out(s->text);
                break;
        }
        out("\n");
    }
    return out_full ? -1 : (int)out_len;
}

/* ----------------------------------------------------------------------- */
/* Model helpers                                                           */
/* ----------------------------------------------------------------------- */

static void changed(void)
{
    dirty = true;
}

static bool is_menu(const struct tn_stmt *s)
{
    return s->kind == K_MENU || s->kind == K_AZ;
}

static struct tn_stmt *find_menu(const char *id)
{
    for (struct tn_stmt *s = model.stmts; s; s = s->next)
        if (is_menu(s) && !rb->strcasecmp(s->id, id))
            return s;
    return NULL;
}

static struct tn_stmt *root_stmt(void)
{
    for (struct tn_stmt *s = model.stmts; s; s = s->next)
        if (s->kind == K_ROOT)
            return s;
    return NULL;
}

/* the start menu; tagtree.c falls back to the first menu */
static struct tn_stmt *top_menu(void)
{
    struct tn_stmt *r = root_stmt();
    struct tn_stmt *m = r ? find_menu(r->id) : NULL;
    if (!m)
        for (m = model.stmts; m && !is_menu(m); m = m->next);
    return m;
}

static bool has_include(void)
{
    for (struct tn_stmt *s = model.stmts; s; s = s->next)
        if (s->kind == K_INCLUDE)
            return true;
    return false;
}

/* new menus go before %root_menu, which must come after the menu it names */
static void insert_menu(struct tn_stmt *m)
{
    struct tn_stmt **pp = &model.stmts;
    while (*pp && (*pp)->kind != K_ROOT)
        pp = &(*pp)->next;
    m->next = *pp;
    *pp = m;
}

static void make_id(const char *title, char *id)
{
    char base[24];
    int n = 0;
    for (const char *p = title; *p && n < (int)sizeof base - 1; p++)
    {
        char c = *p;
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            base[n++] = c;
        else if (n && base[n - 1] != '_')
            base[n++] = '_';
    }
    base[n] = '\0';
    if (!n)
        rb->strcpy(base, "menu");
    rb->strcpy(id, base);
    for (int k = 2; find_menu(id); k++)
        rb->snprintf(id, ID_LEN, "%s_%d", base, k);
}

static int count_visible(const struct tn_stmt *m)
{
    int n = 0;
    for (const struct tn_entry *e = m->entries; e; e = e->next)
        if (e->kind != E_COMMENT)
            n++;
    return n;
}

static void unlink_entry(struct tn_stmt *m, struct tn_entry *e)
{
    for (struct tn_entry **pp = &m->entries; *pp; pp = &(*pp)->next)
        if (*pp == e)
        {
            *pp = e->next;
            return;
        }
}

static void append_entry(struct tn_stmt *m, struct tn_entry *e)
{
    struct tn_entry **pp = &m->entries;
    while (*pp)
        pp = &(*pp)->next;
    e->next = NULL;
    *pp = e;
}

/* swap an entry with the next/previous visible one by swapping contents */
static bool move_entry(struct tn_stmt *m, struct tn_entry *e, int dir)
{
    struct tn_entry *prev = NULL, *other = NULL;
    for (struct tn_entry *x = m->entries; x; x = x->next)
    {
        if (x->kind == E_COMMENT)
            continue;
        if (x == e)
        {
            if (dir < 0)
                other = prev;
            else
                for (other = x->next; other && other->kind == E_COMMENT; other = other->next);
            break;
        }
        prev = x;
    }
    if (!other)
        return false;
    struct tn_entry tmp = *e;
    struct tn_entry *en = e->next, *on = other->next;
    *e = *other;
    *other = tmp;
    e->next = en;
    other->next = on;
    return true;
}

static int count_screens(const struct tn_entry *e)
{
    int n = 0;
    for (const struct tn_screen *s = e->screens; s; s = s->next)
        n++;
    return n;
}

static struct tn_screen *nth_screen(struct tn_entry *e, int i)
{
    struct tn_screen *s = e->screens;
    while (s && i--)
        s = s->next;
    return s;
}

/* Screen edits keep the item's filters on the first screen. */
static struct tn_clause *take_filters(struct tn_entry *e)
{
    struct tn_clause *c = e->screens ? e->screens->clauses : NULL;
    if (e->screens)
        e->screens->clauses = NULL;
    return c;
}

static void give_filters(struct tn_entry *e, struct tn_clause *c)
{
    if (!e->screens || !c)
        return;
    struct tn_clause **pp = &c;
    while (*pp)
        pp = &(*pp)->next;
    *pp = e->screens->clauses;
    e->screens->clauses = c;
}

static void insert_screen(struct tn_entry *e, int at, struct tn_screen *sc)
{
    struct tn_clause *f = take_filters(e);
    struct tn_screen **pp = &e->screens;
    while (*pp && at--)
        pp = &(*pp)->next;
    sc->next = *pp;
    *pp = sc;
    give_filters(e, f);
}

static void remove_screen(struct tn_entry *e, int i)
{
    struct tn_clause *f = take_filters(e);
    struct tn_screen **pp = &e->screens;
    while (*pp && i--)
        pp = &(*pp)->next;
    if (*pp)
        *pp = (*pp)->next;
    give_filters(e, f);
}

static void swap_screens(struct tn_entry *e, int i)   /* i and i+1 */
{
    struct tn_clause *f = take_filters(e);
    struct tn_screen *a = nth_screen(e, i), *b = a ? a->next : NULL;
    if (a && b)
    {
        unsigned char fld = a->fld;
        char *fmt = a->fmt;
        struct tn_clause *c = a->clauses;
        a->fld = b->fld; a->fmt = b->fmt; a->clauses = b->clauses;
        b->fld = fld; b->fmt = fmt; b->clauses = c;
    }
    give_filters(e, f);
}

static bool screen_uses(const struct tn_entry *e, int fld)
{
    for (const struct tn_screen *s = e->screens; s; s = s->next)
        if (s->fld == fld)
            return true;
    return false;
}

static void unlink_clause(struct tn_clause **list, struct tn_clause *c, int count)
{
    for (struct tn_clause **pp = list; *pp; pp = &(*pp)->next)
        if (*pp == c)
        {
            struct tn_clause *end = c;
            while (--count > 0 && end->next)
                end = end->next;
            *pp = end->next;
            return;
        }
}

/* inserts the chain [first..] after `after` (NULL: at the end) */
static void insert_clauses(struct tn_clause **list, struct tn_clause *after, struct tn_clause *first)
{
    struct tn_clause *last = first;
    while (last->next)
        last = last->next;
    if (after)
    {
        last->next = after->next;
        after->next = first;
        return;
    }
    struct tn_clause **pp = list;
    while (*pp)
        pp = &(*pp)->next;
    *pp = first;
}

static const char *format_label(const char *name)
{
    if (!name)
        return "Title only";
    for (unsigned i = 0; i < ARRAYLEN(format_labels); i++)
        if (!rb->strcasecmp(name, format_labels[i].name))
            return format_labels[i].label;
    return name;
}

static bool format_exists(const char *name)
{
    for (struct tn_stmt *s = model.stmts; s; s = s->next)
        if (s->kind == K_FORMAT && !rb->strcasecmp(s->id, name))
            return true;
    return false;
}

static const char *runtime_for(int fld)
{
    for (unsigned i = 0; i < ARRAYLEN(runtime_values); i++)
        if (runtime_values[i].fld == fld)
            return runtime_values[i].value;
    return NULL;
}

static bool is_runtime(const char *v)
{
    return v && v[0] == '#' && v[rb->strlen(v) - 1] == '#' && v[1];
}

/* ----------------------------------------------------------------------- */
/* Filter descriptions and checks                                          */
/* ----------------------------------------------------------------------- */

static bool is_low(const struct tn_clause *c)
{
    return c->op == OP_GTEQ || c->op == OP_GT;
}

static bool is_high(const struct tn_clause *c)
{
    return c->op == OP_LTEQ || c->op == OP_LT;
}

/* a low bound followed by a high bound on one numeric field reads as a range */
static bool is_range(const struct tn_clause *c)
{
    const struct tn_clause *d = c->next;
    return c->op != OP_OR && d && d->op != OP_OR && c->fld == d->fld
           && IS_NUM(c->fld) && is_low(c) && is_high(d);
}

static void value_text(const struct tn_clause *c, char *buf, size_t n)
{
    if (!c->value[0])
        rb->snprintf(buf, n, "(%s)", LSTR(LANG_ASK));
    else if (is_runtime(c->value))
        rb->strlcpy(buf, "(playing track)", n);
    else if (c->op == OP_ONEOF || c->op == OP_NONEOF)
    {
        size_t k = 0;
        for (const char *p = c->value; *p && k + 3 < n; p++)
        {
            if (*p == '|')
            {
                buf[k++] = ',';
                buf[k++] = ' ';
            }
            else
                buf[k++] = *p;
        }
        buf[k] = '\0';
    }
    else
        rb->strlcpy(buf, c->value, n);
}

static void bound_text(const struct tn_clause *c, char *buf, size_t n)
{
    if (!c->value[0] || is_runtime(c->value))
        value_text(c, buf, n);
    else if (c->op == OP_GT)
        rb->snprintf(buf, n, "%d", rb->atoi(c->value) + 1);
    else if (c->op == OP_LT)
        rb->snprintf(buf, n, "%d", rb->atoi(c->value) - 1);
    else
        rb->strlcpy(buf, c->value, n);
}

static void clause_text(const struct tn_clause *c, char *buf, size_t n)
{
    char v[96];
    if (c->op == OP_OR)
    {
        rb->strlcpy(buf, "- or -", n);
        return;
    }
    if (is_range(c))
    {
        char lo[24], hi[24];
        bound_text(c, lo, sizeof lo);
        bound_text(c->next, hi, sizeof hi);
        rb->snprintf(buf, n, "%s %s to %s", fld_label(c->fld), lo, hi);
        return;
    }
    value_text(c, v, sizeof v);
    if (c->fld == FLD_FILENAME && (c->op == OP_BEGINS || c->op == OP_NBEGINS))
        rb->snprintf(buf, n, "%s: %s", c->op == OP_BEGINS ? "In folder" : "Not in folder", v);
    else if ((c->op == OP_ONEOF || c->op == OP_NONEOF) && c->value[0] && !is_runtime(c->value))
        rb->snprintf(buf, n, "%s%s: %s", fld_label(c->fld),
                     c->op == OP_NONEOF ? " not" : "", v);
    else
        rb->snprintf(buf, n, "%s %s %s", fld_label(c->fld), test_label(c->fld, c->op), v);
}

/* Problems that make an "and" group impossible.  Returns how many were written. */
static int find_conflicts(const struct tn_clause *list, char msgs[][64], int max)
{
    int n = 0;
    const struct tn_clause *g = list;
    while (g && n < max)
    {
        /* the group runs until the next "or" */
        const struct tn_clause *end = g;
        while (end && end->op != OP_OR)
            end = end->next;

        for (const struct tn_clause *a = g; a != end && n < max; a = a->next)
        {
            if (!a->value[0] || is_runtime(a->value))
                continue;
            bool first_of_field = true;
            for (const struct tn_clause *b = g; b != a; b = b->next)
                if (b->fld == a->fld)
                    first_of_field = false;

            for (const struct tn_clause *b = a->next; b != end && n < max; b = b->next)
            {
                if (!b->value[0] || is_runtime(b->value) || b->fld != a->fld)
                    continue;
                if (a->op == b->op && !rb->strcasecmp(a->value, b->value))
                    rb->snprintf(msgs[n++], 64, "%s is listed twice", fld_label(a->fld));
                else if (a->op == OP_IS && b->op == OP_IS)
                    rb->snprintf(msgs[n++], 64, "%s can't be both %s and %s",
                                 fld_label(a->fld), a->value, b->value);
            }

            if (!first_of_field || !IS_NUM(a->fld))
                continue;
            /* numeric bounds for this field across the group */
            long lo = -2147483647L, hi = 2147483647L;
            for (const struct tn_clause *b = g; b != end; b = b->next)
            {
                if (b->fld != a->fld || !b->value[0] || is_runtime(b->value))
                    continue;
                long v = rb->atoi(b->value);
                if (b->op == OP_GTEQ && v > lo) lo = v;
                if (b->op == OP_GT && v + 1 > lo) lo = v + 1;
                if (b->op == OP_LTEQ && v < hi) hi = v;
                if (b->op == OP_LT && v - 1 < hi) hi = v - 1;
            }
            if (lo > hi && n < max)
                rb->snprintf(msgs[n++], 64, "%s can't be at least %ld and at most %ld",
                             fld_label(a->fld), lo, hi);
            for (const struct tn_clause *b = g; b != end && n < max; b = b->next)
                if (b->fld == a->fld && b->op == OP_IS && b->value[0] && !is_runtime(b->value)
                    && lo <= hi && (rb->atoi(b->value) < lo || rb->atoi(b->value) > hi))
                    rb->snprintf(msgs[n++], 64, "%s = %s is outside %ld to %ld",
                                 fld_label(a->fld), b->value, lo, hi);
        }
        g = end ? end->next : NULL;
    }
    return n;
}

static int count_filters(const struct tn_clause *c)
{
    int n = 0;
    for (; c; c = c->next)
    {
        if (c->op == OP_OR)
            continue;
        if (is_range(c))
            c = c->next;
        n++;
    }
    return n;
}

#ifndef TNE_HOSTTEST
/* ----------------------------------------------------------------------- */
/* Lists                                                                   */
/* ----------------------------------------------------------------------- */

#define MAX_ROWS     300
#define ROW_POOL     (16 * 1024)
#define ROW_HEADER   1
#define ROW_DISABLED 2

enum { LIST_OK, LIST_CONTEXT, LIST_CANCEL };

struct tn_row {
    const char *text;
    void *ptr;
    short id;
    short icon;
    short aux;
    unsigned char flags;
};

static struct tn_row rows[MAX_ROWS];
static int nrows;
static char row_pool[ROW_POOL];
static size_t row_pool_used;
static struct gui_synclist list;

static void rows_reset(void)
{
    nrows = 0;
    row_pool_used = 0;
}

static struct tn_row *add_row(int id, int icon, void *ptr, const char *fmt, ...)
{
    if (nrows >= MAX_ROWS)
        return &rows[MAX_ROWS - 1];
    struct tn_row *r = &rows[nrows++];
    va_list ap;
    va_start(ap, fmt);
    char *dst = row_pool + row_pool_used;
    size_t room = ROW_POOL - row_pool_used;
    int n = rb->vsnprintf(dst, room, fmt, ap);
    va_end(ap);
    if (n < 0)
        n = 0;
    if ((size_t)n >= room)
        n = room ? room - 1 : 0;
    row_pool_used += n + 1;
    if (row_pool_used > ROW_POOL)
        row_pool_used = ROW_POOL;
    r->text = dst;
    r->id = id;
    r->icon = icon;
    r->ptr = ptr;
    r->aux = 0;
    r->flags = 0;
    return r;
}

static const char *row_name(int i, void *data, char *buf, size_t len)
{
    (void)data; (void)buf; (void)len;
    return rows[i].text;
}

static enum themable_icons row_icon(int i, void *data)
{
    (void)data;
    return rows[i].icon;
}

static bool row_skipped(int i)
{
    return rows[i].flags & ROW_HEADER;
}

/* Shows rows[] until OK, context or cancel.  *sel keeps the position. */
static int list_run(const char *title, int *sel)
{
    rb->gui_synclist_init(&list, row_name, NULL, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&list, row_icon);
    rb->gui_synclist_set_title(&list, title, NOICON);
    rb->gui_synclist_set_nb_items(&list, nrows);
    if (*sel >= nrows)
        *sel = nrows - 1;
    if (*sel < 0)
        *sel = 0;
    for (int g = 0; nrows && row_skipped(*sel) && g < nrows; g++)
        *sel = (*sel + 1) % nrows;
    rb->gui_synclist_select_item(&list, *sel);

    for (;;)
    {
        rb->gui_synclist_draw(&list);
        int action = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);
        int requested = action;
        int before = rb->gui_synclist_get_sel_pos(&list);
        if (rb->gui_synclist_do_button(&list, &action))
        {
            int now = rb->gui_synclist_get_sel_pos(&list);
            if (nrows && row_skipped(now))
            {
                int dir;
                if (requested == ACTION_STD_PREV || requested == ACTION_STD_PREVREPEAT)
                    dir = -1;
                else if (requested == ACTION_STD_NEXT || requested == ACTION_STD_NEXTREPEAT)
                    dir = 1;
                else
                    dir = now < before ? -1 : 1;
                for (int g = 0; row_skipped(now) && g < nrows; g++)
                    now = (now + dir + nrows) % nrows;
                rb->gui_synclist_select_item(&list, now);
            }
            continue;
        }
        *sel = rb->gui_synclist_get_sel_pos(&list);
        switch (action)
        {
            case ACTION_STD_OK:
                if (!nrows || (rows[*sel].flags & (ROW_HEADER | ROW_DISABLED)))
                    break;
                return LIST_OK;
            case ACTION_STD_CONTEXT:
                if (!nrows)
                    break;
                return LIST_CONTEXT;
            case ACTION_STD_CANCEL:
                return LIST_CANCEL;
            default:
                if (rb->default_event_handler(action) == SYS_USB_CONNECTED)
                {
                    usb = true;
                    return LIST_CANCEL;
                }
                break;
        }
    }
}

/* Picks one of n strings.  Returns its index, or -1. */
static int choose(const char *title, const char *const *items, int n, int initial)
{
    int sel = initial;
    for (;;)
    {
        rows_reset();
        for (int i = 0; i < n; i++)
            add_row(i, NOICON, NULL, "%s", items[i]);
        int r = list_run(title, &sel);
        if (r == LIST_OK)
            return sel;
        if (r == LIST_CANCEL)
            return -1;
    }
}

/* Yes/no question, word-wrapped to the screen width in the current font
 * (the yes/no screen does not wrap a single long line). */
static bool confirm(const char *question)
{
    static char lines[10][64];
    const char *ptrs[10];
    int n = 0, len = 0, w, h;
    const char *p = question;
    lines[0][0] = '\0';
    while (*p && n < 10)
    {
        const char *q = p;
        while (*q && *q != ' ')
            q++;
        char word[64];
        int wl = MIN((int)(q - p), (int)sizeof word - 1);
        rb->memcpy(word, p, wl);
        word[wl] = '\0';

        char trial[128];
        rb->snprintf(trial, sizeof trial, "%s%s%s", lines[n], len ? " " : "", word);
        rb->lcd_getstringsize((const unsigned char *)trial, &w, &h);
        if (len && (w > LCD_WIDTH - 8 || rb->strlen(trial) >= sizeof lines[n]))
        {
            if (++n >= 10)
                break;
            rb->strlcpy(lines[n], word, sizeof lines[n]);
        }
        else
            rb->strlcpy(lines[n], trial, sizeof lines[n]);
        len = rb->strlen(lines[n]);
        p = *q ? q + 1 : q;
    }
    if (n < 10 && len)
        n++;
    for (int i = 0; i < n; i++)
        ptrs[i] = lines[i];
    struct text_message msg = { ptrs, n };
    return rb->gui_syncyesno_run(&msg, NULL, NULL) == YESNO_YES;
}

/* Text entry; double quotes are removed because the file format can't hold them. */
static bool input_text(const char *initial, char *buf, size_t size)
{
    rb->strlcpy(buf, initial ? initial : "", size);
    if (rb->kbd_input(buf, size, NULL) < 0)
        return false;
    char *w = buf;
    bool had_quote = false;
    for (char *p = buf; *p; p++)
    {
        if (*p == '"')
            had_quote = true;
        else
            *w++ = *p;
    }
    *w = '\0';
    while (*buf == ' ')
        rb->memmove(buf, buf + 1, rb->strlen(buf));
    for (size_t l = rb->strlen(buf); l && buf[l - 1] == ' '; l--)
        buf[l - 1] = '\0';
    if (had_quote)
        rb->splash(HZ, "Quote marks can't be used here and were removed");
    return buf[0] != '\0';
}

static int pick_field(const char *title, const unsigned char *list_of, int n,
                      const struct tn_entry *exclude_used, int keep, bool screens)
{
    const char *names[FLD_COUNT];
    unsigned char ids[FLD_COUNT];
    int k = 0, initial = 0;
    for (int i = 0; i < n; i++)
    {
        int f = list_of[i];
        if (exclude_used && f != keep && screen_uses(exclude_used, f))
            continue;
        if (f == keep)
            initial = k;
        ids[k] = f;
        names[k++] = screens ? screen_label(f) : fld_label(f);
    }
    if (!k)
    {
        rb->splash(HZ, "All fields are in use");
        return -1;
    }
    int r = choose(title, names, k, initial);
    return r < 0 ? -1 : ids[r];
}


/* ----------------------------------------------------------------------- */
/* Database values                                                         */
/* ----------------------------------------------------------------------- */

#define MAX_VALUES 3000
static uint32_t uniqbuf[4096];
static char **values;
static int nvalues;
static bool values_numeric;

static int value_cmp(const void *a, const void *b)
{
    const char *x = *(char *const *)a, *y = *(char *const *)b;
    if (values_numeric)
        return rb->atoi(x) - rb->atoi(y);
    return rb->strcasecmp(x, y);
}

/* Fills values[] (temporary, in the arena above `mark`). */
static bool load_values(int fld, size_t *mark)
{
    *mark = arena_used;
    nvalues = 0;
    if (!(fields[fld].flags & F_PICK))
        return false;
    struct tagcache_stat *st = rb->tagcache_get_stat();
    if (!st || !st->ready)
    {
        rb->splash(HZ * 2, LSTR(LANG_TAGCACHE_BUSY));
        return false;
    }
    values = alloc(sizeof(char *) * MAX_VALUES);
    if (!values)
        return false;

    struct tagcache_search tcs;
    char buf[VALUE_LEN];
    rb->splash(0, LSTR(LANG_WAIT));
    if (!rb->tagcache_search(&tcs, fields[fld].tag))
    {
        rb->splash(HZ * 2, LSTR(LANG_TAGCACHE_BUSY));
        return false;
    }
    values_numeric = IS_NUM(fld);
    if (!values_numeric)
        rb->tagcache_search_set_uniqbuf(&tcs, uniqbuf, sizeof uniqbuf);
    while (nvalues < MAX_VALUES && rb->tagcache_get_next(&tcs, buf, sizeof buf))
    {
        const char *v = tcs.result;
        if (!v || !*v || !rb->strcmp(v, "<Untagged>") || rb->strchr(v, '"'))
            continue;
        if (values_numeric && rb->atoi(v) <= 0)
            continue;
        char *copy = sdup(v);
        if (!copy)
            break;
        values[nvalues++] = copy;
    }
    rb->tagcache_search_finish(&tcs);

    rb->qsort(values, nvalues, sizeof(char *), value_cmp);
    int w = 0;
    for (int i = 0; i < nvalues; i++)
        if (!w || (values_numeric ? rb->atoi(values[i]) != rb->atoi(values[w - 1])
                                  : rb->strcasecmp(values[i], values[w - 1])))
            values[w++] = values[i];
    nvalues = w;
    if (!nvalues)
        rb->splash(HZ, "No values in the database");
    return nvalues > 0;
}

/* Picks one database value.  Returns a copy, or NULL. */
static char *pick_library(int fld)
{
    size_t mark;
    char picked[VALUE_LEN];
    if (!load_values(fld, &mark))
    {
        arena_used = mark;
        return NULL;
    }
    int r = choose(fld_label(fld), (const char *const *)values, nvalues, 0);
    if (r >= 0)
        rb->strlcpy(picked, values[r], sizeof picked);
    arena_used = mark;
    return r >= 0 ? sdup(picked) : NULL;
}

/* Ticks several database values; returns "a|b|c", or NULL. */
static char *pick_several(int fld, const char *current)
{
    size_t mark;
    char result[VALUE_LEN];
    if (!load_values(fld, &mark))
    {
        arena_used = mark;
        return NULL;
    }
    unsigned char *ticked = alloc(nvalues);
    if (!ticked)
    {
        arena_used = mark;
        return NULL;
    }
    /* tick what is already in the filter */
    if (current && *current && !is_runtime(current))
        for (int i = 0; i < nvalues; i++)
        {
            size_t l = rb->strlen(values[i]);
            for (const char *p = current; *p; )
            {
                const char *q = rb->strchr(p, '|');
                size_t pl = q ? (size_t)(q - p) : rb->strlen(p);
                if (pl == l && !rb->strncasecmp(p, values[i], l))
                    ticked[i] = 1;
                p += pl + (q ? 1 : 0);
            }
        }

    int sel = 0;
    char title[64];
    rb->snprintf(title, sizeof title, "%s: tick values", fld_label(fld));
    for (;;)
    {
        int count = 0;
        for (int i = 0; i < nvalues; i++)
            count += ticked[i];
        rows_reset();
        if (count)
            add_row(-1, Icon_Menu_functioncall, NULL, "Done (%d ticked)", count);
        else
            add_row(-1, Icon_Menu_functioncall, NULL, "Done");
        for (int i = 0; i < nvalues; i++)
            add_row(i, NOICON, NULL, "[%c] %s", ticked[i] ? 'x' : ' ', values[i]);
        int r = list_run(title, &sel);
        if (r == LIST_CANCEL)
        {
            arena_used = mark;
            return NULL;
        }
        if (r != LIST_OK)
            continue;
        if (rows[sel].id >= 0)
        {
            ticked[rows[sel].id] ^= 1;
            continue;
        }
        if (!count)
        {
            rb->splash(HZ, "Tick at least one value");
            continue;
        }
        size_t k = 0;
        bool full = false;
        result[0] = '\0';
        for (int i = 0; i < nvalues; i++)
        {
            if (!ticked[i])
                continue;
            size_t l = rb->strlen(values[i]);
            if (k + l + 2 > sizeof result)
            {
                full = true;
                break;
            }
            if (k)
                result[k++] = '|';
            rb->memcpy(result + k, values[i], l + 1);
            k += l;
        }
        if (full)
            rb->splash(HZ * 2, "Too many values; some were left out");
        arena_used = mark;
        return sdup(result);
    }
}

/* --- folder browser ------------------------------------------------------- */

static int name_cmp(const void *a, const void *b)
{
    return rb->strcasecmp(*(char *const *)a, *(char *const *)b);
}

/* Browses folders from the root (other drives appear as <microSD1> etc).
 * Returns the chosen folder as "/path/", or NULL. */
static char *pick_folder(const char *current)
{
    char path[MAX_PATH] = "/";
    char came_from[MAX_PATH] = "";   /* after going up, select the folder we left */
    int sel = 0;

    /* start in the folder already in the filter, if it still exists */
    if (current && current[0] == '/' && !is_runtime(current))
    {
        rb->strlcpy(path, current, sizeof path);
        size_t l = rb->strlen(path);
        if (l > 1 && path[l - 1] == '/')
            path[l - 1] = '\0';
        if (!rb->dir_exists(path))
            rb->strcpy(path, "/");
    }

    while (!usb)
    {
        size_t mark = arena_used;
        char **dirs = alloc(sizeof(char *) * MAX_VALUES);
        int n = 0;
        DIR *d = dirs ? rb->opendir(path) : NULL;
        if (d)
        {
            struct dirent *ent;
            while (n < MAX_VALUES && (ent = rb->readdir(d)))
            {
                struct dirinfo info = rb->dir_get_info(d, ent);
                if (!(info.attribute & ATTR_DIRECTORY) || (info.attribute & (ATTR_HIDDEN | ATTR_SYSTEM))
                    || ent->d_name[0] == '.' || rb->strchr(ent->d_name, '"'))
                    continue;
                if (!(dirs[n] = sdup(ent->d_name)))
                    break;
                n++;
            }
            rb->closedir(d);
        }
        if (n)
            rb->qsort(dirs, n, sizeof(char *), name_cmp);

        rows_reset();
        add_row(-1, Icon_Menu_functioncall, NULL, "Use this folder");
        for (int i = 0; i < n; i++)
        {
            add_row(i, Icon_Folder, NULL, "%s", dirs[i]);
            if (came_from[0] && !rb->strcmp(dirs[i], came_from))
                sel = i + 1;
        }
        came_from[0] = '\0';
        int r = list_run(path, &sel);

        char picked[MAX_PATH] = "";
        if (r == LIST_OK && rows[sel].id >= 0)
            rb->strlcpy(picked, dirs[rows[sel].id], sizeof picked);
        arena_used = mark;

        if (r == LIST_CANCEL)
        {
            if (!rb->strcmp(path, "/"))
                return NULL;
            char *slash = rb->strrchr(path, '/');   /* up one level */
            if (slash)
                rb->strlcpy(came_from, slash + 1, sizeof came_from);
            if (slash == path)
                slash[1] = '\0';
            else if (slash)
                *slash = '\0';
            sel = 0;
            continue;
        }
        if (r != LIST_OK)
            continue;
        if (picked[0])
        {
            size_t l = rb->strlen(path);
            if (l + rb->strlen(picked) + 2 < sizeof path)
            {
                if (path[l - 1] != '/')
                    rb->strlcat(path, "/", sizeof path);
                rb->strlcat(path, picked, sizeof path);
                sel = 0;
            }
            continue;
        }
        /* "Use this folder": stored with a trailing slash so /Music doesn't match /Music2 */
        if (!rb->strcmp(path, "/"))
        {
            rb->splash(HZ * 2, "Pick a folder below the top level");
            continue;
        }
        rb->strlcat(path, "/", sizeof path);
        if (rb->strlen(path) >= VALUE_LEN)
        {
            rb->splash(HZ * 2, "That folder's path is too long");
            continue;
        }
        return sdup(path);
    }
    return NULL;
}

/* Asks where a filter value comes from.  Returns a new string, or NULL.
 * `label` ("From"/"To") is set for the ends of a range. */
static char *pick_value(int fld, int op, const char *current, const char *label)
{
    bool multi = !IS_NUM(fld) && (op == OP_ONEOF || op == OP_NONEOF);
    const char *items[5];
    int kinds[5], n = 0;
    char playing[48];
    const char *rt = label ? NULL : runtime_for(fld);

    if (fld == FLD_FILENAME)
    {
        items[n] = "Browse folders";
        kinds[n++] = 4;
    }
    if (fields[fld].flags & F_PICK)
    {
        items[n] = "Pick from library";
        kinds[n++] = 0;
    }
    items[n] = fld == FLD_FILENAME ? "Type a path" : multi ? "Type values (a|b|c)" : "Type a value";
    kinds[n++] = 1;
    if (fld != FLD_FILENAME)   /* typing a path each time isn't practical */
    {
        items[n] = "Ask each time";
        kinds[n++] = 2;
    }
    if (rt)
    {
        if (fld == FLD_FILENAME)
            rb->strlcpy(playing, "Playing track's folder", sizeof playing);
        else
        {
            rb->snprintf(playing, sizeof playing, "Playing track's %s", fld_label(fld));
            char *c = playing + rb->strlen("Playing track's ");
            if (*c >= 'A' && *c <= 'Z')
                *c += 'a' - 'A';
        }
        items[n] = playing;
        kinds[n++] = 3;
    }

    char title[48];
    rb->snprintf(title, sizeof title, "%s %s", fld_label(fld),
                 label ? label : (op < OP_COUNT ? test_label(fld, op) : ""));
    int r = choose(title, items, n, 0);
    if (r < 0)
        return NULL;

    switch (kinds[r])
    {
        case 0:
            return multi ? pick_several(fld, current) : pick_library(fld);
        case 1:
        {
            char buf[VALUE_LEN];
            const char *init = (current && !is_runtime(current)) ? current : "";
            if (!input_text(init, buf, sizeof buf))
                return NULL;
            return sdup(buf);
        }
        case 2:
            return sdup("");
        case 4:
            return pick_folder(current);
        default:
            return sdup(rt);
    }
}

/* Two values for "is between".  Year offers whole decades. */
static bool pick_range(int fld, char **lo, char **hi)
{
    if (fld == FLD_YEAR)
    {
        size_t mark;
        int first = 0, last = -1;
        if (load_values(fld, &mark))
        {
            first = rb->atoi(values[0]) / 10 * 10;
            last = rb->atoi(values[nvalues - 1]) / 10 * 10;
        }
        arena_used = mark;
        int count = last >= first ? (last - first) / 10 + 1 : 0;
        if (count > 30)
        {
            first = last - 290;
            count = 30;
        }
        static char labels[31][24];
        const char *items[31];
        for (int i = 0; i < count; i++)
        {
            rb->snprintf(labels[i], sizeof labels[i], "The %ds", first + i * 10);
            items[i] = labels[i];
        }
        items[count] = "Choose from and to...";
        int r = choose("Year is between", items, count + 1, 0);
        if (r < 0)
            return false;
        if (r < count)
        {
            char a[12], b[12];
            rb->snprintf(a, sizeof a, "%d", first + r * 10);
            rb->snprintf(b, sizeof b, "%d", first + r * 10 + 9);
            *lo = sdup(a);
            *hi = sdup(b);
            return *lo && *hi;
        }
    }
    if (!(*lo = pick_value(fld, OP_GTEQ, NULL, "from")))
        return false;
    return (*hi = pick_value(fld, OP_LTEQ, NULL, "to")) != NULL;
}

/* ----------------------------------------------------------------------- */
/* Saving                                                                  */
/* ----------------------------------------------------------------------- */

static bool write_file(const char *path, const char *text, size_t len)
{
    int fd = rb->open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return false;
    bool ok = rb->write(fd, text, len) == (ssize_t)len;
    if (rb->close(fd) < 0)
        ok = false;
    return ok;
}

static int read_file(const char *path, char *buf, size_t cap)
{
    int fd = rb->open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    ssize_t n = rb->read(fd, buf, cap - 1);
    char extra;
    bool more = n == (ssize_t)(cap - 1) && rb->read(fd, &extra, 1) > 0;
    rb->close(fd);
    if (n < 0 || more)
        return -2;
    buf[n] = '\0';
    return n;
}

/* Problems that stop a save.  Returns how many were written. */
static int check_model(char msgs[][64], int max)
{
    int n = 0, menus = 0, formats = 0;
    bool include = has_include();
    struct tn_stmt *r = root_stmt();

    if (!r || !find_menu(r->id))
        rb->strlcpy(msgs[n++], "The top-level menu is missing", 64);
    for (struct tn_stmt *s = model.stmts; s && n < max; s = s->next)
    {
        if (is_menu(s))
            menus++;
        if (s->kind == K_FORMAT)
            formats++;
        if (s->kind != K_MENU)
            continue;
        if (count_visible(s) > TAGMENU_MAX_ITEMS && n < max)
            rb->snprintf(msgs[n++], 64, "%s has more than %d items", s->title, TAGMENU_MAX_ITEMS);
        for (struct tn_entry *e = s->entries; e && n < max; e = e->next)
        {
            if (e->kind == E_LINK && !find_menu(e->text) && !include)
                rb->snprintf(msgs[n++], 64, "%s opens a missing menu", e->name);
            for (struct tn_screen *sc = e->screens; sc && n < max; sc = sc->next)
                if (sc->fmt && !format_exists(sc->fmt))
                    rb->snprintf(msgs[n++], 64, "%s uses a missing format", e->name);
        }
    }
    if (menus > TAGMENU_MAX_MENUS && n < max)
        rb->snprintf(msgs[n++], 64, "More than %d menus", TAGMENU_MAX_MENUS);
    if (formats > TAGMENU_MAX_FMTS && n < max)
        rb->snprintf(msgs[n++], 64, "More than %d format lines", TAGMENU_MAX_FMTS);
    return n;
}

/* Writes the model to ser_buf and checks it reads back to the same text. */
static int serialise_checked(void)
{
    int len = write_model(&model, ser_buf, TEXT_MAX);
    if (len < 0)
        return -1;
    rb->memcpy(text_buf, ser_buf, len + 1);
    size_t mark = arena_used;
    struct tn_model again;
    const char *err = parse_text(text_buf, &again);
    int len2 = err ? -1 : write_model(&again, text_buf, TEXT_MAX);
    arena_used = mark;
    oom = false;
    if (len2 != len || rb->memcmp(text_buf, ser_buf, len))
        return -2;
    return len;
}

static bool save(void)
{
    char msgs[6][64];
    int n = check_model(msgs, 6);
    if (n)
    {
        int sel = 0;
        rows_reset();
        for (int i = 0; i < n; i++)
            add_row(0, NOICON, NULL, "%s", msgs[i])->flags = ROW_DISABLED;
        list_run("Can't save yet", &sel);
        return false;
    }
    int len = serialise_checked();
    if (len == -1)
    {
        rb->splash(HZ * 2, "The menus are too big to save");
        return false;
    }
    if (len < 0)
    {
        rb->splash(HZ * 2, "Save stopped: the file would not read back the same");
        return false;
    }
    if (!write_file(TMP_FILE, ser_buf, len))
    {
        rb->remove(TMP_FILE);
        rb->splash(HZ * 2, "Couldn't write the file");
        return false;
    }
    if (rb->file_exists(USER_FILE))
    {
        rb->remove(BAK_FILE);
        rb->rename(USER_FILE, BAK_FILE);
    }
    if (rb->rename(TMP_FILE, USER_FILE) < 0)
    {
        rb->splash(HZ * 2, "Couldn't replace tagnavi_user.config");
        return false;
    }
    dirty = false;
    return true;
}

static void save_and_report(void)
{
    if (!save())
        return;
    /* the Database reloads its menus the next time it's opened */
    rb->tagtree_request_reload();
    rb->splash(HZ, "Database Updated");
}

/* ----------------------------------------------------------------------- */
/* Loading                                                                 */
/* ----------------------------------------------------------------------- */

static bool load(const char *path)
{
    arena_used = arena_base;
    oom = false;
    int n = read_file(path, text_buf, TEXT_MAX);
    if (n == -2)
    {
        rb->splashf(HZ * 2, "%s is too big", path);
        return false;
    }
    if (n < 0)
    {
        rb->splashf(HZ * 2, LSTR(LANG_PLUGIN_CANT_OPEN), path);
        return false;
    }
    const char *err = parse_text(text_buf, &model);
    if (err)
    {
        rb->splashf(HZ * 3, "%s: %s", path, err);
        return false;
    }
    if (!top_menu())
    {
        rb->splashf(HZ * 3, "%s has no menus", path);
        return false;
    }
    dirty = false;
    return true;
}

/* ----------------------------------------------------------------------- */
/* Screens                                                                 */
/* ----------------------------------------------------------------------- */

static void edit_items(struct tn_stmt *m, bool is_root);
static void edit_az(struct tn_stmt *m);

/* item screen ids */
enum { R_NAME = 1, R_KIND, R_SPECIAL, R_FILTERS, R_SCREEN, R_ADD_SCREEN, R_DELETE,
       R_ENTRY, R_ADD_ITEM, R_SAVE, R_MORE, R_FILTER, R_ADD_FILTER, R_FORMAT,
       R_MOVE_FILTERS, R_FIELD, R_TEST, R_VALUE, R_REMOVE, R_FROM, R_TO,
       R_DECADE, R_ALL_MENUS, R_TOP, R_DISCARD, R_RESTORE, R_MENU, R_INDEX };

static void rename_item(struct tn_entry *e, const char *t)
{
    if (e->kind == E_LINK)
    {
        struct tn_stmt *target = find_menu(e->text);
        if (target && !rb->strcmp(target->title, e->name))
            target->title = sdup(t);   /* keep the submenu's title in step */
    }
    e->name = sdup(t);
    changed();
}

/* items that open a menu usually carry its title */
static void rename_menu(struct tn_stmt *m, const char *t)
{
    for (struct tn_stmt *s = model.stmts; s; s = s->next)
        if (s->kind == K_MENU)
            for (struct tn_entry *e = s->entries; e; e = e->next)
                if (e->kind == E_LINK && !rb->strcasecmp(e->text, m->id)
                    && !rb->strcmp(e->name, m->title))
                    e->name = sdup(t);
    m->title = sdup(t);
    changed();
}

static struct tn_stmt *pick_menu(const char *title)
{
    const char *names[TAGMENU_MAX_MENUS * 2];
    struct tn_stmt *ptrs[TAGMENU_MAX_MENUS * 2];
    int n = 0;
    for (struct tn_stmt *s = model.stmts; s && n < (int)ARRAYLEN(names); s = s->next)
        if (is_menu(s))
        {
            ptrs[n] = s;
            names[n++] = s->title;
        }
    int r = choose(title, names, n, 0);
    return r < 0 ? NULL : ptrs[r];
}

static void delete_menu(struct tn_stmt *m)
{
    if (m == top_menu())
    {
        rb->splash(HZ * 2, "This is the top-level menu. Pick another one first.");
        return;
    }
    int users = 0;
    for (struct tn_stmt *s = model.stmts; s; s = s->next)
        if (s->kind == K_MENU)
            for (struct tn_entry *e = s->entries; e; e = e->next)
                if (e->kind == E_LINK && !rb->strcasecmp(e->text, m->id))
                    users++;
    char q[96];
    if (users)
        rb->snprintf(q, sizeof q, "Delete %s and the %d item(s) that open it?", m->title, users);
    else
        rb->strlcpy(q, LSTR(LANG_REALLY_DELETE), sizeof q);
    if (!confirm(q))
        return;
    for (struct tn_stmt *s = model.stmts; s; s = s->next)
        if (s->kind == K_MENU)
            for (struct tn_entry **pp = &s->entries; *pp; )
            {
                if ((*pp)->kind == E_LINK && !rb->strcasecmp((*pp)->text, m->id))
                    *pp = (*pp)->next;
                else
                    pp = &(*pp)->next;
            }
    for (struct tn_stmt **pp = &model.stmts; *pp; pp = &(*pp)->next)
        if (*pp == m)
        {
            *pp = m->next;
            break;
        }
    changed();
}

static void new_submenu(struct tn_stmt *parent)
{
    static const char *const kinds[] = { "List of items", "A-Z index" };
    int k = choose("New submenu", kinds, 2, 0);
    if (k < 0)
        return;
    int fld = FLD_CARTIST;
    char name[NAME_LEN], init[NAME_LEN] = "";
    if (k == 1)
    {
        static const unsigned char az_fields[] = { FLD_CARTIST, FLD_AARTIST, FLD_ALBUM, FLD_TITLE, FLD_COMPOSER, FLD_GENRE };
        fld = pick_field("Index by", az_fields, ARRAYLEN(az_fields), NULL, -1, true);
        if (fld < 0)
            return;
        rb->snprintf(init, sizeof init, "%s by First Letter",
                     screen_name(fld));
    }
    if (!input_text(init, name, sizeof name))
        return;
    struct tn_stmt *m = alloc(sizeof *m);
    struct tn_entry *e = alloc(sizeof *e);
    char id[ID_LEN];
    make_id(name, id);
    if (!m || !e || !(m->id = sdup(id)) || !(m->title = sdup(name))
        || !(e->name = sdup(name)) || !(e->text = sdup(id)))
    {
        rb->splash(HZ, "Out of memory");
        return;
    }
    m->kind = k ? K_AZ : K_MENU;
    m->fld = fld;
    insert_menu(m);
    e->kind = E_LINK;
    append_entry(parent, e);
    changed();
    if (k)
        edit_az(m);
    else
        edit_items(m, false);
}

/* --- one filter ---------------------------------------------------------- */

static void edit_range(struct tn_clause **list, struct tn_clause *a)
{
    struct tn_clause *b = a->next;
    int sel = 0;
    char title[96];
    while (!usb)
    {
        char lo[24], hi[24];
        bound_text(a, lo, sizeof lo);
        bound_text(b, hi, sizeof hi);
        clause_text(a, title, sizeof title);
        rows_reset();
        add_row(R_FROM, Icon_Menu_setting, NULL, "From: %s", lo);
        add_row(R_TO, Icon_Menu_setting, NULL, "To: %s", hi);
        if (a->fld == FLD_YEAR)
            add_row(R_DECADE, Icon_Menu_functioncall, NULL, "Pick a decade...");
        add_row(R_REMOVE, Icon_Menu_functioncall, NULL, "%s", LSTR(LANG_REMOVE));
        if (list_run(title, &sel) != LIST_OK)
            return;
        char *v, *w;
        switch (rows[sel].id)
        {
            case R_FROM:
                if ((v = pick_value(a->fld, OP_GTEQ, a->value, "from")))
                {
                    a->op = OP_GTEQ; a->value = v; changed();
                }
                break;
            case R_TO:
                if ((v = pick_value(b->fld, OP_LTEQ, b->value, "to")))
                {
                    b->op = OP_LTEQ; b->value = v; changed();
                }
                break;
            case R_DECADE:
                if (pick_range(a->fld, &v, &w))
                {
                    a->op = OP_GTEQ; a->value = v;
                    b->op = OP_LTEQ; b->value = w;
                    changed();
                }
                break;
            case R_REMOVE:
                unlink_clause(list, a, 2);
                changed();
                return;
        }
    }
}

static void edit_clause(struct tn_clause **list, struct tn_clause *c)
{
    int sel = 0;
    char title[96], v[96];
    while (!usb)
    {
        clause_text(c, title, sizeof title);
        value_text(c, v, sizeof v);
        rows_reset();
        add_row(R_FIELD, Icon_Menu_setting, NULL, "Field: %s", fld_label(c->fld));
        add_row(R_TEST, Icon_Menu_setting, NULL, "Test: %s", test_label(c->fld, c->op));
        add_row(R_VALUE, Icon_Menu_setting, NULL, "Value: %s", v);
        add_row(R_REMOVE, Icon_Menu_functioncall, NULL, "%s", LSTR(LANG_REMOVE));
        if (list_run(title, &sel) != LIST_OK)
            return;
        switch (rows[sel].id)
        {
            case R_FIELD:
            {
                int f = pick_field("Field", filter_fields, ARRAYLEN(filter_fields), NULL, c->fld, false);
                if (f >= 0 && f != c->fld)
                {
                    bool op_ok = false;
                    int nops;
                    const unsigned char *ops = ops_for(f, &nops);
                    for (int i = 0; i < nops; i++)
                        if (ops[i] == c->op)
                            op_ok = true;
                    c->fld = f;
                    if (!op_ok)
                        c->op = ops[0];
                    changed();
                }
                break;
            }
            case R_TEST:
            {
                const char *names[10];
                int nops;
                const unsigned char *ops = ops_for(c->fld, &nops);
                int k = 0, initial = 0;
                unsigned char ids[10];
                for (int i = 0; i < nops; i++)
                {
                    if (ops[i] == OP_BETWEEN)
                        continue;   /* a range is made with "Add filter" */
                    if (ops[i] == c->op)
                        initial = k;
                    ids[k] = ops[i];
                    names[k++] = test_label(c->fld, ops[i]);
                }
                int r = choose(fld_label(c->fld), names, k, initial);
                if (r >= 0)
                {
                    c->op = ids[r];
                    changed();
                }
                break;
            }
            case R_VALUE:
            {
                char *nv = pick_value(c->fld, c->op, c->value, NULL);
                if (nv)
                {
                    c->value = nv;
                    changed();
                }
                break;
            }
            case R_REMOVE:
                unlink_clause(list, c, 1);
                changed();
                return;
        }
    }
}

/* field, test, value; inserted after `after` (NULL: at the end) */
static bool add_filter(struct tn_clause **list, struct tn_clause *after)
{
    int fld = pick_field("Filter on", filter_fields, ARRAYLEN(filter_fields), NULL, -1, false);
    if (fld < 0)
        return false;

    int nops;
    const unsigned char *ops = ops_for(fld, &nops);
    const char *names[10];
    for (int i = 0; i < nops; i++)
        names[i] = test_label(fld, ops[i]);
    char title[48];
    rb->snprintf(title, sizeof title, "%s...", fld_label(fld));
    int r = choose(title, names, nops, 0);
    if (r < 0)
        return false;
    int op = ops[r];

    struct tn_clause *first;
    if (op == OP_BETWEEN)
    {
        char *lo, *hi;
        if (!pick_range(fld, &lo, &hi))
            return false;
        first = new_clause(fld, OP_GTEQ, NULL);
        struct tn_clause *second = new_clause(fld, OP_LTEQ, NULL);
        if (!first || !second)
            return false;
        first->value = lo;
        second->value = hi;
        first->next = second;
    }
    else
    {
        char *v = pick_value(fld, op, NULL, NULL);
        if (!v || !(first = new_clause(fld, op, NULL)))
            return false;
        first->value = v;
    }
    insert_clauses(list, after, first);
    changed();
    return true;
}

/* --- an item's filters --------------------------------------------------- */

static void edit_filters(struct tn_entry *e)
{
    int sel = 0;
    char msgs[4][64];
    while (!usb)
    {
        struct tn_clause **list = &e->screens->clauses;
        int nconf = find_conflicts(*list, msgs, 4);
        rows_reset();
        for (int i = 0; i < nconf; i++)
            add_row(0, Icon_Questionmark, NULL, "Never matches: %s", msgs[i])->flags = ROW_DISABLED;
        for (struct tn_clause *c = *list; c; c = c->next)
        {
            char t[128];
            clause_text(c, t, sizeof t);
            add_row(R_FILTER, c->op == OP_OR ? NOICON : Icon_Menu_setting, c, "%s", t);
            if (is_range(c))
                c = c->next;
        }
        add_row(R_ADD_FILTER, Icon_Menu_functioncall, NULL, "Add filter...");

        int r = list_run("Filters", &sel);
        if (r == LIST_CANCEL)
            return;
        struct tn_row rw = rows[sel];
        struct tn_clause *c = rw.ptr;
        if (r == LIST_CONTEXT)
        {
            if (rw.id != R_FILTER)
                continue;
            const char *const acts[] = { LSTR(LANG_REMOVE), "Add \"or\" after this" };
            int a = choose("Filter", acts, c->op == OP_OR ? 1 : 2, 0);
            if (a == 0)
            {
                unlink_clause(list, c, (c->op != OP_OR && is_range(c)) ? 2 : 1);
                changed();
            }
            else if (a == 1)
            {
                struct tn_clause *last = is_range(c) ? c->next : c;
                struct tn_clause *or = new_clause(0, OP_OR, NULL);
                if (!or)
                    continue;
                insert_clauses(list, last, or);
                if (!add_filter(list, or))
                    unlink_clause(list, or, 1);   /* backed out: drop the "or" */
                changed();
            }
            continue;
        }
        if (rw.id == R_ADD_FILTER)
            add_filter(list, NULL);
        else if (rw.id == R_FILTER)
        {
            if (c->op == OP_OR)
            {
                if (confirm("Remove this \"or\"?"))
                {
                    unlink_clause(list, c, 1);
                    changed();
                }
            }
            else if (is_range(c))
                edit_range(list, c);
            else
                edit_clause(list, c);
        }
    }
}

/* --- a screen (only Tracks, or screens with hand-written filters) -------- */

static void pick_screen_field(struct tn_entry *e, struct tn_screen *sc)
{
    int f = pick_field("Show", screen_fields, ARRAYLEN(screen_fields), e, sc->fld, true);
    if (f < 0 || f == sc->fld)
        return;
    sc->fld = f;
    if (f == FLD_TITLE && !sc->fmt && format_exists("fmt_title"))
        sc->fmt = sdup("fmt_title");
    if (!IS_TRACKS(f))
        sc->fmt = NULL;
    changed();
}

static void pick_format(struct tn_screen *sc)
{
    const char *names[TAGMENU_MAX_FMTS + 1];
    const char *ids[TAGMENU_MAX_FMTS + 1];
    int n = 0, initial = 0;
    names[n] = format_label(NULL);
    ids[n++] = NULL;
    for (struct tn_stmt *s = model.stmts; s && n < (int)ARRAYLEN(names); s = s->next)
    {
        if (s->kind != K_FORMAT)
            continue;
        bool seen = false;
        for (int i = 1; i < n; i++)
            if (!rb->strcasecmp(ids[i], s->id))
                seen = true;
        if (seen)
            continue;
        if (sc->fmt && !rb->strcasecmp(sc->fmt, s->id))
            initial = n;
        ids[n] = s->id;
        names[n++] = format_label(s->id);
    }
    int r = choose("Show tracks as", names, n, initial);
    if (r < 0)
        return;
    sc->fmt = ids[r] ? sdup(ids[r]) : NULL;
    changed();
}

static void edit_screen(struct tn_entry *e, int i)
{
    int sel = 0;
    char title[64];
    while (!usb)
    {
        struct tn_screen *sc = nth_screen(e, i);
        if (!sc)
            return;
        int extra = i > 0 ? count_filters(sc->clauses) : 0;
        rb->snprintf(title, sizeof title, "Screen %d: %s", i + 1, screen_label(sc->fld));
        rows_reset();
        add_row(R_FIELD, Icon_Menu_setting, NULL, "Show: %s", screen_label(sc->fld));
        if (IS_TRACKS(sc->fld))
            add_row(R_FORMAT, Icon_Menu_setting, NULL, "Show tracks as: %s", format_label(sc->fmt));
        if (extra)
        {
            add_row(0, NOICON, NULL, "%d extra filter(s) from a hand-written file", extra)->flags = ROW_DISABLED;
            add_row(R_MOVE_FILTERS, Icon_Menu_functioncall, NULL, "Move to the item's filters");
        }
        if (list_run(title, &sel) != LIST_OK)
            return;
        switch (rows[sel].id)
        {
            case R_FIELD:
                pick_screen_field(e, sc);
                break;
            case R_FORMAT:
                pick_format(sc);
                break;
            case R_MOVE_FILTERS:
                if (sc->clauses)
                {
                    insert_clauses(&e->screens->clauses, NULL, sc->clauses);
                    sc->clauses = NULL;
                    changed();
                    rb->splash(HZ, "Moved to the item's filters");
                }
                break;
        }
    }
}

/* --- an item --------------------------------------------------------------- */

static void edit_entry(struct tn_stmt *m, struct tn_entry *e)
{
    int sel = 0;
    char msgs[1][64];
    while (!usb)
    {
        int n = count_screens(e);
        struct tn_screen *last = nth_screen(e, n - 1);
        int nf = count_filters(e->screens->clauses);
        bool never = find_conflicts(e->screens->clauses, msgs, 1) > 0;

        rows_reset();
        add_row(R_NAME, Icon_Menu_setting, NULL, "%s: %s", LSTR(LANG_NAME), e->name);
        add_row(R_KIND, Icon_Menu_setting, NULL, "Opens as: %s", e->kind == E_SHUFFLE ? LSTR(LANG_SHUFFLE) : "List");
        /* the [All tracks] and [Random] rows only appear on lists that aren't tracks */
        bool lists = false;
        for (struct tn_screen *sc = e->screens; sc; sc = sc->next)
            if (!IS_TRACKS(sc->fld))
                lists = true;
        if (e->kind == E_BROWSE && lists)
            add_row(R_SPECIAL, Icon_Menu_setting, NULL, "%s & %s: %s",
                    LSTR(LANG_TAGNAVI_ALL_TRACKS), LSTR(LANG_TAGNAVI_RANDOM),
                    LSTR(e->hide_special ? LANG_OFF : LANG_ON));
        if (never)
            add_row(R_FILTERS, Icon_Menu_setting, NULL, "Filters: never match!");
        else if (nf)
            add_row(R_FILTERS, Icon_Menu_setting, NULL, "Filters: %d", nf);
        else
            add_row(R_FILTERS, Icon_Menu_setting, NULL, "Filters: none");
        /* heading between the item's settings and its screens; shorter if it won't fit */
        const char *head = e->kind == E_SHUFFLE ? "-- Shuffles songs from --" : "-- When opened, shows --";
        int hw, hh;
        rb->lcd_getstringsize((const unsigned char *)head, &hw, &hh);
        if (hw > LCD_WIDTH - 12)
            head = e->kind == E_SHUFFLE ? "-- Songs from --" : "-- Screens --";
        add_row(0, NOICON, NULL, "%s", head)->flags = ROW_HEADER;
        /* indent per screen; narrow screens drop the "> " marker so each
           level is just one space deeper */
        const bool narrow = LCD_WIDTH < 160;
        const int step = narrow ? 1 : 2;
        /* "Add screen" sits where the new screen will go: before the song
           list if the item ends in one, else at the end. It is indented one
           level below the screen above it, which is the one it opens from. */
        const int add_at = n < MAX_SCREENS ? (IS_TRACKS(last->fld) ? n - 1 : n) : -1;
        int i = 0;
        for (struct tn_screen *sc = e->screens; sc; sc = sc->next, i++)
        {
            if (i == add_at)
                add_row(R_ADD_SCREEN, Icon_Menu_functioncall, NULL, "%*sAdd screen...", i * step, "");
            char extra[64] = "";
            if (IS_TRACKS(sc->fld))
                rb->snprintf(extra, sizeof extra, " (%s)", format_label(sc->fmt));
            if (i > 0 && sc->clauses)
                rb->strlcat(extra, " +filter", sizeof extra);
            add_row(R_SCREEN, i ? NOICON : Icon_Audio, NULL, "%*s%s%s%s", i * step, "",
                    (i && !narrow) ? "> " : "", screen_label(sc->fld), extra)->aux = i;
        }
        if (add_at == n)
            add_row(R_ADD_SCREEN, Icon_Menu_functioncall, NULL, "%*sAdd screen...", n * step, "");
        else if (add_at < 0)
            add_row(0, NOICON, NULL, "(%d screens is the limit)", MAX_SCREENS)->flags = ROW_DISABLED;
        add_row(R_DELETE, Icon_Menu_functioncall, NULL, "%s", LSTR(LANG_DELETE));

        int r = list_run(e->name, &sel);
        if (r == LIST_CANCEL)
            return;
        struct tn_row rw = rows[sel];
        if (r == LIST_CONTEXT)
        {
            if (rw.id != R_SCREEN)
                continue;
            const char *acts[3];
            int ids[3], k = 0;
            if (rw.aux > 0) { acts[k] = LSTR(LANG_MOVE_ITEM_UP); ids[k++] = 0; }
            if (rw.aux < n - 1) { acts[k] = LSTR(LANG_MOVE_ITEM_DOWN); ids[k++] = 1; }
            if (n > 1) { acts[k] = LSTR(LANG_REMOVE); ids[k++] = 2; }
            if (!k)
                continue;
            char t[32];
            rb->snprintf(t, sizeof t, "Screen %d", rw.aux + 1);
            int a = choose(t, acts, k, 0);
            if (a < 0)
                continue;
            if (ids[a] == 0)
            {
                swap_screens(e, rw.aux - 1);
                sel--;
            }
            else if (ids[a] == 1)
            {
                swap_screens(e, rw.aux);
                sel++;
            }
            else
                remove_screen(e, rw.aux);
            changed();
            continue;
        }

        switch (rw.id)
        {
            case R_NAME:
            {
                char buf[NAME_LEN];
                if (input_text(e->name, buf, sizeof buf))
                    rename_item(e, buf);
                break;
            }
            case R_KIND:
                e->kind = e->kind == E_SHUFFLE ? E_BROWSE : E_SHUFFLE;
                changed();
                break;
            case R_SPECIAL:
                e->hide_special = !e->hide_special;
                changed();
                break;
            case R_FILTERS:
                edit_filters(e);
                break;
            case R_SCREEN:
            {
                struct tn_screen *sc = nth_screen(e, rw.aux);
                if (IS_TRACKS(sc->fld) || (rw.aux > 0 && sc->clauses))
                    edit_screen(e, rw.aux);
                else
                    pick_screen_field(e, sc);
                break;
            }
            case R_ADD_SCREEN:
            {
                int f = pick_field("Add a screen", screen_fields, ARRAYLEN(screen_fields), e, -1, true);
                if (f < 0)
                    break;
                struct tn_screen *sc = alloc(sizeof *sc);
                if (!sc)
                    break;
                sc->fld = f;
                if (f == FLD_TITLE && format_exists("fmt_title"))
                    sc->fmt = sdup("fmt_title");
                int at = (IS_TRACKS(last->fld) && !IS_TRACKS(f)) ? n - 1 : n;
                insert_screen(e, at, sc);
                changed();
                break;
            }
            case R_DELETE:
            {
                char q[96];
                rb->strlcpy(q, LSTR(LANG_REALLY_DELETE), sizeof q);
                if (confirm(q))
                {
                    unlink_entry(m, e);
                    changed();
                    return;
                }
                break;
            }
        }
    }
}

/* --- a menu's items ---------------------------------------------------------- */

static struct tn_entry *new_entry(int kind, const char *name)
{
    struct tn_entry *e = alloc(sizeof *e);
    if (!e || !(e->name = sdup(name)))
        return NULL;
    e->kind = kind;
    return e;
}

static void add_item(struct tn_stmt *m)
{
    const char *const kinds[] = {
        "Browse by fields", LSTR(LANG_SHUFFLE_SONGS), "New submenu",
        "Link an existing menu", "Reload menus item", LSTR(LANG_TAGNAVI_EDIT_MENUS),
    };
    if (count_visible(m) >= TAGMENU_MAX_ITEMS)
    {
        rb->splashf(HZ * 2, "A menu can hold %d items", TAGMENU_MAX_ITEMS);
        return;
    }
    int k = choose("Add item", kinds, ARRAYLEN(kinds), 0);
    struct tn_entry *e = NULL;
    switch (k)
    {
        case 0:
        case 1:
        {
            int f = FLD_TITLE;
            if (k == 0 && (f = pick_field("First screen", screen_fields, ARRAYLEN(screen_fields), NULL, -1, true)) < 0)
                return;
            e = new_entry(k ? E_SHUFFLE : E_BROWSE, k ? "Shuffle" : screen_name(f));
            struct tn_screen *a = alloc(sizeof *a);
            if (!e || !a)
                break;
            a->fld = f;
            e->screens = a;
            if (!IS_TRACKS(f))
            {
                struct tn_screen *b = alloc(sizeof *b);
                if (!b)
                    break;
                b->fld = FLD_TITLE;
                a->next = b;
                a = b;
            }
            if (a->fld == FLD_TITLE && format_exists("fmt_title"))
                a->fmt = sdup("fmt_title");
            append_entry(m, e);
            changed();
            edit_entry(m, e);
            return;
        }
        case 2:
            new_submenu(m);
            return;
        case 3:
        {
            struct tn_stmt *t = pick_menu("Opens");
            if (t && (e = new_entry(E_LINK, t->title)) && (e->text = sdup(t->id)))
            {
                append_entry(m, e);
                changed();
            }
            return;
        }
        case 4:
        case 5:
            if ((e = k == 4 ? new_entry(E_RELOAD, "Reload menus") : new_entry(E_EDIT, "Edit Menus")))
            {
                append_entry(m, e);
                changed();
            }
            return;
        default:
            return;
    }
    rb->splash(HZ, "Out of memory");
}

static void open_item(struct tn_stmt *m, struct tn_entry *e)
{
    switch (e->kind)
    {
        case E_BROWSE:
        case E_SHUFFLE:
            edit_entry(m, e);
            break;
        case E_LINK:
        {
            struct tn_stmt *t = find_menu(e->text);
            if (!t)
                rb->splashf(HZ * 2, "%s opens a menu from another file", e->name);
            else if (t->kind == K_AZ)
                edit_az(t);
            else
                edit_items(t, false);
            break;
        }
        case E_RELOAD:
            rb->splash(HZ * 2, "Reloads the menu files when opened in the Database");
            break;
        case E_EDIT:
            rb->splash(HZ * 2, "Opens this editor from the Database");
            break;
        case E_RAW:
        {
            /* show the line, split to fit, with a way to delete it */
            int sel = 0;
            while (!usb)
            {
                rows_reset();
                add_row(0, NOICON, NULL, "This line isn't understood:")->flags = ROW_DISABLED;
                const char *p = e->text;
                while (*p)
                {
                    int len = rb->strlen(p) > 28 ? 28 : (int)rb->strlen(p);
                    add_row(0, NOICON, NULL, "%.*s", len, p)->flags = ROW_DISABLED;
                    p += len;
                }
                add_row(R_DELETE, Icon_Menu_functioncall, NULL, "%s", LSTR(LANG_DELETE));
                if (list_run(e->name, &sel) != LIST_OK)
                    return;
                if (rows[sel].id == R_DELETE && confirm(LSTR(LANG_REALLY_DELETE)))
                {
                    unlink_entry(m, e);
                    changed();
                    return;
                }
            }
            break;
        }
    }
}

static void item_options(struct tn_stmt *m, struct tn_entry *e, int *sel)
{
    const char *acts[6];
    int ids[6], k = 0;
    acts[k] = LSTR(LANG_MOVE_ITEM_UP); ids[k++] = 0;
    acts[k] = LSTR(LANG_MOVE_ITEM_DOWN); ids[k++] = 1;
    acts[k] = LSTR(LANG_RENAME); ids[k++] = 2;
    if (e->kind == E_LINK) { acts[k] = "Open a different menu"; ids[k++] = 3; }
    if (e->kind == E_BROWSE || e->kind == E_SHUFFLE) { acts[k] = LSTR(LANG_COPY); ids[k++] = 4; }
    acts[k] = LSTR(LANG_DELETE); ids[k++] = 5;

    int a = choose(e->name, acts, k, 0);
    if (a < 0)
        return;
    switch (ids[a])
    {
        case 0:
            if (move_entry(m, e, -1)) { (*sel)--; changed(); }
            break;
        case 1:
            if (move_entry(m, e, 1)) { (*sel)++; changed(); }
            break;
        case 2:
        {
            char buf[NAME_LEN];
            if (input_text(e->name, buf, sizeof buf))
                rename_item(e, buf);
            break;
        }
        case 3:
        {
            struct tn_stmt *t = pick_menu("Opens");
            if (t)
            {
                e->text = sdup(t->id);
                changed();
            }
            break;
        }
        case 4:
        {
            /* deep copy: screens and clauses */
            struct tn_entry *c = alloc(sizeof *c);
            char name[NAME_LEN];
            if (!c)
                break;
            *c = *e;
            rb->snprintf(name, sizeof name, "%.56s copy", e->name);
            c->name = sdup(name);
            struct tn_screen **st = &c->screens;
            for (struct tn_screen *s = e->screens; s; s = s->next)
            {
                struct tn_screen *ns = alloc(sizeof *ns);
                if (!ns)
                    break;
                *ns = *s;
                ns->next = NULL;
                struct tn_clause **ct = &ns->clauses;
                for (struct tn_clause *cl = s->clauses; cl; cl = cl->next)
                {
                    struct tn_clause *nc = alloc(sizeof *nc);
                    if (!nc)
                        break;
                    *nc = *cl;
                    nc->next = NULL;
                    *ct = nc;
                    ct = &nc->next;
                }
                *st = ns;
                st = &ns->next;
            }
            c->next = e->next;
            e->next = c;
            changed();
            break;
        }
        case 5:
        {
            char q[96];
            rb->strlcpy(q, LSTR(LANG_REALLY_DELETE), sizeof q);
            if (confirm(q))
            {
                unlink_entry(m, e);
                changed();
            }
            break;
        }
    }
}

static void edit_az(struct tn_stmt *m)
{
    static const unsigned char az_fields[] = { FLD_CARTIST, FLD_AARTIST, FLD_ALBUM, FLD_TITLE, FLD_COMPOSER, FLD_GENRE };
    int sel = 0;
    char title[NAME_LEN + 8];
    while (!usb)
    {
        rb->snprintf(title, sizeof title, "%s: %s", LSTR(LANG_EDIT), m->title);
        rows_reset();
        add_row(R_NAME, Icon_Menu_setting, NULL, "%s: %s", LSTR(LANG_NAME), m->title);
        add_row(R_INDEX, Icon_Menu_setting, NULL, "Index by: %s", screen_label(m->fld));
        if (list_run(title, &sel) != LIST_OK)
            return;
        if (rows[sel].id == R_NAME)
        {
            char buf[NAME_LEN];
            if (input_text(m->title, buf, sizeof buf))
                rename_menu(m, buf);
        }
        else
        {
            int f = pick_field("Index by", az_fields, ARRAYLEN(az_fields), NULL, m->fld, true);
            if (f >= 0 && f != m->fld)
            {
                m->fld = f;
                changed();
            }
        }
    }
}

static void edit_all_menus(void)
{
    int sel = 0;
    while (!usb)
    {
        rows_reset();
        for (struct tn_stmt *s = model.stmts; s; s = s->next)
            if (is_menu(s))
                add_row(R_MENU, Icon_Submenu, s, "%s%s", s->title,
                        s->kind == K_AZ ? " (A-Z)" : s == top_menu() ? " (top level)" : "");
        int r = list_run("All menus", &sel);
        if (r == LIST_CANCEL)
            return;
        struct tn_stmt *m = rows[sel].ptr;
        if (r == LIST_OK)
        {
            if (m->kind == K_AZ)
                edit_az(m);
            else
                edit_items(m, false);
            continue;
        }
        const char *const acts[] = { LSTR(LANG_RENAME), LSTR(LANG_DELETE) };
        int a = choose(m->title, acts, 2, 0);
        if (a == 0)
        {
            char buf[NAME_LEN];
            if (input_text(m->title, buf, sizeof buf))
                rename_menu(m, buf);
        }
        else if (a == 1)
            delete_menu(m);
    }
}

/* Returns true if the model was reloaded (callers must drop their pointers). */
static bool more_options(void)
{
    int sel = 0;
    while (!usb)
    {
        rows_reset();
        add_row(R_ALL_MENUS, Icon_Submenu, NULL, "All menus");
        add_row(R_TOP, Icon_Menu_setting, NULL, "Top-level menu: %s", top_menu()->title);
        add_row(R_DISCARD, Icon_Menu_functioncall, NULL, "Discard changes");
        add_row(R_RESTORE, Icon_Menu_functioncall, NULL, "Restore stock menus");
        if (list_run("More options", &sel) != LIST_OK)
            return false;
        switch (rows[sel].id)
        {
            case R_ALL_MENUS:
                edit_all_menus();
                break;
            case R_TOP:
            {
                struct tn_stmt *t = pick_menu("Top-level menu");
                if (!t)
                    break;
                if (t->kind == K_AZ)
                {
                    rb->splash(HZ * 2, "Pick a list menu, not an A-Z index");
                    break;
                }
                struct tn_stmt *r = root_stmt();
                if (!r)
                {
                    if (!(r = alloc(sizeof *r)))
                        break;
                    r->kind = K_ROOT;
                    struct tn_stmt **pp = &model.stmts;
                    while (*pp)
                        pp = &(*pp)->next;
                    *pp = r;
                }
                r->id = sdup(t->id);
                changed();
                break;
            }
            case R_DISCARD:
                if (!dirty)
                    rb->splash(HZ, "No changes to discard");
                else if (confirm("Discard all unsaved changes?"))
                {
                    const char *path = rb->file_exists(USER_FILE) ? USER_FILE : STOCK_FILE;
                    if (load(path))
                        rb->splash(HZ, "Changes discarded");
                    return true;
                }
                break;
            case R_RESTORE:
                if (confirm("Go back to the stock Database menus? Your file is kept as tagnavi_user.bak"))
                {
                    if (rb->file_exists(USER_FILE))
                    {
                        rb->remove(BAK_FILE);
                        rb->rename(USER_FILE, BAK_FILE);
                    }
                    rb->tagtree_request_reload();
                    if (load(STOCK_FILE))
                        rb->splash(HZ * 2, "Stock menus restored");
                    return true;
                }
                break;
        }
    }
    return false;
}

static void edit_items(struct tn_stmt *m, bool is_root)
{
    int sel = 0;
    char title[NAME_LEN + 8];
    while (!usb)
    {
        if (is_root)
            m = top_menu();   /* may change from More options */
        rb->snprintf(title, sizeof title, "%s: %s", LSTR(LANG_EDIT), m->title);
        rows_reset();
        for (struct tn_entry *e = m->entries; e; e = e->next)
        {
            switch (e->kind)
            {
                case E_COMMENT:
                    break;
                case E_LINK:
                    add_row(R_ENTRY, Icon_Submenu, e, "%s%s", e->name, find_menu(e->text) ? "" : " (other file)");
                    break;
                case E_RAW:
                    add_row(R_ENTRY, Icon_Questionmark, e, "%s (not understood)", e->name);
                    break;
                case E_RELOAD:
                case E_EDIT:
                    add_row(R_ENTRY, Icon_Menu_functioncall, e, "%s", e->name);
                    break;
                default:
                    add_row(R_ENTRY, Icon_Audio, e, "%s", e->name);
                    break;
            }
        }
        add_row(R_ADD_ITEM, Icon_Menu_functioncall, NULL, "Add item...");
        if (is_root)
        {
            add_row(R_SAVE, Icon_Menu_functioncall, NULL, dirty ? "Save changes" : "Saved");
            add_row(R_MORE, Icon_Menu_functioncall, NULL, "More options...");
        }

        int r = list_run(title, &sel);
        if (r == LIST_CANCEL)
            return;
        struct tn_row rw = rows[sel];
        if (r == LIST_CONTEXT)
        {
            if (rw.id == R_ENTRY)
                item_options(m, rw.ptr, &sel);
            continue;
        }
        switch (rw.id)
        {
            case R_ENTRY:
                open_item(m, rw.ptr);
                break;
            case R_ADD_ITEM:
                add_item(m);
                break;
            case R_SAVE:
                if (dirty)
                    save_and_report();
                else
                    rb->splash(HZ, "No changes to save");
                break;
            case R_MORE:
                if (more_options())
                    sel = 0;
                break;
        }
    }
}

/* ----------------------------------------------------------------------- */
/* Entry point                                                             */
/* ----------------------------------------------------------------------- */

static enum plugin_status run(void)
{
    size_t size;
    char *buf = rb->plugin_get_buffer(&size);
    if (size < 2 * TEXT_MAX + 64 * 1024)
    {
        rb->splash(HZ * 2, "Not enough memory");
        return PLUGIN_ERROR;
    }
    text_buf = buf;
    ser_buf = buf + TEXT_MAX;
    arena = buf + 2 * TEXT_MAX;
    arena_size = size - 2 * TEXT_MAX;
    arena_used = arena_base = 0;

    bool user = rb->file_exists(USER_FILE);
    if (!load(user ? USER_FILE : STOCK_FILE))
    {
        if (!user || !confirm("Your tagnavi_user.config can't be read. Start from the stock menus?")
            || !load(STOCK_FILE))
            return PLUGIN_ERROR;
        dirty = true;
    }

    edit_items(top_menu(), true);

    if (usb)
        return PLUGIN_USB_CONNECTED;
    if (dirty && confirm(LSTR(LANG_SAVE_CHANGES)))
        save_and_report();
    return PLUGIN_OK;
}

enum plugin_status plugin_start(const void *parameter)
{
    (void)parameter;
    /* keep the status bar and the theme's list layout, like the settings screens */
    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_enable(i, true, NULL);
    enum plugin_status status = run();
    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_undo(i, false);
    return status;
}

#endif /* !TNE_HOSTTEST */
