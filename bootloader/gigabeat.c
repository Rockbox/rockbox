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

void map_memory(void);

int line = 0;

char version[] = APPSVERSION;

/* This section allows you to toggle bits of any memory location  */
/* Touchpad to move around the bits. Select to toggle the red bit */
typedef struct {
    unsigned int address;
    char    *desc;
} memlocation_struct;

/* Just add any address and descriptions here */
/* Must finish with 0xFFFFFFFF                */
const memlocation_struct memlocations[] = {
/* Address    Description */
{ 0x56000000, "GPACON" },
{ 0x56000004, "GPADAT" },
{ 0x56000010, "GPBCON" },
{ 0x56000014, "GPBDAT" },
{ 0x56000020, "GPCCON" },
{ 0x56000024, "GPCDAT" },
{ 0x56000030, "GPDCON" },
{ 0x56000034, "GPDDAT" },
{ 0x56000040, "GPECON" },
{ 0x56000044, "GPEDAT" },
{ 0x56000050, "GPFCON" },
{ 0x56000054, "GPFDAT" },
{ 0x56000060, "GPGCON" },
{ 0x56000064, "GPGDAT" },
{ 0x56000070, "GPHCON" },
{ 0x56000074, "GPHDAT" },
{ 0xFFFFFFFF, 0 }
};

void memdump(void)
{
    int i, j;
    int current=0, bit=0;
    char * bitval;
    int data;
    char tmp[40];

    while(1) {
        i = 0;

        while(memlocations[i].address != 0xFFFFFFFF) {

            data = *(volatile int *)memlocations[i].address;

            snprintf(tmp, sizeof(tmp), "%s %s 0x%08X", 
                 (i==current) ? "*" : " ", 
                 memlocations[i].desc,
                 data);
            lcd_puts(0, i*2+5, tmp);

            /* print out in binary, current bit in red */
            for (j=31; j>=0; j--) {
                if ((bit == j) && (current == i))
                    lcd_set_foreground(LCD_RGBPACK(255,0,0));
                lcd_puts((31-j) + ((31-j) / 8), i*2+6, (data & (1 << j)) ? "1" : "0" );
                lcd_set_foreground(LCD_RGBPACK(0,0,0));
            }

            i++;
        }

        data = *(volatile int *)memlocations[current].address;
        bitval = (data & (1 << bit)) ? "1" : "0";
        snprintf(tmp, sizeof(tmp), "%s bit %ld = %s", memlocations[current].desc, bit, bitval);
        lcd_puts(0, (i*2)+7, tmp);

        lcd_update();

        /* touchpad controls */

        /* Up */
        if (GPJDAT & 0x01) {
            if (current > 0)
                current--;
            while(GPJDAT & 0x01);
        }

        /* Down */
        if (GPJDAT & 0x40) {
            if (current < (i-1))
                current++;
            while(GPJDAT & 0x40);
        }

        /* Left */
        if (GPJDAT & 0x80) {
            if (bit < 31)
                bit++;
            while(GPJDAT & 0x80);
        }

        /* Right */
        if (GPJDAT & 0x1000) {
            if (bit > 0)
                bit--;
            while(GPJDAT & 0x1000);
        }

        /* Centre - Toggle Bit */
        if (GPJDAT & 0x08) {
            data = *(volatile int *)memlocations[current].address;
            data = data ^ (1 << bit);
            *(volatile int *)memlocations[current].address = data;
            while(GPJDAT & 0x08);
        }

        /* Bail out if the power button is pressed */
        if (GPGDAT & 1) {
            break;
        }
    }
}


