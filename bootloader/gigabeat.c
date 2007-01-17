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
#include "button-target.h"
#include "bootsplash-gigabeat.h"

extern void map_memory(void);

int line = 0;

char version[] = APPSVERSION;

static void go_usb_mode(void)
{
    /* Drop into USB mode.  This does not check for disconnection. */
    int i;

    GPBDAT &= 0x7EF;
    GPBCON |= 1<<8;

    GPGDAT &= 0xE7FF;
    GPGDAT |= 1<<11;

    for(i = 0; i < 10000000; i++) {
        continue;
    }

    GPBCON &= 0x2FFCFF;
    GPBDAT |= 1<<5;
    GPBDAT |= 1<<6;
}


/* Restores a factory kernel/bootloader from a known location                   */
/* Restores the FWIMG01.DAT file back in the case of a bootloader failure       */
/* The factory or "good" bootloader must be in /GBSYSTEM/FWIMG/FWIMG01.DAT.ORIG */
/* Returns non-zero on failure */
int restore_fwimg01dat(void)
{
    int orig_file = 0, dest_file = 0;
    int size = 0, size_read;
    static char buf[4096];
    char lcd_buf[64];

    orig_file = open("/GBSYSTEM/FWIMG/FWIMG01.DAT.ORIG", O_RDONLY);
    if(orig_file < 0) {
        /* Couldn't open source file */
        lcd_puts(0, line++, "Couldn't open FWIMG01.DAT.ORIG for reading");
        lcd_update();
        return(1);
    }

    lcd_puts(0, line++, "FWIMG01.DAT.ORIG opened for reading");
    lcd_update();

    dest_file = open("/GBSYSTEM/FWIMG/FWIMG01.DAT", O_RDWR);
    if(dest_file < 0) {
        /* Couldn't open destination file */
        lcd_puts(0, line++, "Couldn't open FWIMG01.DAT.ORIG for writing");
        lcd_update();
        close(orig_file);
        return(2);
    }

    lcd_puts(0, line++, "FWIMG01.DAT opened for writing");
    lcd_update();

    do {
        /* Copy in chunks */
        size_read = read(orig_file, buf, sizeof(buf));
        if(size_read != write(dest_file, buf, size_read)) {
            close(orig_file);
            close(dest_file);
            return(3);
        }
        size += size_read;

    } while(size_read > 0);

    close(orig_file);
    close(dest_file);

    snprintf(lcd_buf, sizeof(lcd_buf), "Finished copying %ld bytes from", size);
    lcd_puts(0, line++, lcd_buf);
    lcd_puts(0, line++, "FWIMG01.DAT.ORIG to FWIMG01.DAT");

    return(0);
}


int load_rockbox(const char* file_name, unsigned char* buf, int buffer_size)
{
    int fd;
    int rc;
    int len;
    char str[256];

    fd = open("/.rockbox/" BOOTFILE, O_RDONLY);
    if(fd < 0) {
        fd = open("/" BOOTFILE, O_RDONLY);
        if(fd < 0)
            return -1;
    }
    fd = open(file_name, O_RDONLY);
    if(fd < 0)
        return -2;

    len = filesize(fd);

    if(len > buffer_size) {
        snprintf(str, sizeof(str), "len: %d buf: %d", len, buffer_size);
        lcd_puts(0, line++, str);
        lcd_update();
        return -6;
    }

    rc = read(fd, buf, len);
    if(rc < len) {
        snprintf(str, sizeof(str), "len: %d rc: %d", len, rc);
        lcd_puts(0, line++, str);
        lcd_update();
        return -4;
    }

    close(fd);

    return len;
}


