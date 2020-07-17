/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Gilles Roux
 *               2003 Garrett Derner
 *               2010 Yoshihisa Uchida
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
#include "ctype.h"
#include "tv_preferences.h"
#include "tv_text_processor.h"

enum{
    TV_TEXT_UNKNOWN,
    TV_TEXT_MAC,
    TV_TEXT_UNIX,
    TV_TEXT_WIN,
};

/* the max characters of each blocks */
#define TV_MAX_CHARS_PER_BLOCK (LCD_WIDTH / 2 + 1)

#define TV_MAX_BLOCKS 5

static unsigned text_type = TV_TEXT_UNKNOWN;

static const unsigned char *end_ptr;

static unsigned short ucsbuf[TV_MAX_BLOCKS][TV_MAX_CHARS_PER_BLOCK];
static unsigned char  utf8buf[TV_MAX_CHARS_PER_BLOCK * (2 * 3)];
static unsigned char  *outbuf;

static int block_count;
static int block_width;

/* if this value is true, then  tv_create_line_text returns a blank line. */
static bool expand_extra_line = false;

/* when a line is divided, this value sets true. */
static bool is_break_line = false;

static unsigned short break_chars[] =
    {
        0,
        /* halfwidth characters */
        '\t', '\n', 0x0b, 0x0c, ' ', '!', ',', '-', '.', ':', ';', '?', 0xb7, 
        /* fullwidth characters */
        0x2010, /* hyphen */
        0x3000, /* fullwidth space */
        0x3001, /* ideographic comma */
        0x3002, /* ideographic full stop */
        0x30fb, /* katakana middle dot */
        0x30fc, /* katakana-hiragana prolonged sound mark */
        0xff01, /* fullwidth exclamation mark */
        0xff0c, /* fullwidth comma */
        0xff0d, /* fullwidth hyphen-minus */
        0xff0e, /* fullwidth full stop */
        0xff1a, /* fullwidth colon */
        0xff1b, /* fullwidth semicolon */
        0xff1f, /* fullwidth question mark */
    };

/* the characters which is not judged as space with isspace() */
static unsigned short extra_spaces[] = { 0, 0x3000 };

static int tv_glyph_width(int ch)
{
    if (ch == '\n')
        return 0;

    if (ch == 0)
        ch = ' ';

    /* the width of the diacritics charcter is 0 */
    if (rb->is_diacritic(ch, NULL))
        return 0;

    return rb->font_get_width(rb->font_get(preferences->font_id), ch);
}

static unsigned char *tv_get_ucs(const unsigned char *str, unsigned short *ch)
{
    int count = 1;
    unsigned char utf8_tmp[3];

    /* distinguish the text_type */
    if (*str == '\r')
    {
        if (text_type == TV_TEXT_WIN || text_type == TV_TEXT_UNKNOWN)
        {
            if (str + 1 < end_ptr && *(str+1) == '\n')
            {
                if (text_type == TV_TEXT_UNKNOWN)
                    text_type = TV_TEXT_WIN;

                *ch = '\n';
                return (unsigned char *)str + 2;
            }

            if (text_type == TV_TEXT_UNKNOWN)
                text_type = TV_TEXT_MAC;
        }
        *ch = (text_type == TV_TEXT_MAC)? '\n' : ' ';
        return (unsigned char *)str + 1;
    }
    else if (*str == '\n')
    {
        if (text_type == TV_TEXT_UNKNOWN)
            text_type = TV_TEXT_UNIX;

        *ch = (text_type == TV_TEXT_UNIX)? '\n' : ' ';
        return (unsigned char *)str + 1;
    }

    if (preferences->encoding == UTF_8)
        return (unsigned char*)rb->utf8decode(str, ch);

    if ((*str >= 0x80) &&
        ((preferences->encoding > SJIS) ||
         (preferences->encoding == SJIS && (*str <= 0xa0 || *str >= 0xe0))))
    {
        if (str + 1 >= end_ptr)
        {
            end_ptr = str;
            *ch = 0;
            return (unsigned char *)str;
        }
        count = 2;
    }

    rb->iso_decode(str, utf8_tmp, preferences->encoding, count);
    rb->utf8decode(utf8_tmp, ch);
    return (unsigned char *)str + count;
}

static void tv_decode2utf8(const unsigned short *ucs, int count)
{
    int i;

    for (i = 0; i < count; i++)
        outbuf = rb->utf8encode(ucs[i], outbuf);

    *outbuf = '\0';
}

static bool tv_is_line_break_char(unsigned short ch)
{
    size_t i;

    /* when the word mode is CHOP, all characters does not break line. */
    if (preferences->word_mode == WM_CHOP)
        return false;

    for (i = 0; i < sizeof(break_chars)/sizeof(unsigned short); i++)
    {
        if (break_chars[i] == ch)
            return true;
    }
    return false;
}

