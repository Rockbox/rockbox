/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * !!! DON'T MESS WITH THIS CODE UNLESS YOU'RE ABSOLUTELY SURE WHAT YOU DO !!!
 * 
 * Copyright (C) 2006 by Miika Pekkarinen
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
#include "plugin.h"

/* All CFI flash routines are copied and ported from firmware_flash.c */

#ifndef SIMULATOR /* only for target */

unsigned char *audiobuf;
ssize_t audiobuf_size;

#ifdef IRIVER_H100_SERIES
#define PLATFORM_ID ID_IRIVER_H100
#else
#undef PLATFORM_ID /* this platform is not (yet) flashable */
#endif

#ifdef PLATFORM_ID

PLUGIN_HEADER

#if CONFIG_KEYPAD == IRIVER_H100_PAD
#define KEY1 BUTTON_OFF
#define KEY2 BUTTON_ON
#define KEY3 BUTTON_SELECT
#define KEYNAME1 "[Stop]"
#define KEYNAME2 "[On]"
#define KEYNAME3 "[Select]"
#endif

struct flash_info
{
    uint8_t manufacturer;
    uint8_t id;
    int size;
    char name[32];
};

#ifdef IRIVER_H100_SERIES
#define SEC_SIZE 4096
#define BOOTLOADER_ERASEGUARD  (BOOTLOADER_ENTRYPOINT / SEC_SIZE)
enum sections {
    SECT_RAMIMAGE = 1,
    SECT_ROMIMAGE = 2,
};

static volatile uint16_t* FB = (uint16_t*)0x00000000; /* Flash base address */
#endif

/* read the manufacturer and device ID */
bool cfi_read_id(volatile uint16_t* pBase, uint8_t* pManufacturerID, uint8_t* pDeviceID)
{
    uint8_t not_manu, not_id; /* read values before switching to ID mode */
    uint8_t manu, id; /* read values when in ID mode */
    
    pBase = (uint16_t*)((uint32_t)pBase & 0xFFF80000); /* down to 512k align */
    
    /* read the normal content */
    not_manu = pBase[0]; /* should be 'A' (0x41) and 'R' (0x52) */
    not_id   = pBase[1]; /*  from the "ARCH" marker */
    
    pBase[0x5555] = 0xAA; /* enter command mode */
    pBase[0x2AAA] = 0x55;
    pBase[0x5555] = 0x90; /* ID command */
    rb->sleep(HZ/50); /* Atmel wants 20ms pause here */
    
    manu = pBase[0];
    id   = pBase[1];
    
    pBase[0] = 0xF0; /* reset flash (back to normal read mode) */
    rb->sleep(HZ/50); /* Atmel wants 20ms pause here */
    
    /* I assume success if the obtained values are different from
    the normal flash content. This is not perfectly bulletproof, they 
    could theoretically be the same by chance, causing us to fail. */
    if (not_manu != manu || not_id != id) /* a value has changed */
    {
        *pManufacturerID = manu; /* return the results */
        *pDeviceID = id;
        return true; /* success */
    }
    return false; /* fail */
}


/* erase the sector which contains the given address */
bool cfi_erase_sector(volatile uint16_t* pAddr)
{
    unsigned timeout = 430000; /* the timeout loop should be no less than 25ms */
    
    FB[0x5555] = 0xAA; /* enter command mode */
    FB[0x2AAA] = 0x55;
    FB[0x5555] = 0x80; /* erase command */
    FB[0x5555] = 0xAA; /* enter command mode */
    FB[0x2AAA] = 0x55;
    *pAddr = 0x30; /* erase the sector */

    /* I counted 7 instructions for this loop -> min. 0.58 us per round */
    /* Plus memory waitstates it will be much more, gives margin */
    while (*pAddr != 0xFFFF && --timeout); /* poll for erased */

    return (timeout != 0);
}


