/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Tomer Shalev
 *
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
#include "fractal_rect.h"

#define RECT_QUEUE_LEN 32

#define QUEUE_NEXT(i) (((i) + 1) % RECT_QUEUE_LEN)
#define QUEUE_PREV(i) (((i) - 1) % RECT_QUEUE_LEN)
#define QUEUE_IS_EMPTY (QUEUE_NEXT(rectq.head) == rectq.tail)
#define QUEUE_TAIL \
    (&rectq.rects[rectq.tail]);
#define QUEUE_ITEM(i) &rectq.rects[(i)]

struct rect_queue
{
    struct fractal_rect rects[RECT_QUEUE_LEN];
    unsigned head;
    unsigned tail;
};

static struct rect_queue rectq;

/*#define FRACTAL_RECT_DEBUG*/
#if defined(FRACTAL_RECT_DEBUG) && defined(SIMULATOR)
#include <stdio.h>

static void rectq_print(void)
{
    unsigned i = rectq.head;

    printf("Rects queue:\n");
    for (i = rectq.head; i != rectq.tail; i = QUEUE_PREV(i))
    {
        printf("\tItem %d/%d; idx %d; (%d,%d),(%d,%d),%svalid%s%s\n",
            (i - rectq.head + 1) % RECT_QUEUE_LEN,
            (rectq.head - rectq.tail) % RECT_QUEUE_LEN,
            i,
            rectq.rects[(i)].px_min,
            rectq.rects[(i)].py_min,
            rectq.rects[(i)].px_max,
            rectq.rects[(i)].py_max,
            (rectq.rects[(i)].valid ? "" : "in"),
            ((i == rectq.head) ? ",head" : ""),
            ((i == rectq.tail) ? ",tail" : ""));
    }
}
#else
#define rectq_print(...)
#endif

static int rects_add(int px_min, int py_min, int px_max, int py_max)
{
    struct fractal_rect *rect = &rectq.rects[rectq.tail];

    if (rectq.head == QUEUE_PREV(rectq.tail)) /* queue is full */
        return 1;

    rect->px_min = px_min;
    rect->py_min = py_min;
    rect->px_max = px_max;
    rect->py_max = py_max;
    rect->valid = 1;
    rectq.tail = QUEUE_PREV(rectq.tail);

    return 0;
}

static void rects_queue_reset(void)
{
    rectq.tail = 0;
    rectq.head = 0;
}

void rects_queue_init(void)
{
    rects_queue_reset();
    rects_add(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

void rects_calc_all(int (*calc)(struct fractal_rect *, int (*)(void *), void *),
        int (*button_yield_cb)(void *), void *ctx)
{
    while (rectq.head != rectq.tail) /* queue is not empty */
    {
        struct fractal_rect *rect = &rectq.rects[rectq.head];

        rectq_print();
        if (rect->valid)
        {
            if (calc(rect, button_yield_cb, ctx))
                return; /* interrupted by key-press */
        }

        rectq.head = QUEUE_PREV(rectq.head); /* dequeue */
    }
    rects_queue_reset();
}

int rects_move_up(void)
{
    unsigned i;

    /* move current regions up */
    for (i = rectq.head; i != rectq.tail; i = QUEUE_PREV((i)))
    {
        struct fractal_rect *rect = QUEUE_ITEM(i);

        rect->py_min -= LCD_SHIFT_Y;
        rect->py_max -= LCD_SHIFT_Y;

        if (rect->py_min < 0)
            rect->py_min = 0;

        if (rect->py_max < 0)
            rect->valid = 0;
    }

    /* add bottom region */
    return rects_add(0, LCD_HEIGHT - LCD_SHIFT_Y, LCD_WIDTH, LCD_HEIGHT);
}

int rects_move_down(void)
{
    unsigned i;

    /* move current regions down */
    for (i = rectq.head; i != rectq.tail; i = QUEUE_PREV((i)))
    {
        struct fractal_rect *rect = QUEUE_ITEM(i);

        rect->py_min += LCD_SHIFT_Y;
        rect->py_max += LCD_SHIFT_Y;

        if (rect->py_max > LCD_HEIGHT)
            rect->py_max = LCD_HEIGHT;

        if (LCD_HEIGHT <= rect->py_min)
            rect->valid = 0;
    }

    /* add top region */
    return rects_add(0, 0, LCD_WIDTH, LCD_SHIFT_Y);
}

int rects_move_left(void)
{
    unsigned i;

    /* move current regions left */
    for (i = rectq.head; i != rectq.tail; i = QUEUE_PREV((i)))
    {
        struct fractal_rect *rect = QUEUE_ITEM(i);

        rect->px_min -= LCD_SHIFT_X;
        rect->px_max -= LCD_SHIFT_X;

        if (rect->px_min < 0)
            rect->px_min = 0;

        if (rect->px_max < 0)
            rect->valid = 0;
    }

    /* add right region */
    return rects_add(LCD_WIDTH - LCD_SHIFT_X, 0, LCD_WIDTH, LCD_HEIGHT);
}

int rects_move_right(void)
{
    unsigned i;

    /* move current regions right */
    for (i = rectq.head; i != rectq.tail; i = QUEUE_PREV((i)))
    {
        struct fractal_rect *rect = QUEUE_ITEM(i);

        rect->px_min += LCD_SHIFT_X;
        rect->px_max += LCD_SHIFT_X;

        if (rect->px_max > LCD_WIDTH)
            rect->px_max = LCD_WIDTH;

        if (LCD_WIDTH <= rect->px_min)
            rect->valid = 0;
    }

    /* add left region */
    return rects_add(0, 0, LCD_SHIFT_X, LCD_HEIGHT);
}

