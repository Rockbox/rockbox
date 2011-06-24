/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008-2009 Teruaki Kawashima
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
#include "lib/playback_control.h"
#include "lib/configfile.h"
#include "lib/helper.h"
#include <ctype.h>



#define MAX_LINE_LEN    256
#define LRC_BUFFER_SIZE 0x3000 /* 12 kiB */
#if PLUGIN_BUFFER_SIZE >= 0x10000 /* no id3 support for low mem targets */
/* define this to read lyrics in id3 tag */
#define LRC_SUPPORT_ID3
#endif
/* define this to show debug info in menu */
/* #define LRC_DEBUG */

enum lrc_screen {
    PLUGIN_OTHER = 0x200,
    LRC_GOTO_MAIN,
    LRC_GOTO_MENU,
    LRC_GOTO_EDITOR,
};

struct lrc_word {
    long time_start;
    short count;
    short width;
    unsigned char *word;
};

struct lrc_brpos {
    short count;
    short width;
};

struct lrc_line {
    long time_start;
    long old_time_start;
    off_t file_offset; /* offset of time tag in file */
    short nword;
    short width;
    short nline[NB_SCREENS];
    struct lrc_line *next;
    struct lrc_word *words;
};

struct preferences {
    /* display settings */
#if LCD_DEPTH > 1
    unsigned active_color;
    unsigned inactive_color;
#endif
#ifdef HAVE_LCD_BITMAP
    bool wrap;
    bool wipe;
    bool active_one_line;
    int  align; /* 0: left, 1: center, 2: right */
    bool statusbar_on;
    bool display_title;
#endif
    bool display_time;
    bool backlight_on;

    /* file settings */
    char lrc_directory[64];
    int encoding;
#ifdef LRC_SUPPORT_ID3
    bool read_id3;
#endif
};

static struct preferences prefs, old_prefs;
static unsigned char *lrc_buffer;
static size_t lrc_buffer_size;
static size_t lrc_buffer_used, lrc_buffer_end;
enum extention_types {LRC, LRC8, SNC, TXT, NUM_TYPES, ID3_SYLT, ID3_USLT};
static const char *extentions[NUM_TYPES] = {
    ".lrc", ".lrc8", ".snc", ".txt",
};
static struct lrc_info {
    struct mp3entry *id3;
    long elapsed;
    long length;
    long ff_rewind;
    int  audio_status;
    char mp3_file[MAX_PATH];
    char lrc_file[MAX_PATH];
    char *title;  /* use lrc_buffer */
    char *artist; /* use lrc_buffer */
    enum extention_types type;
    long offset; /* msec */
    off_t offset_file_offset; /* offset of offset tag in file */
    int  nlrcbrpos;
    int  nlrcline;
    struct lrc_line *ll_head, **ll_tail;
    bool found_lrc;
    bool loaded_lrc;
    bool changed_lrc;
    bool too_many_lines; /* true if nlrcline >= max_lrclines after calc pos */
#ifdef HAVE_LCD_BITMAP
    bool wipe; /* false if lyrics is unsynched */
#endif
} current;
static char temp_buf[MAX(MAX_LINE_LEN,MAX_PATH)];
#ifdef HAVE_LCD_BITMAP
static int uifont = -1;
static int font_ui_height = 1;
static struct viewport vp_info[NB_SCREENS];
#endif
static struct viewport vp_lyrics[NB_SCREENS];

#define AUDIO_PAUSE (current.audio_status & AUDIO_STATUS_PAUSE)
#define AUDIO_PLAY  (current.audio_status & AUDIO_STATUS_PLAY)
#define AUDIO_STOP  (!(current.audio_status & AUDIO_STATUS_PLAY))

/*******************************
 * lrc_set_time
 *******************************/
#define LST_SET_MSEC    0x00010000
#define LST_SET_SEC     0x00020000
#define LST_SET_MIN     0x00040000
#define LST_SET_HOUR    0x00080000

#include "lib/pluginlib_actions.h"
#define LST_SET_TIME    (LST_SET_MSEC|LST_SET_SEC|LST_SET_MIN|LST_SET_HOUR)
#ifdef HAVE_LCD_CHARCELLS
#define LST_OFF_Y 0
#else /* HAVE_LCD_BITMAP */
#define LST_OFF_Y 1
#endif
static int lrc_set_time(const char *title, const char *unit, long *pval,
                 int step, int min, int max, int flags)
{
    const struct button_mapping *lst_contexts[] = {
        pla_main_ctx,
#ifdef HAVE_REMOTE_LCD
        pla_remote_ctx,
#endif
    };
    /* how many */
    const unsigned char formats[4][8] = {"%03ld.", "%02ld.", "%02ld:", "%02ld:"};
    const unsigned int maxs[4] = {1000, 60, 60, 24};
    const unsigned int scls[4] = {1, 1000, 60*1000, 60*60*1000};
    char buffer[32];
    long value = *pval, scl_step = step, i = 0;
    int pos = 0, last_pos = 0, pos_min = 3, pos_max = 0;
    int x = 0, y = 0, p_start = 0, p_end = 0;
    int ret = 10;

    if (!(flags&LST_SET_TIME))
        return -1;

    for (i = 0; i < 4; i++)
    {
        if (flags&(LST_SET_MSEC<<i))
        {
            if (pos_min > i) pos_min = i;
            if (pos_max < i) pos_max = i;
        }
    }
    pos = pos_min;

    rb->button_clear_queue();
    rb->lcd_clear_display();
    rb->lcd_puts_scroll(0, LST_OFF_Y, title);
    while (ret == 10)
    {
        int len = 0;
        long abs_val = value;
        long segvals[4] = {-1, -1, -1, -1};
        /* show negative value like -00:01 but 00:-1 */
        if (value < 0)
        {
            buffer[len++] = '-';
            abs_val = -value;
        }
        buffer[len] = 0;
        /* calc value of each segments */
        for (i = pos_min; i <= pos_max; i++)
        {
            segvals[i] = abs_val % maxs[i];
            abs_val /= maxs[i];
        }
        segvals[i-1] += abs_val * maxs[i-1];
        for (i = pos_max; i >= pos_min; i--)
        {
            if (pos == i)
            {
                rb->lcd_getstringsize(buffer, &x, &y);
                p_start = len;
            }
            rb->snprintf(&buffer[len], 32-len, formats[i], segvals[i]);
            len += rb->strlen(&buffer[len]);
            if (pos == i)
                p_end = len;
        }
        buffer[len-1] = 0; /* remove last separater */
        if (unit != NULL)
        {
            rb->snprintf(&buffer[len], 32-len, " (%s)", unit);
        }
        rb->lcd_puts(0, LST_OFF_Y+1, buffer);
        if (pos_min != pos_max)
        {
            /* draw cursor */
            buffer[p_end-1] = 0;
#ifdef HAVE_LCD_BITMAP
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_putsxy(x, y*(1+LST_OFF_Y), &buffer[p_start]);
            rb->lcd_set_drawmode(DRMODE_SOLID);
#else
            rb->lcd_put_cursor(x+rb->utf8length(&buffer[p_start])-1, y, 0x7F);
#endif
        }
        rb->lcd_update();
        int button = pluginlib_getaction(TIMEOUT_BLOCK, lst_contexts, ARRAYLEN(lst_contexts));
        int mult = 1;
#ifdef HAVE_LCD_CHARCELLS
        if (pos_min != pos_max)
            rb->lcd_remove_cursor();
#endif
        switch (button)
        {
            case PLA_UP_REPEAT:
            case PLA_DOWN_REPEAT:
                mult *= 10;
            case PLA_DOWN:
            case PLA_UP:
                if (button == PLA_DOWN_REPEAT || button == PLA_DOWN)
                    mult *= -1;
                if (pos != last_pos)
                {
                    scl_step = ((scls[pos]/scls[pos_min]+step-1)/step) * step;
                    last_pos = pos;
                }
                value += scl_step * mult;
                if (value > max)
                    value = max;
                if (value < min)
                    value = min;
                break;
            case PLA_LEFT:
            case PLA_LEFT_REPEAT:
                if (++pos > pos_max)
                    pos = pos_min;
                break;
            case PLA_RIGHT:
            case PLA_RIGHT_REPEAT:
                if (--pos < pos_min)
                    pos = pos_max;
                break;
            case PLA_SELECT:
                *pval = value;
                ret = 0;
                break;
            case PLA_CANCEL:
            case PLA_EXIT:
                rb->splash(HZ, "Cancelled");
                ret = -1;
                break;
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    ret = 1;
                break;
        }
    }
    rb->lcd_clear_display();
    rb->lcd_update();
    return ret;
}

/*******************************
 * misc stuff
 *******************************/
static void reset_current_data(void)
{
    current.title = NULL;
    current.artist = NULL;
    current.offset = 0;
    current.offset_file_offset = -1;
    current.nlrcbrpos = 0;
    current.nlrcline = 0;
    current.ll_head = NULL;
    current.ll_tail = &current.ll_head;
    current.loaded_lrc = false;
    current.changed_lrc = false;
    current.too_many_lines = false;
    lrc_buffer_used = 0;
    lrc_buffer_end = lrc_buffer_size;
}

/* check space and add str to lrc_buffer.
 * return NULL if there is not enough buffer. */
static char *lrcbufadd(const char*str, bool join)
{
    if (join) lrc_buffer_used--;
    size_t siz = rb->strlen(str)+1;
    char *pos = &lrc_buffer[lrc_buffer_used];
    if (lrc_buffer_used + siz > lrc_buffer_end)
        return NULL;
    rb->strcpy(pos, str);
    lrc_buffer_used += siz;
    return pos;
}
static void *alloc_buf(size_t siz)
{
    siz = (siz+3) & ~3;
    if (lrc_buffer_used + siz > lrc_buffer_end)
        return NULL;
    lrc_buffer_end -= siz;
    return &lrc_buffer[lrc_buffer_end];
}
static void *new_lrc_word(long time_start, char *word, bool join)
{
    struct lrc_word *lrc_word;
    if ((lrc_word = alloc_buf(sizeof(struct lrc_word))) == NULL)
        return NULL;
    if ((lrc_word->word = lrcbufadd(word, join)) == NULL)
        return NULL;
    lrc_word->time_start = time_start;
    return lrc_word;
}
static bool add_lrc_line(struct lrc_line *lrc_line, char *word)
{
    lrc_line->nword = 0;
    lrc_line->next = NULL;
    lrc_line->words = NULL;
    if (word)
    {
        if ((lrc_line->words = new_lrc_word(-1, word, false)) == NULL)
            return false;
        lrc_line->nword++;
    }
    *current.ll_tail = lrc_line;
    current.ll_tail = &(lrc_line->next);
    current.nlrcline++;
    return true;
}
static struct lrc_line *get_lrc_line(int idx)
{
    static struct lrc_line *lrc_line = NULL;
    static int n = 0;
    if (idx < n)
    {
        lrc_line = current.ll_head;
        n = 0;
    }
    while (n < idx && lrc_line)
    {
        lrc_line = lrc_line->next;
        n++;
    }
    return lrc_line;
}
static char *get_lrc_str(struct lrc_line *lrc_line)
{
    return lrc_line->words[lrc_line->nword-1].word;
}
static long get_time_start(struct lrc_line *lrc_line)
{
    if (!lrc_line) return current.length+20;
    long time = lrc_line->time_start + current.offset;
    return time < 0? 0: time;
}
static void set_time_start(struct lrc_line *lrc_line, long time_start)
{
    time_start -= current.offset;
    time_start -= time_start%10;
    if (lrc_line->time_start != time_start)
    {
        lrc_line->time_start = time_start;
        current.changed_lrc = true;
    }
}
#define get_word_time_start(x)      get_time_start((struct lrc_line *)(x))
#define set_word_time_start(x, t)   set_time_start((struct lrc_line *)(x), (t))

