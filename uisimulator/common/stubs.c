/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg <bjorn@haxx.se>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include "debug.h"

#include "screens.h"
#include "button.h"
#include "menu.h"

void backlight_on(void)
{
  /* we could do something better here! */
}

void backlight_time(int dummy)
{
    (void)dummy;
}

int fat_startsector(void)
{
    return 63;
}

int ata_write_sectors(unsigned long start,
                      unsigned char count,
                      void* buf)
{
    int i;
    
    for (i=0; i<count; i++ ) {
        FILE* f;
        char name[32];

        DEBUGF("Writing sector %X\n",start+i);
        sprintf(name,"sector%lX.bin",start+i);
        f=fopen(name,"w");
        if (f) {
            fwrite(buf,512,1,f);
            fclose(f);
        }
    }
    return 1;
}

int ata_read_sectors(unsigned long start,
                     unsigned char count,
                     void* buf)
{
    int i;
    
    for (i=0; i<count; i++ ) {
        FILE* f;
        char name[32];

        DEBUGF("Reading sector %X\n",start+i);
        sprintf(name,"sector%lX.bin",start+i);
        f=fopen(name,"r");
        if (f) {
            fread(buf,512,1,f);
            fclose(f);
        }
    }
    return 1;
}

void ata_delayed_write(unsigned long sector, void* buf)
{
    ata_write_sectors(sector,1,buf);
}

void ata_flush(void)
{
}

void ata_spin(void)
{
}

void ata_spindown(int s)
{
    (void)s;
}

bool simulate_usb(void)
{
    usb_display_info();
    while (button_get(true) & BUTTON_REL);
    return false;
}

void lcd_define_pattern (int which,char *pattern,int length)
{
    (void)which;
    (void)pattern;
    (void)length;
}

void lcd_putc(int x, int y, unsigned char ch)
{
    (void)x;
    (void)y;
    (void)ch;
}

void lcd_set_contrast( int x )
{
    (void)x;
}

void backlight_set_timeout(int seconds)
{
  (void)seconds;
}

void backlight_set_on_when_charging(bool beep)
{
  (void)beep;
}
