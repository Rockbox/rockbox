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
#include "mp3_playback.h"
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
#include "rolo.h"
#include "screens.h"
#include "power.h"
#include "talk.h"
#include "plugin.h"

/*#define AUTOROCK*/ /* define this to check for "autostart.rock" on boot */

const char appsversion[]=APPSVERSION;

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
    settings_load(SETTINGS_ALL);
    settings_apply();
    sleep(HZ/2);
    tree_init();
    playlist_init();
    mp3_init( global_settings.volume,
              global_settings.bass,
              global_settings.treble,
              global_settings.balance,
              global_settings.loudness,
              global_settings.avc,
              global_settings.channel_config,
              global_settings.mdb_strength,
              global_settings.mdb_harmonics,
              global_settings.mdb_center,
              global_settings.mdb_shape,
              global_settings.mdb_enable,
              global_settings.superbass);
    mpeg_init();
    while (button_get(false) != 0)
      ; /* Empty the keyboard buffer */
}

#else

void init(void)
{
    int rc, i;
    struct partinfo* pinfo;
    /* if nobody initialized ATA before, I consider this a cold start */
    bool coldstart = (PACR2 & 0x4000) != 0; /* starting from Flash */

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
    settings_load(SETTINGS_RTC); /* early load parts of global_settings */
#endif

    adc_init();
    
    usb_init();
    
    backlight_init();

    button_init();

    powermgmt_init();

#ifdef HAVE_BATTERIES
    if (coldstart && charger_inserted() && !global_settings.car_adapter_mode)
    {
        rc = charging_screen(); /* display a "charging" screen */
        if (rc == 1 || rc == 2)  /* charger removed or "Off/Stop" pressed */
            power_off();
        /* "On" pressed or USB connected: proceed */
    }
#else
    (void)coldstart;
#endif

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
        while(!(button_get(true) & BUTTON_REL));
        dbg_ports();
#endif
        panicf("ata: %d", rc);
    }
    
    usb_start_monitoring();

/* temporary hack for Ondio, which can't browse without disk I/O */
#ifdef HAVE_MMC
    while(button_get(true) & BUTTON_REL) {}; /* wait to see the logo */
    while(1) main_menu(); /* just show the main menu, all we can do */
#endif
/* end of Ondio hack */

    pinfo = disk_init();
    if (!pinfo)
    {
        lcd_clear_display();
        lcd_puts(0, 0, "No partition");
        lcd_puts(0, 1, "table.");
#ifdef HAVE_LCD_BITMAP
        lcd_puts(0, 2, "Insert USB cable");
        lcd_puts(0, 3, "and fix it.");
        lcd_update();
#endif
        while(button_get(true) != SYS_USB_CONNECTED) {};
        usb_screen();
        system_reboot();
    }

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
            /* Don't leave until we have been in USB mode */
            while(!dbg_partitions());

            /* The USB thread will panic if the drive still can't be mounted */
        }
    }

    settings_load(SETTINGS_ALL);
    settings_apply();
    
    status_init();
    playlist_init();
    tree_init();

    /* No buffer allocation (see buffer.c) may take place after the call to
       mpeg_init() since the mpeg thread takes the rest of the buffer space */
    mp3_init( global_settings.volume,
              global_settings.bass,
              global_settings.treble,
              global_settings.balance,
              global_settings.loudness,
              global_settings.avc,
              global_settings.channel_config,
              global_settings.mdb_strength,
              global_settings.mdb_harmonics,
              global_settings.mdb_center,
              global_settings.mdb_shape,
              global_settings.mdb_enable,
              global_settings.superbass);
    mpeg_init();
    talk_init();

#ifdef AUTOROCK
    if (!usb_detect())
    {   
        int fd;
        static const char filename[] = PLUGIN_DIR "/autostart.rock"; 

        fd = open(filename, O_RDONLY);
        if(fd >= 0) /* no complaint if it doesn't exit */
        {
            close(fd);
            plugin_load((char*)filename, NULL); /* start if it does */
        }
    }
#endif /* #ifdef AUTOROCK */
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

