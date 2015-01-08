/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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

#include <stdio.h>
#include <system.h>
#include <inttypes.h>
#include "config.h"
#include "../kernel-internal.h"
#include "gcc_extensions.h"
#include "lcd.h"
#include "backlight.h"
#include "button-target.h"
#include "common.h"
#include "rb-loader.h"
#include "loader_strerror.h"
#include "storage.h"
#include "file_internal.h"
#include "disk.h"
#include "panic.h"
#include "power.h"
#include "power-imx233.h"
#include "system-target.h"
#include "fmradio_i2c.h"
#include "version.h"
#include "powermgmt-imx233.h"
#include "partitions-imx233.h"
#include "backlight-target.h"
#include "adc.h"

#include "usb.h"

extern char loadaddress[];
extern char loadaddressend[];

#define MSG(width, short, long) (LCD_WIDTH < (width) ? short : long)

#ifdef HAVE_BOOTLOADER_USB_MODE
static void usb_mode(int connect_timeout)
{
    int button;

    usb_init();
    usb_start_monitoring();

    /* Wait for threads to connect or cable is pulled */
    printf("USB: Connecting");

    long end_tick = current_tick + connect_timeout;

    while(1)
    {
        button = button_get_w_tmo(HZ/10);

        if(button == SYS_USB_CONNECTED)
            break; /* Hit */

        if(TIME_AFTER(current_tick, end_tick))
        {
            /* Timed out waiting for the connect - will happen when connected
             * to a charger through the USB port */
            printf("USB: Timed out");
            break;
        }

        if(usb_detect() == USB_EXTRACTED)
            break; /* Cable pulled */
    }

    if(button == SYS_USB_CONNECTED)
    {
        /* Got the message - wait for disconnect */
        printf("Bootloader USB mode");
        /* Enable power management to charge */
        powermgmt_init();
        adc_init();

        /* ack the SYS_USB_CONNECTED polled from the button queue */
        usb_acknowledge(SYS_USB_CONNECTED_ACK);

        while(1)
        {
            button = button_get_w_tmo(HZ/2);
            if(button == SYS_USB_DISCONNECTED)
                break;
            struct imx233_powermgmt_info_t info = imx233_powermgmt_get_info();
            lcd_putsf(0, 7, "%s: %s", MSG(240, "Status", "Charging status"),
                info.state == CHARGE_STATE_DISABLED ? "disabled" :
                info.state == CHARGE_STATE_ERROR ? "error" :
                info.state == DISCHARGING ? "discharging" :
                info.state == TRICKLE ? "trickle" :
                info.state == TOPOFF ? "topoff" :
                info.state == CHARGING ? "charging" : "<unknown>");
            lcd_putsf(0, 8, "Battery: %d%% (%d mV)", battery_level(), battery_voltage());
            lcd_putsf(0, 9, "%s: %d 'C [%d, %d]", MSG(240, "Die", "Die temp"),
                adc_read(ADC_DIE_TEMP), IMX233_DIE_TEMP_HIGH,
                IMX233_DIE_TEMP_LOW);
            lcd_update();
        }
    }

    /* Put drivers initialized for USB connection into a known state */
    usb_close();
}
#else /* !HAVE_BOOTLOADER_USB_MODE */
static void usb_mode(int connect_timeout)
{
    (void) connect_timeout;
}
#endif /* HAVE_BOOTLOADER_USB_MODE */

void main(uint32_t arg, uint32_t addr) NORETURN_ATTR;
void main(uint32_t arg, uint32_t addr)
{
    unsigned char* loadbuffer;
    int buffer_size;
    void(*kernel_entry)(void);
    int ret;

    system_init();
    kernel_init();

    /* some ixm233 targets needs this because the cpu and/or memory is clocked
     * at 24MHz, resulting in terribly slow boots and unusable usb mode.
     * While we are at it, clock at maximum speed to minimise boot time. */
    imx233_set_cpu_frequency(CPUFREQ_MAX);

    power_init();
    enable_irq();

    lcd_init();
    lcd_clear_display();
    lcd_update();

    backlight_init();

    button_init();

    printf("%s: %s", MSG(240, "Ver", "Boot version"), rbversion);
    printf("%s: %x ", MSG(240, "Arg", "Boot arg"), arg);
    printf("%s: %x", MSG(240, "Addr", "Boot addr"), addr);
#if IMX233_SUBTARGET >= 3780
    printf("Power up source: %x", BF_RD(POWER_STS, PWRUP_SOURCE));
#endif

    if(arg == 0xfee1dead)
    {
        printf("%s", MSG(240, "Disable window", "Disable partitions window"));
        imx233_partitions_enable_window(false);
    }

    ret = storage_init();
    if(ret < 0)
        error(EATA, ret, true);

    filesystem_init();

    /* NOTE: disk_mount_all to fail since we can do USB after.
     * We need this order to determine the correct logical sector size */
    if((ret = disk_mount_all()) <= 0)
        error(EDISK, ret, false);

    if(usb_detect() == USB_INSERTED)
        usb_mode(HZ);

    /* dummy read, might be necessary to init things */
#ifdef HAVE_BUTTON_DATA
    int data;
    button_read_device(&data);
#else
    button_read_device();
#endif

#ifdef HAS_BUTTON_HOLD
    if(button_hold())
    {
        printf("Hold switch on");
        printf("Shutting down...");
        sleep(HZ);
        power_off();
    }
#endif

    printf("Loading firmware");

    loadbuffer = (unsigned char*)loadaddress;
    buffer_size = (int)(loadaddressend - loadaddress);

    while((ret = load_firmware(loadbuffer, BOOTFILE, buffer_size)) <= EFILE_EMPTY)
    {
        error(EBOOTFILE, ret, true);
    }

    kernel_entry = (void*) loadbuffer;
    printf("Executing");
    /* stop what was initialized to start from clean state */
    system_prepare_fw_start();
    /* if target defines lcd_enable() in bootloader, take this as a hint that
     * we should use it to properly stop the lcd before moving one, the
     * backlight_hw_off() routine is supposed to disable the lcd at the same time */
#ifdef HAVE_LCD_ENABLE
    backlight_hw_off();
#endif
    disable_interrupt(IRQ_FIQ_STATUS);
    commit_discard_idcache();
    kernel_entry();
    printf("ERR: Failed to boot");

    /* never returns */
    while(1) ;
}
