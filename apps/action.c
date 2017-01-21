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
#endif /* HAVE_BACKLIGHT */

#define LOGF_ENABLE
#include "logf.h"

static int last_button = BUTTON_NONE|BUTTON_REL; /* allow the ipod wheel to
                                                    work on startup */
static intptr_t last_data = 0;
static int last_action = ACTION_NONE;
static bool repeated = false;
static bool wait_for_release = false;

#ifdef HAVE_TOUCHSCREEN
static bool short_press = false;
#endif

#define REPEAT_WINDOW_TICKS HZ/4
static int last_action_tick = 0;

#if defined(HAVE_BACKLIGHT) || !defined(HAS_BUTTON_HOLD)
static inline bool is_action_normal(int action);
static inline bool mask_has_flag(unsigned int mask, unsigned int flag);
static inline bool is_action_completed(int button);
#define LAST_FILTER_TICKS HZ/2 /* timeout between filtered actions */
#endif /* defined(HAVE_BACKLIGHT) || !defined(HAS_BUTTON_HOLD) */

#ifdef HAVE_BACKLIGHT
static unsigned int backlight_mask = SEL_ACTION_NONE;
static int do_backlight(int action, int context, bool is_pre_btn);
static void handle_backlight(bool backlight, bool ignore_next);
#endif /* HAVE_BACKLIGHT */

/* software keylock stuff */
#ifndef HAS_BUTTON_HOLD
static bool keys_locked = false;
static bool screen_has_lock = false;
static unsigned int softlock_mask = SEL_ACTION_NONE;
static inline int do_softlock_unlock_combo(int button, int context);
static int do_softlock(int button, int action, int context, bool is_pre_btn);
#endif /* HAVE_SOFTWARE_KEYLOCK */
/*
 * do_button_check is the worker function for get_default_action.
 * returns ACTION_UNKNOWN or the requested return value from the list.
 * BE AWARE is_pre_button can miss pre buttons if a match is found first.
 */
static inline int do_button_check(const struct button_mapping *items,int button,
                                    int last_button, int *start, bool *prebtn)
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
        else if (items[i].pre_button_code & button)
            *prebtn = true; /* determine if this could be another action */
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


/*
 * int get_action_worker(int context, int timeout, bool *is_pre_button,
                         struct button_mapping *user_mappings)
   This function searches the button list for the given context for the just
   pressed button.
   If there is a match it returns the value from the list.
   If there is no match..
        the last item in the list "points" to the next context in a chain
        so the "chain" is followed until the button is found.
        putting ACTION_NONE will get CONTEXT_STD which is always the last list checked.
   BE AWARE is_pre_button can miss pre buttons if a match is found first.
    it is more for actions that are not yet completed in the desired context
     but are defined in a lower 'chained' context.
   Timeout can be TIMEOUT_NOBLOCK to return immediatly
                  TIMEOUT_BLOCK   to wait for a button press
   Any number >0   to wait that many ticks for a press
 */
static int get_action_worker(int context, int timeout, bool *is_pre_button,
                             const struct button_mapping* (*get_context_map)(int) )
{
    const struct button_mapping *items = NULL;
    int button;
    int i=0;
    int ret = ACTION_UNKNOWN;
    static int last_context = CONTEXT_STD;

    send_event(GUI_EVENT_ACTIONUPDATE, NULL);

    button = button_get_w_tmo(timeout);

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
    context &= ~ALLOW_SOFTLOCK;
    if (is_keys_locked())
    {
        ret = do_softlock_unlock_combo(button, context);
        if (!is_keys_locked())
            return ret;
    }
#if defined(HAVE_TOUCHPAD) || defined(HAVE_TOUCHSCREEN)
    else if (!mask_has_flag(softlock_mask, SEL_ACTION_NOTOUCH))
    {
        /* make sure touchpad get reactivated if we quit the screen */
        button_enable_touch(true);
    }
#endif

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
    *is_pre_button = false; /* could the button be another actions pre_button */
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

        ret = do_button_check(items, button, last_button, &i, is_pre_button);

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
    if(screen_has_lock && is_action_normal(ret))
        ret = do_softlock(button, ret, last_context & ~ALLOW_SOFTLOCK, is_pre_button);
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
    bool is_pre_button = false;
    int  button = get_action_worker(context, timeout, &is_pre_button, NULL);

#ifdef HAVE_TOUCHSCREEN
    if (button == ACTION_TOUCHSCREEN)
        button = sb_touch_to_button(context);
#endif

#ifdef HAVE_BACKLIGHT
    if (mask_has_flag(backlight_mask, SEL_ACTION_ENABLED) &&
                                                        is_action_normal(button))
        button = do_backlight(button, context & ~ALLOW_SOFTLOCK, is_pre_button);
#endif

    return button;
}

