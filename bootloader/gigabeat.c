#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
#include "panic.h"
#include "power.h"
#include "file.h"

char version[] = APPSVERSION;

void go_usb_mode(void) {
  /* Drop into USB mode.  This does not check for disconnection. */

    int i;

    GPBDAT &= 0x7EF;
    GPBCON |= 1<<8;

    GPGDAT &= 0xE7FF;
    GPGDAT |= 1<<11;

    for (i = 0; i < 10000000; i++) {continue;}

    GPBCON &= 0x2FFCFF;
    GPBDAT |= 1<<5; 
    GPBDAT |= 1<<6;
}

void * main(void)
{
    int line = 0, i;
    char buf[256];
    struct partinfo* pinfo;
    unsigned short* identify_info;
    int testfile;

    lcd_init();
    lcd_setfont(FONT_SYSFIXED);
/*
    lcd_puts(0, line++, "Rockbox boot loader");
    snprintf(buf, sizeof(buf), "Version: 20%s", version);
    lcd_puts(0, line++, buf);
    snprintf(buf, sizeof(buf), "Gigabeat version: 0x%08x", 1);
    lcd_puts(0, line++, buf);
*/

    lcd_puts(0, line++, "Hold MENU when booting for rescue mode.");
    lcd_update();

    /* hold MENU to enter rescue mode */
    if (GPGDAT & 2)  {
        lcd_puts(0, line++, "Entering rescue mode..");
        lcd_update();
        go_usb_mode();
        while(1);
    }

    i = ata_init();
    i = disk_mount_all();

    snprintf(buf, sizeof(buf), "disk_mount_all: %d", i);
    lcd_puts(0, line++, buf);

    identify_info = ata_get_identify();

    for (i=0; i < 20; i++)
        ((unsigned short*)buf)[i]=htobe16(identify_info[i+27]);

    buf[40]=0;

    /* kill trailing space */
    for (i=39; i && buf[i]==' '; i--)
        buf[i] = 0;

    lcd_puts(0, line++, "Model");
    lcd_puts(0, line++, buf);

    for (i=0; i < 4; i++)
        ((unsigned short*)buf)[i]=htobe16(identify_info[i+23]);

    buf[8]=0;

    lcd_puts(0, line++, "Firmware");
    lcd_puts(0, line++, buf);

    pinfo = disk_partinfo(0);
    snprintf(buf, sizeof(buf), "Partition 0: 0x%02x %ld MB", 
                  pinfo->type, pinfo->size / 2048);
    lcd_puts(0, line++, buf);

    testfile = open("/boottest.txt", O_WRONLY|O_CREAT|O_TRUNC);
    write(testfile, "It works!", 9);
    close(testfile);

    lcd_update();

    /* now wait in USB mode so the bootloader can be updated */
    go_usb_mode();
    while(1);

  return((void *)0);
}

