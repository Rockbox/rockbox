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
#include <timefuncs.h>
#include <string.h>
#include <kernel.h>
#include <button.h>

#include <lcd.h>
#include "menu.h"
#include "logf.h"

#ifdef HAVE_LCD_BITMAP
bool logfdisplay(void)

{
    int w, h;
    int lines;
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
    if(!lines)
        return false;

    lcd_setmargins(0, 0);
    lcd_clear_display();
    
    do {
        index = logfindex;
        for(i = lines-1; i>=0; i--) {
            unsigned char buffer[17];

            if(--index < 0) {
                if(logfwrap)
                    index = MAX_LOGF_LINES-1;
                else
                    break; /* done */
            }
        
            memcpy(buffer, logfbuffer[index], 16);
            buffer[16]=0;
            lcd_puts(0, i, buffer);
        }
        lcd_update();
        button = button_get_w_tmo(HZ/2);
    } while(button != BUTTON_OFF);

    return false;
}
#else /* HAVE_LCD_BITMAP */
bool logfdisplay(void)

{
    /* TODO: implement a browser for charcell bitmaps */
    return false;
}
#endif /* HAVE_LCD_BITMAP */

#endif /* ROCKBOX_HAS_LOGF */
