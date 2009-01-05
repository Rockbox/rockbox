/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jonathan Gordon
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "lang.h"

#include "appevents.h"
#include "button.h"
#include "action.h"
#include "kernel.h"
#include "debug.h"
#include "splash.h"
#include "settings.h"
#include "pcmbuf.h"

static int last_button = BUTTON_NONE|BUTTON_REL; /* allow the ipod wheel to
                                                    work on startup */
static intptr_t last_data = 0;
static int last_action = ACTION_NONE;
static bool repeated = false;

#ifdef HAVE_TOUCHSCREEN
static bool short_press = false;
#endif

#define REPEAT_WINDOW_TICKS HZ/10
static int last_action_tick = 0;

/* software keylock stuff */
#ifndef HAS_BUTTON_HOLD
static bool keys_locked = false;
static int unlock_combo = BUTTON_NONE;
static bool screen_has_lock = false;
#endif /* HAVE_SOFTWARE_KEYLOCK */

/*
 * do_button_check is the worker function for get_default_action.
 * returns ACTION_UNKNOWN or the requested return value from the list.
 */
static inline int do_button_check(const struct button_mapping *items,
                                  int button, int last_button, int *start)
{
    int i = 0;
    int ret = ACTION_UNKNOWN;
    if (items == NULL)
        return ACTION_UNKNOWN;

    while (items[i].button_code != BUTTON_NONE)
    {
        if (items[i].button_code == button)
        {
            if ((items[i].pre_button_code == BUTTON_NONE)
                || (items[i].pre_button_code == last_button))
            {
                ret = items[i].action_code;
                break;
            }
        }
        i++;
    }
    *start = i;
    return ret;
}

static inline int get_next_context(const struct button_mapping *items, int i)
{
    while (items[i].button_code != BUTTON_NONE)
        i++;
    return (items[i].action_code == ACTION_NONE ) ?
            CONTEXT_STD :
            items[i].action_code;
}
/*
 * int get_action_worker(int context, struct button_mapping *user_mappings,
   int timeout)
   This function searches the button list for the given context for the just
   pressed button.
   If there is a match it returns the value from the list.
   If there is no match..
        the last item in the list "points" to the next context in a chain
        so the "chain" is followed until the button is found.
        putting ACTION_NONE will get CONTEXT_STD which is always the last list checked.

   Timeout can be TIMEOUT_NOBLOCK to return immediatly
                  TIMEOUT_BLOCK   to wait for a button press
   Any number >0   to wait that many ticks for a press
 */
