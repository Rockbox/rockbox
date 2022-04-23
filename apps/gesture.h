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
#ifndef _GESTURE_H_
#define _GESTURE_H_

#include <stddef.h>
#include <stdbool.h>

struct viewport;
struct touchevent;

/** Events which can be detected by the gesture API.
 *
 * The gesture state machine, informally, looks like this:
 *
 *     NONE --+--> TAP ------------------------------------+
 *            |                                            |
 *            +--> LONG_PRESS -----------------------------+
 *            |             |                              |
 *            |             +---> HOLD ---+                |
 *            |             |             |                |
 *            +-------------+---> DRAG ---+--> RELEASE ----+--> NONE
 *
 * State transitions occur from gesture_process() in response to touch events.
 *
 * - The NONE "event" is returned prior to any other gesture being detected.
 *   Although the graph above depicts a transition back to NONE at the end of
 *   an event chain, that transition in fact happens on the TOUCHEVENT_PRESS
 *   after the end of the last event.
 *
 * - TAP events are reported after getting a TOUCHEVENT_RELEASE event if no
 *   other gestures were detected between the press and release.
 *
 * - LONG_PRESS events are reported on a TOUCHEVENT_CONTACT event if the long
 *   press timeout has expired and the touch point hasn't moved too far from
 *   its initial position.
 *
 * - HOLD events are reported on TOUCHEVENT_CONTACT events after a LONG_PRESS,
 *   provided the touch point hasn't moved too far from its initial position.
 *
 * - DRAG events are reported on TOUCHEVENT_CONTACT events after the touch
 *   point first moves a certain distance from its initial position. The first
 *   time this happens, it is reported as DRAGSTART.
 *
 * - RELEASE events are reported on the TOUCHEVENT_RELEASE event following a
 *   HOLD or DRAG gesture.
 */
enum gesture_id
{
    GESTURE_NONE = 0,       /** No gesture */
    GESTURE_TAP,            /** Quick press & release */
    GESTURE_LONG_PRESS,     /** Start of a long press */
    GESTURE_HOLD,           /** Continuation of a long press */
    GESTURE_DRAGSTART,      /** Start of a DRAG event */
    GESTURE_DRAG,           /** Press and drag on the screen */
    GESTURE_RELEASE,        /** End of a HOLD or DRAG event */
};

enum gesture_flags
{
    GESTURE_F_VALID = 0x01,
    GESTURE_F_PRESSED = 0x02,
};

struct gesture
{
    unsigned int flags;
    int id;
    short x, y;
    short ox, oy;
    long start_tick;
    long last_tick;
};

struct gesture_event
{
    int id;
    short x, y;
    short ox, oy;
    long start_tick;
    long last_tick;
};

void gesture_reset(struct gesture *g);
void gesture_process(struct gesture *g, const struct touchevent *ev);
bool gesture_get_event_in_vp(struct gesture *g, struct gesture_event *gevt,
                             const struct viewport *vp);

static inline bool gesture_get_event(struct gesture *g,
                                     struct gesture_event *gevt)
{
    return gesture_get_event_in_vp(g, gevt, NULL);
}

static inline bool gesture_is_valid(struct gesture *g)
{
    return !!(g->flags & GESTURE_F_VALID);
}

static inline bool gesture_is_pressed(struct gesture *g)
{
    return !!(g->flags & GESTURE_F_PRESSED);
}

/* Helper for computing velocity vectors */
struct gesture_vel
{
    size_t idx;
    size_t cnt;
    short xsamp[4];
    short ysamp[4];
    long tsamp[4];
};

void gesture_vel_reset(struct gesture_vel *gv);
void gesture_vel_process(struct gesture_vel *gv, const struct touchevent *ev);
bool gesture_vel_get(struct gesture_vel *gv, int *xvel, int *yvel);

#endif /* _GESTURE_H_ */
