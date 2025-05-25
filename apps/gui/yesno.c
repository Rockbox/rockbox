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
#include "appevents.h"
#include "splash.h"
#include "backlight.h"

struct gui_yesno
{
    struct viewport vp;
    const struct text_message * main_message;
    struct screen * display;
    int vp_lines;
    /* timeout data */
    long end_tick;
    enum yesno_res tmo_default_res;
};

static void talk_text_message(const struct text_message * message, bool enqueue)
{
    int line;
    for(line=0; line < message->nb_lines; line++)
    {
        long id = P2ID((unsigned char *)message->message_lines[line]);
        if(id>=0)
        {
            talk_id(id, enqueue);
            enqueue = true;
        }
    }
}

static int put_message(struct screen *display,
                        const struct text_message * message,
                        int start, int max_y)
{
    int i;
    int ct = MIN(message->nb_lines, max_y - start);
    for(i=0; i < ct; i++)
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
    struct viewport *vp = &yn->vp;
    int vp_lines = yn->vp_lines;
    enum yesno_res def_res = yn->tmo_default_res;
    const struct text_message *main_message = yn->main_message;
    int line_shift = 0;
    struct viewport *last_vp = display->set_viewport_ex(vp, VP_FLAG_VP_SET_CLEAN);

    /* do our own clear to avoid stopping scrolling */
    int oldmode = vp->drawmode;
    vp->drawmode ^= DRMODE_INVERSEVID;
    vp->drawmode |= DRMODE_SOLID;
    display->fillrect(0, 0, vp->width, vp->height);
    vp->drawmode = oldmode;

    if(main_message->nb_lines + 3 < vp_lines)
        line_shift = 1;

    line_shift += put_message(display, main_message, line_shift, vp_lines);

#ifdef HAVE_TOUCHSCREEN
    if (display->screen_type == SCREEN_MAIN)
    {
        int w,h,tmo_w;
        int tm_rem = 0;
        const char *btn_fmt;
        int rect_w = vp->width/2, rect_h = vp->height/2;
        int old_pattern = vp->fg_pattern;
        vp->fg_pattern = LCD_RGBPACK(0,255,0);
        display->drawrect(0, rect_h, rect_w, rect_h);
        display->getstringsize(str(LANG_SET_BOOL_YES), &w, &h);

        if (def_res == YESNO_YES)
        {
            display->getstringsize(" (00)", &tmo_w, NULL);
            tm_rem = ((yn->end_tick - current_tick) / 100);
            btn_fmt = "%s (%02d)";
        }
        else
        {
            btn_fmt = "%s\0%d";
            tmo_w = 0;
        }

        display->putsxyf((rect_w-(w+tmo_w))/2, rect_h+(rect_h-h)/2,
                         btn_fmt, str(LANG_SET_BOOL_YES), tm_rem);

        vp->fg_pattern = LCD_RGBPACK(255,0,0);
        display->drawrect(rect_w, rect_h, rect_w, rect_h);
        display->getstringsize(str(LANG_SET_BOOL_NO), &w, &h);

        if (def_res == YESNO_NO)
        {
            display->getstringsize(" (00)", &tmo_w, NULL);
            tm_rem = ((yn->end_tick - current_tick) / 100);
            btn_fmt = "%s (%02d)";
        }
        else
        {
            btn_fmt = "%s\0%d";
            tmo_w = 0;
        }

        display->putsxyf(rect_w + (rect_w-(w+tmo_w))/2, rect_h+(rect_h-h)/2,
                         btn_fmt, str(LANG_SET_BOOL_NO), tm_rem);

        vp->fg_pattern = old_pattern;
    }
#else
    /* Space remaining for yes / no text ? */
    if(line_shift + 2 <= vp_lines)
    {
        if(line_shift + 3 <= vp_lines)
            line_shift++;
        display->puts(0, line_shift, str(LANG_CONFIRM_WITH_BUTTON));
        display->puts(0, line_shift+1, str(LANG_CANCEL_WITH_ANY));

        if (def_res == YESNO_YES || def_res == YESNO_NO)
        {
            int tm_rem = ((yn->end_tick - current_tick) / 100);
            if (def_res == YESNO_YES)
                display->putsf(0, line_shift, "%s (%02d)",
                               str(LANG_CONFIRM_WITH_BUTTON), tm_rem);
            else
                display->putsf(0, line_shift+1, "%s (%02d)",
                               str(LANG_CANCEL_WITH_ANY), tm_rem);
        }
    }
#endif
    display->update_viewport();
    display->set_viewport(last_vp);
}