int get_custom_action(int context,int timeout,
                      const struct button_mapping* (*get_context_map)(int))
{
    bool is_pre_button = false;
    return get_action_worker(context,timeout, &is_pre_button, get_context_map);
}

bool action_userabort(int timeout)
{
    bool is_pre_button = false;
    int action = get_action_worker(CONTEXT_STD,timeout, &is_pre_button, NULL);
    bool ret = (action == ACTION_STD_CANCEL);
    if (!ret)
    {
        default_event_handler(action);
    }
    return ret;
}

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
/* HELPER FUNCTIONS
* Selective softlock and backlight use this lookup based on mask to decide
* which actions are filtered, or could be filtered but not currently set.
* returns false if the action isn't found, true if the action is found
*/
static bool is_action_filtered(int action, unsigned int mask, int context)
{
    bool match = false;

    switch (action)
    {
        case ACTION_NONE:
            break;
/*Actions that are not mapped will not turn on the backlight option NOUNMAPPED*/
        case ACTION_UNKNOWN:
            match = mask_has_flag(mask, SEL_ACTION_NOUNMAPPED);
            break;
        case ACTION_WPS_PLAY:
        case ACTION_FM_PLAY:
            match = mask_has_flag(mask, SEL_ACTION_PLAY);
            break;
        case ACTION_STD_PREVREPEAT:
        case ACTION_STD_NEXTREPEAT:
        case ACTION_WPS_SEEKBACK:
        case ACTION_WPS_SEEKFWD:
        case ACTION_WPS_STOPSEEK:
            match = mask_has_flag(mask, SEL_ACTION_SEEK);
            break;
        case ACTION_STD_PREV:
        case ACTION_STD_NEXT:
        case ACTION_WPS_SKIPNEXT:
        case ACTION_WPS_SKIPPREV:
        case ACTION_FM_NEXT_PRESET:
        case ACTION_FM_PREV_PRESET:
            match = mask_has_flag(mask, SEL_ACTION_SKIP);
            break;
        case ACTION_WPS_VOLUP:
        case ACTION_WPS_VOLDOWN:
            match = mask_has_flag(mask, SEL_ACTION_VOL);
            break;
        case ACTION_SETTINGS_INC:/*FMS*/
        case ACTION_SETTINGS_INCREPEAT:/*FMS*/
        case ACTION_SETTINGS_DEC:/*FMS*/
        case ACTION_SETTINGS_DECREPEAT:/*FMS*/
            match = (context == CONTEXT_FM) && mask_has_flag(mask, SEL_ACTION_VOL);
            break;
        default:
            /* display action code of unfiltered actions */
            logf ("unfiltered actions: context: %d action: %d, last btn: %d, \
                  mask: %d", context, action, last_button, mask);
            break;
    }/*switch*/

    return match;
}
/* compares mask to a flag return true if set false otherwise*/
static inline bool mask_has_flag(unsigned int mask, unsigned int flag)
{
    return ((mask & flag) != 0);
}
/* returns true if the supplied context is to be filtered by selective BL/SL*/
static inline bool is_context_filtered(int context)
{
    return (context == CONTEXT_FM || context == CONTEXT_WPS);
}
/* returns true if action can be passed on to selective backlight/softlock */
static inline bool is_action_normal(int action)
{
    return (action != ACTION_REDRAW && (action & SYS_EVENT) == 0);
}
/*returns true if Button & released, repeated; or won't generate those events*/
static inline bool is_action_completed(int button)
{
    return ((button & (BUTTON_REPEAT | BUTTON_REL)) != 0
#ifdef HAVE_SCROLLWHEEL
        /* Scrollwheel doesn't generate release events  */
            || (button & (BUTTON_SCROLL_BACK | BUTTON_SCROLL_FWD)) != 0
#endif
        );
}

