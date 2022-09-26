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
#ifndef _RECTANGLE_H_
#define _RECTANGLE_H_

#include <stdbool.h>

struct rectangle
{
    int x, y, w, h;
};

/** Returns true if the rectangle is non-degenerate (positive width/height). */
static inline bool rect_valid(const struct rectangle *r)
{
    return r->w > 0 && r->h > 0;
}

/** Returns true if ra contains rb. */
bool rect_contains(const struct rectangle *ra, const struct rectangle *rb);

/** Returns true if ra and rb overlap. */
bool rect_overlap(const struct rectangle *ra, const struct rectangle *rb);

/**
 * Computes the biggest rectangle contained in ra and rb.
 * Returns true if the intersection exists, and writes it to r_out.
 *
 * Returns false if there is no intersection, or either input
 * rectangle is degenerate. In this case r_out may be uninitialized.
 */
bool rect_intersect(const struct rectangle *ra, const struct rectangle *rb,
                    struct rectangle *r_out);

/**
 * Computes the smallest rectangle containing both ra and rb.
 *
 * If one input is degenerate, and the other is not, the output is a
 * copy of the non-degenerate input. If both inputs are degenerate,
 * then the output is degenerate but is otherwise unspecified.
 */
void rect_union(const struct rectangle *ra, const struct rectangle *rb,
                struct rectangle *r_out);

/**
 * Computes the result of subtracting 'rsub' from 'rect'. Up to four
 * non-overlapping output rectangles will written to 'rects_out'; the
 * return value is the number of rectangles written.
 *
 * If 'rsub' does not overlap 'rect', or if either of the inputs are
 * degenerate, the output is a single rectangle: a copy of 'rect'.
 */
int rect_difference(const struct rectangle *rect,
                    const struct rectangle *rsub,
                    struct rectangle rects_out[4]);

#endif /* _RECTANGLE_H_ */
