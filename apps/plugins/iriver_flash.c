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

/* cfi_program_word() relies on writing to address 0, which normally is illegal.
   So we need this to ensure we don't helpfully optimize it away into a TRAP
   when compiled with -fdelete-null-pointer-checks, which is enabled by default
   at -Os with our current gcc 4.9.x toolchain.
*/
#pragma GCC optimize "no-delete-null-pointer-checks"

/* All CFI flash routines are copied and ported from firmware_flash.c */

uint8_t* audiobuf;
ssize_t audiobuf_size;

#if !defined(IRIVER_H100_SERIES) && !defined(IRIVER_H300_SERIES)
#error this platform is not (yet) flashable
#endif

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
    uint16_t manufacturer;
    uint16_t id;
    uint32_t size;
    char name[32];
};

#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
#define SEC_SIZE 4096
#define BOOTLOADER_ERASEGUARD  (BOOTLOADER_ENTRYPOINT / SEC_SIZE)
enum sections {
    SECT_RAMIMAGE = 1,
    SECT_ROMIMAGE = 2,
};

static volatile uint16_t* FB = (uint16_t*)0x00000000; /* Flash base address */
#endif

#ifdef IRIVER_H100 /* iRiver H110/H115 */
#define MODEL "h100"
#elif defined(IRIVER_H120) /* iRiver H120/H140 */
#define MODEL "h120"
#elif defined(IRIVER_H300) /* iRiver H320/H340 */
#define MODEL "h300"
#endif

/* read the manufacturer and device ID */
static void cfi_read_id(struct flash_info* pInfo)
{
    FB[0x5555] = 0xAA; /* enter command mode */
    FB[0x2AAA] = 0x55;
    FB[0x5555] = 0x90; /* enter ID mode */
    rb->sleep(HZ/100); /* we only need to sleep 150ns but 10ms is the minimum */

    pInfo->manufacturer = FB[0];
    pInfo->id = FB[1];

    FB[0x5555] = 0xAA; /* enter command mode */
    FB[0x2AAA] = 0x55;
    FB[0x5555] = 0xF0; /* exit ID mode */
    rb->sleep(HZ/100); /* we only need to sleep 150ns but 10ms is the minimum */
}

/* wait until the rom signals completion of an operation */
static bool cfi_wait_for_rom(volatile uint16_t* pAddr)
{
    const unsigned MAX_TIMEOUT = 0xffffff; /* should be sufficient for most targets */
    const unsigned RECOVERY_TIME = 64; /* based on 140MHz MCF 5249 */
    uint16_t old_data = *pAddr & 0x0040; /* we only want DQ6 */
    volatile unsigned i; /* disables certain optimizations */

    /* repeat up to MAX_TIMEOUT times or until DQ6 stops flipping */
    for (i = 0; i < MAX_TIMEOUT; i++)
    {
        uint16_t new_data = *pAddr & 0x0040; /* we only want DQ6 */
        if(old_data == new_data)
            break;
        old_data = new_data;
    }

    bool result = i != MAX_TIMEOUT;

    /* delay at least 1us to give the bus time to recover */
    for (i = 0; i < RECOVERY_TIME; i++);

    return result;
}

/* erase the sector which contains the given address */
static bool cfi_erase_sector(volatile uint16_t* pAddr)
{
    FB[0x5555] = 0xAA; /* enter command mode */
    FB[0x2AAA] = 0x55;
    FB[0x5555] = 0x80; /* erase command */
    FB[0x5555] = 0xAA; /* enter command mode */
    FB[0x2AAA] = 0x55;
    *pAddr = 0x30; /* erase the sector */

    return cfi_wait_for_rom(pAddr);
}

/* address must be in an erased location */
static bool cfi_program_word(volatile uint16_t* pAddr, uint16_t data)
{
    FB[0x5555] = 0xAA; /* enter command mode */
    FB[0x2AAA] = 0x55;
    FB[0x5555] = 0xA0; /* byte program command */
    *pAddr = data; /* write the word data */

    return cfi_wait_for_rom(pAddr);
}

