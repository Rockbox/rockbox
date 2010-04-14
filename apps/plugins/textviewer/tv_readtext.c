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
#include "tv_readtext.h"

#define WRAP_TRIM          44  /* Max number of spaces to trim (arbitrary) */
#define NARROW_MAX_COLUMNS 64  /* Max displayable string len [narrow] (over-estimate) */
#define WIDE_MAX_COLUMNS  128  /* Max displayable string len [wide] (over-estimate) */
#define MAX_WIDTH         910  /* Max line length in WIDE mode */
#define READ_PREV_ZONE    (block_size*9/10)  /* Arbitrary number less than SMALL_BLOCK_SIZE */
#define SMALL_BLOCK_SIZE  block_size /* Smallest file chunk we will read */
#define LARGE_BLOCK_SIZE  (block_size << 1) /* Preferable size of file chunk to read */
#define TOP_SECTOR     buffer
#define MID_SECTOR     (buffer + SMALL_BLOCK_SIZE)
#define BOTTOM_SECTOR  (buffer + (SMALL_BLOCK_SIZE << 1))
#undef SCROLLBAR_WIDTH
#define SCROLLBAR_WIDTH rb->global_settings->scrollbar_width
#define MAX_PAGE         9999

/* Out-Of-Bounds test for any pointer to data in the buffer */
#define BUFFER_OOB(p)    ((p) < buffer || (p) >= buffer_end)

/* Does the buffer contain the beginning of the file? */
#define BUFFER_BOF()     (file_pos==0)

/* Does the buffer contain the end of the file? */
#define BUFFER_EOF()     (file_size-file_pos <= buffer_size)

/* Formula for the endpoint address outside of buffer data */
#define BUFFER_END() \
 ((BUFFER_EOF()) ? (file_size-file_pos+buffer) : (buffer+buffer_size))

/* Is the entire file being shown in one screen? */
#define ONE_SCREEN_FITS_ALL() \
 (next_screen_ptr==NULL && screen_top_ptr==buffer && BUFFER_BOF())

#define ADVANCE_COUNTERS(c) { width += glyph_width(c); k++; }
#define LINE_IS_FULL ((k>=max_columns-1) ||( width >= max_width))
#define LINE_IS_NOT_FULL ((k<max_columns-1) &&( width < max_width))


static unsigned char file_name[MAX_PARH+1];
static int fd = -1;

static unsigned char *buffer;
static long buffer_size;
static long block_size = 0x1000;

void viewer_init_buffer(void)
{
    /* get the plugin buffer */
    buffer = rb->plugin_get_buffer((size_t *)&buffer_size);
    if (buffer_size == 0)
    {
        rb->splash(HZ, "buffer does not allocate !!");
        return PLUGIN_ERROR;
    }
    block_size = buffer_size / 3;
    buffer_size = 3 * block_size;
}

static unsigned char* crop_at_width(const unsigned char* p)
{
    int k,width;
    unsigned short ch;
    const unsigned char *oldp = p;

    k=width=0;

    while (LINE_IS_NOT_FULL) {
        oldp = p;
        if (BUFFER_OOB(p))
           break;
        p = get_ucs(p, &ch);
        ADVANCE_COUNTERS(ch);
    }

    return (unsigned char*)oldp;
}

static unsigned char* find_first_feed(const unsigned char* p, int size)
{
    int i;

    for (i=0; i < size; i++)
        if (p[i] == 0)
            return (unsigned char*) p+i;

    return NULL;
}

static unsigned char* find_last_feed(const unsigned char* p, int size)
{
    int i;

    for (i=size-1; i>=0; i--)
        if (p[i] == 0)
            return (unsigned char*) p+i;

    return NULL;
}

static unsigned char* find_last_space(const unsigned char* p, int size)
{
    int i, j, k;

    k = (prefs.line_mode==JOIN) || (prefs.line_mode==REFLOW) ? 0:1;

    if (!BUFFER_OOB(&p[size]))
        for (j=k; j < ((int) sizeof(line_break)) - 1; j++)
            if (p[size] == line_break[j])
                return (unsigned char*) p+size;

    for (i=size-1; i>=0; i--)
        for (j=k; j < (int) sizeof(line_break); j++)
        {
            if (!((p[i] == '-') && (prefs.word_mode == WRAP)))
                if (p[i] == line_break[j])
                    return (unsigned char*) p+i;
        }

    return NULL;
}