/* most every action takes two rounds through get_action_worker,
 * once for the keypress and once for the key release,
 * actions with pre_button codes take even more, some actions however, only
 * take once; actions defined with only a button and no release/repeat event,
 * these actions should be acted upon immediately except when we have
 * selective backlighting/softlock enabled and in this case we only act upon
 * them immediately if there is no chance they have another event tied to them
 * determined using is_pre_button and is_action_completed()
 *returns true if event was not filtered and false if it was
*/
static bool is_action_unfiltered(int action,int button, bool is_pre_button,
                               bool filtered, int *tick)
{
    bool ret = false;
        /*directly after a match a key release event may trigger another*/
        if (filtered && action != ACTION_UNKNOWN)
            *tick = current_tick + LAST_FILTER_TICKS;
        /* has button been rel/rep or is this the only action it could be */
        if (is_action_completed(button) || !is_pre_button)
        {
            /* reset last action , if the action is not filtered and
               this isn't just a key release event then return true */
            if (!filtered && *tick < current_tick)
            {
                *tick = 0;
                ret = true;
            }
         }/*is_action_completed() || !is_pre_button*/
    return ret;
}
#endif /*defined(HAVE_BACKLIGHT) || !defined(HAS_BUTTON_HOLD) HELPER FUNCTIONS*/

#ifdef HAVE_BACKLIGHT
static void handle_backlight(bool backlight, bool ignore_next)
{
    if (backlight)
    {
        backlight_on_ignore(false, 0);
        backlight_on();
#ifdef HAVE_BUTTON_LIGHT
        buttonlight_on_ignore(false, 0);
        buttonlight_on();
    }
    buttonlight_on_ignore(ignore_next, 5*HZ);/* as a precautionary fallback */
#else
    }
#endif
    backlight_on_ignore(ignore_next, 5*HZ);/*must be set everytime we handle bl*/
}

    /* Need to look up actions before we can decide to turn on backlight, if
     * selective backlighting is true filter first keypress events need to be
     * taken into account as well
     */
static int do_backlight(int action, int context, bool is_pre_btn)
{
    static int last_filtered_tick = 0;

    bool bl_is_active = is_backlight_on(false);
    bool bl_activate = false;
    bool filtered;

#if CONFIG_CHARGING /* disable if on external power */
    if (!bl_is_active && is_context_filtered(context) &&
    !(mask_has_flag(backlight_mask, SEL_ACTION_NOEXT) && power_input_present()))
#else /* skip if backlight is on or incorrect context */
    if (!bl_is_active && is_context_filtered(context))
#endif
    {
        filtered = is_action_filtered(action, backlight_mask, context);
        bl_activate = is_action_unfiltered(action, last_button, is_pre_btn,
                                               filtered, &last_filtered_tick);
    }/*is_context_filtered(context)*/
    else
        bl_activate = true;

    if (action != ACTION_NONE && bl_activate)
    {
        handle_backlight(true, true);

        if (mask_has_flag(backlight_mask, SEL_ACTION_FFKEYPRESS) && !bl_is_active)
        {
            action      = ACTION_NONE;
            last_button = BUTTON_NONE;
        }
    }
    else
        handle_backlight(false, true);/* set ignore next true */

    return action;
}

/* Enable selected actions to leave the backlight off */
void set_selective_backlight_actions(bool selective, unsigned int mask,
                                                              bool filter_fkp)
{
    handle_backlight(true, selective);
    if (selective) /* we will handle filter_first_keypress here so turn it off*/
    {
        set_backlight_filter_keypress(false);/* turnoff ffkp in button.c */
        backlight_mask = mask | SEL_ACTION_ENABLED;
        if(filter_fkp)
            backlight_mask |= SEL_ACTION_FFKEYPRESS;
    }
    else
    {
        set_backlight_filter_keypress(filter_fkp);
        backlight_mask = SEL_ACTION_NONE;
    }
}
#endif /* HAVE_BACKLIGHT */

#ifndef HAS_BUTTON_HOLD
bool is_keys_locked(void)
{
    return (screen_has_lock && keys_locked);
}

static inline void do_key_lock(bool lock)
{
    keys_locked = lock;
    last_button = BUTTON_NONE;
    button_clear_queue();
#if defined(HAVE_TOUCHPAD) || defined(HAVE_TOUCHSCREEN)
 /* disable touch device on keylock if std behavior or selected disable touch */
     if (!mask_has_flag(softlock_mask, SEL_ACTION_ENABLED) ||
          mask_has_flag(softlock_mask, SEL_ACTION_NOTOUCH))
            button_enable_touch(!lock);
#endif
}