/* address must be in an erased location */
inline bool cfi_program_word(volatile uint16_t* pAddr, uint16_t data)
{
    unsigned timeout = 85; /* the timeout loop should be no less than 20us */
    
    if (~*pAddr & data) /* just a safety feature, not really necessary */
        return false; /* can't set any bit from 0 to 1 */
    
    FB[0x5555] = 0xAA; /* enter command mode */
    FB[0x2AAA] = 0x55;
    FB[0x5555] = 0xA0; /* byte program command */
    
    *pAddr = data;
    
    /* I counted 7 instructions for this loop -> min. 0.58 us per round */
    /* Plus memory waitstates it will be much more, gives margin */
    while (*pAddr != data && --timeout); /* poll for programmed */
    
    return (timeout != 0);
}


/* this returns true if supported and fills the info struct */
bool cfi_get_flash_info(struct flash_info* pInfo)
{
    rb->memset(pInfo, 0, sizeof(struct flash_info));
    
    if (!cfi_read_id(FB, &pInfo->manufacturer, &pInfo->id))
        return false;
    
    if (pInfo->manufacturer == 0xBF) /* SST */
    {
        if (pInfo->id == 0xD6)
        {
            pInfo->size = 256* 1024; /* 256k */
            rb->strcpy(pInfo->name, "SST39VF020");
            return true;
        }
        else if (pInfo->id == 0xD7)
        {
            pInfo->size = 512* 1024; /* 512k */
            rb->strcpy(pInfo->name, "SST39VF040");
            return true;
        }
        else if (pInfo->id == 0x82)
        {
            pInfo->size = 2048* 1024; /* 2 MiB */
            rb->strcpy(pInfo->name, "SST39VF160");
            return true;
        }
        else
            return false;
    }
    return false;
}


/*********** Utility Functions ************/


/* Tool function to calculate a CRC32 across some buffer */
/* third argument is either 0xFFFFFFFF to start or value from last piece */
unsigned crc_32(const unsigned char* buf, unsigned len, unsigned crc32)
{
    /* CCITT standard polynomial 0x04C11DB7 */
    static const unsigned crc32_lookup[16] = 
    {   /* lookup table for 4 bits at a time is affordable */
        0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 
        0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005, 
        0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 
        0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD
    };
    
    unsigned char byte;
    unsigned t;

    while (len--)
    {   
        byte = *buf++; /* get one byte of data */

        /* upper nibble of our data */
        t = crc32 >> 28; /* extract the 4 most significant bits */
        t ^= byte >> 4; /* XOR in 4 bits of data into the extracted bits */
        crc32 <<= 4; /* shift the CRC register left 4 bits */     
        crc32 ^= crc32_lookup[t]; /* do the table lookup and XOR the result */

        /* lower nibble of our data */
        t = crc32 >> 28; /* extract the 4 most significant bits */
        t ^= byte & 0x0F; /* XOR in 4 bits of data into the extracted bits */
        crc32 <<= 4; /* shift the CRC register left 4 bits */     
        crc32 ^= crc32_lookup[t]; /* do the table lookup and XOR the result */
    }
    
    return crc32;
}


/***************** User Interface Functions *****************/
int wait_for_button(void)
{
    int button;
    
    do
    {
        button = rb->button_get(true);
    } while (IS_SYSEVENT(button) || (button & BUTTON_REL));
    
    return button;
}

/* helper for DoUserDialog() */
void ShowFlashInfo(struct flash_info* pInfo)
{
    char buf[32];
    
    if (!pInfo->manufacturer)
    {
        rb->lcd_puts(0, 0, "Flash: M=?? D=??");
        rb->lcd_puts(0, 1, "Impossible to program");
    }
    else
    {
        rb->snprintf(buf, sizeof(buf), "Flash: M=%02x D=%02x",
            pInfo->manufacturer, pInfo->id);
        rb->lcd_puts(0, 0, buf);
        
        
        if (pInfo->size)
        {
            rb->lcd_puts(0, 1, pInfo->name);
            rb->snprintf(buf, sizeof(buf), "Size: %d KB", pInfo->size / 1024);
            rb->lcd_puts(0, 2, buf);
        }
        else
        {
            rb->lcd_puts(0, 1, "Unsupported chip");
        }
        
    }
    
    rb->lcd_update();
}