static unsigned char* find_next_line(const unsigned char* cur_line, bool *is_short)
{
    const unsigned char *next_line = NULL;
    int size, i, j, k, width, search_len, spaces, newlines;
    bool first_chars;
    unsigned char c;

    if (is_short != NULL)
        *is_short = true;

    if BUFFER_OOB(cur_line)
        return NULL;

    if (prefs.view_mode == WIDE) {
        search_len = MAX_WIDTH;
    }
    else {   /* prefs.view_mode == NARROW */
        search_len = crop_at_width(cur_line) - cur_line;
    }

    size = BUFFER_OOB(cur_line+search_len) ? buffer_end-cur_line : search_len;

    if ((prefs.line_mode == JOIN) || (prefs.line_mode == REFLOW)) {
        /* Need to scan ahead and possibly increase search_len and size,
         or possibly set next_line at second hard return in a row. */
        next_line = NULL;
        first_chars=true;
        for (j=k=width=spaces=newlines=0; ; j++) {
            if (BUFFER_OOB(cur_line+j))
                return NULL;
            if (LINE_IS_FULL) {
                size = search_len = j;
                break;
            }

            c = cur_line[j];
            switch (c) {
                case ' ':
                    if (prefs.line_mode == REFLOW) {
                        if (newlines > 0) {
                            size = j;
                            next_line = cur_line + size;
                            return (unsigned char*) next_line;
                        }
                        if (j==0) /* i=1 is intentional */
                            for (i=0; i<par_indent_spaces; i++)
                                ADVANCE_COUNTERS(' ');
                    }
                    if (!first_chars) spaces++;
                    break;

                case 0:
                    if (newlines > 0) {
                        size = j;
                        next_line = cur_line + size - spaces;
                        if (next_line != cur_line)
                            return (unsigned char*) next_line;
                        break;
                    }

                    newlines++;
                    size += spaces -1;
                    if (BUFFER_OOB(cur_line+size) || size > 2*search_len)
                        return NULL;
                    search_len = size;
                    spaces = first_chars? 0:1;
                    break;

                default:
                    if (prefs.line_mode==JOIN || newlines>0) {
                        while (spaces) {
                            spaces--;
                            ADVANCE_COUNTERS(' ');
                            if (LINE_IS_FULL) {
                                size = search_len = j;
                                break;
                            }
                        }
                        newlines=0;
                   } else if (spaces) {
                        /* REFLOW, multiple spaces between words: count only
                         * one. If more are needed, they will be added
                         * while drawing. */
                        search_len = size;
                        spaces=0;
                        ADVANCE_COUNTERS(' ');
                        if (LINE_IS_FULL) {
                            size = search_len = j;
                            break;
                        }
                    }
                    first_chars = false;
                    ADVANCE_COUNTERS(c);
                    break;
            }
        }
    }
    else {
        /* find first hard return */
        next_line = find_first_feed(cur_line, size);
    }

    if (next_line == NULL)
        if (size == search_len) {
            if (prefs.word_mode == WRAP)  /* Find last space */
                next_line = find_last_space(cur_line, size);

            if (next_line == NULL)
                next_line = crop_at_width(cur_line);
            else
                if (prefs.word_mode == WRAP)
                    for (i=0;
                    i<WRAP_TRIM && isspace(next_line[0]) && !BUFFER_OOB(next_line);
                    i++)
                        next_line++;
        }

    if (prefs.line_mode == EXPAND)
        if (!BUFFER_OOB(next_line))  /* Not Null & not out of bounds */
            if (next_line[0] == 0)
                if (next_line != cur_line)
                    return (unsigned char*) next_line;

    /* If next_line is pointing to a zero, increment it; i.e.,
     leave the terminator at the end of cur_line. If pointing
     to a hyphen, increment only if there is room to display
     the hyphen on current line (won't apply in WIDE mode,
     since it's guarenteed there won't be room). */
    if (!BUFFER_OOB(next_line))  /* Not Null & not out of bounds */
        if (next_line[0] == 0)/* ||
        (next_line[0] == '-' && next_line-cur_line < draw_columns)) */
            next_line++;

    if (BUFFER_OOB(next_line))
    {
        if (BUFFER_EOF() && next_line != cur_line)
            return (unsigned char*) next_line;
        return NULL;
    }

    if (is_short)
        *is_short = false;

    return (unsigned char*) next_line;
}

static unsigned char* find_prev_line(const unsigned char* cur_line)
{
    const unsigned char *prev_line = NULL;
    const unsigned char *p;

    if BUFFER_OOB(cur_line)
        return NULL;

    /* To wrap consistently at the same places, we must
     start with a known hard return, then work downwards.
     We can either search backwards for a hard return,
     or simply start wrapping downwards from top of buffer.
       If current line is not near top of buffer, this is
     a file with long lines (paragraphs). We would need to
     read earlier sectors before we could decide how to
     properly wrap the lines above the current line, but
     it probably is not worth the disk access. Instead,
     start with top of buffer and wrap down from there.
     This may result in some lines wrapping at different
     points from where they wrap when scrolling down.
       If buffer is at top of file, start at top of buffer. */

    if ((prefs.line_mode == JOIN) || (prefs.line_mode == REFLOW))
        prev_line = p = NULL;
    else
        prev_line = p = find_last_feed(buffer, cur_line-buffer-1);
        /* Null means no line feeds in buffer above current line. */

    if (prev_line == NULL)
        if (BUFFER_BOF() || cur_line - buffer > READ_PREV_ZONE)
            prev_line = p = buffer;
        /* (else return NULL and read previous block) */

    /* Wrap downwards until too far, then use the one before. */
    while (p != NULL && p < cur_line) {
        prev_line = p;
        p = find_next_line(prev_line, NULL);
    }

    if (BUFFER_OOB(prev_line))
        return NULL;

    return (unsigned char*) prev_line;
}

