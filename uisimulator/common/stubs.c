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
#include <time.h>
#include <stdbool.h>

#include "debug.h"

#include "screens.h"
#include "button.h"
#include "menu.h"

#include "string.h"
#include "lcd.h"

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

static char patterns[8][7];

void lcd_define_pattern(int which, char *pattern, int length)
{
    int i, j;
    int pat = which / 8;
    char icon[8];
    memset(icon, 0, sizeof icon);
    
    DEBUGF("Defining pattern %d\n", pat);
    for (j = 0; j <= 5; j++) {
        for (i = 0; i < length; i++) {
            if ((pattern[i])&(1<<(j)))
                icon[j] |= (1<<(i));
        }
    }
    for (i = 0; i <= 5; i++)
    {
        patterns[pat][i] = icon[i];
    }
}

char* get_lcd_pattern(int which)
{
    DEBUGF("Get pattern %d\n", which);
    return patterns[which];
}

extern void lcd_puts(int x, int y, unsigned char *str);

void lcd_putc(int x, int y, unsigned char ch)
{
    static char str[2] = "x";
    if (ch <= 8)
    {
        char* bm = get_lcd_pattern(ch);
        lcd_bitmap(bm, x * 6, (y * 8) + 8, 6, 8, true);
        return;
    }
    if (ch == 137) {
        /* Have no good font yet. Simulate the cursor character. */
        ch = '>';
    }
    str[0] = ch;
    lcd_puts(x, y, str);
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

int rtc_read(int address)
{
  time_t now = time(NULL);
  struct tm *teem = localtime(&now);

  switch(address) {
  case 3: /* hour */
    return (teem->tm_hour%10) | ((teem->tm_hour/10) << 4);

  case 2: /* minute */
    return (teem->tm_min%10) | ((teem->tm_min/10) << 4);
  }

  return address ^ 0x55;
}

int rtc_write(int address, int value)
{
  DEBUGF("write %x to address %x\n", value, address);
  return 0;
}
