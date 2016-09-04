/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Marcin Bukat
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
#include "bookmark.h"

static struct bookmark_t bmk = {
    .magic = PICOTTS_BOOKMARK_MAGIC,
    .filecrc = 0xffffffff,
    .bookmark = 0,
    .txtchunkmap = {0}
};

static int idx = 1;

void txtchunkmap_add(uint32_t txtchunk)
{
    if  (txtchunk > bmk.txtchunkmap[idx-1])
    {
        bmk.txtchunkmap[idx++] = txtchunk;
    }
}

uint32_t txtchunk_prev(uint32_t txtchunk)
{
    for (int i=idx-1; i >= 0; i--)
    {
        if (bmk.txtchunkmap[i] < txtchunk)
        {
            return bmk.txtchunkmap[i];
        }
    }

    return txtchunk;
}

uint32_t txtchunk_next(uint32_t txtchunk)
{
    for (int i=0; i < idx; i++)
    {
        if (bmk.txtchunkmap[i] > txtchunk)
        {
            return bmk.txtchunkmap[i];
        }
    }

    return txtchunk;
}

void dump_txtchunkmap(void)
{
    DEBUGF("txtchunk map %d entries:\n", idx);
    for (int i=0; i<idx; i++)
        DEBUGF("%d ", bmk.txtchunkmap[i]);
    DEBUGF("\n");
}

void save_bookmark(char *filename, uint32_t txtchunk)
{
    char bmkfilename[80];
    rb->strcpy(bmkfilename, filename);
    rb->strcat(bmkfilename, BMK_EXTENSION);

    int fd = rb->creat(bmkfilename, 0666);

    if (fd < 0)
        return;

    bmk.bookmark = txtchunk;
    rb->write(fd, &bmk, 4 + 4 + 4 + idx*4);
    rb->close(fd);
}

uint32_t load_bookmark(char *filename)
{
    char bmkfilename[80];
    rb->strcpy(bmkfilename, filename);
    rb->strcat(bmkfilename, BMK_EXTENSION);

    struct bookmark_t tmp;
    rb->memset(&tmp, 0, sizeof(tmp));

    int fd = rb->open(bmkfilename, O_RDONLY);
    if (fd < 0)
        return 0;

    /* may read less */
    rb->read(fd, &tmp, sizeof(tmp));
    rb->close(fd);

    if (tmp.magic == PICOTTS_BOOKMARK_MAGIC &&
        tmp.filecrc == bmk.filecrc)
    {
        rb->memcpy(&bmk, &tmp, sizeof(tmp));
        return bmk.bookmark;;
    }

    return 0;
}

void bookmark_init(uint32_t crc)
{
    bmk.filecrc = crc;
}
