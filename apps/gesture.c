/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2022 by Aidan MacDonald
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

#include "gesture.h"
#include "button.h"
#include "viewport.h"
#include "system.h"

/* could be a setting */
#define TOUCH_LONG_PRESS_TIME (30 * HZ / 100)

void gesture_reset(struct gesture *g)
{
    g->flags = 0;
}

void gesture_process(struct gesture *g, const struct touchevent *ev)
{
    /* wait for the first press if we haven't seen it */
    if (!gesture_is_pressed(g) && ev->type != TOUCHEVENT_PRESS)
        return;

    int dx, dy, dist_sqr;
    switch (ev->type)
    {
    case TOUCHEVENT_PRESS:
        g->flags |= GESTURE_F_PRESSED | GESTURE_F_VALID;
        g->id = GESTURE_NONE;
        g->ox = g->x = ev->x;
        g->oy = g->y = ev->y;
        g->start_tick = ev->tick;
        g->last_tick = ev->tick;
        break;

    case TOUCHEVENT_CONTACT:
        g->x = ev->x;
        g->y = ev->y;
        g->last_tick = ev->tick;

        if (g->id == GESTURE_LONG_PRESS)
            g->id = GESTURE_HOLD;
        else if (g->id == GESTURE_DRAGSTART)
            g->id = GESTURE_DRAG;
        else if (g->id != GESTURE_DRAG)
        {
            dx = ev->x - g->ox;
            dy = ev->y - g->oy;
            dist_sqr = dx*dx + dy*dy;

            /* if squared distance exceeds a threshold, report as a DRAG. */
            const int thresh = touchscreen_get_scroll_threshold();
            if (dist_sqr > thresh*thresh)
                g->id = GESTURE_DRAGSTART;

            /* report a LONG_PRESS if no motion occurs within a timeout */
            if (g->id == GESTURE_NONE &&
                TIME_AFTER(ev->tick, g->start_tick + TOUCH_LONG_PRESS_TIME))
                g->id = GESTURE_LONG_PRESS;
        }

        break;

    case TOUCHEVENT_RELEASE:
        /* report a RELEASE event after a continuous HOLD or DRAG */
        if (g->id == GESTURE_HOLD ||
            g->id == GESTURE_DRAGSTART ||
            g->id == GESTURE_DRAG)
            g->id = GESTURE_RELEASE;

        /* report a TAP event if we got a press & release without
         * triggering any other gestures */
        else if (g->id == GESTURE_NONE)
            g->id = GESTURE_TAP;

        g->flags &= ~GESTURE_F_PRESSED;
        g->last_tick = ev->tick;
        break;
    }
}

bool gesture_get_event_in_vp(struct gesture *g, struct gesture_event *gevt,
                             const struct viewport *vp)
{
    if (!gesture_is_valid(g))
        return false;

    gevt->id = g->id;
    gevt->x = g->x;
    gevt->y = g->y;
    gevt->ox = g->ox;
    gevt->oy = g->oy;
    gevt->start_tick = g->start_tick;
    gevt->last_tick = g->last_tick;

    if (vp) {
        gevt->x -= vp->x;
        gevt->y -= vp->y;
        gevt->ox -= vp->x;
        gevt->oy -= vp->y;
    }

    return !vp || viewport_point_within_vp(vp, g->ox, g->oy);
}