static int format_time_tag(char *buf, long t)
{
    return rb->snprintf(buf, 16, "%02ld:%02ld.%02ld",
                        t/60000, (t/1000)%60, (t/10)%100);
}
/* find start of next line */
static const char *lrc_skip_space(const char *str)
{
#ifdef HAVE_LCD_BITMAP
    if (prefs.wrap)
    {
        while (*str && *str != '\n' && isspace(*str))
            str++;
    }
#endif
    if (*str == '\n')
        str++;
    return str;
}

#ifdef HAVE_LCD_BITMAP
static bool isbrchr(const unsigned char *str, int len)
{
    const unsigned char *p = "!,-.:;?　、。！，．：；？―";
    if (isspace(*str))
        return true;

    while(*p)
    {
        int n = rb->utf8seek(p, 1);
        if (len == n && !rb->strncmp(p, str, len))
            return true;
        p += n;
    }
    return false;
}
#endif

/* calculate how many lines is needed to display and store it.
 * create cache if there is enough space in lrc_buffer. */
static struct lrc_brpos *calc_brpos(struct lrc_line *lrc_line, int i)
{
    struct lrc_brpos *lrc_brpos;
    struct lrc_word *lrc_word;
    int nlrcbrpos = 0, max_lrcbrpos;
#ifdef HAVE_LCD_BITMAP
    uifont = rb->screens[0]->getuifont();
    struct font* pf = rb->font_get(uifont);
    unsigned short ch;
#endif
    struct snap {
        int count, width;
        int nword;
        int word_count, word_width;
        const unsigned char *str;
    } 
#ifndef HAVE_LCD_CHARCELLS 
        sp, 
#endif
        cr;

    lrc_buffer_used = (lrc_buffer_used+3)&~3; /* 4 bytes aligned */
    lrc_brpos = (struct lrc_brpos *) &lrc_buffer[lrc_buffer_used];
    max_lrcbrpos = (lrc_buffer_end-lrc_buffer_used) / sizeof(struct lrc_brpos);

    if (!lrc_line)
    {
        /* calc info for all lrcs and store them if possible */
        size_t buffer_used = lrc_buffer_used;
        bool too_many_lines = false;
        current.too_many_lines = true;
        for (lrc_line = current.ll_head; lrc_line; lrc_line = lrc_line->next)
        {
            FOR_NB_SCREENS(i)
            {
                lrc_brpos = calc_brpos(lrc_line, i);
                if (!too_many_lines)
                {
                    lrc_buffer_used += lrc_line->nline[i]*sizeof(struct lrc_brpos);
                    if (nlrcbrpos + lrc_line->nline[i] >= max_lrcbrpos)
                    {
                        too_many_lines = true;
                        lrc_buffer_used = buffer_used;
                        calc_brpos(lrc_line, i);
                    }
                }
                nlrcbrpos += lrc_line->nline[i];
            }
        }
        current.too_many_lines = too_many_lines;
        lrc_buffer_used = buffer_used;
        current.nlrcbrpos = nlrcbrpos;
        return NULL;
    }

    if (!current.too_many_lines)
    {
        /* use stored infos. */
        struct lrc_line *temp_lrc = current.ll_head;
        for (; temp_lrc != lrc_line; temp_lrc = temp_lrc->next)
        {
            lrc_brpos += temp_lrc->nline[SCREEN_MAIN];
#ifdef HAVE_REMOTE_LCD
            lrc_brpos += temp_lrc->nline[SCREEN_REMOTE];
#endif
        }
#if NB_SCREENS >= 2
        while (i)
            lrc_brpos += lrc_line->nline[--i];
#endif
        return lrc_brpos;
    }

    /* calculate number of lines, line width and char count for each line. */
    lrc_line->width = 0;
    cr.nword = lrc_line->nword;
    lrc_word = lrc_line->words+cr.nword;
    cr.str = (lrc_word-1)->word;
#ifndef HAVE_LCD_CHARCELLS
    sp.word_count = 0;
    sp.word_width = 0;
    sp.nword = 0;
    sp.count = 0;
    sp.width = 0;
#endif
    do {
        cr.count = 0;
        cr.width = 0;
#ifndef HAVE_LCD_CHARCELLS
        sp.str = NULL;
#endif

        while (1)
        {
            while(cr.nword > 0 && cr.str >= (lrc_word-1)->word)
            {
                cr.nword--;
                lrc_word--;
                lrc_word->count = 0;
                lrc_word->width = 0;
            }
            if (*cr.str == 0 || *cr.str == '\n')
                break;

            int c, w;
#ifdef HAVE_LCD_CHARCELLS
            c = rb->utf8seek(cr.str, 1);
            w = 1;
#else
            c = ((long)rb->utf8decode(cr.str, &ch) - (long)cr.str);
            if (rb->is_diacritic(ch, NULL))
                w = 0;
            else
                w = rb->font_get_width(pf, ch);
            if (cr.count && prefs.wrap && isbrchr(cr.str, c))
            {
                /* remember position of last space */
                rb->memcpy(&sp, &cr, sizeof(struct snap));
                sp.word_count = lrc_word->count;
                sp.word_width = lrc_word->width;
                if (!isspace(*cr.str) && cr.width+w <= vp_lyrics[i].width)
                {
                    sp.count += c;
                    sp.width += w;
                    sp.word_count += c;
                    sp.word_width += w;
                    sp.str += c;
                }
            }
            if (cr.count && cr.width+w > vp_lyrics[i].width)
            {
                if (sp.str != NULL) /* wrap */
                {
                    rb->memcpy(&cr, &sp, sizeof(struct snap));
                    lrc_word = lrc_line->words+cr.nword;
                    lrc_word->count = sp.word_count;
                    lrc_word->width = sp.word_width;
                }
                break;
            }
#endif
            cr.count += c;
            cr.width += w;
            lrc_word->count += c;
            lrc_word->width += w;
            cr.str += c;
        }
        lrc_line->width += cr.width;
        lrc_brpos->count = cr.count;
        lrc_brpos->width = cr.width;
        nlrcbrpos++;
        lrc_brpos++;
        cr.str = lrc_skip_space(cr.str);
    } while (*cr.str && nlrcbrpos < max_lrcbrpos);
    lrc_line->nline[i] = nlrcbrpos;

    while (cr.nword > 0)
    {
        cr.nword--;
        lrc_word--;
        lrc_word->count = 0;
        lrc_word->width = 0;
    }
    return lrc_brpos-nlrcbrpos;
}

/* sort lyrics by time using stable sort. */
static void sort_lrcs(void)
{
    struct lrc_line *p = current.ll_head, **q = NULL, *t;
    long time_max = 0;

    current.ll_head = NULL;
    current.ll_tail = &current.ll_head;
    while (p != NULL)
    {
        t = p->next;
        /* remove problematic lrc_lines.
         * it would cause problem in display_lrc_line() if nword is 0. */
        if (p->nword)
        {
            q = p->time_start >= time_max? current.ll_tail: &current.ll_head;
            while ((*q) && (*q)->time_start <= p->time_start)
                q = &((*q)->next);
            p->next = *q;
            *q = p;
            if (!p->next)
            {
                time_max = p->time_start;
                current.ll_tail = &p->next;
            }
        }
        p = t;
    }
    if (!current.too_many_lines)
        calc_brpos(NULL, 0); /* stored data depends on order of lrcs if exist */
}
static void init_time_tag(void)
{
    struct lrc_line *lrc_line = current.ll_head;
    int nline = 0;
    if (current.type == TXT || current.type == ID3_USLT)
    {
        /* set time tag according to length of audio and total line count
         * for not synched lyrics, so that scroll speed is almost constant. */
        for (; lrc_line; lrc_line = lrc_line->next)
        {
            lrc_line->time_start = nline * current.length / current.nlrcbrpos;
            lrc_line->time_start -= lrc_line->time_start%10;
            lrc_line->old_time_start = -1;
            nline += lrc_line->nline[SCREEN_MAIN];
#ifdef HAVE_REMOTE_LCD
            nline += lrc_line->nline[SCREEN_REMOTE];
#endif
        }
    }
    else
    {
        /* reset timetags to the value read from file */
        for (; lrc_line; lrc_line = lrc_line->next)
        {
            lrc_line->time_start = lrc_line->old_time_start;
        }
        sort_lrcs();
    }
    current.changed_lrc = false;
}

/*******************************
 * Serch lrc file.
 *******************************/

/* search in same or parent directries of playing file.
 * assume playing file is /aaa/bbb/ccc/ddd.mp3,
 * this function searchs lrc file following order.
 * /aaa/bbb/ccc/ddd.lrc
 * /aaa/bbb/ddd.lrc
 * /aaa/ddd.lrc
 * /ddd.lrc
 */

/* taken from apps/recorder/albumart.c */
static void fix_filename(char* name)
{
    static const char invalid_chars[] = "*/:<>?\\|";

    while (1)
    {
        if (*name == 0)
            return;
        if (*name == '"')
            *name = '\'';
        else if (rb->strchr(invalid_chars, *name))
            *name = '_';
        name++;
    }
}
static bool find_lrc_file_helper(const char *base_dir)
{
    char fname[MAX_PATH];
    char *names[3] = {NULL, NULL, NULL};
    char *p, *dir;
    int i, len;
    /* /aaa/bbb/ccc/ddd.mp3
     * dir  <--q    names[0]
     */

    /* assuming file name starts with '/' */
    rb->strcpy(temp_buf, current.mp3_file);
    /* get file name and remove extension */
    names[0] = rb->strrchr(temp_buf, '/')+1;
    if ((p = rb->strrchr(names[0], '.')) != NULL)
        *p = 0;
    if (current.id3->title && rb->strcmp(names[0], current.id3->title))
    {
        rb->strlcpy(fname, current.id3->title, sizeof(fname));
        fix_filename(fname);
        names[1] = fname;
    }

    dir = temp_buf;
    p = names[0]-1;
    do {
        int n;
        *p = 0;
        for (n = 0; ; n++)
        {
            if (n == 0)
            {
                len = rb->snprintf(current.lrc_file, MAX_PATH, "%s%s/",
                                    base_dir, dir);
            }
            else if (n == 1)
            {
                /* check file in subfolder named prefs.lrc_directory
                 * in the directory of mp3 file. */
                if (prefs.lrc_directory[0] == '/')
                {
                    len = rb->snprintf(current.lrc_file, MAX_PATH, "%s%s/",
                                        dir, prefs.lrc_directory);
                }
                else
                    continue;
            }
            else
                break;
            DEBUGF("check file in %s\n", current.lrc_file);
            if (!rb->dir_exists(current.lrc_file))
                continue;
            for (current.type = 0; current.type < NUM_TYPES; current.type++)
            {
                for (i = 0; names[i] != NULL; i++)
                {
                    rb->snprintf(&current.lrc_file[len], MAX_PATH-len,
                            "%s%s", names[i], extentions[current.type]);
                    if (rb->file_exists(current.lrc_file))
                    {
                        DEBUGF("found: `%s'\n", current.lrc_file);
                        return true;
                    }
                }
            }
        }
    } while ((p = rb->strrchr(dir, '/')) != NULL);
    return false;
}