bool show_info(void)
{
    struct flash_info fi;
    
    rb->lcd_clear_display();
    cfi_get_flash_info(&fi);
    ShowFlashInfo(&fi);
    if (fi.size == 0) /* no valid chip */
    {
        rb->splash(HZ*3, "Sorry!");
        return false; /* exit */
    }
    
    return true;
}

bool confirm(const char *msg)
{
    char buf[128];
    bool ret;
    
    rb->snprintf(buf, sizeof buf, "%s ([PLAY] to CONFIRM)", msg);
    rb->splash(0, buf);
    
    ret = (wait_for_button() == BUTTON_ON);
    show_info();
    
    return ret;
}

int load_firmware_file(const char *filename, uint32_t *checksum)
{
    int fd;
    int len, rc;
    int i;
    uint32_t sum;
    
    fd = rb->open(filename,  O_RDONLY);
    if (fd < 0)
        return -1;
    
    len = rb->filesize(fd);
    
    if (audiobuf_size < len)
    {
        rb->splash(HZ*3, "Aborting: Out of memory!");
        rb->close(fd);
        return -2;
    }
    
    rb->read(fd, checksum, 4);
    rb->lseek(fd, FIRMWARE_OFFSET_FILE_DATA, SEEK_SET);
    len -= FIRMWARE_OFFSET_FILE_DATA;
    
    rc = rb->read(fd, audiobuf, len);
    rb->close(fd);
    if (rc != len)
    {
        rb->splash(HZ*3, "Aborting: Read failure");
        return -3;
    }
    
    /* Verify the checksum */
    sum = MODEL_NUMBER;
    for (i = 0; i < len; i++)
        sum += audiobuf[i];
    
    if (sum != *checksum)
    {
        rb->splash(HZ*3, "Aborting: Checksums mismatch!");
        return -4;
    }
    
    return len;
}

unsigned long valid_bootloaders[][2] = { 
    /* Size-8   CRC32 */
#ifdef IRIVER_H120 /* Iriver H120/H140 checksums */
    { 63788, 0x08ff01a9 }, /* 7-pre3, improved failsafe functions */
    { 48764, 0xc674323e }, /* 7-pre4. Fixed audio thump & remote bootup */
#endif
#ifdef IRIVER_H100
    { 48760, 0x2efc3323 }, /* 7-pre4 */
#endif
    { 0,     0 }
};


bool detect_valid_bootloader(const unsigned char *addr, int len)
{
    int i;
    unsigned long crc32;
    
    /* Try to scan through all valid bootloaders. */
    for (i = 0; valid_bootloaders[i][0]; i++)
    {
        if (len > 0 && len != (long)valid_bootloaders[i][0])
            continue;
        
        crc32 = crc_32(addr, valid_bootloaders[i][0], 0xffffffff);
        if (crc32 == valid_bootloaders[i][1])
            return true;
    }
    
    return false;
}

static int get_section_address(int section)
{
    if (section == SECT_RAMIMAGE)
        return FLASH_RAMIMAGE_ENTRY;
    else if (section == SECT_ROMIMAGE)
        return FLASH_ROMIMAGE_ENTRY;
    else
        return -1;
}

