/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include "backlight.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"

int line = 0;

int usb_screen(void)
{
   return 0;
}

void start_iriver_fw(void)
{
    asm(" move.w #0x2700,%sr");
    /* Reset the cookie for the crt0 crash check */
    asm(" move.l #0,%d0");
    asm(" move.l %d0,0x10017ffc");
    asm(" movec.l %d0,%vbr");
    asm(" move.l 0,%sp");
    asm(" lea.l 8,%a0");
    asm(" jmp (%a0)");
}

int load_firmware(void)
{
    int fd;
    int rc;
    int len;
    unsigned long chksum;
    unsigned long sum;
    int i;
    unsigned char *buf = (unsigned char *)0x30000000;
    char str[80];
    
    fd = open("/rockbox.iriver", O_RDONLY);
    if(fd < 0)
        return -1;

    len = filesize(fd) - 8;

    snprintf(str, 80, "Length: %x", len);
    lcd_puts(0, line++, str);
    lcd_update();

    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);
    
    rc = read(fd, &chksum, 4);
    if(rc < 4)
        return -2;

    snprintf(str, 80, "Checksum: %x", chksum);
    lcd_puts(0, line++, str);
    lcd_update();

    lseek(fd, FIRMWARE_OFFSET_FILE_DATA, SEEK_SET);

    rc = read(fd, buf, len);
    if(rc < len)
        return -4;

    close(fd);

    sum = 0;
    
    for(i = 0;i < len;i++) {
        sum += buf[i];
    }

    snprintf(str, 80, "Sum: %x", sum);
    lcd_puts(0, line++, str);
    lcd_update();

    if(sum != chksum)
        return -5;

    return 0;
}

void start_firmware(void)
{
    asm(" move.w #0x2700,%sr");
    /* Reset the cookie for the crt0 crash check */
    asm(" move.l #0,%d0");
    asm(" move.l %d0,0x10017ffc");
    asm(" move.l #0x30000000,%d0");
    asm(" movec.l %d0,%vbr");
    asm(" move.l 0x30000000,%sp");
    asm(" move.l 0x30000004,%a0");
    asm(" jmp (%a0)");
}

void main(void)
{
    int i;
    int rc;
    char buf[256];

    power_init();
    system_init();
    kernel_init();
    backlight_init();
    set_irq_level(0);
    lcd_init();
    font_init();
    adc_init();
    button_init();

    lcd_setfont(FONT_SYSFIXED);

    sleep(HZ/50); /* Allow the button driver to check the buttons */

    if(button_status() & BUTTON_REC ||
        button_status() & BUTTON_RC_ON) {
        lcd_puts(0, 8, "Starting original firmware...");
        lcd_update();
        start_iriver_fw();
    }

    if((button_status() & BUTTON_ON) & button_hold()) {
        lcd_puts(0, 8, "HOLD switch on, power off...");
        lcd_update();
        sleep(HZ/2);
        power_off();
    }
#if 0
    if((button_status() & BUTTON_RC_ON) & remote_button_hold()) {
        lcd_puts(0, 8, "HOLD switch on, power off...");
        lcd_update();
        sleep(HZ/2);
        power_off();
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
        lcd_puts(0, 3, "Press ON to debug");
        lcd_update();
        while(!(button_get(true) & BUTTON_REL));
#endif
        panicf("ata: %d", rc);
    }

    disk_init();

    rc = disk_mount_all();
    if (rc<=0)
    {
        lcd_clear_display();
        lcd_puts(0, 0, "No partition");
        lcd_puts(0, 1, "found.");
        while(button_get(true) != SYS_USB_CONNECTED) {};
    }

    lcd_puts(0, line++, "Loading firmware");
    lcd_update();
    i = load_firmware();
    snprintf(buf, 256, "Result: %d", i);
    lcd_puts(0, line++, buf);
    lcd_update();

    if(i == 0)
        start_firmware();
    
    start_iriver_fw();
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */

void reset_poweroff_timer(void)
{
}

void screen_dump(void)
{
}

int dbg_ports(void)
{
   return 0;
}

void mpeg_stop(void)
{
}

void usb_acknowledge(void)
{
}

void usb_wait_for_disconnect(void)
{
}
