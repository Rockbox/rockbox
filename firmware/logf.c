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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
 * logf() logs MAX_LOGF_ENTRY (29) bytes per entry in a circular buffer. Each
 * logged string is space- padded for easier and faster output on screen. Just
 * output MAX_LOGF_ENTRY characters on each line. MAX_LOGF_ENTRY bytes fit
 * nicely on the iRiver remote LCD (128 pixels with an 8x6 pixels font).
 *
 * When the length of log exceeds MAX_LOGF_ENTRY bytes, dividing into the
 * string of length is MAX_LOGF_ENTRY-1 bytes.
 *
 * logfbuffer[*]:
 *
 *    |<-   MAX_LOGF_ENTRY bytes   ->|1|
 *    | log data area                |T|
 *
 *  T : log terminate flag
 *   == LOGF_TERMINATE_ONE_LINE(0x00)      : log data end (one line)
 *   == LOGF_TERMINATE_CONTINUE_LINE(0x01) : log data continues
 *   == LOGF_TERMINATE_MULTI_LINE(0x02)    : log data end (multi line)
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "config.h"
#include "lcd-remote.h"
#include "logf.h"
#include "serial.h"

#ifdef HAVE_USBSTACK
#include "usb_core.h"
#include "usbstack/usb_serial.h"
#endif

/* Only provide all this if asked to */
#ifdef ROCKBOX_HAS_LOGF

#ifndef __PCTOOL__
unsigned char logfbuffer[MAX_LOGF_LINES][MAX_LOGF_ENTRY+1];
int logfindex;
bool logfwrap;
#endif

#ifdef HAVE_REMOTE_LCD
static void displayremote(void)
{
    /* TODO: we should have a debug option that enables/disables this! */
    int w, h;
    int lines;
    int columns;
    int i;
    int index;

    lcd_remote_getstringsize("A", &w, &h);
    lines = LCD_REMOTE_HEIGHT/h;
    columns = LCD_REMOTE_WIDTH/w;
    lcd_remote_clear_display();
    
    index = logfindex;
    for(i = lines-1; i>=0; i--) {
        unsigned char buffer[columns+1];

        if(--index < 0) {
            if(logfwrap)
                index = MAX_LOGF_LINES-1;
            else
                break; /* done */
        }
        
        memcpy(buffer, logfbuffer[index], columns);
        buffer[columns]=0;
        lcd_remote_puts(0, i, buffer);
    }
    lcd_remote_update();   
}
#else
#define displayremote()
#endif

static void check_logfindex(void)
{
    if(logfindex >= MAX_LOGF_LINES) {
        /* wrap */
        logfwrap = true;
        logfindex = 0;
    }
}

#ifdef __PCTOOL__
void _logf(const char *format, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    
    vsnprintf(buf, sizeof buf, format, ap);
    printf("DEBUG: %s\n", buf);
}
#else
void _logf(const char *format, ...)
{
    int len;
    int tlen;
    unsigned char buf[MAX_LOGF_ONE_LINE_SIZE];
    unsigned char *ptr;
    va_list ap;
    bool multiline = false;

    va_start(ap, format);
    vsnprintf(buf, MAX_LOGF_ONE_LINE_SIZE, format, ap);
    va_end(ap);

    len = strlen(buf);
#ifdef HAVE_SERIAL
    serial_tx(buf);
    serial_tx("\r\n");
#endif
#ifdef USB_SERIAL
    usb_serial_send(buf, len);
    usb_serial_send("\r\n", 2);
#endif

    tlen = 0;
    check_logfindex();
    while(len > MAX_LOGF_ENTRY)
    {
        ptr = logfbuffer[logfindex];
        strncpy(ptr, buf + tlen, MAX_LOGF_ENTRY-1);
        ptr[MAX_LOGF_ENTRY] = LOGF_TERMINATE_CONTINUE_LINE;
        logfindex++;
        check_logfindex();
        len -= MAX_LOGF_ENTRY-1;
        tlen += MAX_LOGF_ENTRY-1;
        multiline = true;
    }
    ptr = logfbuffer[logfindex];
    strcpy(ptr, buf + tlen);

    if(len < MAX_LOGF_ENTRY)
        /* pad with spaces up to the MAX_LOGF_ENTRY byte border */
        memset(ptr+len, ' ', MAX_LOGF_ENTRY-len);
    ptr[MAX_LOGF_ENTRY] = (multiline)?LOGF_TERMINATE_MULTI_LINE:LOGF_TERMINATE_ONE_LINE;

    logfindex++; /* leave it where we write the next time */

    displayremote();
}
#endif

#endif
