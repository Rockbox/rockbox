/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Robert Hak <rhak at ramapo.edu>
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
#include "lib/helper.h"



static const char* const credits[] = {
#include "credits.raw" /* generated list of names from docs/CREDITS */
};

static bool stop_autoscroll(int action)
{
    switch (action)
    {
        case ACTION_STD_CANCEL:
        case ACTION_STD_OK:
        case ACTION_STD_NEXT:
        case ACTION_STD_NEXTREPEAT:
        case ACTION_STD_PREV:
        case ACTION_STD_PREVREPEAT:
            return true;
        default:
            return false;
    }
    return false;
}

static int update_rowpos(int action, int cur_pos, int rows_per_screen, int tot_rows)
{
    switch(action)
    {
        case ACTION_STD_PREV:
        case ACTION_STD_PREVREPEAT:
            cur_pos--;
            break;
        case ACTION_STD_NEXT:
        case ACTION_STD_NEXTREPEAT:
            cur_pos++;
            break;
    }

    if(cur_pos > tot_rows - rows_per_screen)
        cur_pos = 0;
    if(cur_pos < 0)
        cur_pos = tot_rows - rows_per_screen;

    return cur_pos;
}

static void roll_credits(void)
{
    /* to do: use target defines iso keypads to set animation timings */
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_1G2G_PAD)
    #define PAUSE_TIME 0
    #define ANIM_SPEED 100
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
    #define PAUSE_TIME 0
    #define ANIM_SPEED 35
#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
    #define PAUSE_TIME 0
    #define ANIM_SPEED 100
#else
    #define PAUSE_TIME 1
    #define ANIM_SPEED 40