static bool tv_isspace(unsigned short ch)
{
    size_t i;

    if (ch < 128 && isspace(ch))
        return true;

    for (i = 0; i < sizeof(extra_spaces)/sizeof(unsigned short); i++)
    {
        if (extra_spaces[i] == ch)
            return true;
    }
    return false;
}

static bool tv_is_break_line_join_mode(const unsigned char *next_str)
{
    unsigned short ch;

    tv_get_ucs(next_str, &ch);
    return tv_isspace(ch);
}

static int tv_form_reflow_line(unsigned short *ucs, int chars)
{
    unsigned short new_ucs[TV_MAX_CHARS_PER_BLOCK];
    unsigned short *p = new_ucs;
    unsigned short ch;
    int i;
    int k;
    int expand_spaces;
    int indent_chars = 0;
    int nonspace_chars = 0;
    int nonspace_width = 0;
    int remain_spaces;
    int spaces = 0;
    int words_spaces;

    if (preferences->alignment == AL_LEFT)
    {
        while (chars > 0 && ucs[chars-1] == ' ')
            chars--;
    }

    if (chars == 0)
        return 0;

    while (ucs[indent_chars] == ' ')
        indent_chars++;

    for (i = indent_chars; i < chars; i++)
    {
        ch = ucs[i];
        if (ch == ' ')
            spaces++;
        else
        {
            nonspace_chars++;
            nonspace_width += tv_glyph_width(ch);
        }
    }

    if (spaces == 0)
        return chars;

    expand_spaces = (block_width - nonspace_width) / tv_glyph_width(' ') - indent_chars;
    if (indent_chars + nonspace_chars + expand_spaces > TV_MAX_CHARS_PER_BLOCK)
        expand_spaces = TV_MAX_CHARS_PER_BLOCK - indent_chars - nonspace_chars;

    words_spaces  = expand_spaces / spaces;
    remain_spaces = expand_spaces - words_spaces * spaces;

    for (i = 0; i < indent_chars; i++)
        *p++ = ' ';

    for ( ; i < chars; i++)
    {
        ch = ucs[i];
        *p++ = ch;
        if (ch == ' ')
        {
            for (k = ((remain_spaces > 0)? 0 : 1); k < words_spaces; k++)
                *p++ = ch;

            remain_spaces--;
        }
    }

    rb->memcpy(ucs, new_ucs, sizeof(unsigned short) * TV_MAX_CHARS_PER_BLOCK);
    return indent_chars + nonspace_chars + expand_spaces;
}

static void tv_align_right(int *block_chars)
{
    unsigned short *cur_text;
    unsigned short *prev_text;
    unsigned short ch;
    int cur_block = block_count - 1;
    int prev_block;
    int cur_chars;
    int prev_chars;
    int idx;
    int break_pos;
    int break_width = 0;
    int append_width;
    int width;

    while (cur_block > 0)
    {
        cur_text  = ucsbuf[cur_block];
        cur_chars = block_chars[cur_block];
        idx = cur_chars;
        width = 0;
        while(--idx >= 0)
            width += tv_glyph_width(cur_text[idx]);

        width = block_width - width;
        prev_block = cur_block - 1;

        do {
            prev_text  = ucsbuf[prev_block];
            prev_chars = block_chars[prev_block];

            idx = prev_chars;
            append_width = 0;
            break_pos = prev_chars;
            while (append_width < width && idx > 0)
            {
                ch = prev_text[--idx];
                if (tv_is_line_break_char(ch))
                {
                    break_pos = idx + 1;
                    break_width = append_width;
                }
                append_width += tv_glyph_width(ch);
            }
            if (append_width > width)
                idx++;

            if (idx == 0)
            {
                break_pos = 0;
                break_width = append_width;
            }

            if (break_pos < prev_chars)
                append_width = break_width;
            /* the case of
             *   (1) when the first character of the cur_text concatenates
             *       the last character of the prev_text.
             *   (2) the length of ucsbuf[block] is short (< 0.75 * block width)
             */
            else if (((!tv_isspace(*cur_text) && !tv_isspace(prev_text[prev_chars - 1])) ||
                     (4 * width >= 3 * block_width)))
            {
                break_pos = idx;
            }

            if (break_pos < prev_chars)
            {
                rb->memmove(cur_text + prev_chars - break_pos,
                            cur_text, block_chars[cur_block] * sizeof(unsigned short));
                rb->memcpy(cur_text, prev_text + break_pos,
                           (prev_chars - break_pos) * sizeof(unsigned short));

                block_chars[prev_block]  = break_pos;
                block_chars[cur_block ] += prev_chars - break_pos;
            }
        } while ((width -= append_width) > 0 && --prev_block >= 0);
        cur_block--;
    }
}

