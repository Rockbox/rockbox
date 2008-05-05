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

void usb_screen(void)
{
    lcd_clear_display();
    lcd_puts(0, 0, "USB mode");
    lcd_update();

    usb_acknowledge(SYS_USB_CONNECTED_ACK);
    while(usb_wait_for_disconnect_w_tmo(&button_queue, HZ)) {
    }
}

int show_logo(void)
{
    lcd_clear_display();
    lcd_puts(0, 0, "Rockbox");
    lcd_puts(0, 1, "Rescue boot");
    lcd_update();

    return 0;
}

#if CONFIG_CHARGING
/*
bool backlight_get_on_when_charging(void)
{
    return false;
}
*/
void charging_screen(void)
{
    unsigned int button;
    const char* msg;

    ide_power_enable(false); /* power down the disk, else would be spinning */

    lcd_clear_display();

    do
    {
#if CONFIG_CHARGING == CHARGING_CONTROL
        if (charge_state == CHARGING)
            msg = "charging";
        else if (charge_state == TOPOFF)
            msg = "topoff charge";
        else if (charge_state == TRICKLE)
            msg = "trickle charge";
        else
            msg = "not charging";

#else
        msg = "charging";
#endif
        lcd_puts(0, 0, msg);
        {
            char buf[32];
            int battv = battery_voltage();
            snprintf(buf, sizeof(buf), "%d.%02dV %d%%",
                battv / 1000, (battv % 1000) / 10, battery_level());
            lcd_puts(0, 1, buf);
        }
        lcd_update();

        button = button_get_w_tmo(HZ/2);
#ifdef BUTTON_ON
        if (button == (BUTTON_ON | BUTTON_REL))
#else
        if (button == (BUTTON_RIGHT | BUTTON_REL))
#endif
            break; /* start */
        else
        {
            if (usb_detect())
                break;
            else if (!charger_inserted())
                power_off(); /* charger removed: power down */
        }
    } while (1);
}
#endif /* CONFIG_CHARGING */

/* prompt user to plug USB and fix a problem */
void prompt_usb(const char* msg1, const char* msg2)
{
    int button;
    lcd_clear_display();
    lcd_puts(0, 0, msg1);
    lcd_puts(0, 1, msg2);
#ifdef HAVE_LCD_BITMAP
    lcd_puts(0, 2, "Insert USB cable");
    lcd_puts(0, 3, "and fix it.");
#endif
    lcd_update();
    do
    {
        button = button_get(true);
        if (button == SYS_POWEROFF)
        {
            power_off();
        }
    } while (button != SYS_USB_CONNECTED);
    usb_screen();
    system_reboot();
}

void main(void)
{
    int rc;

    power_init();
    system_init();
    kernel_init();
    buffer_init();
    lcd_init();
    show_logo();
    enable_irq();
    adc_init();
    usb_init();
    button_init();
    powermgmt_init();

#if CONFIG_CHARGING && (CONFIG_CPU == SH7034)
    if (charger_inserted()
#ifdef ATA_POWER_PLAYERSTYLE
        && !ide_powered() /* relies on probing result from bootloader */
#endif
        )
    {
        charging_screen(); /* display a "charging" screen */
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
        prompt_usb("No partition", "found.");
    }

    {   // rolo the firmware
        static const char filename[] = "/" BOOTFILE;
        rolo_load((char*)filename); /* won't return if started */

        prompt_usb("No firmware", filename);
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

void audio_stop_recording(void)
{
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