static int get_action_worker(int context, int timeout,
                             const struct button_mapping* (*get_context_map)(int) )
{
    const struct button_mapping *items = NULL;
    int button;
    int i=0;
    int ret = ACTION_UNKNOWN;
    static int last_context = CONTEXT_STD;
    
    
    send_event(GUI_EVENT_ACTIONUPDATE, NULL);

    if (timeout == TIMEOUT_NOBLOCK)
        button = button_get(false);
    else if  (timeout == TIMEOUT_BLOCK)
        button = button_get(true);
    else
        button = button_get_w_tmo(timeout);

    /* Data from sys events can be pulled with button_get_data */
    if (button == BUTTON_NONE || button & SYS_EVENT)
        return button;

#if CONFIG_CODEC == SWCODEC
    /* Produce keyclick */
    if (global_settings.keyclick && !(button & BUTTON_REL))
        if (!(button & BUTTON_REPEAT) || global_settings.keyclick_repeats)
            pcmbuf_beep(4000, KEYCLICK_DURATION, 2500*global_settings.keyclick);
#endif

    if ((context != last_context) && ((last_button & BUTTON_REL) == 0))
    {
        if (button & BUTTON_REL)
        {
            last_button = button;
            last_action = ACTION_NONE;
        }
        /* eat all buttons until the previous button was |BUTTON_REL
           (also eat the |BUTTON_REL button) */
        return ACTION_NONE; /* "safest" return value */
    }
    last_context = context;
#ifdef HAVE_TOUCHSCREEN
    if (button & BUTTON_TOUCHSCREEN)
    {
        repeated = false;
        short_press = false;
        if (last_button & BUTTON_TOUCHSCREEN)
        {
            if ((button & BUTTON_REL) &&
                ((last_button & BUTTON_REPEAT)==0))
            {
                short_press = true;
            }
            else if (button & BUTTON_REPEAT)
                repeated = true;
        }
        last_button = button;
        return ACTION_TOUCHSCREEN;
    }
#endif
#ifndef HAS_BUTTON_HOLD
    screen_has_lock = ((context & ALLOW_SOFTLOCK) == ALLOW_SOFTLOCK);
    if (screen_has_lock && (keys_locked == true))
    {
        if (button == unlock_combo)
        {
            last_button = BUTTON_NONE;
            keys_locked = false;
            splash(HZ/2, str(LANG_KEYLOCK_OFF));
            return ACTION_REDRAW;
        }
        else
#if (BUTTON_REMOTE != 0)
            if ((button & BUTTON_REMOTE) == 0)
#endif
        {
            if ((button & BUTTON_REL))
                splash(HZ/2, str(LANG_KEYLOCK_ON));
            return ACTION_REDRAW;
        }
    }
    context &= ~ALLOW_SOFTLOCK;
#endif /* HAS_BUTTON_HOLD */

    /*   logf("%x,%x",last_button,button); */
    do
    {
        /*     logf("context = %x",context); */
#if (BUTTON_REMOTE != 0)
        if (button & BUTTON_REMOTE)
            context |= CONTEXT_REMOTE;
#endif
        if ((context & CONTEXT_CUSTOM) && get_context_map)
            items = get_context_map(context);
        else
            items = get_context_mapping(context);

        ret = do_button_check(items,button,last_button,&i);

        if ((context ==(int)CONTEXT_STOPSEARCHING) ||
             items == NULL )
            break;

        if (ret == ACTION_UNKNOWN )
        {
            context = get_next_context(items,i);
            i = 0;
        }
        else break;
    } while (1);
    /* DEBUGF("ret = %x\n",ret); */
#ifndef HAS_BUTTON_HOLD
    if (screen_has_lock && (ret == ACTION_STD_KEYLOCK))
    {
        unlock_combo = button;
        keys_locked = true;
        splash(HZ/2, str(LANG_KEYLOCK_ON));

        button_clear_queue();
        return ACTION_REDRAW;
    }
#endif
    if ((current_tick - last_action_tick < REPEAT_WINDOW_TICKS)
         && (ret == last_action))
        repeated = true;
    else
        repeated = false;

    last_button = button;
    last_action = ret;
    last_data   = button_get_data();
    last_action_tick = current_tick;
    return ret;
}

int get_action(int context, int timeout)
{
    return get_action_worker(context,timeout,NULL);
}

int get_custom_action(int context,int timeout,
                      const struct button_mapping* (*get_context_map)(int))
{
    return get_action_worker(context,timeout,get_context_map);
}

bool action_userabort(int timeout)
{
    int action = get_action_worker(CONTEXT_STD,timeout,NULL);
    bool ret = (action == ACTION_STD_CANCEL);
    return ret;
}

#ifndef HAS_BUTTON_HOLD
bool is_keys_locked(void)
{
    return (screen_has_lock && (keys_locked == true));
}
#endif

intptr_t get_action_data(void)
{
    return last_data;
}

int get_action_statuscode(int *button)
{
    int ret = 0;
    if (button)
        *button = last_button;

    if (last_button & BUTTON_REMOTE)
        ret |= ACTION_REMOTE;
    if (repeated)
        ret |= ACTION_REPEAT;
    return ret;
}

#ifdef HAVE_TOUCHSCREEN
/* return BUTTON_NONE               on error
 *        BUTTON_REPEAT             if repeated press
 *        BUTTON_REPEAT|BUTTON_REL  if release after repeated press
 *        BUTTON_REL                if its a short press = release after press
 *        BUTTON_TOUCHSCREEN        if press
 */
int action_get_touchscreen_press(short *x, short *y)
{
    static int last_data = 0;
    int data;
    if ((last_button & BUTTON_TOUCHSCREEN) == 0)
        return BUTTON_NONE;
    data = button_get_data();
    if (last_button & BUTTON_REL)
    {
        *x = (last_data&0xffff0000)>>16;
        *y = (last_data&0xffff);
    }
    else
    {
        *x = (data&0xffff0000)>>16;
        *y = (data&0xffff);
    }
    last_data = data;
    if (repeated)
        return BUTTON_REPEAT;
    if (short_press)
        return BUTTON_REL;
    /* This is to return a BUTTON_REL after a BUTTON_REPEAT. */
    if (last_button & BUTTON_REL)
        return BUTTON_REPEAT|BUTTON_REL;
    return BUTTON_TOUCHSCREEN;
}
#endif
