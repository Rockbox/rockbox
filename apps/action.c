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
#include "misc.h"
#if defined(HAVE_LCD_BITMAP) && !defined(BOOTLOADER)
#include "language.h"
#endif
#include "viewport.h"
#ifdef HAVE_TOUCHSCREEN
#include "statusbar-skinned.h"
#endif

#ifdef HAVE_BACKLIGHT
#include "backlight.h"
#if CONFIG_CHARGING
#include "power.h"
#endif
static bool selective_act = false;
static int selective_act_mask = 0;
static bool filter_first_keypress=false;
static int do_selective_backlight(int action);
#endif /* HAVE_BACKLIGHT */

#if defined(HAVE_BACKLIGHT) || !defined(HAS_BUTTON_HOLD)
static int get_filtered_action(int action, int actmask, int context);
static bool is_filtered_context(int context);
static bool is_action_filterable(int action);
static inline bool is_action_completed(void);
const int act_none = SEL_ACTION_NONE;
const int act_found = SEL_ACTION_FOUND;
#define LAST_ACTION_TICKS HZ/4
#endif


static int last_button = BUTTON_NONE|BUTTON_REL; /* allow the ipod wheel to
                                                    work on startup */
static int last_context = CONTEXT_STD;
static bool action_pending = false;

static intptr_t last_data = 0;
static int last_action = ACTION_NONE;
static bool repeated = false;
static bool wait_for_release = false;

#ifdef HAVE_TOUCHSCREEN
static bool short_press = false;
#endif

#define REPEAT_WINDOW_TICKS HZ/4
static int last_action_tick = 0;

/* software keylock stuff */
#ifndef HAS_BUTTON_HOLD
static bool keys_locked = false;
static int unlock_combo = BUTTON_NONE;
static bool screen_has_lock = false;
static bool selective_softlock = false;
static int selective_softlock_mask = 0;
static int do_selective_softlock(int action);
#endif /* HAVE_SOFTWARE_KEYLOCK */

/*
 * do_button_check is the worker function for get_default_action.
 * returns ACTION_UNKNOWN or the requested return value from the list.
 */
static inline int do_button_check(const struct button_mapping *items,int button, 
                                    int last_button, int *start, bool *pending)
{
    int i = 0;
    int ret = ACTION_UNKNOWN;

    while (items[i].button_code != BUTTON_NONE)
    {
        if (items[i].button_code == button)
        {
            /*
                CAVEAT: This will allways return the action without pre_button_code if it has a
                lower index in the list.
            */
            if ((items[i].pre_button_code == BUTTON_NONE)
                || (items[i].pre_button_code == last_button))
            {
                ret = items[i].action_code;
                break;
            }
        }
        else if (items[i].pre_button_code == button)
            *pending = true; /* determine if this could be another action */
        i++;
    }
    *start = i;
    return ret;
}

#if defined(HAVE_LCD_BITMAP) && !defined(BOOTLOADER)
/*
 * button is horizontally inverted to support RTL language if the given language
 * and context combination require that
 */
static int button_flip_horizontally(int context, int button)
{
    int newbutton;

    if (!(lang_is_rtl() && ((context == CONTEXT_STD) ||
        (context == CONTEXT_TREE) || (context == CONTEXT_LIST) ||
        (context == CONTEXT_MAINMENU))))
    {
        return button;
    }

    newbutton = button &
        ~(BUTTON_LEFT | BUTTON_RIGHT
#if defined(BUTTON_SCROLL_BACK) && defined(BUTTON_SCROLL_FWD) && \
    !defined(SIMULATOR)
        | BUTTON_SCROLL_BACK | BUTTON_SCROLL_FWD
#endif
#if defined(BUTTON_MINUS) && defined(BUTTON_PLUS) && \
    !defined(SIMULATOR)
        | BUTTON_MINUS | BUTTON_PLUS
#endif
        );

    if (button & BUTTON_LEFT)
        newbutton |= BUTTON_RIGHT;
    if (button & BUTTON_RIGHT)
        newbutton |= BUTTON_LEFT;
#if defined(BUTTON_SCROLL_BACK) && defined(BUTTON_SCROLL_FWD) && \
    !defined(SIMULATOR)
    if (button & BUTTON_SCROLL_BACK)
        newbutton |= BUTTON_SCROLL_FWD;
    if (button & BUTTON_SCROLL_FWD)
        newbutton |= BUTTON_SCROLL_BACK;
#endif
#if defined(BUTTON_MINUS) && defined(BUTTON_PLUS) && \
    !defined(SIMULATOR)
    if (button & BUTTON_MINUS)
        newbutton |= BUTTON_PLUS;
    if (button & BUTTON_PLUS)
        newbutton |= BUTTON_MINUS;
#endif

    return newbutton;
}
#endif

