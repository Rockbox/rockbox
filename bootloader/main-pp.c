/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
 * 
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "common.h"
#include "cpu.h"
#include "file.h"
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "font.h"
#include "ata.h"
#include "button.h"
#include "disk.h"
#include <string.h>

/*
 * CRC32 implementation taken from:
 *
 * efone - Distributed internet phone system.
 *
 * (c) 1999,2000 Krzysztof Dabrowski
 * (c) 1999,2000 ElysiuM deeZine
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

/* based on implementation by Finn Yannick Jacobs */

#include <stdio.h>
#include <stdlib.h>

/* crc_tab[] -- this crcTable is being build by chksum_crc32GenTab().
 *		so make sure, you call it before using the other
 *		functions!
 */
static unsigned int crc_tab[256];

/* chksum_crc() -- to a given block, this one calculates the
 *				crc32-checksum until the length is
 *				reached. the crc32-checksum will be
 *				the result.
 */
unsigned int chksum_crc32 (unsigned char *block, unsigned int length)
{
   register unsigned long crc;
   unsigned long i;

   crc = 0;
   for (i = 0; i < length; i++)
   {
      crc = ((crc >> 8) & 0x00FFFFFF) ^ crc_tab[(crc ^ *block++) & 0xFF];
   }
   return (crc);
}

/* chksum_crc32gentab() --      to a global crc_tab[256], this one will
 *				calculate the crcTable for crc32-checksums.
 *				it is generated to the polynom [..]
 */

static void chksum_crc32gentab (void)
{
   unsigned long crc, poly;
   int i, j;

   poly = 0xEDB88320L;
   for (i = 0; i < 256; i++)
   {
      crc = i;
      for (j = 8; j > 0; j--)
      {
	 if (crc & 1)
	 {
	    crc = (crc >> 1) ^ poly;
	 }
	 else
	 {
	    crc >>= 1;
	 }
      }
      crc_tab[i] = crc;
   }
}

/* Button definitions */
#if CONFIG_KEYPAD == IRIVER_H10_PAD
#define BOOTLOADER_BOOT_OF      BUTTON_LEFT

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define BOOTLOADER_BOOT_OF      BUTTON_LEFT

#endif

/* Maximum allowed firmware image size. 10MB is more than enough */
#define MAX_LOADSIZE (10*1024*1024)

/* A buffer to load the original firmware or Rockbox into */
unsigned char *loadbuffer = (unsigned char *)DRAM_START;

/* Bootloader version */
char version[] = APPSVERSION;

/* Locations and sizes in hidden partition on Sansa */
#define PPMI_SECTOR_OFFSET  1024
#define PPMI_SECTORS        1
#define MI4_HEADER_SECTORS  1
#define MI4_HEADER_SIZE     0x200

/* mi4 header structure */
struct mi4header_t {
    unsigned char magic[4];
    uint32_t version;
    uint32_t length;
    uint32_t crc32;
    uint32_t enctype;
    uint32_t mi4size;
    uint32_t plaintext;
    uint32_t dsa_key[10];
    uint32_t pad[109];
    unsigned char type[4];
    unsigned char model[4];
};

/* PPMI header structure */
struct ppmi_header_t {
    unsigned char magic[4];
    uint32_t length;
    uint32_t pad[126];
};