static void go_usb_mode(void) {
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


/* Restores a factory kernel/bootloader from a known location                   */
/* Restores the FWIMG01.DAT file back in the case of a bootloader failure       */
/* The factory or "good" bootloader must be in /GBSYSTEM/FWIMG/FWIMG01.DAT.ORIG */
/* Returns non-zero on failure */
int restore_fwimg01dat(void)
{
    int orig_file = 0, dest_file = 0;
    int size = 0, size_read;
    char buf[256];
    char lcd_buf[64];

    orig_file = open("/GBSYSTEM/FWIMG/FWIMG01.DAT.ORIG", O_RDONLY);
    if (orig_file < 0) {
        /* Couldn't open source file */
        lcd_puts(0, line++, "Couldn't open FWIMG01.DAT.ORIG for reading");
        lcd_update();
        return(1);
    }

    lcd_puts(0, line++, "FWIMG01.DAT.ORIG opened for reading");
    lcd_update();

    dest_file = open("/GBSYSTEM/FWIMG/FWIMG01.DAT", O_RDWR);
    if (dest_file < 0) {
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
        if (size_read != write(dest_file, buf, size_read)) {
            close(orig_file);
            close(dest_file);
            return(3);
        }
        size += size_read;

    } while (size_read > 0);

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
    //unsigned long chksum;
    //char model[5];
    //unsigned long sum;
    //int i;
    //char str[80];

    fd = open("/.rockbox/" BOOTFILE, O_RDONLY);
    if(fd < 0)
    {
        fd = open("/" BOOTFILE, O_RDONLY);
        if(fd < 0)
            return -1;
    }
    fd = open(file_name, O_RDONLY);
    if(fd < 0)
        return -2;

    len = filesize(fd);

    if (len > buffer_size) {
        snprintf(str, sizeof(str), "len: %d buf: %d", len, buffer_size);
        lcd_puts(0, line++, str);
        lcd_update();
        return -6;
    }

    /*lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);

    rc = read(fd, &chksum, 4);
    chksum=betoh32(chksum);*/ /* Rockbox checksums are big-endian */
    /*if(rc < 4)
        return -2;

    rc = read(fd, model, 4);
    if(rc < 4)
        return -3;

    model[4] = 0;

    snprintf(str, 80, "Model: %s", model);
    lcd_puts(0, line++, str);
    snprintf(str, 80, "Checksum: %x", chksum);
    lcd_puts(0, line++, str);
    lcd_update();

    lseek(fd, FIRMWARE_OFFSET_FILE_DATA, SEEK_SET);
*/

    rc = read(fd, buf, len);
    if(rc < len) {
        snprintf(str, sizeof(str), "len: %d rc: %d", len, rc);
       lcd_puts(0, line++, str);
        lcd_update();
        return -4;
    }

    close(fd);

    /*sum = MODEL_NUMBER;

    for(i = 0;i < len;i++) {
        sum += buf[i];
    }

    snprintf(str, 80, "Sum: %x", sum);
    lcd_puts(0, line++, str);
    lcd_update();

    if(sum != chksum)
    return -5;*/

    return len;
}

void * main(void)
{
    int i;
    char buf[256];
    struct partinfo* pinfo;
    unsigned short* identify_info;
    //int testfile;
    unsigned char* loadbuffer;
    int buffer_size;
    bool load_original = false;
    int rc;
    int(*kernel_entry)(void);

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
    lcd_puts(0, line++, "  \"VOL+\" button to restore original kernel");
    lcd_puts(0, line++, "  \"A\" button to load original firmware");
    lcd_update();
    sleep(1*HZ);

    /* hold MENU to enter rescue mode */
    if (GPGDAT & 2)  {
        lcd_puts(0, line++, "Entering rescue mode..");
        lcd_update();
        go_usb_mode();
        while(1);
    }

    sleep(5*HZ);

    if(GPGDAT & 0x10) {
        load_original = true;
        lcd_puts(0, line++, "Loading original firmware...");
        lcd_update();
     }

    i = ata_init();
    i = disk_mount_all();

    snprintf(buf, sizeof(buf), "disk_mount_all: %d", i);
    lcd_puts(0, line++, buf);

    /* hold VOL+ to enter rescue mode to copy old image */
    /* needs to be after ata_init and disk_mount_all    */
    if (GPGDAT & 4)  {

        /* Try to restore the original kernel/bootloader if a copy is found */
        lcd_puts(0, line++, "Restoring FWIMG01.DAT...");
        lcd_update();

        if (!restore_fwimg01dat()) {
            lcd_puts(0, line++, "Restoring FWIMG01.DAT successful.");
        } else {
            lcd_puts(0, line++, "Restoring FWIMG01.DAT failed.");
        }

        lcd_puts(0, line++, "Now power cycle to boot original");
        lcd_update();
        while(1);
    }

    /* Memory dump mode if Vol- pressed */
    if (GPGDAT & 8) {
        memdump();
    }

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
    lcd_update();

    /* Load original firmware */
    if(load_original) {
        loadbuffer = (unsigned char*)0x30008000;
        buffer_size =(unsigned char*)0x31000000 - loadbuffer;
        rc = load_rockbox("/rockbox.gigabeat", loadbuffer, buffer_size);
        if (rc < 0) {
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
    lcd_puts(0, line, "Loading Rockbox...");
    lcd_update();
    sleep(HZ*4);

    // TODO: read those values from somwhere
    loadbuffer = (unsigned char*) 0x100;
    buffer_size = (unsigned char*)0x400000 - loadbuffer;
    rc=load_rockbox("/rockbox.gigabeat", loadbuffer, buffer_size);
    if (rc < 0) {
        snprintf(buf, sizeof(buf), "Rockbox error: %d",rc);
        lcd_puts(0, line++, buf);
        lcd_update();
    } else {
        lcd_puts(0, line++, "Rockbox loaded.");
        lcd_update();
        kernel_entry = (void*)0x100;
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