/* fills in the struct with data about the flash rom */
static void cfi_get_flash_info(struct flash_info* pInfo)
{
    uint32_t size = 0;
    const char* name = "";

    cfi_read_id(pInfo);

    switch (pInfo->manufacturer)
    {
        /* SST */
        case 0x00BF:
            switch (pInfo->id)
            {
                case 0x2782:
                    size = 2048 * 1024; /* 2 MiB */
                    name = "SST39VF160";
                    break;
                case 0x235B:
                    size = 4096 * 1024; /* 4 MiB */
                    name = "SST39VF3201";
                    break;
            }
            break;
    }

    pInfo->size = size;
    rb->strcpy(pInfo->name, name);
}

/***************** User Interface Functions *****************/
static int wait_for_button(void)
{
    int button;

    do
    {
        button = rb->button_get(true);
    } while (IS_SYSEVENT(button) || (button & BUTTON_REL));

    return button;
}

/* helper for DoUserDialog() */
static void ShowFlashInfo(const struct flash_info* pInfo)
{
    if (!pInfo->manufacturer)
    {
        rb->lcd_puts(0, 0, "Flash: M=???? D=????");
        rb->lcd_puts(0, 1, "Impossible to program");
    }
    else
    {
        rb->lcd_putsf(0, 0, "Flash: M=%04x D=%04x",
            pInfo->manufacturer, pInfo->id);

        if (pInfo->size)
        {
            rb->lcd_puts(0, 1, pInfo->name);
            rb->lcd_putsf(0, 2, "Size: %u KB", pInfo->size / 1024);
        }
        else
        {
            rb->lcd_puts(0, 1, "Unsupported chip");
        }
    }

    rb->lcd_update();
}

static bool show_info(void)
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

static bool confirm(const char* msg)
{
    rb->splashf(0, "%s ([PLAY] to CONFIRM)", msg);
    bool ret = (wait_for_button() == BUTTON_ON);
    show_info();
    return ret;
}

static off_t load_firmware_file(const char* filename, uint32_t* out_checksum)
{
    int fd;
    off_t len;
    uint32_t checksum;
    char model[4];
    uint32_t sum;

    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
        return -1;

    len = rb->filesize(fd);
    if (audiobuf_size < len)
    {
        rb->splash(HZ*3, "Aborting: Out of memory!");
        rb->close(fd);
        return -2;
    }

    if (rb->read(fd, &checksum, sizeof(checksum)) != sizeof(checksum))
    {
        rb->splash(HZ*3, "Aborting: Read failure");
        rb->close(fd);
        return -3;
    }

    if (rb->read(fd, model, sizeof(model)) != sizeof(model))
    {
        rb->splash(HZ*3, "Aborting: Read failure");
        rb->close(fd);
        return -4;
    }

    len -= FIRMWARE_OFFSET_FILE_DATA;

    if (rb->read(fd, audiobuf, len) != len)
    {
        rb->splash(HZ*3, "Aborting: Read failure");
        rb->close(fd);
        return -5;
    }

    rb->close(fd);

    /* Verify the checksum */
    sum = MODEL_NUMBER;
    for (off_t i = 0; i < len; i++)
        sum += audiobuf[i];

    if (sum != checksum)
    {
        rb->splash(HZ*3, "Aborting: Checksums mismatch!");
        return -6;
    }

    /* Verify the model */
    if (rb->memcmp(model, MODEL, sizeof(model)) != 0)
    {
        rb->splash(HZ*3, "Aborting: Models mismatch!");
        return -7;
    }

    if (out_checksum != NULL)
        *out_checksum = checksum;

    return len;
}

static const uint32_t valid_bootloaders[][2] = {
    /* Size-8, CRC32 */
#ifdef IRIVER_H100 /* iRiver H110/H115 */
    { 48760, 0x2efc3323 }, /* 7-pre4 */
    { 56896, 0x0cd8dad4 }, /* 7-pre5 */
#elif defined(IRIVER_H120) /* iRiver H120/H140 */
    { 63788, 0x08ff01a9 }, /* 7-pre3, improved failsafe functions */
    { 48764, 0xc674323e }, /* 7-pre4. Fixed audio thump & remote bootup */
    { 56896, 0x167f5d25 }, /* 7-pre5, various ATA fixes */
#elif defined(IRIVER_H300) /* iRiver H320/H340 */
#endif
    {}
};

