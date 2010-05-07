/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Daniel Stenberg
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

#ifdef ROCKBOX_HAS_LOGF
#include <file.h>
#include <timefuncs.h>
#include <string.h>
#include <kernel.h>
#include <action.h>

#include <lcd.h>
#include <font.h>
#include "menu.h"
#include "logf.h"
#include "settings.h"
#include "logfdisp.h"
#include "action.h"
#include "splash.h"

#ifdef HAVE_LCD_BITMAP
int compute_nb_lines(int w, struct font* font)
{
    int i, nb_lines;
    int cur_x, delta_x;
    
    if(logfindex == 0 && !logfwrap)
        return 0;
        
    if(logfwrap)
        i = logfindex;
    else
        i = 0;
    
    cur_x = 0;
    nb_lines = 0;
        
    do {
        if(logfbuffer[i] == '\0')
        {
            cur_x = 0;
            nb_lines++;
        }
        else
        {
            /* does character fit on this line ? */
            delta_x = font_get_width(font, logfbuffer[i]);
            
            if(cur_x + delta_x > w)
            {
                cur_x = 0;
                nb_lines++;
            }
            
            /* update pointer */
            cur_x += delta_x;
        }

        i++;
        if(i >= MAX_LOGF_SIZE)
            i = 0;
    } while(i != logfindex);
    
    return nb_lines;
}

bool logfdisplay(void)
{
    int action;
    int w, h, i, index;
    int fontnr;
    int cur_x, cur_y, delta_y, delta_x;
    struct font* font;
    int user_index;/* user_index will be number of the first line to display (warning: line!=logf entry) */
    char buf[2];
    
    fontnr = lcd_getfont();
    font = font_get(fontnr);
    
    /* get the horizontal size of each line */
    font_getstringsize("A", NULL, &delta_y, fontnr);
    
    buf[1] = '\0';
    w = LCD_WIDTH;
    h = LCD_HEIGHT;
    /* start at the end of the log */
    user_index = compute_nb_lines(w, font) - h/delta_y -1; /* if negative, will be set 0 to zero later */

    do {
        lcd_clear_display();
        
        if(user_index < 0)
            user_index = 0;
        
        if(logfwrap)
            i = logfindex;
        else
            i = 0;
        
        index = 0;
        cur_x = 0;
        cur_y = 0;
        
        /* nothing to print ? */
        if(logfindex == 0 && !logfwrap)
            goto end_print;
        
        do {
            if(logfbuffer[i] == '\0')
            {
                /* should be display a newline ? */
                if(index >= user_index)
                    cur_y += delta_y;
                cur_x = 0;
                index++;
            }
            else
            {
                /* does character fit on this line ? */
                delta_x = font_get_width(font, logfbuffer[i]);
                
                if(cur_x + delta_x > w)
                {
                    /* should be display a newline ? */
                    if(index >= user_index)
                        cur_y += delta_y;
                    cur_x = 0;
                    index++;
                }
                
                /* should we print character ? */
                if(index >= user_index)
                {
                    buf[0] = logfbuffer[i];
                    lcd_putsxy(cur_x, cur_y, buf);
                }
                
                /* update pointer */
                cur_x += delta_x;
            }
            
            /* did we fill the screen ? */
            if(cur_y > h)
                break;
            
            i++;
            if(i >= MAX_LOGF_SIZE)
                i = 0;
        } while(i != logfindex);
        
        end_print:
        lcd_update();
        
        action = get_action(CONTEXT_STD, HZ);
        switch( action )
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_NEXTREPEAT:
                user_index++;
                break;
            case ACTION_STD_PREV:
            case ACTION_STD_PREVREPEAT:
                user_index--;
                break;
            case ACTION_STD_OK:
                user_index = 0;
                break;
#ifdef HAVE_TOUCHSCREEN
            case ACTION_TOUCHSCREEN:
            {
                short x, y;
                static int prev_y;
                
                action = action_get_touchscreen_press(&x, &y);
                
                if(action & BUTTON_REL)
                    prev_y = 0;
                else
                {
                    if(prev_y != 0)
                        user_index += (prev_y - y) / delta_y;
                    
                    prev_y = y;
                }
            }
#endif
            default:
                break;
        }
    } while(action != ACTION_STD_CANCEL);

    return false;
}
#else /* HAVE_LCD_BITMAP */
bool logfdisplay(void)

{
    /* TODO: implement a browser for charcell bitmaps */
    return false;
}
#endif /* HAVE_LCD_BITMAP */

/* Store the logf log to logf.txt in the .rockbox directory. The order of the
 * entries will be "reversed" so that the most recently logged entry is on the
 * top of the file */
bool logfdump(void)
{
    int fd;

    splashf(HZ, "Log File Dumped");
    
    /* nothing to print ? */
    if(logfindex == 0 && !logfwrap)
        /* nothing is logged just yet */
        return false;
    
    fd = open(ROCKBOX_DIR "/logf.txt", O_CREAT|O_WRONLY|O_TRUNC, 0666);
    if(-1 != fd) {
        int i;
        
        if(logfwrap)
            i = logfindex;
        else
            i = 0;
        
        do {
            if(logfbuffer[i]=='\0')
                fdprintf(fd, "\n");
            else
                fdprintf(fd, "%c", logfbuffer[i]);
                
            i++;
            if(i >= MAX_LOGF_SIZE)
                i = 0;
        } while(i != logfindex);
        
        close(fd);
    }
    return false;
}

#endif /* ROCKBOX_HAS_LOGF */
