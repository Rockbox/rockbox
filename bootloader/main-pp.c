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
#include "crc32-mi4.h"
#include <string.h>
#include "power.h"
#if defined(SANSA_E200)
#include "i2c.h"
#include "backlight-target.h"
#endif
#if defined(SANSA_E200) || defined(SANSA_C200)
#include "usb.h"
#include "usb_drv.h"
#endif


/* Button definitions */
#if CONFIG_KEYPAD == IRIVER_H10_PAD
#define BOOTLOADER_BOOT_OF      BUTTON_LEFT

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define BOOTLOADER_BOOT_OF      BUTTON_LEFT

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define BOOTLOADER_BOOT_OF      BUTTON_LEFT

#endif

/* Maximum allowed firmware image size. 10MB is more than enough */
#define MAX_LOADSIZE (10*1024*1024)

/* A buffer to load the original firmware or Rockbox into */
unsigned char *loadbuffer = (unsigned char *)DRAM_START;

/* Bootloader version */
char version[] = APPSVERSION;
        
/* Locations and sizes in hidden partition on Sansa */
#if defined(SANSA_E200) || defined(SANSA_C200)
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

#define NUM_KEYS (sizeof(tea_keytable)/sizeof(tea_keytable[0]))
struct tea_key tea_keytable[] = {
  { "default" ,          { 0x20d36cc0, 0x10e8c07d, 0xc0e7dcaa, 0x107eb080 } },
  { "sansa",             { 0xe494e96e, 0x3ee32966, 0x6f48512b, 0xa93fbb42 } },
  { "sansa_gh",          { 0xd7b10538, 0xc662945b, 0x1b3fce68, 0xf389c0e6 } },
  { "sansa_103",         { 0x1d29ddc0, 0x2579c2cd, 0xce339e1a, 0x75465dfe } },
  { "rhapsody",          { 0x7aa9c8dc, 0xbed0a82a, 0x16204cc7, 0x5904ef38 } },
  { "p610",              { 0x950e83dc, 0xec4907f9, 0x023734b9, 0x10cfb7c7 } },
  { "p640",              { 0x220c5f23, 0xd04df68e, 0x431b5e25, 0x4dcc1fa1 } },
  { "virgin",            { 0xe83c29a1, 0x04862973, 0xa9b3f0d4, 0x38be2a9c } },
  { "20gc_eng",          { 0x0240772c, 0x6f3329b5, 0x3ec9a6c5, 0xb0c9e493 } },
  { "20gc_fre",          { 0xbede8817, 0xb23bfe4f, 0x80aa682d, 0xd13f598c } },
  { "elio_p722",         { 0x6af3b9f8, 0x777483f5, 0xae8181cc, 0xfa6d8a84 } },
  { "c200",              { 0xbf2d06fa, 0xf0e23d59, 0x29738132, 0xe2d04ca7 } },
  { "c200_103",          { 0x2a7968de, 0x15127979, 0x142e60a7, 0xe49c1893 } },
  { "c200_106",          { 0xa913d139, 0xf842f398, 0x3e03f1a6, 0x060ee012 } },
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
    unsigned int i;
    int rc;
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

#if defined(SANSA_E200) || defined(SANSA_C200)
/* Load mi4 firmware from a hidden disk partition */
int load_mi4_part(unsigned char* buf, struct partinfo* pinfo,
                  unsigned int buffer_size, bool disable_rebuild)
{
    struct mi4header_t mi4header;
    struct ppmi_header_t ppmi_header;
    unsigned long sum;
    
    /* Read header to find out how long the mi4 file is. */
    ata_read_sectors(IF_MV2(0,) pinfo->start + PPMI_SECTOR_OFFSET,
                            PPMI_SECTORS, &ppmi_header);
    
    /* The first four characters at 0x80000 (sector 1024) should be PPMI*/
    if( memcmp(ppmi_header.magic, "PPMI", 4) )
        return EFILE_NOT_FOUND;
    
    printf("BL mi4 size: %x", ppmi_header.length);
    
    /* Read mi4 header of the OF */
    ata_read_sectors(IF_MV2(0,) pinfo->start + PPMI_SECTOR_OFFSET + PPMI_SECTORS 
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
    ata_read_sectors(IF_MV2(0,) pinfo->start + PPMI_SECTOR_OFFSET + PPMI_SECTORS
                        + (ppmi_header.length/512) + MI4_HEADER_SECTORS,
                        (mi4header.mi4size-MI4_HEADER_SIZE)/512, buf);

    /* Check CRC32 to see if we have a valid file */
    sum = chksum_crc32 (buf,mi4header.mi4size-MI4_HEADER_SIZE);

    printf("Calculated CRC32: %x", sum);

    if(sum != mi4header.crc32)
        return EBAD_CHKSUM;
    
#ifdef SANSA_E200    
    if (disable_rebuild)
    {
        char block[512];
        
        printf("Disabling database rebuild");
        
        ata_read_sectors(IF_MV2(0,) pinfo->start + 0x3c08, 1, block);
        block[0xe1] = 0;
        ata_write_sectors(IF_MV2(0,) pinfo->start + 0x3c08, 1, block);
    }
#else
    (void) disable_rebuild;
#endif

    return EOK;
}
#endif

void* main(void)
{
    int i;
    int btn;
    int rc;
    int num_partitions;
    struct partinfo* pinfo;
#if defined(SANSA_E200) || defined(SANSA_C200)
    int usb_retry = 0;
    bool usb = false;
#else
    char buf[256];
    unsigned short* identify_info;
#endif

    chksum_crc32gentab ();

    system_init();
    kernel_init();
    lcd_init();
    font_init();
    button_init();
#if defined(SANSA_E200)
    i2c_init();
    _backlight_on();
#endif
    lcd_set_foreground(LCD_WHITE);
    lcd_set_background(LCD_BLACK);
    lcd_clear_display();
    
    if (button_hold())
    {
        verbose = true;
        printf("Hold switch on");
        printf("Shutting down...");
        sleep(HZ);
        power_off();
    }
        
    btn = button_read_device();
#if defined(SANSA_E200) || defined(SANSA_C200)
    usb_init();
    while (usb_drv_powered() && usb_retry < 5 && !usb)
    {
        usb_retry++;
        sleep(HZ/4);
        usb = (usb_detect() == USB_INSERTED);
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
#if !defined(SANSA_E200) && !defined(SANSA_C200)
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
#endif

    disk_init(IF_MV(0));
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

#if defined(SANSA_E200) || defined(SANSA_C200)
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
#if 0 /* e200: enable to be able to dump the hidden partition */
        if(btn & BUTTON_UP)
        {
            int fd;
            pinfo = disk_partinfo(1);
            fd = open("/part.bin", O_CREAT|O_RDWR);
            char sector[512];
            for(i=0; i<40960; i++){
                if (!(i%100))
                {
                    printf("dumping sector %d", i);
                }
                ata_read_sectors(IF_MV2(0,) pinfo->start + i, 1, sector);
                write(fd,sector,512);
            }
            close(fd);
        }
#endif
        printf("Loading Rockbox...");
        rc=load_mi4(loadbuffer, BOOTFILE, MAX_LOADSIZE);
        if (rc < EOK) {
            printf("Can't load %s:", BOOTFILE);
            printf(strerror(rc));

#ifdef OLD_BOOTFILE
            /* Try loading rockbox from old rockbox.e200/rockbox.h10 format */
            rc=load_firmware(loadbuffer, OLD_BOOTFILE, MAX_LOADSIZE);
            if (rc < EOK) {
                printf("Can't load %s:", OLD_BOOTFILE);
                error(EBOOTFILE, rc);
            }
#endif
        }
    }
    
    return (void*)loadbuffer;
}

#if !defined(SANSA_E200) && !defined(SANSA_C200)
/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */
void usb_acknowledge(void)
{
}

void usb_wait_for_disconnect(void)
{
}
#endif