static inline int get_next_context(const struct button_mapping *items, int i)
{
    while (items[i].button_code != BUTTON_NONE)
        i++;
    return (items[i].action_code == ACTION_NONE ) ?
            CONTEXT_STD :
            items[i].action_code;
}

#if defined(HAVE_GUI_BOOST) && defined(HAVE_ADJUSTABLE_CPU_FREQ)

/* Timeout for gui boost in seconds. */
#define GUI_BOOST_TIMEOUT (HZ)

/* Helper function which is called to boost / unboost CPU. This function
 * avoids to increase boost_count with each call of gui_boost(). */
static void gui_boost(bool want_to_boost)
{
    static bool boosted = false;
    
    if (want_to_boost && !boosted)
    {
        cpu_boost(true);
        boosted = true;
    }
    else if (!want_to_boost && boosted)
    {
        cpu_boost(false);
        boosted = false;
    }
}

/* gui_unboost_callback() is called GUI_BOOST_TIMEOUT seconds after the 
 * last wheel scrolling event. */
static int gui_unboost_callback(struct timeout *tmo)
{
    (void)tmo;
    gui_boost(false);
    return 0;
}
#endif

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
    /*static int last_context = CONTEXT_STD;//Moved Global*/
    
    send_event(GUI_EVENT_ACTIONUPDATE, NULL);

    if (timeout == TIMEOUT_NOBLOCK)
        button = button_get(false);
    else if  (timeout == TIMEOUT_BLOCK)
        button = button_get(true);
    else
        button = button_get_w_tmo(timeout);

#if defined(HAVE_GUI_BOOST) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
    static struct timeout gui_unboost;
    /* Boost the CPU in case of wheel scrolling activity in the defined contexts.
     * Call unboost with a timeout of GUI_BOOST_TIMEOUT. */
    if (button != BUTTON_NONE)
    {
        gui_boost(true);
        timeout_register(&gui_unboost, gui_unboost_callback, GUI_BOOST_TIMEOUT, 0);
    }
