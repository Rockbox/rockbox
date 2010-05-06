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
 * logf() logs entries in a circular buffer. Each logged string is null-terminated.
 *
 * When the length of log exceeds MAX_LOGF_SIZE bytes, the buffer wraps.
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "config.h"
#include "system.h"
#include "font.h"
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
unsigned char logfbuffer[MAX_LOGF_SIZE];
int logfindex;
bool logfwrap;
#endif

#ifdef HAVE_REMOTE_LCD
static void displayremote(void)
{
    /* TODO: we should have a debug option that enables/disables this! */
    int w, h, i;
    int fontnr;
    int cur_x, cur_y, delta_y, delta_x;
    struct font* font;
    int nb_lines;
    char buf[2];
    /* Memorize the pointer to the beginning of the last ... lines
       I assume delta_y >= 6 to avoid wasting memory and allocating memory dynamically
       I hope there is no font with height < 6 ! */
    const int NB_ENTRIES=LCD_REMOTE_HEIGHT / 6;
    int line_start_ptr[NB_ENTRIES];
    
    fontnr = lcd_getfont();
    font = font_get(fontnr);
    
    /* get the horizontal size of each line */
    font_getstringsize("A", NULL, &delta_y, fontnr);
    
    /* font too small ? */
    if(delta_y < 6)
        return;
    /* nothing to print ? */
    if(logfindex == 0 && !logfwrap)
        return;
    
    w = LCD_REMOTE_WIDTH;
    h = LCD_REMOTE_HEIGHT;
    nb_lines = 0;
    
    if(logfwrap)
        i = logfindex;
    else
        i = 0;
    
    cur_x = 0;
    
    line_start_ptr[0] = i;
    
    do
    {
        if(logfbuffer[i] == '\0')
        {
            line_start_ptr[++nb_lines % NB_ENTRIES] = i+1;
            cur_x = 0;
        }
        else
        {
            /* does character fit on this line ? */
            delta_x = font_get_width(font, logfbuffer[i]);
            
            if(cur_x + delta_x > w)
            {
                cur_x = 0;
                line_start_ptr[++nb_lines % NB_ENTRIES] = i;
            }
            /* update pointer */
            cur_x += delta_x;
        }
        i++;
        if(i >= MAX_LOGF_SIZE)
            i = 0;
    } while(i != logfindex);
    
    lcd_remote_clear_display();
    
    i = line_start_ptr[ MAX(nb_lines - h / delta_y, 0) % NB_ENTRIES];
    cur_x = 0;
    cur_y = 0;
    buf[1] = '\0';
    
    do {
        if(logfbuffer[i] == '\0')
        {
            cur_y += delta_y;
            cur_x = 0;
        }
        else
        {
            /* does character fit on this line ? */
            delta_x = font_get_width(font, logfbuffer[i]);
            
            if(cur_x + delta_x > w)
            {
                cur_y += delta_y;
                cur_x = 0;
            }
            
            buf[0] = logfbuffer[i];
            lcd_remote_putsxy(cur_x, cur_y, buf);
            cur_x += delta_x;
        }
        
        i++;
        if(i >= MAX_LOGF_SIZE)
            i = 0;
    } while(i != logfindex);
    
    lcd_remote_update();   
}
#else
#define displayremote()
#endif

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
static void check_logfindex(void)
{
    if(logfindex >= MAX_LOGF_SIZE) 
    {
        /* wrap */
        logfwrap = true;
        logfindex = 0;
    }
}

static int logf_push(void *userp, unsigned char c)
{
    (void)userp;
    
    logfbuffer[logfindex++] = c;
    check_logfindex();
    
#if defined(HAVE_SERIAL) && !defined(SIMULATOR)
    if(c != '\0')
    {
        char buf[2];
        buf[0] = c;
        buf[1] = '\0';
        serial_tx(buf);
    }
#endif
    
    return true;
}

void _logf(const char *fmt, ...)
{
    #ifdef USB_ENABLE_SERIAL
    int old_logfindex = logfindex;
    #endif
    va_list ap;

    va_start(ap, fmt);
    
#ifdef SIMULATOR
    char buf[1024];
    vsnprintf(buf, sizeof buf, fmt, ap);
    DEBUGF("%s\n", buf);
#endif

    vuprintf(logf_push, NULL, fmt, ap);
    va_end(ap);
    
    /* add trailing zero */
    logf_push(NULL, '\0');
    
#if defined(HAVE_SERIAL) && !defined(SIMULATOR)
    serial_tx("\r\n");
#endif
#ifdef USB_ENABLE_SERIAL

    if(logfindex < old_logfindex)
    {
        usb_serial_send(logfbuffer + old_logfindex, MAX_LOGF_SIZE - old_logfindex);
        usb_serial_send(logfbuffer, logfindex - 1);
    }
    else
        usb_serial_send(logfbuffer + old_logfindex, logfindex - old_logfindex - 1);
    usb_serial_send("\r\n", 2);
#endif  

    displayremote();
}
#endif

#endif