int flash_rockbox(const char *filename, int section)
{
    struct flash_header hdr;
    char buf[64];
    int pos, i, len, rc;
    unsigned long checksum, sum;
    unsigned char *p8;
    uint16_t *p16;
    
    if (get_section_address(section) < 0)
        return -1;
    
    p8 = (char *)BOOTLOADER_ENTRYPOINT;
    if (!detect_valid_bootloader(p8, 0))
    {
        rb->splash(HZ*3, "Incompatible bootloader");
        return -1;
    }

    if (!rb->detect_original_firmware())
    {
        if (!confirm("Update Rockbox flash image?"))
            return -2;
    }
    else
    {
        if (!confirm("Erase original firmware?"))
            return -3;
    }

    len = load_firmware_file(filename, &checksum);
    if (len <= 0)
        return len * 10;
    
    pos = get_section_address(section);
    
    /* Check if image relocation seems to be sane. */
    if (section == SECT_ROMIMAGE)
    {
        uint32_t *p32 = (uint32_t *)audiobuf;

        if (pos+sizeof(struct flash_header) != *p32)
        {
            rb->snprintf(buf, sizeof(buf), "Incorrect relocation: 0x%08lx/0x%08lx",
                         *p32, pos+sizeof(struct flash_header));
            rb->splash(HZ*10, buf);
            return -1;
        }
        
    }
    
    /* Erase the program flash. */
    for (i = 0; i + pos < BOOTLOADER_ENTRYPOINT && i < len + 32; i += SEC_SIZE)
    {
        /* Additional safety check. */
        if (i + pos < SEC_SIZE)
            return -1;
        
        rb->snprintf(buf, sizeof(buf), "Erasing...  %d%%", 
                     (i+SEC_SIZE)*100/len);
        rb->lcd_puts(0, 3, buf);
        rb->lcd_update();
        
        rc = cfi_erase_sector(FB + (i + pos)/2);
    }
    
    /* Write the magic and size. */
    rb->memset(&hdr, 0, sizeof(struct flash_header));
    hdr.magic = FLASH_MAGIC;
    hdr.length = len;
    // rb->strncpy(hdr.version, APPSVERSION, sizeof(hdr.version)-1);
    p16 = (uint16_t *)&hdr;
    
    rb->snprintf(buf, sizeof(buf), "Programming...");
    rb->lcd_puts(0, 4, buf);
    rb->lcd_update();
    
    pos = get_section_address(section)/2;
    for (i = 0; i < (long)sizeof(struct flash_header)/2; i++)
    {
        cfi_program_word(FB + pos, p16[i]);
        pos++;
    }
    
    p16 = (uint16_t *)audiobuf;
    for (i = 0; i < len/2 && pos + i < (BOOTLOADER_ENTRYPOINT/2); i++)
    {
        if (i % SEC_SIZE == 0)
        {
            rb->snprintf(buf, sizeof(buf), "Programming...  %d%%",
                         (i+1)*100/(len/2));
            rb->lcd_puts(0, 4, buf);
            rb->lcd_update();
        }
        
        cfi_program_word(FB + pos + i, p16[i]);
    }
    
    /* Verify */
    rb->snprintf(buf, sizeof(buf), "Verifying");
    rb->lcd_puts(0, 5, buf);
    rb->lcd_update();
    
    p8 = (char *)get_section_address(section);
    p8 += sizeof(struct flash_header);
    sum = MODEL_NUMBER;
    for (i = 0; i < len; i++)
        sum += p8[i];
    
    if (sum != checksum)
    {
        rb->splash(HZ*3, "Verify failed!");
        /* Erase the magic sector so bootloader does not try to load
         * rockbox from flash and crash. */
        if (section == SECT_RAMIMAGE)
            cfi_erase_sector(FB + FLASH_RAMIMAGE_ENTRY/2);
        else
            cfi_erase_sector(FB + FLASH_ROMIMAGE_ENTRY/2);
        return -5;
    }
    
    rb->splash(HZ*2, "Success");
    
    return 0;
}

void show_fatal_error(void)
{
    rb->splash(HZ*30, "Disable idle poweroff, connect AC power and DON'T TURN PLAYER OFF!!");
    rb->splash(HZ*30, "Contact Rockbox developers as soon as possible!");
    rb->splash(HZ*30, "Your device won't be bricked unless you turn off the power");
    rb->splash(HZ*30, "Don't use the device before further instructions from Rockbox developers");
}