/* check if the bootloader is a known good one */
static bool detect_valid_bootloader(const uint8_t* pAddr, uint32_t len)
{
    for (size_t i = 0; valid_bootloaders[i][0]; i++)
    {
        if (len != valid_bootloaders[i][0])
            continue;

        uint32_t crc32 = rb->crc_32(pAddr, valid_bootloaders[i][0], 0xffffffff);
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
    int pos, i, len/*, rc */;
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
            rb->splashf(HZ*10, "Incorrect relocation: 0x%08lx/0x%08lx",
                         *p32, pos+sizeof(struct flash_header));
            return -1;
        }

    }

    /* Erase the program flash. */
    for (i = 0; i + pos < BOOTLOADER_ENTRYPOINT && i < len + 32; i += SEC_SIZE)
    {
        /* Additional safety check. */
        if (i + pos < SEC_SIZE)
            return -1;

        rb->lcd_putsf(0, 3, "Erasing...  %d%%", (i+SEC_SIZE)*100/len);
        rb->lcd_update();

        /*rc = */cfi_erase_sector(FB + (i + pos)/2);
    }

    /* Write the magic and size. */
    rb->memset(&hdr, 0, sizeof(struct flash_header));
    hdr.magic = FLASH_MAGIC;
    hdr.length = len;
    // rb->strncpy(hdr.version, rb->rbversion , sizeof(hdr.version)-1);
    p16 = (uint16_t *)&hdr;

    rb->lcd_puts(0, 4, "Programming...");
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
            rb->lcd_putsf(0, 4, "Programming...  %d%%", (i+1)*100/(len/2));
            rb->lcd_update();
        }

        cfi_program_word(FB + pos + i, p16[i]);
    }

    /* Verify */
    rb->lcd_puts(0, 5, "Verifying");
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

static void show_fatal_error(void)
{
    rb->splash(HZ*30, "Disable idle poweroff, connect AC power and DON'T TURN PLAYER OFF!!");
    rb->splash(HZ*30, "Contact Rockbox developers as soon as possible!");
    rb->splash(HZ*30, "Your device won't be bricked unless you turn off the power");
    rb->splash(HZ*30, "Don't use the device before further instructions from Rockbox developers");
}

int flash_bootloader(const char *filename)
{
    char *bootsector;
    int pos, i, len/*, rc*/;
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
    for (i = BOOTLOADER_ERASEGUARD; i < BOOTLOADER_ERASEGUARD+16; i++)
        /*rc =*/ cfi_erase_sector(FB + (SEC_SIZE/2) * i);

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
    int pos, i, rc;
    unsigned char *p8;
    uint16_t *p16;

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
        rb->lcd_putsf(0, 5, "Erase: 0x%03x  (%d)", i, rc);
        rb->lcd_update();
    }

    rb->lcd_puts(0, 6, "Programming");
    rb->lcd_update();

    pos = 0x00000008/2;
    p16 = (uint16_t *)audiobuf;
    for (i = 0; i < len/2 && pos + i < (BOOTLOADER_ENTRYPOINT/2); i++)
        cfi_program_word(FB + pos + i, p16[i]);

    rb->lcd_puts(0, 7, "Verifying");
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
            rb->splashf(HZ*10, "at: 0x%08x", i);
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
static void DoUserDialog(const char* filename)
{
    /* check whether we're running from ROM */
    uint16_t* RB = (uint16_t*) rb;
    if (RB >= FB && RB < FB + (FLASH_SIZE / sizeof(*FB)))
    {
        rb->splash(HZ*3, "Not from ROM");
        return; /* exit */
    }

    /* refuse to work if the power may fail meanwhile */
    if (!rb->battery_level_safe())
    {
        rb->splash(HZ*3, "Battery too low!");
        return; /* exit */
    }

    if (!show_info())
        return; /* exit */

    if (filename == NULL)
    {
        rb->splash(HZ*3, "Please use this plugin with \"Open with...\"");
        return; /* exit */
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
    rb->lcd_setfont(FONT_SYSFIXED);
    DoUserDialog(parameter);
    rb->lcd_setfont(FONT_UI);
    rb->system_memory_guard(oldmode);              /* re-enable memory guard */

    return PLUGIN_OK;
}
