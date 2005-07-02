/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Jörg Hohensohn  aka [IDC]Dragon
 *
 * This is "Bootbox", a minimalistic loader, rescue firmware for just
 *   booting into a full features one. Aside from that it does charging
 *   and USB mode, to enable copying the desired firmware.
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "buffer.h"
#include "rolo.h"
#include "usb.h"
#include "powermgmt.h"

int line = 0;

void usb_screen(void)
{
    lcd_clear_display();
    lcd_puts(0, 0, "USB mode");
#ifdef HAVE_LCD_BITMAP
    lcd_update();
#endif
    usb_acknowledge(SYS_USB_CONNECTED_ACK);
    while(usb_wait_for_disconnect_w_tmo(&button_queue, HZ)) {
    }
}

int show_logo(void)
{
    lcd_clear_display();
    lcd_puts(0, 0, "Rockbox");
    lcd_puts(0, 1, "Rescue boot");
#ifdef HAVE_LCD_BITMAP
    lcd_update();
#endif
    return 0;
}

#ifdef HAVE_CHARGING
int charging_screen(void)
{
    unsigned int button;
    int rc = 0;
#ifdef BUTTON_OFF
    const unsigned int offbutton = BUTTON_OFF;
#else
    const unsigned int offbutton = BUTTON_STOP;
#endif

    ide_power_enable(false); /* power down the disk, else would be spinning */

    lcd_clear_display();
    lcd_puts(0, 0, "charging...");
#ifdef HAVE_LCD_BITMAP
    lcd_update();
#endif

    do
    {
        button = button_get_w_tmo(HZ/2);
#ifdef BUTTON_ON
        if (button == (BUTTON_ON | BUTTON_REL))
#else
        if (button == (BUTTON_RIGHT | BUTTON_REL))
#endif
            rc = 3;
        else if (button == offbutton)
            rc = 2;
        else
        {
            if (usb_detect())
                rc = 4;
            else if (!charger_inserted())
                rc = 1;
        }
    } while (!rc);

    return rc;
}
#endif /* HAVE_CHARGING */

#ifdef HAVE_MMC
int mmc_remove_request(void)
{
    /* A dummy function here, we won't access the card
       before entering USB mode */
    return 0;
}
#endif /* HAVE_MMC */


void main(void)
{
    int rc;

    power_init();
    system_init();
    kernel_init();
    buffer_init();
    lcd_init();
    show_logo();
    set_irq_level(0);
    adc_init();
    usb_init();
    button_init();
    powermgmt_init();

#if defined(HAVE_CHARGING) && (CONFIG_CPU == SH7034)
    if (charger_inserted()
#ifdef ATA_POWER_PLAYERSTYLE
        && !ide_powered() /* relies on probing result from bootloader */
#endif
        )
    {
        rc = charging_screen(); /* display a "charging" screen */
        if (rc == 1 || rc == 2)  /* charger removed or "Off/Stop" pressed */
            power_off();
        /* "On" pressed or USB connected: proceed */
        show_logo();  /* again, to provide better visual feedback */
    }
#endif

    rc = ata_init();
    if(rc)
    {
#ifdef HAVE_LCD_BITMAP
        char str[32];
        lcd_clear_display();
        snprintf(str, 31, "ATA error: %d", rc);
        lcd_puts(0, 1, str);
        lcd_update();
        while(!(button_get(true) & BUTTON_REL));
#endif
        panicf("ata: %d", rc);
    }

    //disk_init();
    usb_start_monitoring();
    while (usb_detect())
    {   /* enter USB mode early, before trying to mount */
        if (button_get_w_tmo(HZ/10) == SYS_USB_CONNECTED)
        {
            usb_screen();
        }
    }

    rc = disk_mount_all();
    if (rc<=0)
    {
        lcd_clear_display();
        lcd_puts(0, 0, "No partition");
        lcd_puts(0, 1, "found.");
#ifdef HAVE_LCD_BITMAP
        lcd_puts(0, 2, "Insert USB cable");
        lcd_puts(0, 3, "and fix it.");
        lcd_update();
#endif
        while(button_get(true) != SYS_USB_CONNECTED) {};
        usb_screen();
        system_reboot();
    }

    {   // rolo the firmware
        static const char filename[] = "/" BOOTFILE; 
        rolo_load((char*)filename); /* won't return if started */

        lcd_clear_display();
        lcd_puts(0, 0, "No firmware");
        lcd_puts(0, 1, filename);
#ifdef HAVE_LCD_BITMAP
        lcd_update();
#endif
        while(!(button_get(true) & BUTTON_REL));
        system_reboot();
    }


}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */

void screen_dump(void)
{
}

int dbg_ports(void)
{
   return 0;
}

void audio_stop(void)
{
}

int audio_status(void)
{
    return 0;
}

void mp3_shutdown(void)
{
}
/*
void i2c_init(void)
{
}

void backlight_on(void)
{
}
*/