static void check_bom(void)
{
    unsigned char bom[BOM_SIZE];
    off_t orig = rb->lseek(fd, 0, SEEK_CUR);

    is_bom = false;

    rb->lseek(fd, 0, SEEK_SET);

    if (rb->read(fd, bom, BOM_SIZE) == BOM_SIZE)
        is_bom = !memcmp(bom, BOM, BOM_SIZE);

    rb->lseek(fd, orig, SEEK_SET);
}

static void fill_buffer(long pos, unsigned char* buf, unsigned size)
{
    /* Read from file and preprocess the data */
    /* To minimize disk access, always read on sector boundaries */
    unsigned numread, i;
    bool found_CR = false;
    off_t offset = rb->lseek(fd, pos, SEEK_SET);

    if (offset == 0 && prefs.encoding == UTF_8 && is_bom)
        rb->lseek(fd, BOM_SIZE, SEEK_SET);

    numread = rb->read(fd, buf, size);
    buf[numread] = 0;
    rb->button_clear_queue(); /* clear button queue */

    for(i = 0; i < numread; i++) {
        switch(buf[i]) {
            case '\r':
                if (mac_text) {
                    buf[i] = 0;
                }
                else {
                    buf[i] = ' ';
                    found_CR = true;
                }
                break;

            case '\n':
                buf[i] = 0;
                found_CR = false;
                break;

            case 0:  /* No break between case 0 and default, intentionally */
                buf[i] = ' ';
            default:
                if (found_CR) {
                    buf[i - 1] = 0;
                    found_CR = false;
                    mac_text = true;
                }
                break;
        }
    }
}

static int read_and_synch(int direction)
{
/* Read next (or prev) block, and reposition global pointers. */
/* direction: 1 for down (i.e., further into file), -1 for up */
    int move_size, move_vector, offset;
    unsigned char *fill_buf;

    if (direction == -1) /* up */ {
        move_size = SMALL_BLOCK_SIZE;
        offset = 0;
        fill_buf = TOP_SECTOR;
        rb->memcpy(BOTTOM_SECTOR, MID_SECTOR, SMALL_BLOCK_SIZE);
        rb->memcpy(MID_SECTOR, TOP_SECTOR, SMALL_BLOCK_SIZE);
    }
    else /* down */ {
        if (prefs.view_mode == WIDE) {
            /* WIDE mode needs more buffer so we have to read smaller blocks */
            move_size = SMALL_BLOCK_SIZE;
            offset = LARGE_BLOCK_SIZE;
            fill_buf = BOTTOM_SECTOR;
            rb->memcpy(TOP_SECTOR, MID_SECTOR, SMALL_BLOCK_SIZE);
            rb->memcpy(MID_SECTOR, BOTTOM_SECTOR, SMALL_BLOCK_SIZE);
        }
        else {
            move_size = LARGE_BLOCK_SIZE;
            offset = SMALL_BLOCK_SIZE;
            fill_buf = MID_SECTOR;
            rb->memcpy(TOP_SECTOR, BOTTOM_SECTOR, SMALL_BLOCK_SIZE);
        }
    }
    move_vector = direction * move_size;
    screen_top_ptr -= move_vector;
    file_pos += move_vector;
    buffer_end = BUFFER_END();  /* Update whenever file_pos changes */
    fill_buffer(file_pos + offset, fill_buf, move_size);
    return move_vector;
}

static void get_next_line_position(unsigned char **line_begin,
                                   unsigned char **line_end,
                                   bool *is_short)
{
    int resynch_move;

    *line_begin = *line_end;
    *line_end = find_next_line(*line_begin, is_short);

    if (*line_end == NULL && !BUFFER_EOF())
    {
        resynch_move = read_and_synch(1); /* Read block & move ptrs */
        *line_begin -= resynch_move;
        if (next_line_ptr > buffer)
            next_line_ptr -= resynch_move;

        *line_end = find_next_line(*line_begin, is_short);
    }
}

/* open, close, get file size functions */

bool viewer_open(const unsigned char *fname)
{
    if (fname == 0)
        return false;

    rb->strlcpy(file_name, fname, MAX_PATH+1);
    fd = rb->open(fname, O_RDONLY);
    return (fd >= 0);
}

void viewer_close(void)
{
    if (fd >= 0)
        rb->close(fd);
}

/* When a file is UTF-8 file with BOM, if prefs.encoding is UTF-8,
 * then file size decreases only BOM_SIZE.
 */
static void get_filesize(void)
{
    file_size = rb->filesize(fd);
    if (file_size == -1)
        return;

    if (prefs.encoding == UTF_8 && is_bom)
        file_size -= BOM_SIZE;
}
