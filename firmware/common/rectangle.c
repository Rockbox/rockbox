/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2022 Aidan MacDonald
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

#include "rectangle.h"
#include "system.h"

bool rect_contains(const struct rectangle *ra, const struct rectangle *rb)
{
    return ra->x <= rb->x && rb->x + rb->w < ra->x + ra->w &&
           ra->y <= rb->y && rb->y + rb->h < ra->y + ra->h;
}

bool rect_overlap(const struct rectangle *ra, const struct rectangle *rb)
{
    return ra->x + ra->w > rb->x && rb->x + rb->w > ra->x &&
           ra->y + ra->h > rb->y && rb->y + rb->h > ra->y;
}

bool rect_intersect(const struct rectangle *ra, const struct rectangle *rb,
                    struct rectangle *r_out)
{
    if (!rect_valid(ra) || !rect_valid(rb))
        return false;

    int x = MAX(ra->x, rb->x);
    int y = MAX(ra->y, rb->y);
    int w = MIN(ra->x + ra->w, rb->x + rb->w) - x;
    int h = MIN(ra->y + ra->h, rb->y + rb->h) - y;

    r_out->x = x;
    r_out->y = y;
    r_out->w = w;
    r_out->h = h;

    return w > 0 && h > 0;
}

void rect_union(const struct rectangle *ra, const struct rectangle *rb,
                struct rectangle *r_out)
{
    if (!rect_valid(ra)) {
        *r_out = *rb;
    } else if (!rect_valid(rb)) {
        *r_out = *ra;
    } else {
        int x = MIN(ra->x, rb->x);
        int y = MIN(ra->y, rb->y);
        int w = MAX(ra->x + ra->w, rb->x + rb->w) - x;
        int h = MAX(ra->y + ra->h, rb->y + rb->h) - y;

        r_out->x = x;
        r_out->y = y;
        r_out->w = w;
        r_out->h = h;
    }
}

int rect_difference(const struct rectangle *rect,
                    const struct rectangle *rsub,
                    struct rectangle rects_out[4])
{
    if (!rect_valid(rect) || !rect_valid(rsub)) {
        rects_out[0] = *rect;
        return 1;
    }

    int x0 = MAX(rect->x, rsub->x);
    int y0 = MAX(rect->y, rsub->y);
    int x1 = MIN(rect->x + rect->w, rsub->x + rsub->w);
    int y1 = MIN(rect->y + rect->h, rsub->y + rsub->h);

    /* no intersection */
    if (x1 - x0 <= 0 || y1 - y0 <= 0) {
        rects_out[0] = *rect;
        return 1;
    }

    /*  rect
     * +-------------+
     * |   .  2  .   |
     * |   +-----+   |
     * | 0 |rsub | 1 |
     * |   +-----+   |
     * |   .  3  .   |
     * +-------------+
     */

    int n = 0;

    if (rect->x < x0) {
        rects_out[n].x = rect->x;
        rects_out[n].y = rect->y;
        rects_out[n].w = x0 - rect->x;
        rects_out[n].h = rect->h;
        n++;
    }

    if (x1 < rect->x + rect->w) {
        rects_out[n].x = x1;
        rects_out[n].y = rect->y;
        rects_out[n].w = rect->x + rect->w - x1;
        rects_out[n].h = rect->h;
        n++;
    }

    if (rect->y < y0) {
        rects_out[n].x = x0;
        rects_out[n].y = rect->y;
        rects_out[n].w = x1 - x0;
        rects_out[n].h = y0 - rect->y;
        n++;
    }

    if (y1 < rect->y + rect->h) {
        rects_out[n].x = x0;
        rects_out[n].y = y1;
        rects_out[n].w = x1 - x0;
        rects_out[n].h = rect->y + rect->h - y1;
        n++;
    }

    return n;
}