#endif

    /* Data from sys events can be pulled with button_get_data
     * multimedia button presses don't go through the action system */
    if (button == BUTTON_NONE || button & (SYS_EVENT|BUTTON_MULTIMEDIA))
    {
        /* no button pressed so no point in waiting for release */
        if (button == BUTTON_NONE)
            wait_for_release = false;
        return button;
    }

    /* the special redraw button should result in a screen refresh */
    if (button == BUTTON_REDRAW)
        return ACTION_REDRAW;

    /* if action_wait_for_release() was called without a button being pressed
     * then actually waiting for release would do the wrong thing, i.e.
     * the next key press is entirely ignored. So, if here comes a normal
     * button press (neither release nor repeat) the press is a fresh one and
     * no point in waiting for release
     *
     * This logic doesn't work for touchscreen which can send normal
     * button events repeatedly before the first repeat (as in BUTTON_REPEAT).
     * These cannot be distinguished from the very first touch
     * but there's nothing we can do about it here */
    if ((button & (BUTTON_REPEAT|BUTTON_REL)) == 0)
        wait_for_release = false;

    /* Don't send any buttons through untill we see the release event */
    if (wait_for_release)
    {
        if (button&BUTTON_REL)
        {
            /* remember the button for the below button eating on context
             * change */
            last_button = button;
            wait_for_release = false;
        }
        return ACTION_NONE;
    }
        
    if ((context != last_context) && ((last_button & BUTTON_REL) == 0)
#ifdef HAVE_SCROLLWHEEL
        /* Scrollwheel doesn't generate release events  */
        && !(last_button & (BUTTON_SCROLL_BACK | BUTTON_SCROLL_FWD))
#endif
        )
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
#ifndef HAS_BUTTON_HOLD
    screen_has_lock = ((context & ALLOW_SOFTLOCK) == ALLOW_SOFTLOCK);
    if (is_keys_locked() && !selective_softlock) /*selective softlock disables*/
    {
        if (button == unlock_combo)
        {
            last_button = BUTTON_NONE;
            keys_locked = false;
#if defined(HAVE_TOUCHPAD) || defined(HAVE_TOUCHSCREEN)
            /* enable back touch device */
            button_enable_touch(true);
#endif
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
#if defined(HAVE_TOUCHPAD) || defined(HAVE_TOUCHSCREEN)
    else
    {
        /* make sure touchpad get reactivated if we quit the screen */
        button_enable_touch(true);
    }
#endif
    context &= ~ALLOW_SOFTLOCK;
#endif /* HAS_BUTTON_HOLD */

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

#if defined(HAVE_LCD_BITMAP) && !defined(BOOTLOADER)
    button = button_flip_horizontally(context, button);
#endif

    /*   logf("%x,%x",last_button,button); */
    action_pending = false; /* could the button be another actions pre_button */
    while (1)
    {
        /*     logf("context = %x",context); */
#if (BUTTON_REMOTE != 0)
        if (button & BUTTON_REMOTE)
            context |= CONTEXT_REMOTE;
#endif
        if ((context & CONTEXT_PLUGIN) && get_context_map)
            items = get_context_map(context);
        else
            items = get_context_mapping(context);

        if (items == NULL)
            break;
        ret = do_button_check(items, button, last_button, &i, &action_pending);

        if (ret == ACTION_UNKNOWN)
        {
            context = get_next_context(items,i);

            if (context != (int)CONTEXT_STOPSEARCHING)
            {
                i = 0;
                continue;
            }
        }

        /* Action was found or STOPSEARCHING was specified */
        break;
    }
    /* DEBUGF("ret = %x\n",ret); */
#ifndef HAS_BUTTON_HOLD
    if (screen_has_lock && (ret == ACTION_STD_KEYLOCK) && !selective_softlock)
    {/* sel softlock disable this */
        unlock_combo = button;
        keys_locked = true;
        splash(HZ/2, str(LANG_KEYLOCK_ON));
 #if defined(HAVE_TOUCHPAD) || defined(HAVE_TOUCHSCREEN)
        /* disable touch device on keylock */
        button_enable_touch(false);
 #endif
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

#if CONFIG_CODEC == SWCODEC
    /* Produce keyclick */
    keyclick_click(false, ret);
#endif

    return ret;
}/* get_action_worker */

int get_action(int context, int timeout)
{
    int button = get_action_worker(context,timeout,NULL);
#ifdef HAVE_TOUCHSCREEN
    if (button == ACTION_TOUCHSCREEN)
        button = sb_touch_to_button(context);
#endif


#ifndef HAS_BUTTON_HOLD
    if (selective_softlock && is_action_filterable(button))
        button = do_selective_softlock(button);
#endif
#ifdef HAVE_BACKLIGHT
    if (selective_act && is_action_filterable(button))
        button = do_selective_backlight(button);
#endif


    return button;
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
    if (!ret)
    {
        default_event_handler(action);
    }
    return ret;
}

#ifndef HAS_BUTTON_HOLD
bool is_keys_locked(void)
{
    return (screen_has_lock && keys_locked);
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
 *        BUTTON_REL                if it's a short press = release after press
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

int action_get_touchscreen_press_in_vp(short *x1, short *y1, struct viewport *vp)
{
    short x, y;
    int ret;

    ret = action_get_touchscreen_press(&x, &y);

    if (ret != BUTTON_NONE && viewport_point_within_vp(vp, x, y))
    {
        *x1 = x - vp->x;
        *y1 = y - vp->y;
        return ret;
    }
    if (ret & BUTTON_TOUCHSCREEN)
        return ACTION_UNKNOWN;
    return BUTTON_NONE;
}
#endif

/* Don't let get_action*() return any ACTION_* values until the current buttons
 * have been released. SYS_* and BUTTON_NONE will go through.
 * Any actions relying on _RELEASE won't get seen.
 *
 * Note this doesn't currently work for touchscreen targets if called
 * when the screen isn't currently touched, because they can send normal
 * (non-BUTTON_REPEAT) events repeatedly, if the touch coordinates change.
 * This cannot be distinguished from normal buttons events.
 */
void action_wait_for_release(void)
{
    wait_for_release = true;
}

#if defined(HAVE_BACKLIGHT) || !defined(HAS_BUTTON_HOLD)
/*
* Selective softlock and backlight use this lookup based on mask to decide
* which actions are filtered, or could be filtered but not currently set.
* returns act_none if the action isn't found, act_found if the action is found
* but not set and (SEL_ACTION|found) if the action is set
*/
static int get_filtered_action(int action, int actmask, int context)
{
    int match = act_none;
    if (action != ACTION_NONE && action != ACTION_UNKNOWN &&
                                                   is_filtered_context(context))
        {           /* prevents useless comparisons */

            if (action == ACTION_WPS_PLAY ||
                 action == ACTION_FM_PLAY)
                match = (actmask & SEL_ACTION_PLAY) | act_found;
            else if (action == ACTION_WPS_SEEKFWD    ||
                     action == ACTION_WPS_SEEKBACK   ||
                     action == ACTION_WPS_STOPSEEK   ||
                     action == ACTION_STD_PREVREPEAT ||
                     action == ACTION_STD_NEXTREPEAT)
                match = (actmask & SEL_ACTION_SEEK) | act_found;
            else if (action == ACTION_WPS_VOLUP ||
                     action == ACTION_WPS_VOLDOWN)
                match = (actmask & SEL_ACTION_VOL) | act_found;
            else if (action == ACTION_WPS_SKIPNEXT  ||
                    action == ACTION_WPS_SKIPPREV   ||
                    action == ACTION_FM_NEXT_PRESET ||
                    action == ACTION_FM_PREV_PRESET ||
                    action == ACTION_STD_PREV ||
                    action == ACTION_STD_NEXT)
                match = (actmask & SEL_ACTION_SKIP) | act_found;
            else if ((context == CONTEXT_FM ) && /* Sansa Fuze+ fm volume? */
                    (action == ACTION_SETTINGS_INC       ||
                     action == ACTION_SETTINGS_INCREPEAT ||
                     action == ACTION_SETTINGS_DEC       ||
                     action == ACTION_SETTINGS_DECREPEAT))
                match = (actmask & SEL_ACTION_VOL) | act_found;

            /*display action code of unfiltered actions
            else
            {
                static char buf[32];
                snprintf(buf,31,"#action %d - %d",action, last_button);
                splash(HZ/10, buf);
            }*/
        }

    return match;
}
/* returns true if the supplied context is to be filtered by selective BL/SL*/
static bool is_filtered_context(int context)
{
return (context == ( CONTEXT_FM ) || context == ( CONTEXT_WPS ));
}
/* returns true if action can be passed on to selective backlight/softlock */
static bool is_action_filterable(int action)
{
    return (action != ACTION_REDRAW && !(action & (SYS_EVENT)));
}
/*returns true if Button & released, repeated; or won't generate those events*/
static inline bool is_action_completed(void)
{
return (((last_button & (BUTTON_REPEAT | BUTTON_REL)) != 0)
#ifdef HAVE_SCROLLWHEEL
        /* Scrollwheel doesn't generate release events  */
            || (last_button & (BUTTON_SCROLL_BACK | BUTTON_SCROLL_FWD))
#endif
        );
}
#endif /* defined(HAVE_BACKLIGHT) || !defined(HAS_BUTTON_HOLD) */

#ifdef HAVE_BACKLIGHT
    /* Need to look up actions before we can decide to turn on backlight, if
     * selective backlighting is true filter first keypress events need to be
     * taken into account as well, most every action takes two rounds through
     * get_action_worker, once for the keypress and once for the key release,
     * actions with pre_button codes take even more, some actions however, only
     * take once; actions defined with only a button and no release/repeat event
     * these actions should be acted upon immediately except when we have 
     * selective backlighting enabled and in this case we only act upon them
     * immediately if there is no chance they have another even tied to them
     * determined using action_pending and is_action_completed()
     */
static int do_selective_backlight(int action)
{
    static int last_filtered = act_none;
    static int last_filtered_action_tick = 0;
    int context = last_context & ~ALLOW_SOFTLOCK;
    bool backlight_is_active = is_backlight_on(false);
    bool backlight_activate = false;

    int filtered = get_filtered_action(action, selective_act_mask, context);

     if (filtered > act_found)
        { /*directly after a match a key release event may trigger another*/
            last_filtered_action_tick = current_tick;
        }
    if(is_action_completed() || !action_pending)
    { /* has button been rel/repeated or is this the only action it could be */
        if (filtered == act_none && last_filtered == act_found)
        { /* no action found this time but action was not filtered last time */
            if (action == ACTION_NONE)
                action = ACTION_UNKNOWN;
            backlight_activate = true;
            last_filtered = act_none;
        }
        else
        { /* reset last filtered , if the action is not filtered and this
                        isn't just a key release event then turn on backlight*/
            last_filtered = act_none;
            if (filtered <= act_found && 
                last_filtered_action_tick + LAST_ACTION_TICKS < current_tick)
                backlight_activate = true;
        }
    }
    else if (filtered > act_none)
        last_filtered = filtered;

#if CONFIG_CHARGING
    if ((SEL_ACTION_NOEXT & selective_act_mask) && power_input_present())
        backlight_activate = true; /* disabled on external power*/
#endif

    if (action != ACTION_NONE &&
    (backlight_is_active || backlight_activate || !is_filtered_context(context)))
    {/* turn on backlight if: already on, action is not filtered, or any button 
                                            pressed in a non-filtered context */
  
        backlight_on_force();
#ifdef HAVE_BUTTON_LIGHT
        buttonlight_on_force();
#endif
        if (filter_first_keypress && !backlight_is_active)
        {
            action      = ACTION_NONE;
            last_button = BUTTON_NONE;
        }
    }
    ignore_next_backlight_on(true);/*must be set everytime we handle backlight*/
#ifdef HAVE_BUTTON_LIGHT
    ignore_next_buttonlight_on(true);/* as a precautionary fallback */
#endif
    return action;
}

/* Enable selected actions to leave the screen off */
void set_selective_backlight_actions(bool selective, int mask, bool filter_fkp)
{
    if (selective)
    {
        backlight_on_force();
        ignore_next_backlight_on(true);/* must be set everytime we handle bl */
#ifdef HAVE_BUTTON_LIGHT
        buttonlight_on_force();
        ignore_next_buttonlight_on(true);/* as a precautionary fallback */
#endif

        set_backlight_filter_keypress(false);/* turnoff ffkp in button.c */
    }
    else
    {
        backlight_on_force();
        ignore_next_backlight_on(false);
#ifdef HAVE_BUTTON_LIGHT
        buttonlight_on_force();
        ignore_next_buttonlight_on(false);
#endif
        set_backlight_filter_keypress(filter_fkp);
    }
    selective_act = selective;
    selective_act_mask = mask;
    filter_first_keypress=filter_fkp;
}
#endif /* HAVE_BACKLIGHT */


#ifndef HAS_BUTTON_HOLD
/* Handles softlock when selective softlock enabled */
static int do_selective_softlock(int action)
{
    static int last_filtered=act_none;
    bool notify_user = false;
    int context = last_context & ~ALLOW_SOFTLOCK;
    int filtered = get_filtered_action(action, selective_softlock_mask, context);

    if (screen_has_lock)
    {
        if (action == ACTION_STD_KEYLOCK && !is_keys_locked())
        {
            keys_locked = true;
            notify_user = true;
            last_button = BUTTON_NONE;
            action      = ACTION_REDRAW;
        }
        else if (action == ACTION_STD_KEYLOCK)
        {
            keys_locked = false;
            notify_user = true;
            last_button = BUTTON_NONE;
            action      = ACTION_REDRAW;
        }
        else if (is_keys_locked())
        {

            if(is_action_completed())
            {
                if (filtered <= act_found && last_filtered == act_found)
                { /* notify user if this action filtered and last was filtered*/
                    notify_user = true;
                    action = ACTION_REDRAW;
                    last_filtered = act_none;
                }
                else if (filtered <= act_found && action != ACTION_NONE)
                {/* wait for completion before notifying user */
                    action = ACTION_NONE;
                    last_filtered = filtered;
                }
                else
                    last_filtered = act_none;/* action found let it through*/

            }
            else if (filtered <= act_found && action != ACTION_NONE)
            { /* wait for completion before notifying user */
                action = ACTION_NONE;
                last_filtered = act_found;
            }
            else
                last_filtered = act_none;/* action found let it through*/

        }
            if (notify_user)
            {
#ifdef HAVE_BACKLIGHT
                backlight_on_force();
#endif
#ifdef HAVE_BUTTON_LIGHT
                buttonlight_on_force();
#endif
                if (keys_locked)
                    splash(HZ/2, str(LANG_KEYLOCK_ON));
                else
                    splash(HZ/2, str(LANG_KEYLOCK_OFF));
            }
    }
    return action;
}

void set_selective_softlock_actions(bool selective, int mask)
{
    selective_softlock = selective;
    selective_softlock_mask = mask;
}

#endif /* HAS_BUTTON_HOLD */
