/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
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
#include "config.h"
#include "yesno.h"
#include "system.h"
#include "kernel.h"
#include "misc.h"
#include "lang.h"
#include "action.h"
#include "talk.h"
#include "settings.h"
#include "viewport.h"


struct gui_yesno
{
    const struct text_message * main_message;
    const struct text_message * result_message[2];

    struct viewport *vp;
    struct screen * display;
};

static void talk_text_message(const struct text_message * message, bool enqueue)
{
    int line;
    if(message)
    {
        for(line=0; line<message->nb_lines; line++)
        {
            long id = P2ID((unsigned char *)message->message_lines[line]);
            if(id>=0)
            {
                talk_id(id, enqueue);
                enqueue = true;
            }
        }
    }
}

static int put_message(struct screen *display,
                        const struct text_message * message,
                        int start, int max_y)
{
    int i;
    for(i=0; i<message->nb_lines && i+start<max_y; i++)
    {
        display->puts_scroll(0, i+start,
                             P2STR((unsigned char *)message->message_lines[i]));
    }
    return i;
}

/*
 * Draws the yesno
 *  - yn : the yesno structure
 */
static void gui_yesno_draw(struct gui_yesno * yn)
{
    struct screen * display=yn->display;
    struct viewport *vp = yn->vp;
    int nb_lines, vp_lines, line_shift=0;
    struct viewport *last_vp;

    last_vp = display->set_viewport(vp);
    display->clear_viewport();
    nb_lines = yn->main_message->nb_lines;
    vp_lines = viewport_get_nb_lines(vp);

    if(nb_lines+3< vp_lines)
        line_shift=1;

    line_shift += put_message(display, yn->main_message,
                              line_shift, vp_lines);
#ifdef HAVE_TOUCHSCREEN
    if (display->screen_type == SCREEN_MAIN)
    {
        int w,h;
        int rect_w = vp->width/2, rect_h = vp->height/2;
        int old_pattern = vp->fg_pattern;
        vp->fg_pattern = LCD_RGBPACK(0,255,0);
        display->drawrect(0, rect_h, rect_w, rect_h);
        display->getstringsize(str(LANG_SET_BOOL_YES), &w, &h);
        display->putsxy((rect_w-w)/2, rect_h+(rect_h-h)/2, str(LANG_SET_BOOL_YES));
        vp->fg_pattern = LCD_RGBPACK(255,0,0);
        display->drawrect(rect_w, rect_h, rect_w, rect_h);
        display->getstringsize(str(LANG_SET_BOOL_NO), &w, &h);
        display->putsxy(rect_w + (rect_w-w)/2, rect_h+(rect_h-h)/2, str(LANG_SET_BOOL_NO));
        vp->fg_pattern = old_pattern;
    }
#else
    /* Space remaining for yes / no text ? */
    if(line_shift+2 <= vp_lines)
    {
        if(line_shift+3 <= vp_lines)
            line_shift++;
        display->puts(0, line_shift, str(LANG_CONFIRM_WITH_BUTTON));
        display->puts(0, line_shift+1, str(LANG_CANCEL_WITH_ANY));
    }
#endif
    display->update_viewport();
    display->set_viewport(last_vp);
}

/*
 * Draws the yesno result
 *  - yn : the yesno structure
 *  - result : the result tha must be displayed :
 *    YESNO_NO if no
 *    YESNO_YES if yes
 */
static bool gui_yesno_draw_result(struct gui_yesno * yn, enum yesno_res result)
{
    const struct text_message * message=yn->result_message[result];
    struct viewport *vp = yn->vp;
    struct screen * display=yn->display;
    if(message==NULL)
        return false;
    struct viewport *last_vp = display->set_viewport(vp);
    display->clear_viewport();
    put_message(yn->display, message, 0, viewport_get_nb_lines(vp));
    display->update_viewport();
    display->set_viewport(last_vp);
    return(true);
}

enum yesno_res gui_syncyesno_run(const struct text_message * main_message,
                                 const struct text_message * yes_message,
                                 const struct text_message * no_message)
{
    int button;
    int result=-1;
    bool result_displayed;
    struct gui_yesno yn[NB_SCREENS];
    struct viewport vp[NB_SCREENS];
    long talked_tick = 0;
    FOR_NB_SCREENS(i)
    {
        yn[i].main_message=main_message;
        yn[i].result_message[YESNO_YES]=yes_message;
        yn[i].result_message[YESNO_NO]=no_message;
        yn[i].display=&screens[i];
        yn[i].vp = &vp[i];
        viewportmanager_theme_enable(i, true, yn[i].vp);
        screens[i].scroll_stop();
        gui_yesno_draw(&(yn[i]));
    }

    /* make sure to eat any extranous keypresses */
    action_wait_for_release();

    while (result==-1)
    {
        /* Repeat the question every 5secs (more or less) */
        if (global_settings.talk_menu
            && (talked_tick==0 || TIME_AFTER(current_tick, talked_tick+HZ*5)))
        {
            talked_tick = current_tick;
            talk_text_message(main_message, false);
        }
        button = get_action(CONTEXT_YESNOSCREEN, HZ*5);
        switch (button)
        {
#ifdef HAVE_TOUCHSCREEN
            case ACTION_TOUCHSCREEN:
                {
                    short int x, y;
                    if (action_get_touchscreen_press_in_vp(&x, &y, yn[0].vp) == BUTTON_REL)
                    {
                        if (y > yn[0].vp->height/2)
                        {
                            if (x <= yn[0].vp->width/2)
                                result = YESNO_YES;
                            else
                                result = YESNO_NO;
                        }
                    }
                }
                break;
#endif
            case ACTION_YESNO_ACCEPT:
                result=YESNO_YES;
                break;
            case ACTION_NONE:
            case SYS_CHARGER_DISCONNECTED:
            case SYS_BATTERY_UPDATE:
                /* ignore some SYS events that can happen */
                continue;
            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return(YESNO_USB);
                result = YESNO_NO;
        }
    }

    FOR_NB_SCREENS(i)
        result_displayed=gui_yesno_draw_result(&(yn[i]), result);

    if (global_settings.talk_menu)
    {
        talk_text_message((result == YESNO_YES) ? yes_message
                          : no_message, false);
        talk_force_enqueue_next();
    }
    if(result_displayed)
        sleep(HZ);

    FOR_NB_SCREENS(i)
    {
        screens[i].scroll_stop_viewport(yn[i].vp);
        viewportmanager_theme_undo(i, true);
    }

    return(result);
}


/* Function to manipulate all yesno dialogues.
   This function needs the output text as an argument. */
bool yesno_pop(const char* text)
{
    const char *lines[]={text};
    const struct text_message message={lines, 1};
    bool ret = (gui_syncyesno_run(&message,NULL,NULL)== YESNO_YES);
    FOR_NB_SCREENS(i)
        screens[i].clear_viewport();
    return ret;
}
