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
#include "rolo.h"
#include "screens.h"
#include "power.h"

/* diagnostic, to be removed later: */
/* snapshot of the I/O space on startup, to compare initialisations */
union
{
    unsigned char  port8 [512];
    unsigned short port16[256];
    unsigned       port32[128];
} startup_io;


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
    playlist_init();
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
#endif

    adc_init();
    
    usb_init();
    
    backlight_init();

    button_init();

    powermgmt_init();

    if (coldstart && charger_inserted())
    {
        rc = charging_screen(); /* display a "charging" screen */
        if (rc == 1 || rc == 2)  /* charger removed or "Off/Stop" pressed */
            power_off();
        /* "On" pressed or USB connected: proceed */
        ide_power_enable(true);
    }

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
    playlist_init();
    tree_init();

    /* No buffer allocation (see buffer.c) may take place after the call to
       mpeg_init() since the mpeg thread takes the rest of the buffer space */
    mpeg_init( global_settings.volume,
               global_settings.bass,
               global_settings.treble,
               global_settings.balance,
               global_settings.loudness,
               global_settings.bass_boost,
               global_settings.avc,
               global_settings.channel_config );

    /* no auto-rolo on startup any more, but I leave it here for reference */
#if 0
    if (coldstart && !usb_detect())
    {   /* when starting from flash, this time _we_ have to yield */
        int fd;
#ifdef ARCHOS_PLAYER
        static const char filename[] = "/archos.mod"; 
#else
        static const char filename[] = "/ajbrec.ajz";
#endif
        fd = open(filename, O_RDONLY);
        if(fd >= 0) /* no complaint if it doesn't exit */
        {
            close(fd);
            rolo_load((char*)filename); /* start if it does */
        }
    }
#endif // #if 0
}

int main(void)
{
    /* diagnostic, to be removed later: dump I/O space */
    int i;
    for (i = 0; i < 512; i++)
    {   // most can be read with 8 bit access
        startup_io.port8[i] = *((volatile unsigned char*)0x5FFFE00 + i);
    }
    // some don't allow that, read with 32 bit if aligned
    startup_io.port32[0x140/4] = *((volatile unsigned char*)0x5FFFF40);
    startup_io.port32[0x144/4] = *((volatile unsigned char*)0x5FFFF44);
    startup_io.port32[0x150/4] = *((volatile unsigned char*)0x5FFFF50);
    startup_io.port32[0x154/4] = *((volatile unsigned char*)0x5FFFF54);
    startup_io.port32[0x160/4] = *((volatile unsigned char*)0x5FFFF60);
    startup_io.port32[0x170/4] = *((volatile unsigned char*)0x5FFFF70);
    startup_io.port32[0x174/4] = *((volatile unsigned char*)0x5FFFF74);

    // read the rest with 16 bit
    startup_io.port16[0x14A/2] = *((volatile unsigned short*)0x5FFFF4A);
    startup_io.port16[0x15A/2] = *((volatile unsigned short*)0x5FFFF5A);
    startup_io.port16[0x16A/2] = *((volatile unsigned short*)0x5FFFF6A);
    startup_io.port16[0x17A/2] = *((volatile unsigned short*)0x5FFFF7A);
    /* end of diagnostic */
    
    app_main();

    while(1) {
        led(true); sleep(HZ/10);
        led(false); sleep(HZ/10);
    }
    return 0;
}
#endif