/* user selected autolock based on backlight timeout; toggles autolock on / off
    by ACTION_STD_KEYLOCK presses, Activates autolock if set on backlight timeout
*/
#ifdef HAVE_BACKLIGHT
static inline int do_auto_softlock(int button, int action, int *unlock_combo)
{
    if(mask_has_flag(softlock_mask, SEL_ACTION_ALOCK_OK) && !is_backlight_on(false))
        do_key_lock(true);

    else if (action == ACTION_STD_KEYLOCK)
    {
        *unlock_combo = button;/* set unlock combo to allow unlock */
        softlock_mask ^= SEL_ACTION_ALOCK_OK;
        handle_backlight(true, false);
            /* If we don't wait for a moment for the backlight queue
             *  to process, the user will never see the message */
        if (!is_backlight_on(false))
            sleep(HZ/2);

        if (mask_has_flag(softlock_mask, SEL_ACTION_ALOCK_OK))
        {
            splash(HZ/2, ID2P(LANG_ACTION_AUTOLOCK_ON));
            action = ACTION_REDRAW;
        }
        else
            splash(HZ/2, ID2P(LANG_ACTION_AUTOLOCK_OFF));
    }
    return action;
}
#endif

/* Allows unlock softlock when action is not yet known but unlock_combo set*/
static inline int do_softlock_unlock_combo(int button, int context)
{
return do_softlock(button, ACTION_NONE, context, false);
}

/* Handles softlock once action is known */
static int do_softlock(int button, int action, int context, bool is_pre_btn)
{
    static int last_filtered_tick = 0;
    static int unlock_combo = BUTTON_NONE; /*Moved from GLOBAL*/
    bool filtered = true;
    bool notify_user = false;
    bool sl_activate = true; /* standard softlock behavior */

#ifdef HAVE_BACKLIGHT
    if (!keys_locked && mask_has_flag(softlock_mask, SEL_ACTION_AUTOLOCK))
        action = do_auto_softlock(button, action, &unlock_combo);
#endif
    /* Lock/Unlock toggled by ACTION_STD_KEYLOCK presses*/
    if ((action == ACTION_STD_KEYLOCK) || (keys_locked && unlock_combo == button))
    {
        unlock_combo = button;
        do_key_lock(!keys_locked);
        notify_user = true;
    }
#if (BUTTON_REMOTE != 0)/* Allow remote actions through */
    else if (mask_has_flag(button, BUTTON_REMOTE))
            notify_user = false;
#endif
    else if (keys_locked && action != ACTION_NONE && action != ACTION_REDRAW)
    {
        if (mask_has_flag(softlock_mask, SEL_ACTION_ENABLED))
        {
            filtered = is_action_filtered(action, softlock_mask, context);

            sl_activate = is_action_unfiltered(action, button, is_pre_btn,
                                                   filtered, &last_filtered_tick);
        }
        /*All non-std softlock options are set to 0 if advanced sl is disabled*/
        if (sl_activate)
        {
            if (!mask_has_flag(softlock_mask, SEL_ACTION_NONOTIFY))
            {   /* always true on standard softlock behavior*/
                notify_user = mask_has_flag(button, BUTTON_REL);
                action = ACTION_REDRAW;
            }
            else
                action = ACTION_NONE;
        }
        else if (!filtered)/*catch blocked actions on fast repeated presses*/
                action = ACTION_NONE;
     } /* keys_locked */

#ifdef BUTTON_POWER /*always notify if power button pressed while keys locked*/
    notify_user |= (mask_has_flag(button, BUTTON_POWER) && keys_locked);
#endif

    if (notify_user)
    {
#ifdef HAVE_BACKLIGHT
        handle_backlight(true, false);
            /* If we don't wait for a moment for the backlight queue
             *  to process, the user will never see the message */
        if (!is_backlight_on(false))
            sleep(HZ/2);
#endif
        if (keys_locked)
            splash(HZ/2, ID2P(LANG_KEYLOCK_ON));
        else
            splash(HZ/2, ID2P(LANG_KEYLOCK_OFF));

        last_button = BUTTON_NONE;
        action      = ACTION_REDRAW;
        button_clear_queue();
    }

    return action;
}

void set_selective_softlock_actions(bool selective, unsigned int mask)
{
    keys_locked = false;
    if(selective)
        softlock_mask = mask | SEL_ACTION_ENABLED;
    else
        softlock_mask = SEL_ACTION_NONE;
}

#endif /* HAS_BUTTON_HOLD */
