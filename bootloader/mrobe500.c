/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2007 by Karl Kurbjun
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#include "inttypes.h"
#include "string.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"
#include "rbunicode.h"
#include "usb.h"
#include "spi.h"
#include "uart-target.h"
#include "tsc2100.h"

extern int line;

void main(void)
{
    unsigned char* loadbuffer;
    int buffer_size;
    int rc;
    int(*kernel_entry)(void);

    power_init();
    lcd_init();
    system_init();
    kernel_init();
    adc_init();
    button_init();
    backlight_init();
    uart_init();

    font_init();
    spi_init();

    lcd_setfont(FONT_SYSFIXED);

    /* Show debug messages if button is pressed */
//    if(button_read_device())
        verbose = true;

    printf("Rockbox boot loader");
    printf("Version %s", APPSVERSION);

    usb_init();

    /* Enter USB mode without USB thread */
    if(usb_detect())
    {
        const char msg[] = "Bootloader USB mode";
        reset_screen();
        lcd_putsxy( (LCD_WIDTH - (SYSFONT_WIDTH * strlen(msg))) / 2,
                    (LCD_HEIGHT - SYSFONT_HEIGHT) / 2, msg);
        lcd_update();

        ide_power_enable(true);
        ata_enable(false);
        sleep(HZ/20);
        usb_enable(true);

        while (usb_detect())
        {
            ata_spin(); /* Prevent the drive from spinning down */
            sleep(HZ);
        }

        usb_enable(false);

        reset_screen();
        lcd_update();
    }
#if 0
    int button=0, *address=0x0, count=0;
    while(true)
    {
        button = button_read_device();
        if (button == BUTTON_POWER)
        {
            printf("reset");
            IO_GIO_BITSET1|=1<<10;
        }
        if(button==BUTTON_RC_PLAY)
            address+=0x02;
        else if (button==BUTTON_RC_DOWN)
            address-=0x02;
        else if (button==BUTTON_RC_FF)
            address+=0x1000;
        else if (button==BUTTON_RC_REW)
            address-=0x1000;
        if (button&BUTTON_TOUCHPAD)
        {
            int touch = button_get_last_touch();
            printf("x: %d, y: %d", (touch>>16), touch&0xffff);
            line--;
        }
//         if ((IO_GIO_BITSET0&(1<<14) == 0)
//         {
//             short x,y,z1,z2, reg;
//             extern int uart1count;
//             tsc2100_read_values(&x, &y, &z1, &z2);
//             printf("x: %04x y: %04x z1: %04x z2: %04x", x, y, z1, z2);
//             printf("tsadc: %4x", tsc2100_readreg(TSADC_PAGE, TSADC_ADDRESS)&0xffff);
//             printf("current tick: %04x", current_tick);
//             printf("Address: 0x%08x Data: 0x%08x", address, *address);
//             printf("Address: 0x%08x Data: 0x%08x", address+1, *(address+1));
//             printf("Address: 0x%08x Data: 0x%08x", address+2, *(address+2));
//             printf("uart1count: %d", uart1count);
//             printf("%x %x", IO_UART1_RFCR & 0x3f, IO_UART1_DTRR & 0xff);
//             tsc2100_keyclick(); /* doesnt work :( */
//             line -= 8;
//         }
    }
#endif
    printf("ATA");
    rc = ata_init();
    if(rc)
    {
        reset_screen();
        error(EATA, rc);
    }

    printf("disk");
    disk_init();

    printf("mount");
    rc = disk_mount_all();
    if (rc<=0)
    {
        error(EDISK,rc);
    }

    printf("Loading firmware");

    loadbuffer = (unsigned char*) 0x00900000;
    buffer_size = (unsigned char*)0x04900000 - loadbuffer;

    rc = load_firmware(loadbuffer, BOOTFILE, buffer_size);
    if(rc < 0)
        error(EBOOTFILE, rc);

    if (rc == EOK)
    {
        kernel_entry = (void*) loadbuffer;
        rc = kernel_entry();
    }
}
