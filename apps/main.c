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
#include "powermgmt.h"
#include "adc.h"
#include "i2c.h"
#ifndef DEBUG
#include "serial.h"
#endif
#include "mpeg.h"
#include "main_menu.h"
#include "thread.h"
#include "settings.h"
#include "backlight.h"
#include "status.h"
#include "debug_menu.h"
#include "version.h"
#include "sprintf.h"
#include "font.h"
#include "language.h"
#include "wps-display.h"

char appsversion[]=APPSVERSION;

void init(void);

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
    font_init();
    show_logo();
    settings_reset();
    settings_load();
    sleep(HZ/2);
}

#else

/* defined in linker script */
extern int poolstart[];
extern int poolend[];

void init(void)
{
    int rc, i;
    struct partinfo* pinfo;

    system_init();
    kernel_init();

    settings_reset();
    
    lcd_init();

    font_init();
    show_logo();

#ifdef DEBUG
    debug_init();
#else
    serial_setup();
#endif
    set_irq_level(0);

    i2c_init();

#ifdef HAVE_RTC
    rtc_init();
#endif

    adc_init();
    
    usb_init();
    
    backlight_init();

    button_init();

    rc = ata_init();
    if(rc)
    {
#ifdef HAVE_LCD_BITMAP
        char str[32];
        lcd_clear_display();
        snprintf(str, 31, "ATA error: %d", rc);
        lcd_puts(0, 1, str);
        lcd_puts(0, 3, "Press ON to debug");
        lcd_update();
        while(button_get(true) != BUTTON_ON);
        dbg_ports();
#endif
        panicf("ata: %d", rc);
    }
    
    pinfo = disk_init();
    if (!pinfo)
        panicf("disk: NULL");

    for ( i=0; i<4; i++ ) {
        if (!fat_mount(pinfo[i].start))
            break;
    }
    if ( i==4 ) {
        DEBUGF("No partition found, trying to mount sector 0.\n");
        rc = fat_mount(0);
        if(rc)
            panicf("No FAT32 partition!");
    }
    
    settings_load();
    
    mpeg_init( global_settings.volume,
               global_settings.bass,
               global_settings.treble,
               global_settings.balance,
               global_settings.loudness,
               global_settings.bass_boost,
               global_settings.avc );

    status_init();
    usb_start_monitoring();
    power_init();
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