#endif

    #define NUM_VISIBLE_LINES (LCD_HEIGHT/font_h - 1)
    #define CREDITS_TARGETPOS ((LCD_WIDTH/2)-(credits_w/2))

    int i=0, j=0, namepos=0, offset_dummy;
    int name_w, name_h, name_targetpos=1, font_h;
    int credits_w, credits_pos;
    int numnames = (sizeof(credits)/sizeof(char*));
    char name[40], elapsednames[32];
    int action = ACTION_NONE;

    /* control if scrolling is automatic (with animation) or manual */
    bool manual_scroll = false;

    rb->lcd_setfont(FONT_UI);
    rb->lcd_clear_display();
    rb->lcd_update();

    rb->lcd_getstringsize("A", NULL, &font_h);

    /* snprintf "credits" text, and save the width and height */
    rb->snprintf(elapsednames, sizeof(elapsednames), "[Credits] %d/%d",
                 j+1, numnames);
    rb->lcd_getstringsize(elapsednames, &credits_w, NULL);

    /* fly in "credits" text from the left */
    for(credits_pos = 0 - credits_w; credits_pos <= CREDITS_TARGETPOS;
        credits_pos += (CREDITS_TARGETPOS-credits_pos + 14) / 7)
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(0, 0, LCD_WIDTH, font_h);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_putsxy(credits_pos, 0, elapsednames);
        rb->lcd_update_rect(0, 0, LCD_WIDTH, font_h);
        rb->sleep(HZ/ANIM_SPEED);
    }

    /* first screen's worth of lines fly in */
    for(i=0; i<NUM_VISIBLE_LINES; i++)
    {
        rb->snprintf(name, sizeof(name), "%s", credits[i]);
        rb->lcd_getstringsize(name, &name_w, &name_h);

        rb->snprintf(elapsednames, sizeof(elapsednames), "[Credits] %d/%d",
                     i+1, numnames);
        rb->lcd_getstringsize(elapsednames, &credits_w, NULL);
        rb->lcd_putsxy(CREDITS_TARGETPOS, 0, elapsednames);
        rb->lcd_update_rect(CREDITS_TARGETPOS, 0, credits_w, font_h);

        for(namepos = 0-name_w; namepos <= name_targetpos;
            namepos += (name_targetpos - namepos + 14) / 7)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            /* clear any trails left behind */
            rb->lcd_fillrect(0, font_h*(i+1), LCD_WIDTH, font_h);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_putsxy(namepos, font_h*(i+1), name);
            rb->lcd_update_rect(0, font_h*(i+1), LCD_WIDTH, font_h);

            /* exit on abort, switch to manual on up/down */
            action = rb->get_action(CONTEXT_LIST, HZ/ANIM_SPEED);
            if(stop_autoscroll(action))
                break;
        }
        if(stop_autoscroll(action))
            break;
    }

    /* process user actions (if any) */
    if(ACTION_STD_CANCEL == action)
        return;
    if(stop_autoscroll(action))
        manual_scroll = true; /* up/down - abort was catched above */

    if(!manual_scroll)
    {
        j+= i;

        /* pause for a bit if needed */
        action = rb->get_action(CONTEXT_LIST, HZ*PAUSE_TIME);
        if(ACTION_STD_CANCEL == action)
            return;
        if(stop_autoscroll(action))
            manual_scroll = true;
    }

    if(!manual_scroll)
    {
        while(j < numnames)
        {
            /* just a screen's worth at a time */
            for(i=0; i<NUM_VISIBLE_LINES; i++)
            {
                if(j+i >= numnames)
                    break;
                offset_dummy=1;

                rb->snprintf(name, sizeof(name), "%s",
                             credits[j+i-NUM_VISIBLE_LINES]);
                rb->lcd_getstringsize(name, &name_w, &name_h);

                /* fly out an existing line.. */
                while(namepos<LCD_WIDTH+offset_dummy)
                {
                    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                    /* clear trails */
                    rb->lcd_fillrect(0, font_h*(i+1), LCD_WIDTH, font_h);
                    rb->lcd_set_drawmode(DRMODE_SOLID);
                    rb->lcd_putsxy(namepos, font_h*(i+1), name);
                    rb->lcd_update_rect(0, font_h*(i+1), LCD_WIDTH, font_h);

                    /* exit on keypress, react to scrolling */
                    action = rb->get_action(CONTEXT_LIST, HZ/ANIM_SPEED);
                    if(stop_autoscroll(action))
                        break;

                    namepos += offset_dummy;
                    offset_dummy++;
                } /* while(namepos<LCD_WIDTH+offset_dummy) */
                if(stop_autoscroll(action))
                    break;

                rb->snprintf(name, sizeof(name), "%s", credits[j+i]);
                rb->lcd_getstringsize(name, &name_w, &name_h);

                rb->snprintf(elapsednames, sizeof(elapsednames),
                             "[Credits] %d/%d", j+i+1, numnames);
                rb->lcd_getstringsize(elapsednames, &credits_w, NULL);
                rb->lcd_putsxy(CREDITS_TARGETPOS, 0, elapsednames);
                if (j+i < NUM_VISIBLE_LINES) /* takes care of trail on loop */
                    rb->lcd_update_rect(0, 0, LCD_WIDTH, font_h);

                for(namepos = 0-name_w; namepos <= name_targetpos;
                    namepos += (name_targetpos - namepos + 14) / 7)
                {
                    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                    rb->lcd_fillrect(0, font_h*(i+1), LCD_WIDTH, font_h);
                    rb->lcd_set_drawmode(DRMODE_SOLID);
                    rb->lcd_putsxy(namepos, font_h*(i+1), name);
                    rb->lcd_update_rect(0, font_h*(i+1), LCD_WIDTH, font_h);
                    rb->lcd_update_rect(CREDITS_TARGETPOS, 0, credits_w,font_h);

                    /* stop on keypress */
                    action = rb->get_action(CONTEXT_LIST, HZ/ANIM_SPEED);
                    if(stop_autoscroll(action))
                        break;
                }
                if(stop_autoscroll(action))
                    break;
                namepos = name_targetpos;
            } /* for(i=0; i<NUM_VISIBLE_LINES; i++) */
            if(stop_autoscroll(action))
                break;

            action = rb->get_action(CONTEXT_LIST, HZ*PAUSE_TIME);
            if(stop_autoscroll(action))
                break;

            j+=i; /* no user intervention, draw the next screen-full */
        } /* while(j < numnames) */

        /* handle the keypress that we intercepted during autoscroll */
        if(ACTION_STD_CANCEL == action)
            return;
        if(stop_autoscroll(action))
            manual_scroll = true;
    } /* if(!manual_scroll) */

    if(manual_scroll)
    {
        /* user went into manual scrolling, handle it here */
        rb->lcd_set_drawmode(DRMODE_SOLID);
        while(ACTION_STD_CANCEL != action)
        {
            rb->lcd_clear_display();
            rb->snprintf(elapsednames, sizeof(elapsednames),
                         "[Credits] %d-%d/%d", j+1,
                         j+NUM_VISIBLE_LINES, numnames);
            rb->lcd_getstringsize(elapsednames, &credits_w, NULL);
            rb->lcd_putsxy(CREDITS_TARGETPOS, 0, elapsednames);

            for(i=0; i<NUM_VISIBLE_LINES; i++)
                rb->lcd_putsxyf(0, font_h*(i+1), "%s", credits[j+i]);

            rb->lcd_update();

            rb->yield();

            /* wait for user action */
            action = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);
            if(ACTION_STD_CANCEL == action)
                return;
            j = update_rowpos(action, j, NUM_VISIBLE_LINES, numnames);
        }
        return; /* exit without animation */
    }

    action = rb->get_action(CONTEXT_LIST, HZ*3);
    if(ACTION_STD_CANCEL == action)
        return;

    offset_dummy = 1;

    /* now make the text exit to the right */
    for(credits_pos = (LCD_WIDTH/2)-(credits_w/2);
        credits_pos <= LCD_WIDTH+offset_dummy;
        credits_pos += offset_dummy, offset_dummy++)
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(0, 0, LCD_WIDTH, font_h);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_putsxy(credits_pos, 0, elapsednames);
        rb->lcd_update();
    }
}

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

#ifdef HAVE_BACKLIGHT
    /* Turn off backlight timeout */
    backlight_ignore_timeout();
#endif

#if LCD_DEPTH>=16
    rb->lcd_set_foreground (LCD_WHITE);
    rb->lcd_set_background (LCD_BLACK);
#endif
    rb->show_logo();

    /* Show the logo for about 3 secs allowing the user to stop */
    if(!rb->action_userabort(3*HZ))
        roll_credits();

#ifdef HAVE_BACKLIGHT
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings();
#endif

    return PLUGIN_OK;
}
