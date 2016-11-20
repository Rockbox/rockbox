#include <stdbool.h>
#include <string.h>
#include "thread.h"
#include "backlight_sel.h"
#include "backlight.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "lang.h"
#include "button.h"
#include "action.h"
#include "kernel.h"
#include "settings.h"
#include "misc.h"
#if defined(HAVE_LCD_BITMAP) && !defined(BOOTLOADER)
#include "language.h"
#endif
#include "viewport.h"
#ifdef HAVE_TOUCHSCREEN
#include "statusbar-skinned.h"
#endif

#define CONTEXT_FALLTHROUGH false /*GET ACTION ALLOWS CONTEXT ACTION
                                    FALLTHROUGH THIS DISABLES*/
static bool selective_backlight = false;
static int selective_backlight_mask = 0;
static bool bl_filter_fkp = false;

static bool quit_thread = true;
static int get_selective_action(int timeout);

static int last_button = BUTTON_NONE|BUTTON_REL;
static int last_context = CONTEXT_STD;
static bool wait_for_release = false;

static int action=ACTION_NONE;

static const int action_contexts[22]=
    {
        /*[]=;*/
        [ACTIVITY_UNKNOWN]=CONTEXT_STD,
        [ACTIVITY_MAINMENU]=CONTEXT_MAINMENU,
        [ACTIVITY_WPS]=CONTEXT_WPS,
        [ACTIVITY_RECORDING]=CONTEXT_RECORD,
        [ACTIVITY_FM]=CONTEXT_FM,
        [ACTIVITY_PLAYLISTVIEWER]=CONTEXT_LIST,
        [ACTIVITY_SETTINGS]=CONTEXT_SETTINGS,
        [ACTIVITY_FILEBROWSER]=CONTEXT_TREE,
        [ACTIVITY_DATABASEBROWSER]=CONTEXT_TREE,
        [ACTIVITY_PLUGINBROWSER]=CONTEXT_TREE,
        [ACTIVITY_QUICKSCREEN]=CONTEXT_QUICKSCREEN,
        [ACTIVITY_PITCHSCREEN]=CONTEXT_PITCHSCREEN,
        [ACTIVITY_OPTIONSELECT]=CONTEXT_STD,
        [ACTIVITY_PLAYLISTBROWSER]=CONTEXT_LIST,
        [ACTIVITY_PLUGIN]=CONTEXT_STD,
        [ACTIVITY_CONTEXTMENU]=CONTEXT_STD,
        [ACTIVITY_SYSTEMSCREEN]=CONTEXT_STD,
        [ACTIVITY_TIMEDATESCREEN]=CONTEXT_SETTINGS_TIME,
        [ACTIVITY_BOOKMARKSLIST]=CONTEXT_BOOKMARKSCREEN,
        [ACTIVITY_SHORTCUTSMENU]=CONTEXT_LIST,
        [ACTIVITY_ID3SCREEN]=CONTEXT_ID3DB,
        [ACTIVITY_USBSCREEN]=CONTEXT_USB_HID
    };

static void selective_backlight_thread(void)

{
    while(!quit_thread)
    {
        action = get_selective_action(TIMEOUT_BLOCK);
        if(action == ACTION_UNKNOWN)
        {
          action = get_selective_action(HZ/4);
        }

        if(action != ACTION_NONE && (!is_filtered_context_action(action, 
            selective_backlight_mask, last_context) || is_backlight_on(false)) )
        {
            backlight_on_force();
#ifdef HAVE_BUTTON_LIGHT
            buttonlight_on_force();
#endif
        }
        if(action & SYS_USB_CONNECTED)
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
        ignore_next_backlight_on(true);
#ifdef HAVE_BUTTON_LIGHT
        ignore_next_buttonlight_on(true);
#endif
        yield();
    }
    disable_backlight_queue();
    thread_exit();
    
}

