/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Miscellaneous helper API definitions
 *
 * Copyright (c) 2007 Michael Sevakis
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
#include "mpegplayer.h"

/** Streams **/

/* Initializes the cursor */
void stream_scan_init(struct stream_scan *sk)
{
    dbuf_l2_init(&sk->l2);
}

/* Ensures direction is -1 or 1 and margin is properly initialized */
void stream_scan_normalize(struct stream_scan *sk)
{
    if (sk->dir >= 0)
    {
        sk->dir = SSCAN_FORWARD;
        sk->margin = sk->len;
    }
    else if (sk->dir < 0)
    {
        sk->dir = SSCAN_REVERSE;
        sk->margin = 0;
    }
}

/* Moves a scan cursor. If amount is positive, the increment is in the scan
 * direction, otherwise opposite the scan direction */
void stream_scan_offset(struct stream_scan *sk, off_t by)
{
    off_t bydir = by*sk->dir;
    sk->pos += bydir;
    sk->margin -= bydir;
    sk->len -= by;
}

/** Time helpers **/
void ts_to_hms(uint32_t pts, struct hms *hms)
{
    hms->frac = pts % TS_SECOND;
    hms->sec = pts / TS_SECOND;
    hms->min = hms->sec / 60;
    hms->hrs = hms->min / 60;
    hms->sec %= 60;
    hms->min %= 60;
}

void hms_format(char *buf, size_t bufsize, struct hms *hms)
{
    /* Only display hours if nonzero */
    if (hms->hrs != 0)
    {
        rb->snprintf(buf, bufsize, "%u:%02u:%02u",
                     hms->hrs, hms->min, hms->sec);
    }
    else
    {
        rb->snprintf(buf, bufsize, "%u:%02u",
                     hms->min, hms->sec);
    }
}

/** Maths **/
uint32_t muldiv_uint32(uint32_t multiplicand,
                       uint32_t multiplier,
                       uint32_t divisor)
{
    if (divisor != 0)
    {
        uint64_t prod = (uint64_t)multiplier*multiplicand + divisor/2;

        if ((uint32_t)(prod >> 32) < divisor)
            return (uint32_t)(prod / divisor);
    }
    else if (multiplicand == 0 || multiplier == 0)
    {
        return 0; /* 0/0 = 0 : yaya */
    }
    /* else (> 0) / 0 = UINT32_MAX */

    return UINT32_MAX; /* Saturate */
}


/** Lists **/

/* Does the list have any members? */
bool list_is_empty(void **list)
{
    return *list == NULL;
}

/* Is the item inserted into a particular list? */
bool list_is_member(void **list, void *item)
{
    return *rb->find_array_ptr(list, item) != NULL;
}

/* Removes an item from a list - returns true if item was found
 * and thus removed. */
bool list_remove_item(void **list, void *item)
{
    return rb->remove_array_ptr(list, item) != -1;
}

/* Adds a list item, insert last, if not already present. */
void list_add_item(void **list, void *item)
{
    void **item_p = rb->find_array_ptr(list, item);
    if (*item_p == NULL)
        *item_p = item;
}

/* Clears the entire list. */
void list_clear_all(void **list)
{
    while (*list != NULL)
        *list++ = NULL;
}

/* Enumerate all items in the array, passing each item in turn to the
 * callback as well as the data value. The current item may be safely
 * removed. Other changes during enumeration are undefined. The callback
 * may return 'false' to stop the enumeration early. */
void list_enum_items(void **list,
                     list_enum_callback_t callback,
                     intptr_t data)
{
    for (;;)
    {
        void *item = *list;

        if (item == NULL)
            break;

        if (callback != NULL && !callback(item, data))
            break;

        if (*list == item)
            list++; /* Item still there */
    }
}


/** System events **/
static long mpeg_sysevent_id;

void mpeg_sysevent_clear(void)
{
    mpeg_sysevent_id = 0;
}

void mpeg_sysevent_set(void)
{
    /* Nonzero and won't invoke anything in default event handler */
    mpeg_sysevent_id = ACTION_STD_CANCEL;
}

long mpeg_sysevent(void)
{
    return mpeg_sysevent_id;
}

int mpeg_sysevent_callback(int btn, const struct menu_item_ex *menu)
{
    switch (btn)
    {
    case SYS_USB_CONNECTED:
    case SYS_POWEROFF:
        mpeg_sysevent_id = btn;
        return ACTION_STD_CANCEL;
    }

    return btn;
    (void)menu;
}

void mpeg_sysevent_handle(void)
{
    long id = mpeg_sysevent();
    if (id != 0)
        rb->default_event_handler(id);
}


/** Buttons **/

int mpeg_button_get(int timeout)
{
    int button;

    mpeg_sysevent_clear();
    button = timeout == TIMEOUT_BLOCK ? rb->button_get(true) :
                rb->button_get_w_tmo(timeout);

    /* Produce keyclick */
    rb->keyclick_click(true, button);

    return mpeg_sysevent_callback(button, NULL);
}

