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
#include "tv_bookmark.h"

/* text viewer bookmark functions */

enum
{
    BOOKMARK_LAST = 1,
    BOOKMARK_USER,
};

#ifndef HAVE_LCD_BITMAP
#define BOOKMARK_ICON       "\xee\x84\x81\x00"
#endif

#define BOOKMARK_SIZE       8
#define MAX_BOOKMARKS      10 /* user setting bookmarks + last read page */


struct bookmark_info bookmarks[MAX_BOOKMARKS];
static int bookmark_count;

static int bm_comp(const void *a, const void *b)
{
    struct bookmark_info *pa;
    struct bookmark_info *pb;

    pa = (struct bookmark_info*)a;
    pb = (struct bookmark_info*)b;

    if (pa->page != pb->page)
        return pa->page - pb->page;

    return pa->line - pb->line;
}

int viewer_find_bookmark(int page, int line)
{
    int i;

    for (i = 0; i < bookmark_count; i++)
    {
        if (bookmarks[i].page == page && bookmarks[i].line == line)
            return i;
    }
    return -1;
}

void viewer_add_bookmark(int page, int line)
{
    if (bookmark_count >= MAX_BOOKMARKS-1)
        return;

    bookmarks[bookmark_count].file_position
        = file_pos + screen_top_ptr - buffer;
    bookmarks[bookmark_count].page = page;
    bookmarks[bookmark_count].line = line;
    bookmarks[bookmark_count].flag = BOOKMARK_USER;
    bookmark_count++;
}

static int viewer_add_last_read_bookmark(void)
{
    int i;

    i = viewer_find_bookmark(cpage, cline);
    if (i >= 0)
        bookmarks[i].flag |= BOOKMARK_LAST;
    else
    {
        viewer_add_bookmark();
        i = bookmark_count-1;
        bookmarks[i].flag = BOOKMARK_LAST;
    }
    return i;
}

static void viewer_remove_bookmark(int i)
{
    int j;

    if (i < 0 || i >= bookmark_count)
        return;

    for (j = i+1; j < bookmark_count; j++)
        rb->memcpy(&bookmarks[j-1], &bookmarks[j],
                   sizeof(struct bookmark_info));

    bookmark_count--;
}

void viewer_remove_last_read_bookmark(void)
{
    int i, j;

    for (i = 0; i < bookmark_count; i++)
    {
        if (bookmarks[i].flag & BOOKMARK_LAST)
        {
            if (bookmarks[i].flag == BOOKMARK_LAST)
            {
                for (j = i+1; j < bookmark_count; j++)
                    rb->memcpy(&bookmarks[j-1], &bookmarks[j],
                               sizeof(struct bookmark_info));

                bookmark_count--;
            }
            else
                bookmarks[i].flag = BOOKMARK_USER;
            break;
        }
    }
}

/*
 * the function that a bookmark add when there is not a bookmark in the given position
 * or the bookmark remove when there exist a bookmark in the given position.
 *
 */
void viewer_add_remove_bookmark(int page, int line)
{
    int idx = viewer_find_bookmark(cpage, cline);

    if (idx < 0)
    {
        if (bookmark_count >= MAX_BOOKMARKS-1)
            rb->splash(HZ/2, "No more add bookmark.");
        else
        {
            viewer_add_bookmark(page, line);
            rb->splash(HZ/2, "Bookmark add.");
        }
    }
    viewer_remove_bookmark(idx);
    rb->splash(HZ/2, "Bookmark remove.");
}

static int viewer_get_last_read_bookmark(void)
{
    int i;

    for (i = 0; i < bookmark_count; i++)
    {
        if (bookmarks[i].flag & BOOKMARK_LAST)
            return i;
    }
    return -1;
}

