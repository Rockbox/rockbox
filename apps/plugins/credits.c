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

PLUGIN_HEADER

static const struct plugin_api* rb;

static const char* const credits[] = {
#include "credits.raw" /* generated list of names from docs/CREDITS */
};

#ifdef HAVE_LCD_CHARCELLS

static void roll_credits(void)
{
    int numnames = sizeof(credits)/sizeof(char*);
    int curr_name = 0;
    int curr_len = rb->utf8length(credits[0]);
    int curr_index = 0;
    int curr_line = 0;
    int name, len, new_len, line, x;

    while (1)
    {
        rb->lcd_clear_display();

        name = curr_name;
        x = -curr_index;
        len = curr_len;
        line = curr_line;

        while (x < 11)
        {
            int x2;

            if (x < 0)
                rb->lcd_puts(0, line,
                             credits[name] + rb->utf8seek(credits[name], -x));
            else
                rb->lcd_puts(x, line, credits[name]);

            if (++name >= numnames)
                break;

            line ^= 1;

            x2 = x + len/2;
            if ((unsigned)x2 < 11)
                rb->lcd_putc(x2, line, '*');

            new_len = rb->utf8length(credits[name]);
            x += MAX(len/2 + 2, len - new_len/2 + 1);
            len = new_len;
        }
        rb->lcd_update();

        /* abort on keypress */
        if(rb->action_userabort(HZ/8))
            return;

        if (++curr_index >= curr_len)
        {
            if (++curr_name >= numnames)
                break;
            new_len = rb->utf8length(credits[curr_name]);
            curr_index -= MAX(curr_len/2 + 2, curr_len - new_len/2 + 1);
            curr_len = new_len;
            curr_line ^= 1;
        }
    }
}

#else

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
#if (CONFIG_KEYPAD == RECORDER_PAD)
    #define PAUSE_TIME 1.2
    #define ANIM_SPEED 35
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
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
            {
                rb->snprintf(name, sizeof(name), "%s", credits[j+i]);
                rb->lcd_putsxy(0, font_h*(i+1), name);
            }
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

#endif

enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    long finish;
    (void)parameter;
    rb = api;

    /* Turn off backlight timeout */
    backlight_force_on(rb); /* backlight control in lib/helper.c */

    rb->show_logo();
#ifdef HAVE_LCD_CHARCELLS
    rb->lcd_double_height(false);
#endif

    /* Show the logo for about 3 secs allowing the user to stop */
    finish = *rb->current_tick + 3*HZ;
    while (*rb->current_tick < finish)
        if(rb->action_userabort(finish - *rb->current_tick))
            break;
    roll_credits();
      
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(rb); /* backlight control in lib/helper.c */

    return PLUGIN_OK;
}
