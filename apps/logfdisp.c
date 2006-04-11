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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include <button.h>

#include <lcd.h>
#include "menu.h"
#include "logf.h"

#if CONFIG_KEYPAD == IPOD_4G_PAD
#define LOGF_BUTTON_QUIT BUTTON_MENU
#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
#define LOGF_BUTTON_QUIT BUTTON_POWER
#else
#define LOGF_BUTTON_QUIT BUTTON_OFF
#endif

#ifdef HAVE_LCD_BITMAP
bool logfdisplay(void)

{
    int w, h;
    int lines;
    int columns;
    int i;
    int button;

    bool lcd = false; /* fixed atm */
    int index;

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

    if (columns > MAX_LOGF_ENTRY)
        columns = MAX_LOGF_ENTRY;
            
    if(!lines)
        return false;

    lcd_setmargins(0, 0);
    lcd_clear_display();
    
    do {
        index = logfindex;
        for(i = lines-1; i>=0; i--) {
            unsigned char buffer[columns + 1];

            if(--index < 0) {
                if(logfwrap)
                    index = MAX_LOGF_LINES-1;
                else
                    break; /* done */
            }
        
            memcpy(buffer, logfbuffer[index], columns);
            buffer[columns]=0;
            lcd_puts(0, i, buffer);
        }
        lcd_update();
        button = button_get_w_tmo(HZ/2);
    } while(button != LOGF_BUTTON_QUIT);

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
    
    fd = open("/.rockbox/logf.txt", O_CREAT|O_WRONLY|O_TRUNC);
    if(-1 != fd) {
        unsigned char buffer[MAX_LOGF_ENTRY +1];
        int index = logfindex-1;
        int stop = logfindex;


        while(index != stop) {
            if(index < 0) {
                if(logfwrap)
                    index = MAX_LOGF_LINES-1;
                else
                    break; /* done */
            }
        
            memcpy(buffer, logfbuffer[index], MAX_LOGF_ENTRY);
            buffer[MAX_LOGF_ENTRY]=0;
            fdprintf(fd, "%s\n", buffer);
            index--;
        }
        close(fd);
    }
    return false;
}

#endif /* ROCKBOX_HAS_LOGF */