/* return true if a lrc file is found */
static bool find_lrc_file(void)
{
    reset_current_data();

    DEBUGF("find lrc file for `%s'\n", current.mp3_file);
    /* find .lrc file */
    if (find_lrc_file_helper(""))
        return true;
    if (prefs.lrc_directory[0] == '/' && rb->dir_exists(prefs.lrc_directory))
    {
        if (find_lrc_file_helper(prefs.lrc_directory))
            return true;
    }

    current.lrc_file[0] = 0;
    return false;
}

/*******************************
 * Load file.
 *******************************/

/* check tag format and calculate value of the tag.
 * supported tag: ti, ar, offset
 * supported format of time tag: [mm:ss], [mm:ss.xx], [mm:ss.xxx]
 * returns value of timega if tag is time tag, -1 if tag is supported tag,
 * -10 otherwise.
 */
static char *parse_int(char *ptr, int *val)
{
    *val = rb->atoi(ptr);
    while (isdigit(*ptr)) ptr++;
    return ptr;
}
static long get_time_value(char *tag, bool read_id_tags, off_t file_offset)
{
    long time;
    char *ptr;
    int val;

    if (read_id_tags)
    {
        if (!rb->strncmp(tag, "ti:", 3))
        {
            if (!current.id3->title || rb->strcmp(&tag[3], current.id3->title))
                current.title = lrcbufadd(&tag[3], false);
            return -1;
        }
        if (!rb->strncmp(tag, "ar:", 3))
        {
            if (!current.id3->artist || rb->strcmp(&tag[3], current.id3->artist))
                current.artist = lrcbufadd(&tag[3], false);
            return -1;
        }
        if (!rb->strncmp(tag, "offset:", 7))
        {
            current.offset = rb->atoi(&tag[7]);
            current.offset_file_offset = file_offset;
            return -1;
        }
    }

    /* minute */
    ptr = parse_int(tag, &val);
    if (ptr-tag < 1 || ptr-tag > 2 || *ptr != ':')
        return -10;
    time = val * 60000;
    /* second */
    tag = ptr+1;
    ptr = parse_int(tag, &val);
    if (ptr-tag != 2 || (*ptr != '.' && *ptr != ':' && *ptr != '\0'))
        return -10;
    time += val * 1000;

    if (*ptr != '\0')
    {
        /* milliseccond */
        tag = ptr+1;
        ptr = parse_int(tag, &val);
        if (ptr-tag < 2 || ptr-tag > 3 || *ptr != '\0')
            return -10;
        time += ((ptr-tag)==3 ?val: val*10);
    }

    return time;
}

/* format:
 * [time tag]line
 * [time tag]...[time tag]line
 * [time tag]<word time tag>word<word time tag>...<word time tag>
 */
static bool parse_lrc_line(char *line, off_t file_offset)
{
    struct lrc_line *lrc_line = NULL, *first_lrc_line = NULL;
    long time, time_start;
    char *str, *tagstart, *tagend;
    struct lrc_word *lrc_word;
    int nword = 0;

    /* parse [time tag]...[time tag] type tags */
    str = line;
    while (1)
    {
        if (*str != '[') break;
        tagend = rb->strchr(str, ']');
        if (tagend == NULL) break;
        *tagend = 0;
        time = get_time_value(str+1, !lrc_line, file_offset);
        *tagend++ = ']';
        if (time < 0)
            break;
        lrc_line = alloc_buf(sizeof(struct lrc_line));
        if (lrc_line == NULL)
            return false;
        if (!first_lrc_line)
            first_lrc_line = lrc_line;
        lrc_line->file_offset = file_offset;
        lrc_line->time_start = (time/10)*10;
        lrc_line->old_time_start = lrc_line->time_start;
        add_lrc_line(lrc_line, NULL);
        file_offset += (long)tagend - (long)str;
        str = tagend;
    }
    if (!first_lrc_line)
        return true; /* no time tag in line */

    lrc_line = first_lrc_line;
    if (lrcbufadd("", false) == NULL)
        return false;

    /* parse <word time tag>...<word time tag> type tags */
    /* [time tag]...[time tag]line type tags share lrc_line->words and can't
     * use lrc_line->words->timestart. use lrc_line->time_start instead. */
    time_start = -1;
    tagstart = str;
    while (*tagstart)
    {
        tagstart = rb->strchr(tagstart, '<');
        if (!tagstart) break;
        tagend = rb->strchr(tagstart, '>');
        if (!tagend) break;
        *tagend = 0;
        time = get_time_value(tagstart+1, false,
                                file_offset + ((long)tagstart - (long)str));
        *tagend++ = '>';
        if (time < 0)
        {
            tagstart++;
            continue;
        }
        *tagstart = 0;
        /* found word time tag. */
        if (*str || time_start != -1)
        {
            if ((lrc_word = new_lrc_word(time_start, str, true)) == NULL)
                return false;
            nword++;
        }
        file_offset += (long)tagend - (long)str;
        tagstart = str = tagend;
        time_start = time;
    }
    if ((lrc_word = new_lrc_word(time_start, str, true)) == NULL)
        return false;
    nword++;

    /* duplicate lrc_lines */
    while (lrc_line)
    {
        lrc_line->nword = nword;
        lrc_line->words = lrc_word;
        lrc_line = lrc_line->next;
    }

    return true;
}

/* format:
 * \xa2\xe2hhmmssxx\xa2\xd0
 * line 1
 * line 2
 * \xa2\xe2hhmmssxx\xa2\xd0
 * line 3
 * ...
 */
static bool parse_snc_line(char *line, off_t file_offset)
{
#define SNC_TAG_START "\xa2\xe2"
#define SNC_TAG_END   "\xa2\xd0"

    /* SNC_TAG can be dencoded, so use
     * temp_buf which contains native data */
    if (!rb->memcmp(temp_buf, SNC_TAG_START, 2)
     && !rb->memcmp(temp_buf+10, SNC_TAG_END, 2)) /* time tag */
    {
        const char *pos = temp_buf+2; /* skip SNC_TAG_START */
        int hh, mm, ss, xx;

        hh = (pos[0]-'0')*10+(pos[1]-'0'); pos += 2;
        mm = (pos[0]-'0')*10+(pos[1]-'0'); pos += 2;
        ss = (pos[0]-'0')*10+(pos[1]-'0'); pos += 2;
        xx = (pos[0]-'0')*10+(pos[1]-'0'); pos += 2;
        pos += 2; /* skip SNC_TAG_END */

        /* initialize */
        struct lrc_line *lrc_line = alloc_buf(sizeof(struct lrc_line));
        if (lrc_line == NULL)
            return false;
        lrc_line->file_offset = file_offset+2;
        lrc_line->time_start = hh*3600000+mm*60000+ss*1000+xx*10;
        lrc_line->old_time_start = lrc_line->time_start;
        if (!add_lrc_line(lrc_line, ""))
            return false;
        if (pos[0]==0)
            return true;

        /* encode rest of line and add to buffer */
        rb->iso_decode(pos, line, prefs.encoding, rb->strlen(pos)+1);
    }
    if (current.ll_head)
    {
        rb->strcat(line, "\n");
        if (lrcbufadd(line, true) == NULL)
            return false;
    }
    return true;
}

static bool parse_txt_line(char *line, off_t file_offset)
{
    /* initialize */
    struct lrc_line *lrc_line = alloc_buf(sizeof(struct lrc_line));
    if (lrc_line == NULL)
        return false;
    lrc_line->file_offset = file_offset;
    lrc_line->time_start = 0;
    lrc_line->old_time_start = -1;
    if (!add_lrc_line(lrc_line, line))
        return false;
    return true;
}

static void load_lrc_file(void)
{
    char utf8line[MAX_LINE_LEN*3];
    int fd;
    int encoding = prefs.encoding;
    bool (*line_parser)(char *line, off_t) = NULL;
    off_t file_offset, readsize;

    switch(current.type)
    {
        case LRC8:
            encoding = UTF_8; /* .lrc8 is utf8 */
            /* fall through */
        case LRC:
            line_parser = parse_lrc_line;
            break;
        case SNC:
            line_parser = parse_snc_line;
            break;
        case TXT:
            line_parser = parse_txt_line;
            break;
        default:
            return;
    }

    fd = rb->open(current.lrc_file, O_RDONLY);
    if (fd < 0) return;

    {
        /* check encoding */
        #define BOM "\xef\xbb\xbf"
        #define BOM_SIZE 3
        unsigned char header[BOM_SIZE];
        unsigned char* (*utf_decode)(const unsigned char *,
                                     unsigned char *, int) = NULL;
        rb->read(fd, header, BOM_SIZE);
        if (!rb->memcmp(header, BOM, BOM_SIZE))      /* UTF-8 */
        {
            encoding = UTF_8;
        }
        else if (!rb->memcmp(header, "\xff\xfe", 2)) /* UTF-16LE */
        {
            utf_decode = rb->utf16LEdecode;
        }
        else if (!rb->memcmp(header, "\xfe\xff", 2)) /* UTF-16BE */
        {
            utf_decode = rb->utf16BEdecode;
        }
        else
        {
            rb->lseek(fd, 0, SEEK_SET);
        }

        if (utf_decode)
        {
            /* convert encoding of file from UTF-16 to UTF-8 */
            char temp_file[MAX_PATH];
            int fe;
            rb->lseek(fd, 2, SEEK_SET);
            rb->snprintf(temp_file, MAX_PATH, "%s~", current.lrc_file);
            fe = rb->creat(temp_file, 0666);
            if (fe < 0)
            {
                rb->close(fd);
                return;
            }
            rb->write(fe, BOM, BOM_SIZE);
            while ((readsize = rb->read(fd, temp_buf, MAX_LINE_LEN)) > 0)
            {
                char *end = utf_decode(temp_buf, utf8line, readsize/2);
                rb->write(fe, utf8line, end-utf8line);
            }
            rb->close(fe);
            rb->close(fd);
            rb->remove(current.lrc_file);
            rb->rename(temp_file, current.lrc_file);
            fd = rb->open(current.lrc_file, O_RDONLY);
            if (fd < 0) return;
            rb->lseek(fd, BOM_SIZE, SEEK_SET); /* skip bom */
            encoding = UTF_8;
        }
    }

    file_offset = rb->lseek(fd, 0, SEEK_CUR); /* used in line_parser */
    while ((readsize = rb->read_line(fd, temp_buf, MAX_LINE_LEN)) > 0)
    {
        /* note: parse_snc_line() reads temp_buf for native data. */
        rb->iso_decode(temp_buf, utf8line, encoding, readsize+1);
        if (!line_parser(utf8line, file_offset))
            break;
        file_offset += readsize;
    }
    rb->close(fd);

    current.loaded_lrc = true;
    calc_brpos(NULL, 0);
    init_time_tag();

    return;
}

