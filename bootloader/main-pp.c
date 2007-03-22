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
#include <stdio.h>
#include <stdlib.h>
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
#ifdef SANSA_E200
#include "usb.h"
#endif


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
#ifdef SANSA_E200
#define PPMI_SECTOR_OFFSET  1024
#define PPMI_SECTORS        1
#define MI4_HEADER_SECTORS  1
#define NUM_PARTITIONS      2

#else
#define NUM_PARTITIONS      1

#endif

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

inline unsigned int le2int(unsigned char* buf)
{
   int32_t res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

   return res;
}

inline void int2le(unsigned int val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
    addr[2] = (val >> 16) & 0xff;
    addr[3] = (val >> 24) & 0xff;
}

struct tea_key {
  const char * name;
  uint32_t     key[4];
};

#define NUM_KEYS 11
struct tea_key tea_keytable[] = {
  { "default" ,          { 0x20d36cc0, 0x10e8c07d, 0xc0e7dcaa, 0x107eb080 } },
  { "sansa",             { 0xe494e96e, 0x3ee32966, 0x6f48512b, 0xa93fbb42 } },
  { "sansa_gh",          { 0xd7b10538, 0xc662945b, 0x1b3fce68, 0xf389c0e6 } },
  { "rhapsody",          { 0x7aa9c8dc, 0xbed0a82a, 0x16204cc7, 0x5904ef38 } },
  { "p610",              { 0x950e83dc, 0xec4907f9, 0x023734b9, 0x10cfb7c7 } },
  { "p640",              { 0x220c5f23, 0xd04df68e, 0x431b5e25, 0x4dcc1fa1 } },
  { "virgin",            { 0xe83c29a1, 0x04862973, 0xa9b3f0d4, 0x38be2a9c } },
  { "20gc_eng",          { 0x0240772c, 0x6f3329b5, 0x3ec9a6c5, 0xb0c9e493 } },
  { "20gc_fre",          { 0xbede8817, 0xb23bfe4f, 0x80aa682d, 0xd13f598c } },
  { "elio_p722",         { 0x6af3b9f8, 0x777483f5, 0xae8181cc, 0xfa6d8a84 } },
  { "c200",              { 0xbf2d06fa, 0xf0e23d59, 0x29738132, 0xe2d04ca7 } },
};

/*

tea_decrypt() from http://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm

"Following is an adaptation of the reference encryption and decryption
routines in C, released into the public domain by David Wheeler and
Roger Needham:"

*/

/* NOTE: The mi4 version of TEA uses a different initial value to sum compared
         to the reference implementation and the main loop is 8 iterations, not
         32.
*/

static void tea_decrypt(uint32_t* v0, uint32_t* v1, uint32_t* k) {
    uint32_t sum=0xF1BBCDC8, i;                    /* set up */
    uint32_t delta=0x9E3779B9;                     /* a key schedule constant */
    uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */
    for(i=0; i<8; i++) {                               /* basic cycle start */
        *v1 -= ((*v0<<4) + k2) ^ (*v0 + sum) ^ ((*v0>>5) + k3);
        *v0 -= ((*v1<<4) + k0) ^ (*v1 + sum) ^ ((*v1>>5) + k1);
        sum -= delta;                                   /* end cycle */
    }
}

/* mi4 files are encrypted in 64-bit blocks (two little-endian 32-bit
   integers) and the key is incremented after each block
 */

static void tea_decrypt_buf(unsigned char* src, unsigned char* dest, size_t n, uint32_t * key)
{
    uint32_t v0, v1;
    unsigned int i;

    for (i = 0; i < (n / 8); i++) {
        v0 = le2int(src);
        v1 = le2int(src+4);

        tea_decrypt(&v0, &v1, key);

        int2le(v0, dest);
        int2le(v1, dest+4);

        src += 8;
        dest += 8;

        /* Now increment the key */
        key[0]++;
        if (key[0]==0) {
            key[1]++;
            if (key[1]==0) {
                key[2]++;
                if (key[2]==0) {
                    key[3]++;
                }
            }
        }
    }
}

static inline bool tea_test_key(unsigned char magic_enc[8], uint32_t * key, int unaligned)
{
    unsigned char magic_dec[8];
    tea_decrypt_buf(magic_enc, magic_dec, 8, key);

    return (le2int(&magic_dec[4*unaligned]) == 0xaa55aa55);
}