/* Load mi4 format firmware image */
int load_mi4(unsigned char* buf, char* firmware, unsigned int buffer_size)
{
    int fd;
    struct mi4header_t mi4header;
    int rc;
    unsigned long sum;
    char filename[MAX_PATH];

    snprintf(filename,sizeof(filename),"/.rockbox/%s",firmware);
    fd = open(filename, O_RDONLY);
    if(fd < 0)
    {
        snprintf(filename,sizeof(filename),"/%s",firmware);
        fd = open(filename, O_RDONLY);
        if(fd < 0)
            return EFILE_NOT_FOUND;
    }

    read(fd, &mi4header, MI4_HEADER_SIZE);

    /* We don't support encrypted mi4 files yet */
    if( (mi4header.plaintext + MI4_HEADER_SIZE) != mi4header.mi4size)
        return EINVALID_FORMAT;

    /* MI4 file size */
    printf("mi4 size: %x", mi4header.mi4size);

    if ((mi4header.mi4size-MI4_HEADER_SIZE) > buffer_size)
        return EFILE_TOO_BIG;

    /* CRC32 */
    printf("CRC32: %x", mi4header.crc32);

    /* Rockbox model id */
    printf("Model id: %.4s", mi4header.model);

    /* Read binary type (RBOS, RBBL) */
    printf("Binary type: %.4s", mi4header.type);

    /* Load firmware */
    lseek(fd, MI4_HEADER_SIZE, SEEK_SET);
    rc = read(fd, buf, mi4header.mi4size-MI4_HEADER_SIZE);
    if(rc < (int)mi4header.mi4size-MI4_HEADER_SIZE)
        return EREAD_IMAGE_FAILED;

    /* Check CRC32 to see if we have a valid file */
    sum = chksum_crc32 (buf,mi4header.mi4size-MI4_HEADER_SIZE);

    printf("Calculated CRC32: %x", sum);

    if(sum != mi4header.crc32)
        return EBAD_CHKSUM;
    
    return EOK;
}

#ifdef SANSA_E200
/* Load mi4 firmware from a hidden disk partition */
int load_mi4_part(unsigned char* buf, struct partinfo* pinfo, unsigned int buffer_size)
{
    struct mi4header_t mi4header;
    struct ppmi_header_t ppmi_header;
    unsigned long sum;
    
    /* Read header to find out how long the mi4 file is. */
    ata_read_sectors(pinfo->start + PPMI_SECTOR_OFFSET,
                            PPMI_SECTORS, &ppmi_header);
    
    /* The first four characters at 0x80000 (sector 1024) should be PPMI*/
    if( memcmp(ppmi_header.magic, "PPMI", 4) )
        return EFILE_NOT_FOUND;
    
    printf("BL mi4 size: %x", ppmi_header.length);
    
    /* Read mi4 header of the OF */
    ata_read_sectors(pinfo->start + PPMI_SECTOR_OFFSET + PPMI_SECTORS 
                       + (ppmi_header.length/512), MI4_HEADER_SECTORS, &mi4header);
    
    /* We don't support encrypted mi4 files yet */
    if( (mi4header.plaintext) != (mi4header.mi4size-MI4_HEADER_SIZE))
        return EINVALID_FORMAT;

    /* MI4 file size */
    printf("OF mi4 size: %x", mi4header.mi4size);

    if ((mi4header.mi4size-MI4_HEADER_SIZE) > buffer_size)
        return EFILE_TOO_BIG;

    /* CRC32 */
    printf("CRC32: %x", mi4header.crc32);

    /* Rockbox model id */
    printf("Model id: %.4s", mi4header.model);

    /* Read binary type (RBOS, RBBL) */
    printf("Binary type: %.4s", mi4header.type);

    /* Load firmware */
    ata_read_sectors(pinfo->start + PPMI_SECTOR_OFFSET + PPMI_SECTORS
                        + (ppmi_header.length/512) + MI4_HEADER_SECTORS,
                        (mi4header.mi4size-MI4_HEADER_SIZE)/512, buf);

    /* Check CRC32 to see if we have a valid file */
    sum = chksum_crc32 (buf,mi4header.mi4size-MI4_HEADER_SIZE);

    printf("Calculated CRC32: %x", sum);

    if(sum != mi4header.crc32)
        return EBAD_CHKSUM;
    
    return EOK;
}
#endif