#ifdef LRC_SUPPORT_ID3
/*******************************
 * read lyrics from id3
 *******************************/
static unsigned long unsync(unsigned long b0, unsigned long b1,
                            unsigned long b2, unsigned long b3)
{
    return (((long)(b0 & 0x7F) << (3*7)) |
            ((long)(b1 & 0x7F) << (2*7)) |
            ((long)(b2 & 0x7F) << (1*7)) |
            ((long)(b3 & 0x7F) << (0*7)));
}

static unsigned long bytes2int(unsigned long b0, unsigned long b1,
                               unsigned long b2, unsigned long b3)
{
    return (((long)(b0 & 0xFF) << (3*8)) |
            ((long)(b1 & 0xFF) << (2*8)) |
            ((long)(b2 & 0xFF) << (1*8)) |
            ((long)(b3 & 0xFF) << (0*8)));
}

static int unsynchronize(char* tag, int len, bool *ff_found)
{
    int i;
    unsigned char c;
    unsigned char *rp, *wp;
    bool _ff_found = false;
    if(ff_found) _ff_found = *ff_found;

    wp = rp = (unsigned char *)tag;

    rp = (unsigned char *)tag;
    for(i = 0; i<len; i++) {
        /* Read the next byte and write it back, but don't increment the
           write pointer */
        c = *rp++;
        *wp = c;
        if(_ff_found) {
            /* Increment the write pointer if it isn't an unsynch pattern */
            if(c != 0)
                wp++;
            _ff_found = false;
        } else {
            if(c == 0xff)
                _ff_found = true;
            wp++;
        }
    }
    if(ff_found) *ff_found = _ff_found;
    return (long)wp - (long)tag;
}

static int read_unsynched(int fd, void *buf, int len, bool *ff_found)
{
    int i;
    int rc;
    int remaining = len;
    char *wp;

    wp = buf;

    while(remaining) {
        rc = rb->read(fd, wp, remaining);
        if(rc <= 0)
            return rc;

        i = unsynchronize(wp, remaining, ff_found);
        remaining -= i;
        wp += i;
    }

    return len;
}

static unsigned char* utf8cpy(const unsigned char *src,
                              unsigned char *dst, int count)
{
    rb->strlcpy(dst, src, count+1);
    return dst+rb->strlen(dst);
}

static void parse_id3v2(int fd)
{
    int minframesize;
    int size;
    long framelen;
    char header[10];
    char tmp[8];
    unsigned char version;
    int bytesread = 0;
    unsigned char global_flags;
    int flags;
    bool global_unsynch = false;
    bool global_ff_found = false;
    bool unsynch = false;
    int rc;
    enum {NOLT, SYLT, USLT} type = NOLT;

    /* Bail out if the tag is shorter than 10 bytes */
    if(current.id3->id3v2len < 10)
        return;

    /* Read the ID3 tag version from the header */
    if(10 != rb->read(fd, header, 10))
        return;

    /* Get the total ID3 tag size */
    size = current.id3->id3v2len - 10;

    version = current.id3->id3version;
    switch ( version )
    {
        case ID3_VER_2_2:
            minframesize = 8;
            break;

        case ID3_VER_2_3:
            minframesize = 12;
            break;

        case ID3_VER_2_4:
            minframesize = 12;
            break;

        default:
            /* unsupported id3 version */
            return;
    }

    global_flags = header[5];

    /* Skip the extended header if it is present */
    if(global_flags & 0x40) {

        if(version == ID3_VER_2_3) {
            if(10 != rb->read(fd, header, 10))
                return;
            /* The 2.3 extended header size doesn't include the header size
               field itself. Also, it is not unsynched. */
            framelen =
                bytes2int(header[0], header[1], header[2], header[3]) + 4;

            /* Skip the rest of the header */
            rb->lseek(fd, framelen - 10, SEEK_CUR);
        }

        if(version >= ID3_VER_2_4) {
            if(4 != rb->read(fd, header, 4))
                return;

            /* The 2.4 extended header size does include the entire header,
               so here we can just skip it. This header is unsynched. */
            framelen = unsync(header[0], header[1],
                              header[2], header[3]);

            rb->lseek(fd, framelen - 4, SEEK_CUR);
        }
    }

    /* Is unsynchronization applied? */
    if(global_flags & 0x80) {
        global_unsynch = true;
    }

    /* We must have at least minframesize bytes left for the
     * remaining frames to be interesting */
    while (size >= minframesize) {
        flags = 0;

        /* Read frame header and check length */
        if(version >= ID3_VER_2_3) {
            if(global_unsynch && version <= ID3_VER_2_3)
                rc = read_unsynched(fd, header, 10, &global_ff_found);
            else
                rc = rb->read(fd, header, 10);
            if(rc != 10)
                return;
            /* Adjust for the 10 bytes we read */
            size -= 10;

            flags = bytes2int(0, 0, header[8], header[9]);

            if (version >= ID3_VER_2_4) {
                framelen = unsync(header[4], header[5],
                                  header[6], header[7]);
            } else {
                /* version .3 files don't use synchsafe ints for
                 * size */
                framelen = bytes2int(header[4], header[5],
                                     header[6], header[7]);
            }
        } else {
            if(6 != rb->read(fd, header, 6))
                return;
            /* Adjust for the 6 bytes we read */
            size -= 6;

            framelen = bytes2int(0, header[3], header[4], header[5]);
        }

        if(framelen == 0){
            if (header[0] == 0 && header[1] == 0 && header[2] == 0)
                return;
            else
                continue;
        }

        unsynch = false;

        if(flags)
        {
            if (version >= ID3_VER_2_4) {
                if(flags & 0x0040) { /* Grouping identity */
                    rb->lseek(fd, 1, SEEK_CUR); /* Skip 1 byte */
                    framelen--;
                }
            } else {
                if(flags & 0x0020) { /* Grouping identity */
                    rb->lseek(fd, 1, SEEK_CUR); /* Skip 1 byte */
                    framelen--;
                }
            }

            if(flags & 0x000c) /* Compression or encryption */
            {
                /* Skip it */
                size -= framelen;
                rb->lseek(fd, framelen, SEEK_CUR);
                continue;
            }

            if(flags & 0x0002) /* Unsynchronization */
                unsynch = true;

            if (version >= ID3_VER_2_4) {
                if(flags & 0x0001) { /* Data length indicator */
                    if(4 != rb->read(fd, tmp, 4))
                        return;

                    /* We don't need the data length */
                    framelen -= 4;
                }
            }
        }

        if (framelen == 0)
            continue;

        if (framelen < 0)
            return;

        if(!rb->memcmp( header, "SLT", 3 ) ||
           !rb->memcmp( header, "SYLT", 4 ))
        {
            /* found a supported tag */
            type = SYLT;
            break;
        }
        else if(!rb->memcmp( header, "ULT", 3 ) ||
                !rb->memcmp( header, "USLT", 4 ))
        {
            /* found a supported tag */
            type = USLT;
            break;
        }
        else
        {
            /* not a supported tag*/
            if(global_unsynch && version <= ID3_VER_2_3) {
                size -= read_unsynched(fd, lrc_buffer, framelen, &global_ff_found);
            } else {
                size -= framelen;
                if( rb->lseek(fd, framelen, SEEK_CUR) == -1 )
                    return;
            }
        }
    }
    if(type == NOLT)
        return;

    int encoding = 0, chsiz;
    char *tag, *p, utf8line[MAX_LINE_LEN*3];
    unsigned char* (*utf_decode)(const unsigned char *,
                                 unsigned char *, int) = NULL;
    /* use middle of lrc_buffer to store tag data. */
    if(framelen >= LRC_BUFFER_SIZE/3)
        framelen = LRC_BUFFER_SIZE/3-1;
    tag = lrc_buffer+LRC_BUFFER_SIZE*2/3-framelen-1;
    if(global_unsynch && version <= ID3_VER_2_3)
        bytesread = read_unsynched(fd, tag, framelen, &global_ff_found);
    else
        bytesread = rb->read(fd, tag, framelen);

    if( bytesread != framelen )
        return;

    if(unsynch || (global_unsynch && version >= ID3_VER_2_4))
        bytesread = unsynchronize(tag, bytesread, NULL);

    tag[bytesread] = 0;
    encoding = tag[0];
    p = tag;
    /* skip some data */
    if(type == SYLT) {
        p += 6;
    } else {
        p += 4;
    }

    /* check encoding and skip content descriptor */
    switch (encoding) {
        case 0x01: /* Unicode with or without BOM */
        case 0x02:

            /* Now check if there is a BOM
               (zero-width non-breaking space, 0xfeff)
               and if it is in little or big endian format */
            if(!rb->memcmp(p, "\xff\xfe", 2)) { /* Little endian? */
                utf_decode = rb->utf16LEdecode;
            } else if(!rb->memcmp(p, "\xfe\xff", 2)) { /* Big endian? */
                utf_decode = rb->utf16BEdecode;
            } else
                utf_decode = NULL;

            encoding = NUM_CODEPAGES;
            do {
                size = p[0] | p[1];
                p += 2;
            } while(size);
            chsiz = 2;
            break;

        default:
            utf_decode = utf8cpy;
            if(encoding == 0x03) /* UTF-8 encoded string */
                encoding = UTF_8;
            else
                encoding = prefs.encoding;
            p += rb->strlen(p)+1;
            chsiz = 1;
            break;
    }
    if(encoding == NUM_CODEPAGES)
    {
        /* check if there is a BOM */
        if(!rb->memcmp(p, "\xff\xfe", 2)) { /* Little endian? */
            utf_decode = rb->utf16LEdecode;
            p += 2;
        } else if(!rb->memcmp(p, "\xfe\xff", 2)) { /* Big endian? */
            utf_decode = rb->utf16BEdecode;
            p += 2;
        } else if(!utf_decode) {
            /* If there is no BOM (which is a specification violation),
               let's try to guess it. If one of the bytes is 0x00, it is
               probably the most significant one. */
            if(p[1] == 0)
                utf_decode = rb->utf16LEdecode;
            else
                utf_decode = rb->utf16BEdecode;
        }
    }
    bytesread -= (long)p - (long)tag;
    tag = p;

    while ( bytesread > 0
        && lrc_buffer_used+bytesread < LRC_BUFFER_SIZE*2/3
        && LRC_BUFFER_SIZE*2/3 < lrc_buffer_end)
    {
        bool is_crlf = false;
        struct lrc_line *lrc_line = alloc_buf(sizeof(struct lrc_line));
        if(!lrc_line)
            break;
        lrc_line->file_offset = -1;
        if(type == USLT)
        {
            /* replace 0x0a and 0x0d with 0x00 */
            p = tag;
            while(1) {
                utf_decode(p, tmp, 2);
                if(!tmp[0]) break;
                if(tmp[0] == 0x0d || tmp[0] == 0x0a)
                {
                    if(tmp[0] == 0x0d && tmp[1] == 0x0a)
                        is_crlf = true;
                    p[0] = 0;
                    p[chsiz-1] = 0;
                    break;
                }
                p += chsiz;
            }
        }
        if(encoding == NUM_CODEPAGES)
        {
            unsigned char* utf8 = utf8line;
            p = tag;
            do {
                utf8 = utf_decode(p, utf8, 1);
                p += 2;
            } while(*(utf8-1));
        }
        else
        {
            size = rb->strlen(tag)+1;
            rb->iso_decode(tag, utf8line, encoding, size);
            p = tag+size;
        }

        if(type == SYLT) { /* timestamp */
            lrc_line->time_start = bytes2int(p[0], p[1], p[2], p[3]);
            lrc_line->old_time_start = lrc_line->time_start;
            p += 4;
            utf_decode(p, tmp, 1);
            if(tmp[0] == 0x0a)
                p += chsiz;
        } else { /* USLT */
            lrc_line->time_start = 0;
            lrc_line->old_time_start = -1;
            if(is_crlf) p += chsiz;
        }
        bytesread -= (long)p - (long)tag;
        tag = p;
        if(!add_lrc_line(lrc_line, utf8line))
            break;
    }

    current.type = ID3_SYLT-SYLT+type;
    rb->strcpy(current.lrc_file, current.mp3_file);

    current.loaded_lrc = true;
    calc_brpos(NULL, 0);
    init_time_tag();

    return;
}