static void selective_backlight_init(bool start)
{
    static long sel_bl_stack[(DEFAULT_STACK_SIZE/4)/sizeof(long)];
    backlight_on();
#ifdef HAVE_BUTTON_LIGHT
    buttonlight_on();
#endif

    if(start)
    {
        quit_thread = false;
        enable_backlight_queue();
        create_thread(selective_backlight_thread, sel_bl_stack,
              sizeof(sel_bl_stack), 0,
              "sel backlight thread" IF_PRIO(, PRIORITY_BACKGROUND ) IF_COP(, CPU));

        backlight_on_force();
#ifdef HAVE_BUTTON_LIGHT
        buttonlight_on_force();
#endif

    }
    else
        quit_thread = true;
}


/*
 * do_button_check is the worker function for get_default_action.
 * returns ACTION_UNKNOWN or the requested return value from the list.
 */
static inline int do_button_check(const struct button_mapping *items,
                                  int button, int last_button, int *start)
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
 * int get_selective_action_worker(int timeout,struct button_mapping *user_mappings)
   This function searches the button list for the given context for the just
   pressed button.
   If there is a match it returns the value from the list.
   If there is no match ACTION_UNKNOWN is returned
   Timeout can be TIMEOUT_NOBLOCK to return immediatly
                  TIMEOUT_BLOCK   to wait for a button press
 */
static int get_selective_action_worker(int timeout,
                             const struct button_mapping* (*get_context_map)(int) )
{
    const struct button_mapping *items = NULL;
    int button;
    int context;
    int i=0;
    int ret = ACTION_UNKNOWN;
    /*static int last_context = CONTEXT_STD;//MovedGlobal*/

    if (timeout == TIMEOUT_NOBLOCK)
        button = bl_btn_get(false);
    else if  (timeout == TIMEOUT_BLOCK)
        button = bl_btn_get(true);//button_get(true);
    else
        button = bl_btn_get_w_tmo(timeout);

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
        return ACTION_NONE;//ACTION_REDRAW;

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

    context = action_contexts[get_current_activity()];

    if ((context != last_context) && ((last_button & BUTTON_REL) == 0)
#ifdef HAVE_SCROLLWHEEL
        /* Scrollwheel doesn't generate release events  */
        && !(last_button & (BUTTON_SCROLL_BACK | BUTTON_SCROLL_FWD))
#endif
        )
    {
        if (button & BUTTON_REL)
            last_button = button;
        /* eat all buttons until the previous button was |BUTTON_REL
           (also eat the |BUTTON_REL button) */
        return ACTION_NONE; /* "safest" return value */
    }
    last_context = context;

#ifdef HAVE_TOUCHSCREEN
    if (button & BUTTON_TOUCHSCREEN)
    {
        last_button = button;
        return ACTION_TOUCHSCREEN;
    }
#endif

#if defined(HAVE_LCD_BITMAP) && !defined(BOOTLOADER)
    button = button_flip_horizontally(context, button);
#endif

    /*   logf("%x,%x",last_button,button); */
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

        ret = do_button_check(items,button,last_button,&i);

        if (ret == ACTION_UNKNOWN && CONTEXT_FALLTHROUGH)
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

    last_button = button;

    return ret;
}

static int get_selective_action(int timeout)
{
    int button = get_selective_action_worker(timeout,NULL);
#ifdef HAVE_TOUCHSCREEN
    if (button == ACTION_TOUCHSCREEN)
        button = sb_touch_to_button(context);
#endif
    return button;
}

/*Enable selected actions to leave the screen off*/
void set_selective_backlight_actions(bool selective, int mask, bool filter_fkp)
{

    selective_backlight = selective;
    selective_backlight_mask = mask;
    bl_filter_fkp=filter_fkp;

    if(selective_backlight == true)
    {
        if(quit_thread == true)
            selective_backlight_init(true);
        set_backlight_filter_keypress(false);/*turnoff ffkp in button.c*/
    }
    else
    {
        if(quit_thread == false)
            selective_backlight_init(false);
        set_backlight_filter_keypress(filter_fkp);
    }


}

bool get_selective_backlight_status(void)
{
return selective_backlight;
}

bool get_selective_backlight_filter_fkp(void)
{
return (selective_backlight && bl_filter_fkp && !is_backlight_on(false));
}

int get_selective_backlight_mask(void)
{
return selective_backlight_mask;
}


