/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "ata.h"
#include "disk.h"
#include "fat.h"
#include "lcd.h"
#include "rtc.h"
#include "debug.h"
#include "led.h"
#include "kernel.h"
#include "button.h"
#include "tree.h"
#include "panic.h"
#include "menu.h"
#include "system.h"
#include "usb.h"
#ifndef SIMULATOR
#include "dmalloc.h"
#include "bmalloc.h"
#endif
#include "mpeg.h"
#include "main_menu.h"
#include "thread.h"
#include "settings.h"
#include "backlight.h"

#include "version.h"

#include "sh7034.h"
#include "sprintf.h"

char appsversion[]=APPSVERSION;

void init(void);

/* Test code!!! */
void dbg_ports(void)
{
   unsigned short porta;
   unsigned short portb;
   unsigned char portc;
   char buf[32];

   lcd_clear_display();

   while(1)
   {
      porta = PADR;
      portb = PBDR;
      portc = PCDR;
      
      snprintf(buf, 32, "PCDR: %04x", porta);
      lcd_puts(0, 0, buf);
      snprintf(buf, 32, "PCDR: %04x", portb);
      lcd_puts(0, 1, buf);
      snprintf(buf, 32, "PCDR: %02x", portc);
      lcd_puts(0, 2, buf);
      lcd_update();
      sleep(HZ/10);
   }
}

void app_main(void)
{
    init();
    browse_root();
}

#ifdef SIMULATOR

void init(void)
{
    init_threads();
    lcd_init();
    show_logo();
    reset_settings(&global_settings);
    sleep(HZ/2);
}

#else

/* defined in linker script */
extern int poolstart[];
extern int poolend[];

void init(void)
{
    int rc;
    struct partinfo* pinfo;

    system_init();
    kernel_init();

    reset_settings(&global_settings);
    
    dmalloc_initialize();
    bmalloc_add_pool(poolstart, poolend-poolstart);
    lcd_init();

    show_logo();

#ifdef DEBUG
    debug_init();
#endif
    set_irq_level(0);

    usb_init();
    
    rc = ata_init();
    if(rc)
        panicf("ata: %d",rc);

    pinfo = disk_init();
    if (!pinfo)
        panicf("disk: NULL");

    rc = fat_mount(pinfo[0].start);
    if(rc)
        panicf("mount: %d",rc);

    backlight_init();

    button_init();
    mpeg_init( global_settings.volume,
               global_settings.bass,
               global_settings.treble );

    usb_start_monitoring();
}

int main(void)
{
    app_main();

    while(1) {
        led(true); sleep(HZ/10);
        led(false); sleep(HZ/10);
    }
    return 0;
}
#endif