void viewer_select_bookmark(int initval)
{
    int i;
    int ipage = 0;
    int iline = 0;
    int screen_pos;
    int screen_top;
    int selected = -1;

    struct opt_items items[bookmark_count];
    unsigned char names[bookmark_count][38];

    if (initval >= 0 && initval < bookmark_count)
    {
        ipage = bookmarks[initval].page;
        iline = bookmarks[initval].line;
    }

    rb->qsort(bookmarks, bookmark_count, sizeof(struct bookmark_info),
              bm_comp);

    for (i = 0; i < bookmark_count; i++)
    {
        rb->snprintf(names[i], sizeof(names[0]),
#if CONFIG_KEYPAD != PLAYER_PAD
                     "%sPage: %d  Line: %d",
#else
                     "%sP:%d  L:%d",
#endif
                     (bookmarks[i].flag&BOOKMARK_LAST)? "*":" ",
                     bookmarks[i].page,
                     bookmarks[i].line);
        items[i].string = names[i];
        items[i].voice_id = -1;
        if (selected < 0 && bookmarks[i].page == ipage && bookmarks[i].line == iline)
            selected = i;
    }

    rb->set_option("Select bookmark", &selected, INT, items,
                          sizeof(items) / sizeof(items[0]), NULL);

    if (selected < 0 || selected >= bookmark_count)
    {
        if (initval < 0 || (selected = viewer_get_last_read_bookmark()) < 0)
        {
            if (initval < 0)
                rb->splash(HZ, "Start the first page.");
            file_pos = 0;
            screen_top_ptr = buffer;
            cpage = 1;
            cline = 1;
            buffer_end = BUFFER_END();
            return;
        }
    }

    screen_pos = bookmarks[selected].file_position;
    screen_top = screen_pos % buffer_size;
    file_pos = screen_pos - screen_top;
    screen_top_ptr = buffer + screen_top;
    cpage = bookmarks[selected].page;
    cline = bookmarks[selected].line;
    buffer_end = BUFFER_END();
}


static bool viewer_read_bookmark_info(int bfd, struct bookmark_info *b)
{
    unsigned char buf[BOOKMARK_SIZE];

    if (rb->read(bfd, buf, sizeof(buf)) != sizeof(buf))
        return false;

    b->file_position = (buf[0] << 24)|(buf[1] << 16)|(buf[2] << 8)|buf[3];
    b->page = (buf[4] << 8)|buf[5];
    b->line = buf[6];
    b->flag = buf[7];

    return true;
}

bool viewer_read_bookmark_infos(int fd)
{
    unsigned char c;
    int i;

    if (rb->read(fd, &c, 1) != 1)
    {
        bookmark_count = 0;
        return false;
    }

    bookmark_count = c;
    if (bookmark_count > MAX_BOOKMARKS)
        bookmark_count = MAX_BOOKMARKS;

    for (i = 0; i < bookmark_count; i++)
    {
        if (!viewer_read_bookmark_info(fd, &bookmarks[i]))
        {
            bookmark_count = i;
            return false;
        }
    }
    return true;
}

static bool viewer_write_bookmark_info(int bfd, struct bookmark_info *b)
{
    unsigned char buf[BOOKMARK_SIZE];
    unsigned char *p = buf;
    unsigned long ul;

    ul = b->file_position;
    *p++ = ul >> 24;
    *p++ = ul >> 16;
    *p++ = ul >> 8;
    *p++ = ul;

    ul = b->page;
    *p++ = ul >> 8;
    *p++ = ul;

    *p++ = b->line;
    *p = b->flag;

    return (rb->write(bfd, buf, sizeof(buf)) == sizeof(buf));
}

bool viewer_write_bookmark_infos(int fd)
{
    unsigned char c = bookmark_count;
    int i;

    if (rb->write(fd, &c, 1) != 1)
        return false;

    for (i = 0; i < bookmark_count; i++)
    {
        if (!viewer_write_bookmark_info(fd, &bookmarks[i]))
            return false;
    }

    return true;
}

bool copy_bookmark_file(int sfd, int dfd, off_t start, off_t size)
{
    off_t rsize;

    if (rb->lseek(sfd, start, SEEK_SET) < 0)
        return false;

    while (size > 0)
    {
        if (size > buffer_size)
            rsize = buffer_size;
        else
            rsize = size;
        size -= rsize;

        if (rb->read(sfd, buffer, rsize) != rsize ||
            rb->write(dfd, buffer, rsize) != rsize)
            return false;
    }
    return true;
}
