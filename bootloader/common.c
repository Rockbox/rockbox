/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: main.c 11997 2007-01-13 09:08:18Z miipekk $
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include "lcd.h"
#include "lcd-remote.h"
#include "font.h"
#include "system.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include "cpu.h"
#include "common.h"
#include "power.h"
#include "kernel.h"
#include "config.h"
#include "logf.h"
#include "button.h"
#include "string.h"
#include "usb.h"
#include "file.h"
#include "loader_strerror.h"
#if defined(MI4_FORMAT)
#include "mi4-loader.h"
#elif defined(RKW_FORMAT)
#include "rkw-loader.h"
#else
#include "rb-loader.h"
#endif

/* TODO: Other bootloaders need to be adjusted to set this variable to true
   on a button press - currently only the ipod, H10, Vibe 500 and Sansa versions do. */
#if defined(IPOD_ARCH) || defined(IRIVER_H10)  || defined(IRIVER_H10_5GB) \
 || defined(SANSA_E200) || defined(SANSA_C200) || defined(GIGABEAT_F) \
 || (CONFIG_CPU == AS3525) || (CONFIG_CPU == AS3525v2) || defined(COWON_D2) \
 || defined(MROBE_100) || defined(MROBE_500) \
 || defined(SAMSUNG_YH925) || defined(SAMSUNG_YH920) \
 || defined(SAMSUNG_YH820) || defined(PHILIPS_SA9200) \
 || defined(PHILIPS_HDD1630) || defined(PHILIPS_HDD6330) \
 || defined(ONDA_VX747) || defined(PBELL_VIBE500) \
 || defined(TOSHIBA_GIGABEAT_S) || defined(XDUOO_X3) \
 || defined(IHIFI770) || defined(IHIFI770C) || defined(IHIFI800)
bool verbose = false;
#else
bool verbose = true;
#endif

#if !(CONFIG_PLATFORM & PLATFORM_HOSTED)
int line = 0;
#ifdef HAVE_REMOTE_LCD
int remote_line = 0;
#endif

void reset_screen(void)
{
    lcd_clear_display();
    line = 0;
#ifdef HAVE_REMOTE_LCD
    lcd_remote_clear_display();
    remote_line = 0;
#endif
}

int printf(const char *format, ...)
{
    static char printfbuf[256];
    int len;
    unsigned char *ptr;
    va_list ap;
    va_start(ap, format);

    ptr = printfbuf;
    len = vsnprintf(ptr, sizeof(printfbuf), format, ap);
    va_end(ap);

    lcd_puts(0, line++, ptr);
    if (verbose)
        lcd_update();
    if(line >= LCD_HEIGHT/SYSFONT_HEIGHT)
        line = 0;
#ifdef HAVE_REMOTE_LCD
    lcd_remote_puts(0, remote_line++, ptr);
    if (verbose)
        lcd_remote_update();
    if(remote_line >= LCD_REMOTE_HEIGHT/SYSFONT_HEIGHT)
        remote_line = 0;
#endif
    return len;
}
#endif

void error(int errortype, int error, bool shutdown)
{
    switch(errortype)
    {
    case EATA:
        printf("ATA error: %d", error);
        break;

    case EDISK:
        printf("No partition found");
        break;

    case EBOOTFILE:
        printf(loader_strerror(error));
        break;
    }

    lcd_update();
    sleep(5*HZ);

    if(shutdown)
        power_off();
}

/* Load raw binary image. */
int load_raw_firmware(unsigned char* buf, char* firmware, int buffer_size)
{
    int fd;
    int rc;
    int len;
    char filename[MAX_PATH];

    snprintf(filename,sizeof(filename),"%s",firmware);
    fd = open(filename, O_RDONLY);
    if(fd < 0)
    {
        return EFILE_NOT_FOUND;
    }

    len = filesize(fd);
    
    if (len > buffer_size)
        return EFILE_TOO_BIG;

    rc = read(fd, buf, len);
    if(rc < len)
        return EREAD_IMAGE_FAILED;

    close(fd);
    return len;
}

/* FIXME?: unused broken code */
#if 0
#ifdef ROCKBOX_HAS_LOGF /* Logf display helper for the bootloader */

#define LINES   (LCD_HEIGHT/SYSFONT_HEIGHT)
#define COLUMNS ((LCD_WIDTH/SYSFONT_WIDTH) > MAX_LOGF_ENTRY ? \
                            MAX_LOGF_ENTRY : (LCD_WIDTH/SYSFONT_WIDTH))

#ifdef ONDA_VX747
#define LOGF_UP     BUTTON_VOL_UP
#define LOGF_DOWN   BUTTON_VOL_DOWN
#define LOGF_CLEAR  BUTTON_MENU
#else
#warning No keymap defined for this target
#endif

void display_logf(void) /* Doesn't return! */
{
    int i, index, button, user_index=0;
#ifdef HAVE_TOUCHSCREEN
    int touch, prev_y=0;
#endif
    char buffer[COLUMNS+1];
    
    while(1)
    {
        index = logfindex + user_index;
        
        lcd_clear_display();
        for(i = LINES-1; i>=0; i--)
        {
            if(--index < 0)
            {
                if(logfwrap)
                    index = MAX_LOGF_LINES-1;
                else
                    break; /* done */
            }
            
            memcpy(buffer, logfbuffer[index], COLUMNS);
            
            if (logfbuffer[index][MAX_LOGF_ENTRY] == LOGF_TERMINATE_CONTINUE_LINE)
                buffer[MAX_LOGF_ENTRY-1] = '>';
            else if (logfbuffer[index][MAX_LOGF_ENTRY] == LOGF_TERMINATE_MULTI_LINE)
                buffer[MAX_LOGF_ENTRY-1] = '\0';
            
            buffer[COLUMNS] = '\0';
            
            lcd_puts(0, i, buffer);
        }
        
        button = button_get(false);
        if(button == SYS_USB_CONNECTED)
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
        else if(button == SYS_USB_DISCONNECTED)
            ;
        else if(button & LOGF_UP)
            user_index++;
        else if(button & LOGF_DOWN)
            user_index--;
        else if(button & LOGF_CLEAR)
            user_index = 0;
#ifdef HAVE_TOUCHSCREEN
        else if(button & BUTTON_TOUCHSCREEN)
        {
            touch = button_get_data();
            
            if(button & BUTTON_REL)
                prev_y = 0;
            
            if(prev_y != 0)
                user_index += (prev_y - (touch & 0xFFFF)) / SYSFONT_HEIGHT;
            prev_y = touch & 0xFFFF;
        }
#endif
        
        lcd_update();
        sleep(HZ/16);
    }
}
#endif
#endif
