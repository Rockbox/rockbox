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
#include "common.h"

extern void map_memory(void);

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
    
    orig_file = open("/GBSYSTEM/FWIMG/FWIMG01.DAT.ORIG", O_RDONLY);
    if(orig_file < 0) {
        /* Couldn't open source file */
        printf("Couldn't open FWIMG01.DAT.ORIG for reading");
        return(1);
    }

    printf("FWIMG01.DAT.ORIG opened for reading");

    dest_file = open("/GBSYSTEM/FWIMG/FWIMG01.DAT", O_RDWR);
    if(dest_file < 0) {
        /* Couldn't open destination file */
        printf("Couldn't open FWIMG01.DAT.ORIG for writing");
        close(orig_file);
        return(2);
    }

    printf("FWIMG01.DAT opened for writing");

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

    printf("Finished copying %ld bytes from", size);
    printf("FWIMG01.DAT.ORIG to FWIMG01.DAT");

    return(0);
}

char buf[256];

void display_instructions(void)
{
    lcd_setfont(FONT_SYSFIXED);
    printf("Hold MENU when booting for rescue mode.");
    printf("  \"VOL+\" button to restore original kernel");
    printf("  \"A\" button to load original firmware");
    printf("");
    printf("FRAME %x TTB %x", FRAME, TTB_BASE);
}

void * main(void)
{
    int i;
    struct partinfo* pinfo;
    unsigned short* identify_info;
    unsigned char* loadbuffer;
    int buffer_size;
    bool load_original = false;
    int rc;
    int(*kernel_entry)(void);

    bool show_bootsplash = true;

    if(GPGDAT & 2)
        show_bootsplash = false;

    if(!show_bootsplash) {
        lcd_init();
        display_instructions();
        sleep(2*HZ);
    }
    if(GPGDAT & 2) {
        lcd_init();
        printf("Entering rescue mode..");
        go_usb_mode();
        while(1);
    }
    if(GPGDAT & 0x10) {
        lcd_init();
        load_original = true;
        printf("Loading original firmware...");
    }

    i = ata_init();
    i = disk_mount_all();
    if(!show_bootsplash) {
        printf("disk_mount_all: %d", i);
    }
    if(show_bootsplash) {
        int fd = open("/bootsplash.raw", O_RDONLY);
        if(fd < 0)  {
            show_bootsplash = false;
            lcd_init();
            display_instructions();
        }
        else {
            read(fd, lcd_framebuffer, LCD_WIDTH*LCD_HEIGHT*2);
            close(fd);
            lcd_update();
            lcd_init();
        }
    } 
    /* hold VOL+ to enter rescue mode to copy old image */
    /* needs to be after ata_init and disk_mount_all    */
    if(GPGDAT & 4) {

        /* Try to restore the original kernel/bootloader if a copy is found */
        printf("Restoring FWIMG01.DAT...");

        if(!restore_fwimg01dat()) {
            printf("Restoring FWIMG01.DAT successful.");
        } else {
            printf("Restoring FWIMG01.DAT failed.");
        }

        printf("Now power cycle to boot original");
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

        printf("Model");
        printf(buf);

        for(i=0; i < 4; i++)
            ((unsigned short*)buf)[i]=htobe16(identify_info[i+23]);

        buf[8]=0;

        printf("Firmware");
        printf(buf);

        pinfo = disk_partinfo(0);
        printf("Partition 0: 0x%02x %ld MB", pinfo->type, pinfo->size / 2048);
    }
    /* Load original firmware */
    if(load_original) {
        loadbuffer = (unsigned char*)0x30008000;
        buffer_size =(unsigned char*)0x31000000 - loadbuffer;
        rc = load_firmware("rockbox.gigabeat", loadbuffer, buffer_size);
        if(rc < EOK) {
            printf("Error!");
            printf("Failed to load original firmware:");
            printf(strerror(rc));
            printf("Loading rockbox");
            sleep(2*HZ);
            goto load_rockbox;
        }

        printf("Loaded: %d", rc);
        sleep(2*HZ);

        (*((int*)0x7000000)) = 333;
        rc = *((int*)0x7000000+0x8000000);
        printf("Bank0 mem test: %d", rc);
        sleep(3*HZ);

        printf("Woops, should not return from firmware!");
        goto usb;
    }

load_rockbox:
    map_memory();
    if(!show_bootsplash) {
        printf("Loading Rockbox...");
    }

    loadbuffer = (unsigned char*) 0x100;
    buffer_size = (unsigned char*)0x400000 - loadbuffer;
    rc=load_firmware("rockbox.gigabeat", loadbuffer, buffer_size);
    if(rc < EOK) {
            printf("Error!");
            printf("Can't load rockbox.gigabeat:");
            printf(strerror(rc));
    } else {
        if(!show_bootsplash) {
            printf("Rockbox loaded.");
        }
        kernel_entry = (void*) loadbuffer;
        rc = kernel_entry();
        printf("Woops, should not return from firmware: %d", rc);
        goto usb;
    }
usb:
    /* now wait in USB mode so the bootloader can be updated */
    go_usb_mode();
    while(1);

    return((void *)0);
}

