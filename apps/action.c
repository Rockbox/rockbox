/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2006 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "button.h"
#include "action.h"
#include "kernel.h"
#include "debug.h"

bool ignore_until_release = false;
int last_button = BUTTON_NONE;
int soft_unlock_action = ACTION_NONE;
bool allow_remote_actions = true;
/*
 * do_button_check is the worker function for get_default_action.
 * returns ACTION_UNKNOWN or the requested return value from the list.
 */
inline int do_button_check(struct button_mapping *items, 
                           int button, int last_button, int *start)
{
    int i = 0;
    int ret = ACTION_UNKNOWN;
    if (items == NULL)
        return ACTION_UNKNOWN;

    /* Special case to make the keymaps smaller */
    if(button & BUTTON_REPEAT)
        last_button &= ~BUTTON_REPEAT;
    
    while (items[i].button_code != BUTTON_NONE)
    {
        if (items[i].button_code == button) 
        {
            if (items[i].pre_button_code != BUTTON_NONE)
            {
                if ((items[i].pre_button_code == last_button) ||
                     (items[i].button_code == last_button))
                {
                    ret = items[i].action_code;
                    break;
                }
            }
            else
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

inline int get_next_context(struct button_mapping *items, int i)
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
int get_action_worker(int context, int timeout,
                      struct button_mapping* (*get_context_map)(int) )
{
    struct button_mapping *items = NULL;
    int button;
    int i=0;
    int ret = ACTION_UNKNOWN;
    if (timeout == TIMEOUT_NOBLOCK)
        button = button_get(false);
    else if  (timeout == TIMEOUT_BLOCK)
        button = button_get(true);
    else
        button = button_get_w_tmo(timeout);

    
    if (button == BUTTON_NONE || button&SYS_EVENT)
    {
        return button;
    }
    
    if (ignore_until_release == true)
    {
        if (button&BUTTON_REL)
        {
            ignore_until_release = false;
        }
        /*last_button = BUTTON_NONE; this is done by action_signalscreenchange() */
        return ACTION_UNKNOWN; /* "safest" return value */
    }
    
    if (soft_unlock_action != ACTION_NONE)
    {
        if ((button&BUTTON_REMOTE) && !allow_remote_actions)
            return ACTION_NONE;
    }
    /*   logf("%x,%x",last_button,button); */
    do 
    {
        /*     logf("context = %x",context); */
#if (BUTTON_REMOTE != 0)
        if (button&BUTTON_REMOTE)
            context |= CONTEXT_REMOTE;
#endif
        if ((context&CONTEXT_CUSTOM) && get_context_map)
            items = get_context_map(context);
        else
            items = get_context_mapping(context);
        
        ret = do_button_check(items,button,last_button,&i);
        
        if ((context == CONTEXT_STD)
#if (BUTTON_REMOTE != 0)
             || ((unsigned)context == (CONTEXT_STD|CONTEXT_REMOTE))
#endif
           ) break;
        
        if (ret == ACTION_UNKNOWN )
        {
            context = get_next_context(items,i);
            i = 0;
        }
        else break;
    } while (1);
    /* DEBUGF("ret = %x\n",ret); */
    
    if (soft_unlock_action != ACTION_NONE)
    {
        if ((button&BUTTON_REMOTE) == 0)
        {
            if (soft_unlock_action == ret)
            {
                soft_unlock_action = ACTION_NONE;
                ret = ACTION_NONE; /* no need to return the code */
            }
        }
        else if (!allow_remote_actions)
            ret = ACTION_NONE;
    }
    
    last_button = button;
    return ret;
}

int get_action(int context, int timeout)
{
    return get_action_worker(context,timeout,NULL);
}

int get_custom_action(int context,int timeout,
                      struct button_mapping* (*get_context_map)(int))
{
    return get_action_worker(context,timeout,get_context_map);
}

bool action_userabort(int timeout)
{
    action_signalscreenchange();
    int action = get_action_worker(CONTEXT_STD,timeout,NULL);
    bool ret = (action == ACTION_STD_CANCEL);
    action_signalscreenchange();
    return ret;
}

void action_signalscreenchange(void)
{
    if ((last_button != BUTTON_NONE) && 
         !(last_button&BUTTON_REL))
    {
        ignore_until_release = true;
    }
    last_button = BUTTON_NONE;
}

void action_setsoftwarekeylock(int unlock_action, bool allow_remote)
{
    soft_unlock_action = unlock_action;
    allow_remote_actions = allow_remote;
    last_button = BUTTON_NONE;
}