static bool read_id3(void)
{
    int fd;

    if(current.id3->codectype != AFMT_MPA_L1
    && current.id3->codectype != AFMT_MPA_L2
    && current.id3->codectype != AFMT_MPA_L3)
        return false;

    fd = rb->open(current.mp3_file, O_RDONLY);
    if(fd < 0) return false;
    current.loaded_lrc = false;
    parse_id3v2(fd);
    rb->close(fd);
    return current.loaded_lrc;
}
#endif /* LRC_SUPPORT_ID3 */

/*******************************
 * Display information
 *******************************/
static void display_state(void)
{
    const char *str = NULL;

    if (AUDIO_STOP)
        str = "Audio Stopped";
    else if (current.found_lrc)
    {
        if (!current.loaded_lrc)
            str = "Loading lrc";
        else if (!current.ll_head)
            str = "No lyrics";
    }

#ifdef HAVE_LCD_BITMAP
    const char *info = NULL;

    if (AUDIO_PLAY && prefs.display_title)
    {
        char *title = (current.title? current.title: current.id3->title);
        char *artist = (current.artist? current.artist: current.id3->artist);

        if (artist != NULL && title != NULL)
        {
            rb->snprintf(temp_buf, MAX_LINE_LEN, "%s/%s", title, artist);
            info = temp_buf;
        }
        else if (title != NULL)
            info = title;
        else if (current.mp3_file[0] == '/')
            info = rb->strrchr(current.mp3_file, '/')+1;
        else
            info = "(no info)";
    }

    int w, h;
    struct screen* display;
    FOR_NB_SCREENS(i)
    {
        display = rb->screens[i];
        display->set_viewport(&vp_info[i]);
        display->clear_viewport();
        if (info)
            display->puts_scroll(0, 0, info);
        if (str)
        {
            display->set_viewport(&vp_lyrics[i]);
            display->clear_viewport();
            display->getstringsize(str, &w, &h);
            if (vp_lyrics[i].width - w < 0)
                display->puts_scroll(0, vp_lyrics[i].height/font_ui_height/2,
                                     str);
            else
                display->putsxy((vp_lyrics[i].width - w)*prefs.align/2,
                                (vp_lyrics[i].height-font_ui_height)/2, str);
            display->set_viewport(&vp_info[i]);
        }
        display->update_viewport();
        display->set_viewport(NULL);
    }
#else
    /* there is no place to display title or artist. */
    rb->lcd_clear_display();
    if (str)
        rb->lcd_puts_scroll(0, 0, str);
    rb->lcd_update();
#endif /* HAVE_LCD_BITMAP */
}

static void display_time(void)
{
    rb->snprintf(temp_buf, MAX_LINE_LEN, "%ld:%02ld/%ld:%02ld",
                            current.elapsed/60000, (current.elapsed/1000)%60,
                            current.length/60000, (current.length)/1000%60);
#ifdef HAVE_LCD_BITMAP
    int y = (prefs.display_title? font_ui_height:0);
    FOR_NB_SCREENS(i)
    {
        struct screen* display = rb->screens[i];
        display->set_viewport(&vp_info[i]);
        display->setfont(FONT_SYSFIXED);
        display->putsxy(0, y, temp_buf);
        rb->gui_scrollbar_draw(display, 0, y+SYSFONT_HEIGHT+1,
                               vp_info[i].width, SYSFONT_HEIGHT-2,
                               current.length, 0, current.elapsed, HORIZONTAL);
        display->update_viewport_rect(0, y, vp_info[i].width, SYSFONT_HEIGHT*2);
        display->setfont(uifont);
        display->set_viewport(NULL);
    }
#else
    rb->lcd_puts(0, 0, temp_buf);
    rb->lcd_update();
#endif /* HAVE_LCD_BITMAP */
}

/*******************************
 * Display lyrics
 *******************************/
#ifdef HAVE_LCD_BITMAP
static inline void set_to_default(struct screen *display)
{
#if (LCD_DEPTH > 1)
#ifdef HAVE_REMOTE_LCD
    if (display->screen_type != SCREEN_REMOTE)
#endif
        display->set_foreground(prefs.active_color);
#endif
    display->set_drawmode(DRMODE_SOLID);
}
static inline void set_to_active(struct screen *display)
{
#if (LCD_DEPTH > 1)
#ifdef HAVE_REMOTE_LCD
    if (display->screen_type == SCREEN_REMOTE)
        display->set_drawmode(DRMODE_INVERSEVID);
    else
#endif
    {
        display->set_foreground(prefs.active_color);
        display->set_drawmode(DRMODE_SOLID);
    }
#else /* LCD_DEPTH == 1 */
    display->set_drawmode(DRMODE_INVERSEVID);
#endif
}
static inline void set_to_inactive(struct screen *display)
{
#if (LCD_DEPTH > 1)
#ifdef HAVE_REMOTE_LCD
    if (display->screen_type != SCREEN_REMOTE)
#endif
        display->set_foreground(prefs.inactive_color);
#endif
    display->set_drawmode(DRMODE_SOLID);
}

static int display_lrc_line(struct lrc_line *lrc_line, int ypos, int i)
{
    struct screen *display = rb->screens[i];
    struct lrc_word *lrc_word;
    struct lrc_brpos *lrc_brpos;
    long time_start, time_end, elapsed;
    int count, width, nword;
    int xpos;
    const char *str;
    bool active_line;

    time_start = get_time_start(lrc_line);
    time_end = get_time_start(lrc_line->next);
    active_line = (time_start <= current.elapsed
                    && time_end > current.elapsed);

    if (!lrc_line->width)
    {
        /* empty line. draw bar during interval. */
        long rin = current.elapsed - time_start;
        long len = time_end - time_start;
        if (current.wipe && active_line && len >= 3500)
        {
            elapsed = rin * vp_lyrics[i].width / len;
            set_to_inactive(display);
            display->fillrect(elapsed, ypos+font_ui_height/4+1,
                 vp_lyrics[i].width-elapsed-1, font_ui_height/2-2);
            set_to_active(display);
            display->drawrect(0, ypos+font_ui_height/4,
                              vp_lyrics[i].width, font_ui_height/2);
            display->fillrect(1, ypos+font_ui_height/4+1,
                              elapsed-1, font_ui_height/2-2);
            set_to_default(display);
        }
        return ypos + font_ui_height;
    }

    lrc_brpos = calc_brpos(lrc_line, i);
    /* initialize line */
    xpos = (vp_lyrics[i].width - lrc_brpos->width)*prefs.align/2;
    count = 0;
    width = 0;

    active_line = active_line || !prefs.active_one_line;
    nword = lrc_line->nword-1;
    lrc_word = lrc_line->words + nword;
    str = lrc_word->word;
    /* only time_start of first word could be -1 */
    if (lrc_word->time_start != -1)
        time_end = get_word_time_start(lrc_word);
    else
        time_end = time_start;
    do {
        time_start = time_end;
        if (nword > 0)
            time_end = get_word_time_start(lrc_word-1);
        else
            time_end = get_time_start(lrc_line->next);

        if (time_start > current.elapsed || !active_line)
        {
            /* inactive */
            elapsed = 0;
        }
        else if (!current.wipe || time_end <= current.elapsed)
        {
            /* active whole word */
            elapsed = lrc_word->width;
        }
        else
        {
            /* wipe word */
            long rin = current.elapsed - time_start;
            long len = time_end - time_start;
            elapsed = rin * lrc_word->width / len;
        }

        int word_count = lrc_word->count;
        int word_width = lrc_word->width;
        set_to_active(display);
        while (word_count > 0 && word_width > 0)
        {
            int c = lrc_brpos->count - count;
            int w = lrc_brpos->width - width;
            if (c > word_count || w > word_width)
            {
                c = word_count;
                w = word_width;
            }
            if (elapsed <= 0)
            {
                set_to_inactive(display);
            }
            else if (elapsed < w)
            {
                /* wipe text */
                display->fillrect(xpos, ypos, elapsed, font_ui_height);
                set_to_inactive(display);
                display->fillrect(xpos+elapsed, ypos,
                                  w-elapsed, font_ui_height);
#if (LCD_DEPTH > 1)
#ifdef HAVE_REMOTE_LCD
                if (display->screen_type == SCREEN_REMOTE)
                    display->set_drawmode(DRMODE_INVERSEVID);
                else
#endif
                    display->set_drawmode(DRMODE_BG);
#else
                display->set_drawmode(DRMODE_INVERSEVID);
#endif
            }
            rb->strlcpy(temp_buf, str, c+1);
            display->putsxy(xpos, ypos, temp_buf);
            str += c;
            xpos += w;
            count += c;
            width += w;
            word_count -= c;
            word_width -= w;
            elapsed -= w;
            if (count >= lrc_brpos->count || width >= lrc_brpos->width)
            {
                /* prepare for next line */
                lrc_brpos++;
                str = lrc_skip_space(str);
                xpos = (vp_lyrics[i].width - lrc_brpos->width)*prefs.align/2;
                ypos += font_ui_height;
                count = 0;
                width = 0;
            }
        }
        lrc_word--;
    } while (nword--);
    set_to_default(display);
    return ypos;
}
#endif /* HAVE_LCD_BITMAP */

