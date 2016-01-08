/*
 * xrick/rects.c
 *
 * Copyright (C) 1998-2002 BigOrno (bigorno@bigorno.net).
 * Copyright (C) 2008-2014 Pierluigi Vicinanza.
 * All rights reserved.
 *
 * The use and distribution terms for this software are contained in the file
 * named README, which can be found in the root of this distribution. By
 * using this software in any fashion, you are agreeing to be bound by the
 * terms of this license.
 *
 * You must not remove this notice, or any other, from this software.
 */

#include "xrick/rects.h"
#include "xrick/system/system.h"

/*
 * Free a list of rectangles and set the pointer to NULL.
 *
 * p: rectangle list CHANGED to NULL
 */
void
rects_free(rect_t *r)
{
    while (r)
    {
        rect_t * next = r->next;
        sysmem_pop(r);
        r = next;
    }
}


/*
 * Add a rectangle to a list of rectangles
 */
rect_t *
rects_new(U16 x, U16 y, U16 width, U16 height, rect_t *next)
{
    rect_t *r;

    r = sysmem_push(sizeof(*r));
    if (!r)
    {
        return NULL;
    }
    r->x = x;
    r->y = y;
    r->width = width;
    r->height = height;
    r->next = next;
    return r;
}

/* eof */