void * main(void)
{
    int i;
    char buf[256];
    struct partinfo* pinfo;
    unsigned short* identify_info;
    unsigned char* loadbuffer;
    int buffer_size;
    bool load_original = false;
    int rc;
    int(*kernel_entry)(void);

    lcd_init();
    bool show_bootsplash = true;

    if(GPGDAT & 2)
        show_bootsplash = false;

    if(!show_bootsplash) {
		lcd_setfont(FONT_SYSFIXED);
        lcd_puts(0, line++, "Hold MENU when booting for rescue mode.");
        lcd_puts(0, line++, "  \"VOL+\" button to restore original kernel");
        lcd_puts(0, line++, "  \"A\" button to load original firmware");
        line++;
        snprintf(buf, sizeof(buf), "FRAME %x TTB %x", FRAME, TTB_BASE);
        lcd_puts(0, line++, buf);
        lcd_update();
        sleep(2*HZ);
    } else
        memcpy(FRAME, bootsplash, LCD_WIDTH*LCD_HEIGHT*2);
    if(GPGDAT & 2) {
        lcd_puts(0, line++, "Entering rescue mode..");
        lcd_update();
        go_usb_mode();
        while(1);
    }
    if(GPGDAT & 0x10) {
        load_original = true;
        lcd_puts(0, line++, "Loading original firmware...");
        lcd_update();
    }

    i = ata_init();
    i = disk_mount_all();
    if(!show_bootsplash) {
        snprintf(buf, sizeof(buf), "disk_mount_all: %d", i);
        lcd_puts(0, line++, buf);
    }
    /* hold VOL+ to enter rescue mode to copy old image */
    /* needs to be after ata_init and disk_mount_all    */
    if(GPGDAT & 4) {

        /* Try to restore the original kernel/bootloader if a copy is found */
        lcd_puts(0, line++, "Restoring FWIMG01.DAT...");
        lcd_update();

        if(!restore_fwimg01dat()) {
            lcd_puts(0, line++, "Restoring FWIMG01.DAT successful.");
        } else {
            lcd_puts(0, line++, "Restoring FWIMG01.DAT failed.");
        }

        lcd_puts(0, line++, "Now power cycle to boot original");
        lcd_update();
        while(1);
    }

    if(!show_bootsplash) {
        identify_info = ata_get_identify();

        for(i=0; i < 20; i++)
            ((unsigned short*)buf)[i]=htobe16(identify_info[i+27]);

        buf[40]=0;

        /* kill trailing space */
        for(i=39; i && buf[i]==' '; i--)
            buf[i] = 0;

        lcd_puts(0, line++, "Model");
        lcd_puts(0, line++, buf);

        for(i=0; i < 4; i++)
            ((unsigned short*)buf)[i]=htobe16(identify_info[i+23]);

        buf[8]=0;

        lcd_puts(0, line++, "Firmware");
        lcd_puts(0, line++, buf);

        pinfo = disk_partinfo(0);
        snprintf(buf, sizeof(buf), "Partition 0: 0x%02x %ld MB",
                 pinfo->type, pinfo->size / 2048);
        lcd_puts(0, line++, buf);
        lcd_update();
    }
    /* Load original firmware */
    if(load_original) {
        loadbuffer = (unsigned char*)0x30008000;
        buffer_size =(unsigned char*)0x31000000 - loadbuffer;
        rc = load_rockbox("/rockbox.gigabeat", loadbuffer, buffer_size);
        if(rc < 0) {
            lcd_puts(0, line++, "failed to load original firmware. Loading rockbox");
            lcd_update();
            sleep(2*HZ);
            goto load_rockbox;
        }

        snprintf(buf, sizeof(buf), "Loaded: %d", rc);
        lcd_puts(0, line++, buf);
        lcd_update();
        sleep(2*HZ);

        (*((int*)0x7000000)) = 333;
        rc = *((int*)0x7000000+0x8000000);
        snprintf(buf, sizeof(buf), "Bank0 mem test: %d", rc);
        lcd_puts(0, line++, buf);
        lcd_update();
        sleep(3*HZ);

        lcd_puts(0, line++, "Woops, should not return from firmware!");
        lcd_update();
        goto usb;
    }

load_rockbox:
    map_memory();
    if(!show_bootsplash) {
        lcd_puts(0, line, "Loading Rockbox...");
        lcd_update();
    }

    loadbuffer = (unsigned char*) 0x100;
    buffer_size = (unsigned char*)0x400000 - loadbuffer;
    rc=load_rockbox("/rockbox.gigabeat", loadbuffer, buffer_size);
    if(rc < 0) {
        snprintf(buf, sizeof(buf), "Rockbox error: %d",rc);
        lcd_puts(0, line++, buf);
        lcd_update();
    } else {
        if(!show_bootsplash) {
            lcd_puts(0, line++, "Rockbox loaded.");
            lcd_update();
        }
        kernel_entry = (void*) loadbuffer;
        rc = kernel_entry();
        snprintf(buf, sizeof(buf), "Woops, should not return from firmware: %d", rc);
        lcd_puts(0, line++, buf);
        lcd_update();
        goto usb;
    }
usb:
    /* now wait in USB mode so the bootloader can be updated */
    go_usb_mode();
    while(1);

    return((void *)0);
}