int flash_bootloader(const char *filename)
{
    char *bootsector;
    int pos, i, len, rc;
    unsigned long checksum, sum;
    unsigned char *p8;
    uint16_t *p16;

    bootsector = audiobuf;
    audiobuf += SEC_SIZE;
    audiobuf_size -= SEC_SIZE;
    
    if (!confirm("Update bootloader?"))
        return -2;
    
    len = load_firmware_file(filename, &checksum);
    if (len <= 0)
        return len * 10;
    
    if (len > 0xFFFF)
    {
        rb->splash(HZ*3, "Too big bootloader");
        return -1;
    }
    
    /* Verify the crc32 checksum also. */
    if (!detect_valid_bootloader(audiobuf, len))
    {
        rb->splash(HZ*3, "Incompatible/Untested bootloader");
        return -1;
    }

    rb->lcd_puts(0, 3, "Flashing...");
    rb->lcd_update();

    /* Backup the bootloader sector first. */
    p8 = (char *)FB;
    rb->memcpy(bootsector, p8, SEC_SIZE);
    
    /* Erase the boot sector and write a proper reset vector. */
    cfi_erase_sector(FB);
    p16 = (uint16_t *)audiobuf;
    for (i = 0; i < 8/2; i++)
        cfi_program_word(FB + i, p16[i]);
    
    /* And restore original content for original FW to function. */
    p16 = (uint16_t *)bootsector;
    for (i = 8/2; i < SEC_SIZE/2; i++)
        cfi_program_word(FB + i, p16[i]);
    
    /* Erase the bootloader flash section. */
    for (i = BOOTLOADER_ENTRYPOINT/SEC_SIZE; i < 0x200; i++)
        rc = cfi_erase_sector(FB + (SEC_SIZE/2) * i);
    
    pos = BOOTLOADER_ENTRYPOINT/2;
    p16 = (uint16_t *)audiobuf;
    for (i = 0; i < len/2; i++)
        cfi_program_word(FB + pos + i, p16[i]);
    
    /* Verify */
    p8 = (char *)BOOTLOADER_ENTRYPOINT;
    sum = MODEL_NUMBER;
    for (i = 0; i < len; i++)
        sum += p8[i];
    
    if (sum != checksum)
    {
        rb->splash(HZ*3, "Verify failed!");
        show_fatal_error();
        return -5;
    }
    
    p8 = (char *)FB;
    for (i = 0; i < 8; i++)
    {
        if (p8[i] != audiobuf[i])
        {
            rb->splash(HZ*3, "Bootvector corrupt!");
            show_fatal_error();
            return -6;
        }
    }
    
    rb->splash(HZ*2, "Success");
    
    return 0;
}

int flash_original_fw(int len)
{
    unsigned char reset_vector[8];
    char buf[32];
    int pos, i, rc;
    unsigned char *p8;
    uint16_t *p16;
    
    (void)buf;
    
    rb->lcd_puts(0, 3, "Critical section...");
    rb->lcd_update();

    p8 = (char *)FB;
    rb->memcpy(reset_vector, p8, sizeof reset_vector);
    
    /* Erase the boot sector and write back the reset vector. */
    cfi_erase_sector(FB);
    p16 = (uint16_t *)reset_vector;
    for (i = 0; i < (long)sizeof(reset_vector)/2; i++)
        cfi_program_word(FB + i, p16[i]);
    
    rb->lcd_puts(0, 4, "Flashing orig. FW");
    rb->lcd_update();
    
    /* Erase the program flash. */
    for (i = 1; i < BOOTLOADER_ERASEGUARD && (i-1)*4096 < len; i++)
    {
        rc = cfi_erase_sector(FB + (SEC_SIZE/2) * i);
        rb->snprintf(buf, sizeof(buf), "Erase: 0x%03x  (%d)", i, rc);
        rb->lcd_puts(0, 5, buf);
        rb->lcd_update();
    }
    
    rb->snprintf(buf, sizeof(buf), "Programming");
    rb->lcd_puts(0, 6, buf);
    rb->lcd_update();
    
    pos = 0x00000008/2;
    p16 = (uint16_t *)audiobuf;
    for (i = 0; i < len/2 && pos + i < (BOOTLOADER_ENTRYPOINT/2); i++)
        cfi_program_word(FB + pos + i, p16[i]);
    
    rb->snprintf(buf, sizeof(buf), "Verifying");
    rb->lcd_puts(0, 7, buf);
    rb->lcd_update();
    
    /* Verify reset vectors. */
    p8 = (char *)FB;
    for (i = 0; i < 8; i++)
    {
        if (p8[i] != reset_vector[i])
        {
            rb->splash(HZ*3, "Bootvector corrupt!");
            show_fatal_error();
            break;
        }
    }
    
    /* Verify */
    p8 = (char *)0x00000008;
    for (i = 0; i < len; i++)
    {
        if (p8[i] != audiobuf[i])
        {
            rb->splash(HZ*3, "Verify failed!");
            rb->snprintf(buf, sizeof buf, "at: 0x%08x", i);
            rb->splash(HZ*10, buf);
            return -5;
        }
    }
    
    rb->splash(HZ*2, "Success");
    
    return 0;
}