static void display_lrcs(void)
{
    long time_start, time_end, rin, len;
    int nline[NB_SCREENS] = {0};
    struct lrc_line *lrc_center = current.ll_head;

    if (!lrc_center) return;

    while (get_time_start(lrc_center->next) <= current.elapsed)
    {
        nline[SCREEN_MAIN] += lrc_center->nline[SCREEN_MAIN];
#ifdef HAVE_REMOTE_LCD
        nline[SCREEN_REMOTE] += lrc_center->nline[SCREEN_REMOTE];
#endif
        lrc_center = lrc_center->next;
    }

    time_start = get_time_start(lrc_center);
    time_end = get_time_start(lrc_center->next);
    rin = current.elapsed - time_start;
    len = time_end - time_start;

    struct screen *display;
    FOR_NB_SCREENS(i)
    {
        display = rb->screens[i];
        /* display current line at the center of the viewport */
        display->set_viewport(&vp_lyrics[i]);
        display->clear_viewport();
#ifdef HAVE_LCD_BITMAP
        struct lrc_line *lrc_line;
        int y, ypos = 0, nblines = vp_lyrics[i].height/font_ui_height;
        y = (nblines-1)/2;
        if (rin < 0)
        {
            /* current.elapsed < time of first lrc */
            if (!current.wipe)
                ypos = (time_start - current.elapsed)
                            * font_ui_height / time_start;
            else
                y++;
        }
        else if (len > 0)
        {
            if (!current.wipe)
                ypos = - rin * lrc_center->nline[i] * font_ui_height / len;
            else
            {
                long elapsed = rin * lrc_center->width / len;
                struct lrc_brpos *lrc_brpos = calc_brpos(lrc_center, i);
                while (elapsed > lrc_brpos->width)
                {
                    elapsed -= lrc_brpos->width;
                    y--;
                    lrc_brpos++;
                }
            }
        }

        /* find first line to display */
        y -= nline[i];
        lrc_line = current.ll_head;
        while (y < -lrc_line->nline[i])
        {
            y += lrc_line->nline[i];
            lrc_line = lrc_line->next;
        }

        ypos += y*font_ui_height;
        while (lrc_line && ypos < vp_lyrics[i].height)
        {
            ypos = display_lrc_line(lrc_line, ypos, i);
            lrc_line = lrc_line->next;
        }
        if (!lrc_line && ypos < vp_lyrics[i].height)
            display->putsxy(0, ypos, "[end]");
#else /* HAVE_LCD_CHARCELLS */
        struct lrc_line *lrc_line = lrc_center;
        struct lrc_brpos *lrc_brpos = calc_brpos(lrc_line, i);
        long elapsed = 0;
        const char *str = get_lrc_str(lrc_line);
        int x = vp_lyrics[i].width/2, y = 0;

        if (rin >= 0 && len > 0)
        {
            elapsed = rin * lrc_center->width / len;
            while (elapsed > lrc_brpos->width)
            {
                elapsed -= lrc_brpos->width;
                str = lrc_skip_space(str+lrc_brpos->count);
                lrc_brpos++;
            }
        }
        rb->strlcpy(temp_buf, str, lrc_brpos->count+1);

        x -= elapsed;
        if (x < 0)
            display->puts(0, y, temp_buf + rb->utf8seek(temp_buf, -x));
        else
            display->puts(x, y, temp_buf);
        x += rb->utf8length(temp_buf)+1;
        lrc_line = lrc_line->next;
        if (!lrc_line && x < vp_lyrics[i].width)
        {
            if (x < vp_lyrics[i].width/2)
                x = vp_lyrics[i].width/2;
            display->puts(x, y, "[end]");
        }
#endif /* HAVE_LCD_BITMAP */
        display->update_viewport();
        display->set_viewport(NULL);
    }
}

/*******************************
 * Browse lyrics and edit time.
 *******************************/
/* point playing line in lyrics */
static enum themable_icons get_icon(int selected, void * data)
{
    (void) data;
    struct lrc_line *lrc_line = get_lrc_line(selected);
    if (lrc_line)
    {
        long time_start = get_time_start(lrc_line);
        long time_end = get_time_start(lrc_line->next);
        long elapsed = current.id3->elapsed;
        if (time_start <= elapsed && time_end > elapsed)
            return Icon_Moving;
    }
    return Icon_NOICON;
}
static const char *get_lrc_timeline(int selected, void *data,
                                    char *buffer, size_t buffer_len)
{
    (void) data;
    struct lrc_line *lrc_line = get_lrc_line(selected);
    if (lrc_line)
    {
        format_time_tag(temp_buf, get_time_start(lrc_line));
        rb->snprintf(buffer, buffer_len, "[%s]%s",
                     temp_buf, get_lrc_str(lrc_line));
        return buffer;
    }
    return NULL;
}