static int tv_parse_text(const unsigned char *src, unsigned short *ucs,
                             int *ucs_chars, bool is_indent)
{
    const unsigned char *cur  = src;
    const unsigned char *next = src;
    const unsigned char *line_break_ptr = NULL;
    const unsigned char *line_end_ptr   = NULL;
    unsigned short ch = 0;
    unsigned short prev_ch;
    int chars = 0;
    int gw;
    int line_break_width = 0;
    int line_end_chars   = 0;
    int width = 0;
    bool is_space = false;

    while (true) {
        cur = next;
        if (cur >= end_ptr)
        {
            line_end_ptr   = cur;
            line_end_chars = chars;
            is_break_line  = true;
            break;
        }

        prev_ch = ch;
        next = tv_get_ucs(cur, &ch);
        if (ch == '\n')
        {
            if (preferences->line_mode != LM_JOIN || tv_is_break_line_join_mode(next))
            {
                line_end_ptr   = next;
                line_end_chars = chars;
                is_break_line  = false;
                break;
            }

            if (preferences->word_mode == WM_CHOP || tv_isspace(prev_ch))
                continue;

            /*
             * when the line mode is JOIN and the word mode is WRAP,
             * the next character does not concatenate with the
             * previous character.
             */
            ch = ' ';
        }
        else if ((is_space = tv_isspace(ch)) == true)
        {
            /*
             * when the line mode is REFLOW:
             *     (1) spacelike character convert to ' '
             *     (2) plural spaces are collected to one
             */
            if (preferences->line_mode == LM_REFLOW)
            {
                ch = ' ';
                if (prev_ch == ch)
                    continue;
            }

            /* when the alignment is RIGHT, ignores indent spaces. */
            if (preferences->alignment == AL_RIGHT && is_indent)
                continue;
        }
        else
            is_indent = false;

        if (preferences->line_mode == LM_REFLOW && is_indent)
            gw = tv_glyph_width(ch) * preferences->indent_spaces;
        else
            gw = tv_glyph_width(ch);

        width += gw;
        if (width > block_width)
        {
            width -= gw;
            if (is_space)
            {
                line_end_ptr   = cur;
                line_end_chars = chars;
            }
            is_break_line = true;
            break;
        }

        if (preferences->line_mode != LM_REFLOW || !is_indent)
            ucs[chars++] = ch;
        else
        {
            unsigned char i;
            for (i = 0; i < preferences->indent_spaces; i++)
                ucs[chars++] = ch;
        }

        if (tv_is_line_break_char(ch))
        {
            line_break_ptr   = next;
            line_break_width = width;
            line_end_chars   = chars;
        }
        if (chars >= TV_MAX_CHARS_PER_BLOCK)
        {
            is_break_line = true;
            break;
        }
    }

    /* set the end position and character count */
    if (line_end_ptr == NULL)
    {
        /*
         * when the last line break position is too short (line length < 0.75 * block width),
         * the line is cut off at the position where it is closest to the displayed width.
         */
        if ((preferences->line_mode == LM_REFLOW && line_break_ptr == NULL) ||
            (4 * line_break_width < 3 * block_width))
        {
            line_end_ptr   = cur;
            line_end_chars = chars;
        }
        else
            line_end_ptr = line_break_ptr;
    }

    *ucs_chars = line_end_chars;
    return line_end_ptr - src;
}

int tv_create_formed_text(const unsigned char *src, ssize_t bufsize,
                              int block, bool is_multi, const unsigned char **dst)
{
    unsigned short ch;
    int chars[block_count];
    int i;
    int size = 0;
    bool is_indent;

    outbuf = utf8buf;
    *outbuf = '\0';

    for (i = 0; i < block_count; i++)
        chars[i] = 0;

    if (dst != NULL)
        *dst = utf8buf;

    if (preferences->line_mode == LM_EXPAND && (expand_extra_line = !expand_extra_line) == true)
        return 0;

    end_ptr = src + bufsize;

    tv_get_ucs(src, &ch);
    is_indent = (tv_isspace(ch) && !is_break_line);

    if (is_indent && preferences->line_mode == LM_REFLOW && preferences->indent_spaces == 0
                  && (expand_extra_line = !expand_extra_line) == true)
        return 0;

    for (i = 0; i < block_count; i++)
    {
        size += tv_parse_text(src + size, ucsbuf[i], &chars[i], is_indent);
        if (!is_break_line)
            break;

        is_indent = false;
    }

    if (dst != NULL)
    {
        if (preferences->alignment == AL_RIGHT)
            tv_align_right(chars);

        for (i = 0; i < block_count; i++)
        {
            if (i == block || (is_multi && i == block + 1))
            {
                if (is_break_line && preferences->line_mode == LM_REFLOW)
                    chars[i] = tv_form_reflow_line(ucsbuf[i], chars[i]);

                tv_decode2utf8(ucsbuf[i], chars[i]);
            }
        }
    }

    return size;
}

bool tv_init_text_processor(unsigned char **buf, size_t *size)
{
    /* unused : no need for dynamic buffer yet */
    (void)buf;
    (void)size;

    text_type = TV_TEXT_UNKNOWN;
    expand_extra_line = false;
    is_break_line = false;
    return true;
}

void tv_set_creation_conditions(int blocks, int width)
{
    block_count = blocks;
    block_width = width;
}