static int tea_find_key(struct mi4header_t *mi4header, int fd)
{
    int i, rc;
    unsigned int j;
    uint32_t key[4];
    unsigned char magic_enc[8];
    int key_found = -1;
    unsigned int magic_location = mi4header->length-4;
    int unaligned = 0;
    
    if ( (magic_location % 8) != 0 )
    {
        unaligned = 1;
        magic_location -= 4;
    }
    
    /* Load encrypted magic 0xaa55aa55 to check key */
    lseek(fd, MI4_HEADER_SIZE + magic_location, SEEK_SET);
    rc = read(fd, magic_enc, 8);
    if(rc < 8 )
        return EREAD_IMAGE_FAILED;

    printf("Searching for key:");

    for (i=0; i < NUM_KEYS && (key_found<0) ; i++) {
        key[0] = tea_keytable[i].key[0];
        key[1] = tea_keytable[i].key[1];
        key[2] = tea_keytable[i].key[2];
        key[3] = tea_keytable[i].key[3];
        
        /* Now increment the key */
        for(j=0; j<((magic_location-mi4header->plaintext)/8); j++){
            key[0]++;
            if (key[0]==0) {
                key[1]++;
                if (key[1]==0) {
                    key[2]++;
                    if (key[2]==0) {
                        key[3]++;
                    }
                }
            }
        }
        
        if (tea_test_key(magic_enc,key,unaligned))
        {
            key_found = i;
            printf("%s...found", tea_keytable[i].name);
        } else {
           /* printf("%s...failed", tea_keytable[i].name); */
        }
    }
    
    return key_found;
}
        
/*
 * We can't use the CRC32 implementation in the firmware library as it uses a
 * different polynomial. The polynomial needed is 0xEDB88320L
 *
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

    /* Load firmware file */
    lseek(fd, MI4_HEADER_SIZE, SEEK_SET);
    rc = read(fd, buf, mi4header.mi4size-MI4_HEADER_SIZE);
    if(rc < (int)mi4header.mi4size-MI4_HEADER_SIZE)
        return EREAD_IMAGE_FAILED;
 
    /* Check CRC32 to see if we have a valid file */
    sum = chksum_crc32 (buf, mi4header.mi4size - MI4_HEADER_SIZE);

    printf("Calculated CRC32: %x", sum);

    if(sum != mi4header.crc32)
        return EBAD_CHKSUM;
        
    if( (mi4header.plaintext + MI4_HEADER_SIZE) != mi4header.mi4size)
    {
        /* Load encrypted firmware */
        int key_index = tea_find_key(&mi4header, fd);
        
        if (key_index < 0)
            return EINVALID_FORMAT;
        
        /* Plaintext part is already loaded */
        buf += mi4header.plaintext;
        
        /* Decrypt in-place */
        tea_decrypt_buf(buf, buf, 
                        mi4header.mi4size-(mi4header.plaintext+MI4_HEADER_SIZE),
                        tea_keytable[key_index].key);

        printf("%s key used", tea_keytable[key_index].name);
        
        /* Check decryption was successfull */
        if(le2int(&buf[mi4header.length-mi4header.plaintext-4]) != 0xaa55aa55)
        {
            return EREAD_IMAGE_FAILED;
        }
    }

    return EOK;
}

#ifdef SANSA_E200
/* Load mi4 firmware from a hidden disk partition */
int load_mi4_part(unsigned char* buf, struct partinfo* pinfo,
                  unsigned int buffer_size, bool disable_rebuild)
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
    
    if (disable_rebuild)
    {
        char block[512];
        int  sector = 0, offset = 0;
    
        /* check which known version we have */
        /* These are taken from the PPPS section, 0x00780240 */
        ata_read_sectors(pinfo->start + 0x3C01, 1, block);
        if (!memcmp(&block[0x40], 
            "PP5022AF-05.51-S301-01.11-S301.01.11A-D", 39))
        {   /* American e200, OF version 1.01.11A */
            sector = pinfo->start + 0x3c08;
            offset  = 0xe1;
        }
        else if (!memcmp(&block[0x40], 
                "PP5022AF-05.51-S301-00.12-S301.00.12E-D", 39))
        {   /* European e200, OF version 1.00.12 */
            sector = pinfo->start + 0x3c5c;
            offset  = 0x2;
        }
        else 
            return EOK;
        ata_read_sectors(sector, 1, block);
        block[offset] = 0;
        ata_write_sectors(sector, 1, block);
    }
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
#ifdef SANSA_E200
    int usb_retry = 0;
    bool usb = false;
#endif

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
#ifdef SANSA_E200
    usb_init();
    while (usb_retry < 5 && !usb)
    {
        usb_retry++;
        sleep(HZ/4);
        usb = usb_detect();
    }
    if (usb)
        btn |= BOOTLOADER_BOOT_OF;
#endif
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
    for(i=0; i<NUM_PARTITIONS; i++)
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
            rc = load_mi4_part(loadbuffer, pinfo, MAX_LOADSIZE, usb);
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
        
        error(0, 0);

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

#ifndef SANSA_E200
/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */
void usb_acknowledge(void)
{
}

void usb_wait_for_disconnect(void)
{
}
#endif