static void save_changes(void)
{
    char new_file[MAX_PATH], *p;
    bool success = false;
    int fd, fe;
    if (!current.changed_lrc)
        return;
    rb->splash(HZ/2, "Saving changes...");
    if (current.type == TXT || current.type > NUM_TYPES)
    {
        /* save changes to new .lrc file */
        rb->strcpy(new_file, current.lrc_file);
        p = rb->strrchr(new_file, '.');
        rb->strcpy(p, extentions[LRC]);
    }
    else
    {
        /* file already exists. use temp file. */
        rb->snprintf(new_file, MAX_PATH, "%s~", current.lrc_file);
    }
    fd = rb->creat(new_file, 0666);
    fe = rb->open(current.lrc_file, O_RDONLY);
    if (fd >= 0 && fe >= 0)
    {
        struct lrc_line *lrc_line, *temp_lrc;
        off_t curr = 0, next = 0, size = 0, offset = 0;
        for (lrc_line = current.ll_head; lrc_line;
            lrc_line = lrc_line->next)
        {
            /* apply offset and set old_time_start -1 to indicate
               that time tag is not saved yet. */
            lrc_line->time_start = get_time_start(lrc_line);
            lrc_line->old_time_start = -1;
        }
        current.offset = 0;
        if (current.type > NUM_TYPES)
        {
            curr = -1;
            rb->write(fd, BOM, BOM_SIZE);
        }
        else
            size = rb->filesize(fe);
        while (curr < size)
        {
            /* find offset of next tag */
            lrc_line = NULL;
            for (temp_lrc = current.ll_head, next = size;
                temp_lrc; temp_lrc = temp_lrc->next)
            {
                offset = temp_lrc->file_offset;
                if (offset < next && temp_lrc->old_time_start == -1)
                {
                    lrc_line = temp_lrc;
                    next = offset;
                    if (offset <= curr) break;
                }
            }
            offset = current.offset_file_offset;
            if (offset >= 0 && offset < next)
            {
                lrc_line = NULL;
                next = offset;
                current.offset_file_offset = -1;
            }
            if (next > curr)
            {
                if (curr == -1) curr = 0;
                /* copy before the next tag */
                while (curr < next)
                {
                    ssize_t count = next-curr;
                    if (count > MAX_LINE_LEN)
                        count = MAX_LINE_LEN;
                    if (rb->read(fe, temp_buf, count)!=count)
                        break;
                    rb->write(fd, temp_buf, count);
                    curr += count;
                }
                if (curr < next || curr >= size) break;
            }
            /* write tag to new file and skip tag in backup */
            if (lrc_line != NULL)
            {
                lrc_line->file_offset = rb->lseek(fd, 0, SEEK_CUR);
                lrc_line->old_time_start = lrc_line->time_start;
                long t = lrc_line->time_start;
                if (current.type == SNC)
                {
                    rb->fdprintf(fd, "%02ld%02ld%02ld%02ld", (t/3600000)%100,
                                     (t/60000)%60, (t/1000)%60, (t/10)%100);
                    /* skip time tag */
                    curr += rb->read(fe, temp_buf, 8);
                }
                else /* LRC || LRC8 */
                {
                    format_time_tag(temp_buf, t);
                    rb->fdprintf(fd, "[%s]", temp_buf);
                }
                if (next == -1)
                {
                    rb->fdprintf(fd, "%s\n", get_lrc_str(lrc_line));
                }
            }
            if (current.type == LRC || current.type == LRC8)
            {
                /* skip both time tag and offset tag */
                while (curr++<size && rb->read(fe, temp_buf, 1)==1)
                    if (temp_buf[0]==']') break;
            }
        }
        success = (curr>=size);
    }
    if (fe >= 0) rb->close(fe);
    if (fd >= 0) rb->close(fd);

    if (success)
    {
        if (current.type == TXT || current.type > NUM_TYPES)
        {
            current.type = LRC;
            rb->strcpy(current.lrc_file, new_file);
        }
        else
        {
            rb->remove(current.lrc_file);
            rb->rename(new_file, current.lrc_file);
        }
    }
    else
    {
        rb->remove(new_file);
        rb->splash(HZ, "Could not save changes.");
    }
    rb->reload_directory();
    current.changed_lrc = false;
}
static int timetag_editor(void)
{
    struct gui_synclist gui_editor;
    struct lrc_line *lrc_line;
    bool exit = false;
    int button, idx, selected = 0;

    if (current.id3 == NULL || !current.ll_head)
    {
        rb->splash(HZ, "No lyrics");
        return LRC_GOTO_MAIN;
    }

    get_lrc_line(-1); /* initialize static variables */

    for (lrc_line = current.ll_head, idx = 0;
        lrc_line; lrc_line = lrc_line->next, idx++)
    {
        long time_start = get_time_start(lrc_line);
        long time_end = get_time_start(lrc_line->next);
        long elapsed = current.id3->elapsed;
        if (time_start <= elapsed && time_end > elapsed)
            selected = idx;
    }

    rb->gui_synclist_init(&gui_editor, &get_lrc_timeline, NULL, false, 1, NULL);
    rb->gui_synclist_set_nb_items(&gui_editor, current.nlrcline);
    rb->gui_synclist_set_icon_callback(&gui_editor, get_icon);
    rb->gui_synclist_set_title(&gui_editor, "Timetag Editor",
                               Icon_Menu_functioncall);
    rb->gui_synclist_select_item(&gui_editor, selected);
    rb->gui_synclist_draw(&gui_editor);

    while (!exit)
    {
        button = rb->get_action(CONTEXT_TREE, TIMEOUT_BLOCK);
        if (rb->gui_synclist_do_button(&gui_editor, &button,
                                      LIST_WRAP_UNLESS_HELD))
            continue;

        switch (button)
        {
            case ACTION_STD_OK:
                idx = rb->gui_synclist_get_sel_pos(&gui_editor);
                lrc_line = get_lrc_line(idx);
                if (lrc_line)
                {
                    set_time_start(lrc_line, current.id3->elapsed-500);
                    rb->gui_synclist_draw(&gui_editor);
                }
                break;
            case ACTION_STD_CONTEXT:
                idx = rb->gui_synclist_get_sel_pos(&gui_editor);
                lrc_line = get_lrc_line(idx);
                if (lrc_line)
                {
                    long temp_time = get_time_start(lrc_line);
                    if (lrc_set_time(get_lrc_str(lrc_line), NULL,
                                     &temp_time, 10, 0, current.length,
                                     LST_SET_MSEC|LST_SET_SEC|LST_SET_MIN) == 1)
                        return PLUGIN_USB_CONNECTED;
                    set_time_start(lrc_line, temp_time);
                    rb->gui_synclist_draw(&gui_editor);
                }
                break;
            case ACTION_TREE_STOP:
            case ACTION_STD_CANCEL:
                exit = true;
                break;
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
    }

    FOR_NB_SCREENS(idx)
        rb->screens[idx]->stop_scroll();

    if (current.changed_lrc)
    {
        MENUITEM_STRINGLIST(save_menu, "Save Changes?", NULL,
                            "Yes", "No (save later)", "Discard All Changes")
        button = 0;
        exit = false;
        while (!exit)
        {
            switch (rb->do_menu(&save_menu, &button, NULL, false))
            {
                case 0:
                    sort_lrcs();
                    save_changes();
                    exit = true;
                    break;
                case 1:
                    sort_lrcs();
                    exit = true;
                    break;
                case 2:
                    init_time_tag();
                    exit = true;
                    break;
                case MENU_ATTACHED_USB:
                    return PLUGIN_USB_CONNECTED;
            }
        }
    }
    return LRC_GOTO_MAIN;
}

/*******************************
 * Settings.
 *******************************/
static void load_or_save_settings(bool save)
{
    static const char config_file[] = "lrcplayer.cfg";
    static struct configdata config[] = {
#ifdef HAVE_LCD_COLOR
        { TYPE_INT, 0, 0xffffff, { .int_p = &prefs.inactive_color },
            "inactive color", NULL },
#endif
#ifdef HAVE_LCD_BITMAP
        { TYPE_BOOL, 0, 1, { .bool_p = &prefs.wrap }, "wrap", NULL },
        { TYPE_BOOL, 0, 1, { .bool_p = &prefs.wipe }, "wipe", NULL },
        { TYPE_BOOL, 0, 1, { .bool_p = &prefs.active_one_line },
            "active one line", NULL },
        { TYPE_INT,  0, 2, { .int_p =  &prefs.align }, "align", NULL },
        { TYPE_BOOL, 0, 1, { .bool_p = &prefs.statusbar_on },
            "statusbar on", NULL },
        { TYPE_BOOL, 0, 1, { .bool_p = &prefs.display_title },
            "display title", NULL },
#endif
        { TYPE_BOOL, 0, 1, { .bool_p = &prefs.display_time },
            "display time", NULL },
        { TYPE_BOOL, 0, 1, { .bool_p = &prefs.backlight_on },
            "backlight on", NULL },

        { TYPE_STRING, 0, sizeof(prefs.lrc_directory),
            { .string = prefs.lrc_directory }, "lrc directory", NULL },
        { TYPE_INT,  -1, NUM_CODEPAGES-1, { .int_p = &prefs.encoding },
            "encoding", NULL },
#ifdef LRC_SUPPORT_ID3
        { TYPE_BOOL, 0, 1, { .bool_p = &prefs.read_id3 }, "read id3", NULL },
#endif
    };

    if (!save)
    {
        /* initialize setting */
#if LCD_DEPTH > 1
        prefs.active_color = rb->lcd_get_foreground();
        prefs.inactive_color = LCD_LIGHTGRAY;
#endif
#ifdef HAVE_LCD_BITMAP
        prefs.wrap = true;
        prefs.wipe = true;
        prefs.active_one_line = false;
        prefs.align = 1; /* center */
        prefs.statusbar_on = false;
        prefs.display_title = true;
#endif
        prefs.display_time = true;
        prefs.backlight_on = false;
#ifdef LRC_SUPPORT_ID3
        prefs.read_id3 = true;
#endif
        rb->strcpy(prefs.lrc_directory, "/Lyrics");
        prefs.encoding = -1; /* default codepage */

        configfile_load(config_file, config, ARRAYLEN(config), 0);
    }
    else if (rb->memcmp(&old_prefs, &prefs, sizeof(prefs)))
    {
        rb->splash(0, "Saving Settings");
        configfile_save(config_file, config, ARRAYLEN(config), 0);
    }
    rb->memcpy(&old_prefs, &prefs, sizeof(prefs));
}

static bool lrc_theme_menu(void)
{
    enum {
#ifdef HAVE_LCD_BITMAP
        LRC_MENU_STATUSBAR,
        LRC_MENU_DISP_TITLE,
#endif
        LRC_MENU_DISP_TIME,
#ifdef HAVE_LCD_COLOR
        LRC_MENU_INACTIVE_COLOR,
#endif
        LRC_MENU_BACKLIGHT,
    };

    int selected = 0;
    bool exit = false, usb = false;

    MENUITEM_STRINGLIST(menu, "Theme Settings", NULL,
#ifdef HAVE_LCD_BITMAP
                        "Show Statusbar", "Display Title",
#endif
                        "Display Time",
#ifdef HAVE_LCD_COLOR
                        "Inactive Colour",
#endif
                        "Backlight Always On");

    while (!exit && !usb)
    {
        switch (rb->do_menu(&menu, &selected, NULL, false))
        {
#ifdef HAVE_LCD_BITMAP
            case LRC_MENU_STATUSBAR:
                usb = rb->set_bool("Show Statusbar", &prefs.statusbar_on);
                break;
            case LRC_MENU_DISP_TITLE:
                usb = rb->set_bool("Display Title", &prefs.display_title);
                break;
#endif
            case LRC_MENU_DISP_TIME:
                usb = rb->set_bool("Display Time", &prefs.display_time);
                break;
#ifdef HAVE_LCD_COLOR
            case LRC_MENU_INACTIVE_COLOR:
                usb = rb->set_color(NULL, "Inactive Colour",
                                    &prefs.inactive_color, -1);
                break;
#endif
            case LRC_MENU_BACKLIGHT:
                usb = rb->set_bool("Backlight Always On", &prefs.backlight_on);
                break;
            case MENU_ATTACHED_USB:
                usb = true;
                break;
            default:
                exit = true;
                break;
        }
    }

    return usb;
}

#ifdef HAVE_LCD_BITMAP
static bool lrc_display_menu(void)
{
    enum {
        LRC_MENU_WRAP,
        LRC_MENU_WIPE,
        LRC_MENU_ALIGN,
        LRC_MENU_LINE_MODE,
    };

    int selected = 0;
    bool exit = false, usb = false;

    MENUITEM_STRINGLIST(menu, "Display Settings", NULL,
                        "Wrap", "Wipe", "Alignment",
                        "Activate Only Current Line");

    struct opt_items align_names[] = {
        {"Left", -1}, {"Centre", -1}, {"Right", -1},
    };

    while (!exit && !usb)
    {
        switch (rb->do_menu(&menu, &selected, NULL, false))
        {
            case LRC_MENU_WRAP:
                usb = rb->set_bool("Wrap", &prefs.wrap);
                break;
            case LRC_MENU_WIPE:
                usb = rb->set_bool("Wipe", &prefs.wipe);
                break;
            case LRC_MENU_ALIGN:
                usb = rb->set_option("Alignment", &prefs.align, INT,
                                     align_names, 3, NULL);
                break;
            case LRC_MENU_LINE_MODE:
                usb = rb->set_bool("Activate Only Current Line",
                                        &prefs.active_one_line);
                break;
            case MENU_ATTACHED_USB:
                usb = true;
                break;
            default:
                exit = true;
                break;
        }
    }

    return usb;
}
#endif /* HAVE_LCD_BITMAP */

static bool lrc_lyrics_menu(void)
{
    enum {
        LRC_MENU_ENCODING,
#ifdef LRC_SUPPORT_ID3
        LRC_MENU_READ_ID3,
#endif
        LRC_MENU_LRC_DIR,
    };

    int selected = 0;
    bool exit = false, usb = false;

    struct opt_items cp_names[NUM_CODEPAGES+1];
    int old_val;

    MENUITEM_STRINGLIST(menu, "Lyrics Settings", NULL,
                        "Encoding",
#ifdef LRC_SUPPORT_ID3
                        "Read ID3 tag",
#endif
                        "Lrc Directry");

    cp_names[0].string = "Use default codepage";
    cp_names[0].voice_id = -1;
    for (old_val = 1; old_val < NUM_CODEPAGES+1; old_val++)
    {
        cp_names[old_val].string = rb->get_codepage_name(old_val-1);
        cp_names[old_val].voice_id = -1;
    }

    while (!exit && !usb)
    {
        switch (rb->do_menu(&menu, &selected, NULL, false))
        {
            case LRC_MENU_ENCODING:
                prefs.encoding++;
                old_val = prefs.encoding;
                usb = rb->set_option("Encoding", &prefs.encoding, INT,
                                     cp_names, NUM_CODEPAGES+1, NULL);
                if (prefs.encoding != old_val)
                {
                    save_changes();
                    if (current.type < NUM_TYPES)
                    {
                        /* let reload lrc file to apply encoding setting */
                        reset_current_data();
                    }
                }
                prefs.encoding--;
                break;
#ifdef LRC_SUPPORT_ID3
            case LRC_MENU_READ_ID3:
                usb = rb->set_bool("Read ID3 tag", &prefs.read_id3);
                break;
#endif
            case LRC_MENU_LRC_DIR:
                rb->strcpy(temp_buf, prefs.lrc_directory);
                if (!rb->kbd_input(temp_buf, sizeof(prefs.lrc_directory)))
                    rb->strcpy(prefs.lrc_directory, temp_buf);
                break;
            case MENU_ATTACHED_USB:
                usb = true;
                break;
            default:
                exit = true;
                break;
        }
    }

    return usb;
}

#ifdef LRC_DEBUG
static const char* lrc_debug_data(int selected, void * data,
                                  char * buffer, size_t buffer_len)
{
    (void)data;
    switch (selected)
    {
        case 0:
            rb->strlcpy(buffer, current.mp3_file, buffer_len);
            break;
        case 1:
            rb->strlcpy(buffer, current.lrc_file, buffer_len);
            break;
        case 2:
            rb->snprintf(buffer, buffer_len, "buf usage: %d,%d/%d",
                            (int)lrc_buffer_used, (int)lrc_buffer_end,
                            (int)lrc_buffer_size);
            break;
        case 3:
            rb->snprintf(buffer, buffer_len, "line count: %d,%d",
                            current.nlrcline, current.nlrcbrpos);
            break;
        case 4:
            rb->snprintf(buffer, buffer_len, "loaded lrc? %s",
                            current.loaded_lrc?"yes":"no");
            break;
        case 5:
            rb->snprintf(buffer, buffer_len, "too many lines? %s",
                            current.too_many_lines?"yes":"no");
            break;
        default:
            return NULL;
    }
    return buffer;
}

static bool lrc_debug_menu(void)
{
    struct simplelist_info info;
    rb->simplelist_info_init(&info, "Debug Menu", 6, NULL);
    info.hide_selection = true;
    info.scroll_all = true;
    info.get_name = lrc_debug_data;
    return rb->simplelist_show_list(&info);
}
#endif

/* returns one of enum lrc_screen or enum plugin_status */
static int lrc_menu(void)
{
    enum {
        LRC_MENU_THEME,
#ifdef HAVE_LCD_BITMAP
        LRC_MENU_DISPLAY,
#endif
        LRC_MENU_LYRICS,
        LRC_MENU_PLAYBACK,
#ifdef LRC_DEBUG
        LRC_MENU_DEBUG,
#endif
        LRC_MENU_OFFSET,
        LRC_MENU_TIMETAG_EDITOR,
        LRC_MENU_QUIT,
    };

    MENUITEM_STRINGLIST(menu, "Lrcplayer Menu", NULL,
                        "Theme Settings",
#ifdef HAVE_LCD_BITMAP
                        "Display Settings",
#endif
                        "Lyrics Settings",
                        "Playback Control",
#ifdef LRC_DEBUG
                        "Debug Menu",
#endif
                        "Time Offset", "Timetag Editor",
                        "Quit");
    int selected = 0, ret = LRC_GOTO_MENU;
    bool usb = false;

    while (ret == LRC_GOTO_MENU)
    {
        switch (rb->do_menu(&menu, &selected, NULL, false))
        {
            case LRC_MENU_THEME:
                usb = lrc_theme_menu();
                break;
#ifdef HAVE_LCD_BITMAP
            case LRC_MENU_DISPLAY:
                usb = lrc_display_menu();
                break;
#endif
            case LRC_MENU_LYRICS:
                usb = lrc_lyrics_menu();
                break;
            case LRC_MENU_PLAYBACK:
                usb = playback_control(NULL);
                ret = LRC_GOTO_MAIN;
                break;
#ifdef LRC_DEBUG
            case LRC_MENU_DEBUG:
                usb = lrc_debug_menu();
                ret = LRC_GOTO_MAIN;
                break;
#endif
            case LRC_MENU_OFFSET:
                usb = (lrc_set_time("Time Offset", "sec", &current.offset,
                                    10, -60*1000, 60*1000,
                                    LST_SET_MSEC|LST_SET_SEC) == 1);
                ret = LRC_GOTO_MAIN;
                break;
            case LRC_MENU_TIMETAG_EDITOR:
                ret = LRC_GOTO_EDITOR;
                break;
            case LRC_MENU_QUIT:
                ret = PLUGIN_OK;
                break;
            case MENU_ATTACHED_USB:
                usb = true;
                break;
            default:
                ret = LRC_GOTO_MAIN;
                break;
        }
        if (usb)
            ret = PLUGIN_USB_CONNECTED;
    }
    return ret;
}

/*******************************
 * Main.
 *******************************/
/* returns true if song has changed to know when to load new lyrics. */
static bool check_audio_status(void)
{
    static int last_audio_status = 0;
    if (current.ff_rewind == -1)
        current.audio_status = rb->audio_status();
    current.id3 = rb->audio_current_track();
    if ((last_audio_status^current.audio_status)&AUDIO_STATUS_PLAY)
    {
        last_audio_status = current.audio_status;
        return true;
    }
    if (AUDIO_STOP || current.id3 == NULL)
        return false;
    if (rb->strcmp(current.mp3_file, current.id3->path))
    {
        return true;
    }
    return false;
}
static void ff_rewind(long time, bool resume)
{
    if (AUDIO_PLAY)
    {
        if (!AUDIO_PAUSE)
        {
            resume = true;
            rb->audio_pause();
        }
        rb->audio_ff_rewind(time);
        rb->sleep(HZ/10); /* take affect seeking */
        if (resume)
            rb->audio_resume();
    }
}

static int handle_button(void)
{
    int ret = LRC_GOTO_MAIN;
    static int step = 0;
    int limit, button = rb->get_action(CONTEXT_WPS, HZ/10);
    switch (button)
    {
        case ACTION_WPS_BROWSE:
#if CONFIG_KEYPAD == ONDIO_PAD
            /* ondio doesn't have ACTION_WPS_MENU,
               so use ACTION_WPS_BROWSE for menu */
            ret = LRC_GOTO_MENU;
            break;
#endif
        case ACTION_WPS_STOP:
            save_changes();
            ret = PLUGIN_OK;
            break;
        case ACTION_WPS_PLAY:
            if (AUDIO_STOP && rb->global_status->resume_index != -1)
            {
                if (rb->playlist_resume() != -1)
                {
                    rb->playlist_start(rb->global_status->resume_index,
                        rb->global_status->resume_offset);
                }
            }
            else if (AUDIO_PAUSE)
                rb->audio_resume();
            else
                rb->audio_pause();
            break;
        case ACTION_WPS_SEEKFWD:
        case ACTION_WPS_SEEKBACK:
            if (AUDIO_STOP)
                break;
            if (current.ff_rewind > -1)
            {
                if (button == ACTION_WPS_SEEKFWD)
                    /* fast forwarding, calc max step relative to end */
                    limit = (current.length - current.ff_rewind) * 3 / 100;
                else
                    /* rewinding, calc max step relative to start */
                    limit = (current.ff_rewind) * 3 / 100;
                limit = MAX(limit, 500);

                if (step > limit)
                    step = limit;

                if (button == ACTION_WPS_SEEKFWD)
                    current.ff_rewind += step;
                else
                    current.ff_rewind -= step;

                if (current.ff_rewind > current.length-100)
                    current.ff_rewind = current.length-100;
                if (current.ff_rewind < 0)
                    current.ff_rewind = 0;

                /* smooth seeking by multiplying step by: 1 + (2 ^ -accel) */
                step += step >> (rb->global_settings->ff_rewind_accel + 3);
            }
            else
            {
                current.ff_rewind = current.elapsed;
                if (!AUDIO_PAUSE)
                    rb->audio_pause();
                step = 1000 * rb->global_settings->ff_rewind_min_step;
            }
            break;
        case ACTION_WPS_STOPSEEK:
            if (current.ff_rewind == -1)
                break;
            ff_rewind(current.ff_rewind, !AUDIO_PAUSE);
            current.elapsed = current.ff_rewind;
            current.ff_rewind = -1;
            break;
        case ACTION_WPS_SKIPNEXT:
            rb->audio_next();
            break;
        case ACTION_WPS_SKIPPREV:
            if (current.elapsed < 3000)
                rb->audio_prev();
            else
                ff_rewind(0, false);
            break;
        case ACTION_WPS_VOLDOWN:
            limit = rb->sound_min(SOUND_VOLUME);
            if (--rb->global_settings->volume < limit)
                rb->global_settings->volume = limit;
            rb->sound_set(SOUND_VOLUME, rb->global_settings->volume);
            break;
        case ACTION_WPS_VOLUP:
            limit = rb->sound_max(SOUND_VOLUME);
            if (++rb->global_settings->volume > limit)
                rb->global_settings->volume = limit;
            rb->sound_set(SOUND_VOLUME, rb->global_settings->volume);
            break;
        case ACTION_WPS_CONTEXT:
            ret = LRC_GOTO_EDITOR;
            break;
        case ACTION_WPS_MENU:
            ret = LRC_GOTO_MENU;
            break;
        default:
            if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                ret = PLUGIN_USB_CONNECTED;
            break;
    }
    return ret;
}

static int lrc_main(void)
{
    int ret = LRC_GOTO_MAIN;
    long id3_timeout = 0;
    bool update_display_state = true;

#ifdef HAVE_LCD_BITMAP
    /* y offset of vp_lyrics */
    int h = (prefs.display_title?font_ui_height:0)+
            (prefs.display_time?SYSFONT_HEIGHT*2:0);

#endif

    FOR_NB_SCREENS(i)
    {
#ifdef HAVE_LCD_BITMAP
        rb->viewportmanager_theme_enable(i, prefs.statusbar_on, &vp_info[i]);
        vp_lyrics[i] = vp_info[i];
        vp_lyrics[i].flags &= ~VP_FLAG_ALIGNMENT_MASK;
        vp_lyrics[i].y += h;
        vp_lyrics[i].height -= h;
#else
        rb->viewport_set_defaults(&vp_lyrics[i], i);
        if (prefs.display_time)
        {
            vp_lyrics[i].y += 1; /* time */
            vp_lyrics[i].height -= 1;
        }
#endif
    }

    if (prefs.backlight_on)
        backlight_ignore_timeout();

#ifdef HAVE_LCD_BITMAP
    /* in case settings that may affect break position 
     * are changed (statusbar_on and wrap). */
    if (!current.too_many_lines)
        calc_brpos(NULL, 0);
#endif

    while (ret == LRC_GOTO_MAIN)
    {
        if (check_audio_status())
        {
            update_display_state = true;
            if (AUDIO_STOP)
            {
                current.id3 = NULL;
                id3_timeout = 0;
            }
            else if (rb->strcmp(current.mp3_file, current.id3->path))
            {
                save_changes();
                reset_current_data();
                rb->strcpy(current.mp3_file, current.id3->path);
                id3_timeout = *rb->current_tick+HZ*3;
                current.found_lrc = false;
            }
        }
        if (current.id3 && current.id3->length)
        {
            if (current.ff_rewind == -1)
            {
                long di = current.id3->elapsed - current.elapsed;
                if (di < -250 || di > 0)
                    current.elapsed = current.id3->elapsed;
            }
            else
                current.elapsed = current.ff_rewind;
            current.length = current.id3->length;
            if (current.elapsed > current.length)
                current.elapsed = current.length;
        }
        else
        {
            current.elapsed = 0;
            current.length =  1;
        }

        if (current.id3 && id3_timeout &&
            (TIME_AFTER(*rb->current_tick, id3_timeout) ||
                current.id3->artist))
        {
            update_display_state = true;
            id3_timeout = 0;

            current.found_lrc = find_lrc_file();
#ifdef LRC_SUPPORT_ID3
            if (!current.found_lrc && prefs.read_id3)
            {
                /* no lyrics file found. try to read from id3 tag. */
                current.found_lrc = read_id3();
            }
#endif
        }
        else if (current.found_lrc && !current.loaded_lrc)
        {
            /* current.loaded_lrc is false after changing encode setting */
            update_display_state = true;
            display_state();
            load_lrc_file();
        }
        if (update_display_state)
        {
#ifdef HAVE_LCD_BITMAP
            if (current.type == TXT || current.type == ID3_USLT)
                current.wipe = false;
            else
                current.wipe = prefs.wipe;
#endif
            display_state();
            update_display_state = false;
        }
        if (AUDIO_PLAY)
        {
            if (prefs.display_time)
                display_time();
            if (!id3_timeout)
                display_lrcs();
        }

        ret = handle_button();
    }

#ifdef HAVE_LCD_BITMAP
    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_undo(i, false);
#endif
    if (prefs.backlight_on)
        backlight_use_settings();

    return ret;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    int ret = LRC_GOTO_MAIN;

    /* initialize settings. */
    load_or_save_settings(false);

#ifdef HAVE_LCD_BITMAP
    uifont = rb->screens[0]->getuifont();
    font_ui_height = rb->font_get(uifont)->height;
#endif

    lrc_buffer = rb->plugin_get_buffer(&lrc_buffer_size);
    lrc_buffer = (void *)(((long)lrc_buffer+3)&~3); /* 4 bytes aligned */
    lrc_buffer_size = (lrc_buffer_size - 4)&~3;

    reset_current_data();
    current.id3 = NULL;
    current.mp3_file[0] = 0;
    current.lrc_file[0] = 0;
    current.ff_rewind = -1;
    current.found_lrc = false;
    if (parameter && check_audio_status())
    {
        const char *ext;
        rb->strcpy(current.mp3_file, current.id3->path);
        /* use passed parameter as lrc file. */
        rb->strcpy(current.lrc_file, parameter);
        if (!rb->file_exists(current.lrc_file))
        {
            rb->splash(HZ, "Specified file dose not exist.");
            return PLUGIN_ERROR;
        }
        ext = rb->strrchr(current.lrc_file, '.');
        if (!ext) ext = current.lrc_file;
        for (current.type = 0; current.type < NUM_TYPES; current.type++)
        {
            if (!rb->strcasecmp(ext, extentions[current.type]))
                break;
        }
        if (current.type == NUM_TYPES)
        {
            rb->splashf(HZ, "%s is not supported", ext);
            return PLUGIN_ERROR;
        }
        current.found_lrc = true;
    }

    while (ret >= PLUGIN_OTHER)
    {
        switch (ret)
        {
            case LRC_GOTO_MAIN:
                ret = lrc_main();
                break;
            case LRC_GOTO_MENU:
                ret = lrc_menu();
                break;
            case LRC_GOTO_EDITOR:
                ret = timetag_editor();
                break;
            default:
                ret = PLUGIN_ERROR;
                break;
        }
    }

    load_or_save_settings(true);
    return ret;
}
