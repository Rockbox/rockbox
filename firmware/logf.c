/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
 * logf() logs 16 bytes in a circular buffer. Each logged string is space-
 * padded for easier and faster output on screen. Just output 16 lines on each
 * line. 16 bytes fit nicely on the iRiver remote LCD (128 pixels with an 8
 * pixels font).
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sprintf.h>
#include <stdbool.h>
#include "config.h"
#include "lcd-remote.h"
#include "logf.h"

unsigned char logfbuffer[MAX_LOGF_LINES][16];
int logfindex;
bool logfwrap;

#ifdef HAVE_REMOTE_LCD
static void displayremote(void)
{
    /* TODO: we should have a debug option that enables/disables this! */
    int w, h;
    int lines;
    int i;
    int index;

    lcd_remote_getstringsize("A", &w, &h);
    lines = LCD_REMOTE_HEIGHT/h;

    lcd_remote_setmargins(0, 0);
    lcd_remote_clear_display();
    
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
        lcd_remote_puts(0, i, buffer);
    }
    lcd_remote_update();   
}
#else
#define displayremote()
#endif

void logf(const char *format, ...)
{
    int len;
    unsigned char *ptr;
    va_list ap;
    va_start(ap, format);

    if(logfindex >= MAX_LOGF_LINES) {
        /* wrap */
        logfwrap = true;
        logfindex = 0;
    }
    ptr = logfbuffer[logfindex];
    len = vsnprintf(ptr, 16, format, ap);
    va_end(ap);
    if(len < 16)
        /* pad with spaces up to the 16 byte border */
        memset(ptr+len, ' ', 16-len);

    logfindex++; /* leave it where we write the next time */

    displayremote();
}