/*
 * Draws the yesno result
 *  - yn : the yesno structure
 *  - result : the result to be displayed :
 *    YESNO_NO if no
 *    YESNO_YES if yes
 */
static void gui_yesno_draw_result(struct gui_yesno * yn, const struct text_message * message)
{
    struct viewport *vp = &yn->vp;
    struct screen * display=yn->display;
    struct viewport *last_vp = display->set_viewport_ex(vp, VP_FLAG_VP_SET_CLEAN);

    display->clear_viewport();
    put_message(display, message, 0, yn->vp_lines);
    display->update_viewport();
    display->set_viewport(last_vp);
}
#if 0
static void gui_yesno_ui_update(unsigned short id, void *event_data, void *user_data)
{
    (void)id;
    (void)event_data;

    struct gui_yesno* yn = (struct gui_yesno*)user_data;
    FOR_NB_SCREENS(i)
    {
        gui_yesno_draw(&yn[i]);
    }
}
#endif
/* Display a YES_NO prompt to the user
 *
 * ticks < HZ will be ignored and the prompt will be blocking
 * tmo_default_res is the answer that is returned when the timeout expires
 * a default result of YESNO_TMO will also make the prompt blocking
 * if tmo_default_res is YESNO_YES or YESNO_NO a seconds countdown will
 * be present next to the default option
 *
 *        ticks - timeout if (>=HZ) otherwise ignored
 *  default_res - result returned on timeout YESNO_TMO creates a blocking prompt
 * main_message - prompt to the user
 *  yes_message - displayed when YESNO_YES is choosen
 *   no_message - displayed when YESNO_NO is choosen
*/
enum yesno_res gui_syncyesno_run_w_tmo(int ticks, enum yesno_res tmo_default_res,
                                       const struct text_message * main_message,
                                       const struct text_message * yes_message,
                                       const struct text_message * no_message)
{
    #define YESNO_NONE (-1)
    int action;
    bool backlight_on;
    bool talk_menu = global_settings.talk_menu;
    int result = YESNO_NONE;
    struct gui_yesno yn[NB_SCREENS];
    long talked_tick = current_tick - 1;
    long end_tick = current_tick + ticks;

    if (ticks < HZ) /* Display a prompt with NO timeout to the user */
    {
        tmo_default_res = YESNO_TMO;
    }

    FOR_NB_SCREENS(i)
    {
        yn[i].end_tick = end_tick;
        yn[i].tmo_default_res = tmo_default_res;
        yn[i].main_message=main_message;
        yn[i].display=&screens[i];
        screens[i].scroll_stop();
        viewportmanager_theme_enable(i, true, &(yn[i].vp));
        yn[i].vp_lines = viewport_get_nb_lines(&(yn[i].vp));
    }

#ifdef HAVE_TOUCHSCREEN
    /* switch to point mode because that's more intuitive */
    enum touchscreen_mode old_mode = touchscreen_get_mode();
    touchscreen_set_mode(TOUCHSCREEN_POINT);
#endif

    /* make sure to eat any extranous keypresses */
    action_wait_for_release();

    /* hook into UI update events to avoid the dialog disappearing
     * in case the skin decides to do a full refresh */
    /*add_event_ex(GUI_EVENT_NEED_UI_UPDATE, false, gui_yesno_ui_update, &yn[0]);*/
    /* probably no longer needed --Bilgus 2023*/

    while (result==YESNO_NONE)
    {

        FOR_NB_SCREENS(i)
            gui_yesno_draw(&yn[i]);

        /* Repeat the question every 5secs (more or less) */
        if (talk_menu && TIME_AFTER(current_tick, talked_tick))
        {
            talked_tick = current_tick + (HZ*5);
            talk_text_message(main_message, false);
        }
        backlight_on = is_backlight_on(false);
        action = get_action(CONTEXT_YESNOSCREEN, HZ / 2); /* for statubar and tmo */
        switch (action)
        {
#ifdef HAVE_TOUCHSCREEN
            case ACTION_TOUCHSCREEN:
                {
                    int btn;
                    short int x, y;
                    btn = action_get_touchscreen_press_in_vp(&x, &y, &(yn[0].vp));
                    if (btn == BUTTON_REL)
                    {
                        if (y > yn[0].vp.height/2)
                        {
                            if (x <= yn[0].vp.width/2)
                                result = YESNO_YES;
                            else
                                result = YESNO_NO;
                        }
                    }
                }
                break;
#endif
            case ACTION_YESNO_ACCEPT:
                result = YESNO_YES;
                break;
            case ACTION_NONE:
                if(tmo_default_res != YESNO_TMO && TIME_AFTER(current_tick, end_tick))
                {
                    splash(HZ/2, ID2P(LANG_TIMEOUT));
                    result = tmo_default_res;
                    goto exit;
                }
            /*fall-through*/
            case ACTION_UNKNOWN:
            case ACTION_REDRAW:
                continue;
            default:
                if(default_event_handler(action) == SYS_USB_CONNECTED) {
                    result = YESNO_USB;
                    goto exit;
                }
                if (!IS_SYSEVENT(action)) /* ignore SYS events that can happen */
                    result = YESNO_NO;
        }

        if (!backlight_on)
            result = YESNO_NONE; /* don't allow results if the screen is off */
    }

exit:

    /*remove_event_ex(GUI_EVENT_NEED_UI_UPDATE, gui_yesno_ui_update, &yn[0]);*/

    if (result == YESNO_YES || result == YESNO_NO)
    {
        const struct text_message * resmsg;
        if (result == YESNO_YES)
            resmsg = yes_message;
        else
            resmsg = no_message;

        if (resmsg != NULL)
        {
            FOR_NB_SCREENS(i)
                gui_yesno_draw_result(&(yn[i]), resmsg);

            if (talk_menu)
            {
                talk_text_message(resmsg, false);
                talk_force_enqueue_next();
            }

            sleep(HZ);
        }
    }

    FOR_NB_SCREENS(i)
    {
        screens[i].scroll_stop_viewport(&(yn[i].vp));
        viewportmanager_theme_undo(i, true);
    }

#ifdef HAVE_TOUCHSCREEN
    touchscreen_set_mode(old_mode);
#endif
    return result;
}

enum yesno_res gui_syncyesno_run(const struct text_message * main_message,
                                 const struct text_message * yes_message,
                                 const struct text_message * no_message)
{
    return gui_syncyesno_run_w_tmo(TIMEOUT_BLOCK, YESNO_TMO,
                             main_message, yes_message, no_message);
}

static bool yesno_pop_lines(const char *lines[], int line_cnt)
{
    const struct text_message message={lines, line_cnt};
    bool ret = (gui_syncyesno_run(&message,NULL,NULL)== YESNO_YES);
    FOR_NB_SCREENS(i)
        screens[i].clear_viewport();
    return ret;
}

/* YES/NO dialog, uses text parameter as prompt */
bool yesno_pop(const char* text)
{
    const char *lines[]= {text};
    return yesno_pop_lines(lines, 1);
}

/* YES/NO dialog, asks "Are you sure?", displays
   text parameter on second line.

   Says "Cancelled" if answered negatively.
*/
bool yesno_pop_confirm(const char* text)
{
    bool confirmed;
    const char *lines[] = {ID2P(LANG_ARE_YOU_SURE), text};
    confirmed = yesno_pop_lines(lines, 2);

    if (!confirmed)
        splash(HZ, ID2P(LANG_CANCEL));

    return confirmed;
}
