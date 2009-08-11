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
#include <sprintf.h>
#include <timefuncs.h>
#include <string.h>
#include <kernel.h>
#include <action.h>

#include <lcd.h>
#include "menu.h"
#include "logf.h"
#include "settings.h"
#include "logfdisp.h"
#include "action.h"

#ifdef HAVE_LCD_BITMAP
bool logfdisplay(void)
{
    int w, h;
    int lines;
    int columns;
    int i;
    int action;

    bool lcd = false; /* fixed atm */
    int index, user_index=0;

    lcd_getstringsize("A", &w, &h);
    lines = (lcd?
#ifdef HAVE_REMOTE_LCD
             LCD_REMOTE_HEIGHT
#else
             0
#endif
             :LCD_HEIGHT)/h;
    columns = (lcd?
#ifdef HAVE_REMOTE_LCD
             LCD_REMOTE_WIDTH
#else
             0
#endif
             :LCD_WIDTH)/w;

    if (columns > MAX_LOGF_ENTRY+1)
        columns = MAX_LOGF_ENTRY+1;
            
    if(!lines)
        return false;

    do {
        lcd_clear_display();
        
        index = logfindex + user_index;
        for(i = lines-1; i>=0; i--) {
            unsigned char buffer[columns + 1];

            if(--index < 0) {
                if(logfwrap)
                    index = MAX_LOGF_LINES-1;
                else
                    break; /* done */
            }
        
            memcpy(buffer, logfbuffer[index], columns);
            if (logfbuffer[index][MAX_LOGF_ENTRY] == LOGF_TERMINATE_CONTINUE_LINE)
                buffer[columns-1] = '>';
            else if (logfbuffer[index][MAX_LOGF_ENTRY] == LOGF_TERMINATE_MULTI_LINE)
                buffer[columns-1] = '\0';
            buffer[columns] = '\0';
        
            lcd_puts(0, i, buffer);
        }
        lcd_update();
        
        action = get_action(CONTEXT_STD, HZ);
        if(action == ACTION_STD_NEXT)
            user_index++;
        else if(action == ACTION_STD_PREV)
            user_index--;
        else if(action == ACTION_STD_OK)
            user_index = 0;
#ifdef HAVE_TOUCHSCREEN
        else if(action == ACTION_TOUCHSCREEN)
        {
            short x, y;
            static int prev_y;
            
            action = action_get_touchscreen_press(&x, &y);
            
            if(action & BUTTON_REL)
                prev_y = 0;
            else
            {
                if(prev_y != 0)
                    user_index += (prev_y - y) / h;
                
                prev_y = y;
            }
        }
#endif
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

    if(!logfindex && !logfwrap)
        /* nothing is logged just yet */
        return false;
    
    fd = open(ROCKBOX_DIR "/logf.txt", O_CREAT|O_WRONLY|O_TRUNC);
    if(-1 != fd) {
        unsigned char buffer[MAX_LOGF_ONE_LINE_SIZE +1];
        unsigned char *ptr;
        int index = logfindex-1;
        int stop = logfindex;
        int tindex;
        bool dumpwrap = false;
        bool multiline;

        while(!dumpwrap || (index >= stop)) {
            if(index < 0) {
                if(logfwrap)
                {
                    index = MAX_LOGF_LINES-1;
                    dumpwrap = true;
                }
                else
                    break; /* done */
            }

            multiline = false;
            if (logfbuffer[index][MAX_LOGF_ENTRY] == LOGF_TERMINATE_MULTI_LINE)
            {
                multiline = true;
                do {
                    index--;
                    if(index < 0) {
                        if(logfwrap)
                        {
                            index = MAX_LOGF_LINES-1;
                            dumpwrap = true;
                        }
                        else
                            goto end_loop;
                    }
                } while(logfbuffer[index][MAX_LOGF_ENTRY] == LOGF_TERMINATE_CONTINUE_LINE);
                index++;
                if (index >= MAX_LOGF_LINES)
                    index = 0;
            }

            tindex = index-1;
            ptr = buffer;
            do {
                tindex++;
                memcpy(ptr, logfbuffer[tindex], MAX_LOGF_ENTRY-1);
                ptr += MAX_LOGF_ENTRY-1;
                if (tindex >= MAX_LOGF_LINES)
                    tindex = 0;
            } while(logfbuffer[tindex][MAX_LOGF_ENTRY] == LOGF_TERMINATE_CONTINUE_LINE);
            *ptr = '\0';
        
            fdprintf(fd, "%s\n", buffer);
            index--;
        }
end_loop:
        close(fd);
    }
    return false;
}

#endif /* ROCKBOX_HAS_LOGF */
