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
#include "playlist.h"
#include "buffer.h"

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
    buffer_init();
    lcd_init();
    font_init();
    show_logo();
    settings_reset();
    settings_load();
    sleep(HZ/2);
    tree_init();
    mpeg_init( global_settings.volume,
               global_settings.bass,
               global_settings.treble,
               global_settings.balance,
               global_settings.loudness,
               global_settings.bass_boost,
               global_settings.avc,
               global_settings.channel_config );
    while (button_get(false) != 0)
      ; /* Empty the keyboard buffer */
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

    buffer_init();

    settings_reset();
    
    lcd_init();

    font_init();
    show_logo();

    set_irq_level(0);
#ifdef DEBUG
    debug_init();
#else
    serial_setup();
#endif

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
        if(rc) {
            lcd_clear_display();
            lcd_puts(0,0,"No FAT32");
            lcd_puts(0,1,"partition!");
            lcd_update();
            sleep(HZ);
            while(1)
                dbg_partitions();
        }
    }
    
    settings_load();
    
    status_init();
    usb_start_monitoring();
    power_init();
    playlist_init();
    tree_init();

    /* This one must be the last one, since it wants the rest of the buffer
       space */
    mpeg_init( global_settings.volume,
               global_settings.bass,
               global_settings.treble,
               global_settings.balance,
               global_settings.loudness,
               global_settings.bass_boost,
               global_settings.avc,
               global_settings.channel_config );
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