void* main(void)
{
    char buf[256];
    int i;
    int btn;
    int rc;
    int num_partitions;
    unsigned short* identify_info;
    struct partinfo* pinfo;

    chksum_crc32gentab ();

    system_init();
    kernel_init();
    lcd_init();
    font_init();
    button_init();

    lcd_set_foreground(LCD_WHITE);
    lcd_set_background(LCD_BLACK);
    lcd_clear_display();
    
    btn = button_read_device();

    /* Enable bootloader messages if any button is pressed */
    if (btn)
        verbose = true;

    lcd_setfont(FONT_SYSFIXED);

    printf("Rockbox boot loader");
    printf("Version: %s", version);
    printf(MODEL_NAME);

    i=ata_init();
    if (i==0) {
        identify_info=ata_get_identify();
        /* Show model */
        for (i=0; i < 20; i++) {
            ((unsigned short*)buf)[i]=htobe16(identify_info[i+27]);
        }
        buf[40]=0;
        for (i=39; i && buf[i]==' '; i--) {
            buf[i]=0;
        }
        printf(buf);
    } else {
        error(EATA, i);
    }

    disk_init();
    num_partitions = disk_mount_all();
    if (num_partitions<=0)
    {
        error(EDISK,num_partitions);
    }

    /* Just list the first 2 partitions since we don't have any devices yet 
       that have more than that */
    for(i=0; i<2; i++)
    {
        pinfo = disk_partinfo(i);
        printf("Partition %d: 0x%02x %ld MB",
                i, pinfo->type, pinfo->size / 2048);
    }

    if(btn & BOOTLOADER_BOOT_OF)
    {
        /* Load original mi4 firmware in to a memory buffer called loadbuffer.
           The rest of the loading is done in crt0.S.
           1) First try reading from the hidden partition (on Sansa only).
           2) Next try a decrypted mi4 file in /System/OF.mi4
           3) Finally, try a raw firmware binary in /System/OF.mi4. It should be
              a mi4 firmware decrypted and header stripped using mi4code.
        */
        printf("Loading original firmware...");

#ifdef SANSA_E200        
        /* First try a (hidden) firmware partition */
        printf("Trying firmware partition");
        pinfo = disk_partinfo(1);
        if(pinfo->type == PARTITION_TYPE_OS2_HIDDEN_C_DRIVE)
        {
            rc = load_mi4_part(loadbuffer, pinfo, MAX_LOADSIZE);
            if (rc < EOK) {
                printf("Can't load from partition");
                printf(strerror(rc));
            } else {
                return (void*)loadbuffer;
            }
        } else {
            printf("No hidden partition found.");
        }
#endif

        printf("Trying /System/OF.mi4");
        rc=load_mi4(loadbuffer, "/System/OF.mi4", MAX_LOADSIZE);
        if (rc < EOK) {
            printf("Can't load /System/OF.mi4");
            printf(strerror(rc));
        } else {
            return (void*)loadbuffer;
        }

        printf("Trying /System/OF.bin");
        rc=load_raw_firmware(loadbuffer, "/System/OF.bin", MAX_LOADSIZE);
        if (rc < EOK) {
            printf("Can't load /System/OF.bin");
            printf(strerror(rc));
        } else {
            return (void*)loadbuffer;
        }
        
        error(EBOOTFILE, rc);

    } else {
        printf("Loading Rockbox...");
        rc=load_mi4(loadbuffer, BOOTFILE, MAX_LOADSIZE);
        if (rc < EOK) {
            printf("Can't load %s:", BOOTFILE);
            printf(strerror(rc));

            /* Try loading rockbox from old rockbox.e200/rockbox.h10 format */
            rc=load_firmware(loadbuffer, OLD_BOOTFILE, MAX_LOADSIZE);
            if (rc < EOK) {
                printf("Can't load %s:", OLD_BOOTFILE);
                error(EBOOTFILE, rc);
            }
        }
    }
    
    return (void*)loadbuffer;
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */
void usb_acknowledge(void)
{
}

void usb_wait_for_disconnect(void)
{
}