int load_original_bin(const char *filename)
{
    unsigned long magic[2];
    int len, rc;
    int fd;
    
    if (!confirm("Restore original firmware (bootloader will be kept)?"))
        return -2;
    
    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
        return -1;
    
    len = rb->filesize(fd) - 0x228;
    rb->lseek(fd, 0x220, SEEK_SET);
    rb->read(fd, magic, 8);
    if (magic[1] != 0x00000008 || len <= 0 || len > audiobuf_size)
    {
        rb->splash(HZ*2, "Not an original firmware file");
        rb->close(fd);
        return -1;
    }
    
    rc = rb->read(fd, audiobuf, len);
    rb->close(fd);
    
    if (rc != len)
    {
        rb->splash(HZ*2, "Read error");
        return -2;
    }
    
    if (len % 2)
        len++;
    
    return flash_original_fw(len);
}

int load_romdump(const char *filename)
{
    int len, rc;
    int fd;
    
    if (!confirm("Restore firmware section (bootloader will be kept)?"))
        return -2;
    
    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
        return -1;
    
    len = rb->filesize(fd) - 8;
    if (len <= 0)
        return -1;
    
    rb->lseek(fd, 8, SEEK_SET);
    rc = rb->read(fd, audiobuf, len);
    rb->close(fd);
    
    if (rc != len)
    {
        rb->splash(HZ*2, "Read error");
        return -2;
    }
    
    if (len % 2)
        len++;
    
    if (len > BOOTLOADER_ENTRYPOINT - 8)
        len = BOOTLOADER_ENTRYPOINT - 8;
    
    return flash_original_fw(len);
}

/* Kind of our main function, defines the application flow. */
void DoUserDialog(char* filename)
{
    /* this can only work if Rockbox runs in DRAM, not flash ROM */
    if ((uint16_t*)rb >= FB && (uint16_t*)rb < FB + 4096*1024) /* 4 MB max */
    {   /* we're running from flash */
        rb->splash(HZ*3, "Not from ROM");
        return; /* exit */
    }

    /* refuse to work if the power may fail meanwhile */
    if (!rb->battery_level_safe())
    {
        rb->splash(HZ*3, "Battery too low!");
        return; /* exit */
    }
    
    rb->lcd_setfont(FONT_SYSFIXED);
    if (!show_info())
        return ;

    if (filename == NULL)
    {
        rb->splash(HZ*3, "Please use this plugin with \"Open with...\"");
        return ;
    }
    
    audiobuf = rb->plugin_get_audio_buffer((size_t *)&audiobuf_size);
    
    if (rb->strcasestr(filename, "/rockbox.iriver"))
        flash_rockbox(filename, SECT_RAMIMAGE);
    else if (rb->strcasestr(filename, "/rombox.iriver"))
        flash_rockbox(filename, SECT_ROMIMAGE);
    else if (rb->strcasestr(filename, "/bootloader.iriver"))
        flash_bootloader(filename);
    else if (rb->strcasestr(filename, "/ihp_120.bin"))
        load_original_bin(filename);
    else if (rb->strcasestr(filename, "/internal_rom_000000-1FFFFF.bin"))
        load_romdump(filename);
    else
        rb->splash(HZ*3, "Unknown file type");
}


/***************** Plugin Entry Point *****************/

enum plugin_status plugin_start(const void* parameter)
{
    int oldmode;

    /* now go ahead and have fun! */
    oldmode = rb->system_memory_guard(MEMGUARD_NONE); /*disable memory guard */
    DoUserDialog((char*) parameter);
    rb->system_memory_guard(oldmode);              /* re-enable memory guard */

    return PLUGIN_OK;
}

#endif /* ifdef PLATFORM_ID */
#endif /* #ifndef SIMULATOR */
