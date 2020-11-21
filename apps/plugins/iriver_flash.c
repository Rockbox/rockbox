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
 * Copyright (C) 2020 by James Buren (refactor + H300 support)
 * Copyright (C) 2006 by Miika Pekkarinen (original + H100/H120 support)
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
#include "lib/helper.h"
#include "checksum.h"

/*
 * Flash commands may rely on null pointer dereferences to work correctly.
 * Disable this feature of GCC that may interfere with proper code generation.
 */
#pragma GCC optimize "no-delete-null-pointer-checks"

enum firmware
{
    FIRMWARE_ROCKBOX, /* all .iriver firmwares */
    FIRMWARE_ROMDUMP, /* a debug romdump */
    FIRMWARE_ORIGINAL, /* an unscrambled original firmware */
};

#define WORD_SIZE 2
#define BOOT_VECTOR_SIZE 8
#define BOOT_SECTOR_OFFSET 0
#define SECTOR_SIZE 4096
#define BOOTLOADER_MAX_SIZE 65536
#define BOOTLOADER_SECTORS (BOOTLOADER_MAX_SIZE / SECTOR_SIZE)
#define RAM_IMAGE_RAW_SIZE (FLASH_ROMIMAGE_ENTRY - FLASH_RAMIMAGE_ENTRY)
#define RAM_IMAGE_MAX_SIZE (RAM_IMAGE_RAW_SIZE - sizeof(struct flash_header))
#define RAM_IMAGE_SECTORS (RAM_IMAGE_RAW_SIZE / SECTOR_SIZE)
#define ROM_IMAGE_RAW_SIZE (BOOTLOADER_ENTRYPOINT - FLASH_ROMIMAGE_ENTRY)
#define ROM_IMAGE_MAX_SIZE (ROM_IMAGE_RAW_SIZE - sizeof(struct flash_header))
#define ROM_IMAGE_SECTORS (ROM_IMAGE_RAW_SIZE / SECTOR_SIZE)
#define ROM_IMAGE_RELOCATION (FLASH_ROMIMAGE_ENTRY + sizeof(struct flash_header))
#define WHOLE_FIRMWARE_SECTORS (BOOTLOADER_ENTRYPOINT / SECTOR_SIZE)
#define FIRMWARE_OFFSET 544

#if BOOTLOADER_ENTRYPOINT + BOOTLOADER_MAX_SIZE != FLASH_SIZE
#error "Bootloader is not located at the end of flash."
#endif

#if FLASH_ROMIMAGE_ENTRY < FLASH_RAMIMAGE_ENTRY
#error "RAM image must be located before the ROM image."
#endif

#if BOOTLOADER_ENTRYPOINT < FLASH_ROMIMAGE_ENTRY
#error "ROM image must be located before the bootloader."
#endif

#if FLASH_SIZE == 2048 * 1024
#define ROMDUMP "/internal_rom_000000-1FFFFF.bin"
#elif FLASH_SIZE == 4096 * 1024
#define ROMDUMP "/internal_rom_000000-3FFFFF.bin"
#endif

#ifdef IRIVER_H100
#define MODEL (const uint8_t[]) { 'h', '1', '0', '0' }
#define ORIGINAL "/ihp_100.bin"
#elif defined(IRIVER_H120)
#define MODEL (const uint8_t[]) { 'h', '1', '2', '0' }
#define ORIGINAL "/ihp_120.bin"
#elif defined(IRIVER_H300)
#define MODEL (const uint8_t[]) { 'h', '3', '0', '0' }
#define ORIGINAL "/H300.bin"
#else
#error "Unsupported target."
#endif

struct flash_info
{
    uint16_t vendor;
    uint16_t product;
    uint32_t size;
    char name[16];
};

/* checks if the region has a valid bootloader */
static bool detect_valid_bootloader(const void* ptr, uint32_t size)
{
    static const struct
    {
        uint32_t size;
        uint32_t crc32;
    }
    bootloaders[] =
    {
#ifdef IRIVER_H100
        { 53556, 0x76541ebd }, /* 8 */
#elif defined(IRIVER_H120)
        { 53556, 0xd262b12b }, /* 8 */
#elif defined(IRIVER_H300)
        { 57048, 0x59ba2459 }, /* 8 */
#endif
        {0}
    };

    for (size_t i = 0; bootloaders[i].size != 0; i++)
    {
        uint32_t crc32;

        if (size != 0 && size != bootloaders[i].size)
            continue;

        crc32 = rb->crc_32(ptr, bootloaders[i].size, 0xFFFFFFFF);
        if (crc32 == bootloaders[i].crc32)
            return true;
    }

    return false;
}

/* get read-only access to flash at the given offset */
static const void* flash(uint32_t offset)
{
    const uint16_t* FB = (uint16_t*) FLASH_BASE;
    return &FB[offset / WORD_SIZE];
}

/* queries the rom for information and returns it if it is known */
static bool flash_get_info(const struct flash_info** out_info)
{
    static const struct flash_info roms[] =
    {
#if FLASH_SIZE == 2048 * 1024
        { 0x00BF, 0x2782, 2048 * 1024, "SST39VF160"  },
#elif FLASH_SIZE == 4096 * 1024
        { 0x00BF, 0x235B, 4096 * 1024, "SST39VF3201" },
#else
#error "Unsupported rom chip."
#endif
        {0}
    };
    static struct flash_info unknown_rom = {0};
    volatile uint16_t* FB = (uint16_t*) FLASH_BASE;
    uint16_t vendor;
    uint16_t product;

    /* execute the software ID entry command */
    FB[0x5555] = 0xAA;
    FB[0x2AAA] = 0x55;
    FB[0x5555] = 0x90;
    rb->sleep(HZ / 100);

    /* copy the IDs from the previous command */
    vendor = FB[0];
    product = FB[1];

    /* execute the software ID exit command */
    FB[0x5555] = 0xAA;
    FB[0x2AAA] = 0x55;
    FB[0x5555] = 0xF0;
    rb->sleep(HZ / 100);

    /* search for a known match */
    for (size_t i = 0; roms[i].size != 0; i++)
    {
        if (roms[i].vendor == vendor && roms[i].product == product)
        {
            *out_info = &roms[i];
            return true;
        }
    }

    /* return only the vendor / product ids if unknown */
    unknown_rom.vendor = vendor;
    unknown_rom.product = product;
    *out_info = &unknown_rom;
    return false;
}

/* wait until the rom signals completion of an operation */
static bool flash_wait_for_rom(uint32_t offset)
{
    const size_t MAX_TIMEOUT = 0xFFFFFF; /* should be sufficient for most targets */
    const size_t RECOVERY_TIME = 64; /* based on 140MHz MCF 5249 */
    volatile uint16_t* FB = (uint16_t*) FLASH_BASE;
    uint16_t old_data = FB[offset / WORD_SIZE] & 0x0040; /* we only want DQ6 */
    volatile size_t i; /* disables certain optimizations */
    bool result;

    /* repeat up to MAX_TIMEOUT times or until DQ6 stops flipping */
    for (i = 0; i < MAX_TIMEOUT; i++)
    {
        uint16_t new_data = FB[offset / WORD_SIZE] & 0x0040; /* we only want DQ6 */
        if (old_data == new_data)
            break;
        old_data = new_data;
    }

    result = i != MAX_TIMEOUT;

    /* delay at least 1us to give the bus time to recover */
    for (i = 0; i < RECOVERY_TIME; i++);

    return result;
}

/* erase the sector at the given offset */
static bool flash_erase_sector(uint32_t offset)
{
    volatile uint16_t* FB = (uint16_t*) FLASH_BASE;

    /* execute the sector erase command */
    FB[0x5555] = 0xAA;
    FB[0x2AAA] = 0x55;
    FB[0x5555] = 0x80;
    FB[0x5555] = 0xAA;
    FB[0x2AAA] = 0x55;
    FB[offset / WORD_SIZE] = 0x30;

    return flash_wait_for_rom(offset);
}

/* program a word at the given offset */
static bool flash_program_word(uint32_t offset, uint16_t word)
{
    volatile uint16_t* FB = (uint16_t*) FLASH_BASE;

    /* execute the word program command */
    FB[0x5555] = 0xAA;
    FB[0x2AAA] = 0x55;
    FB[0x5555] = 0xA0;
    FB[offset / WORD_SIZE] = word;

    return flash_wait_for_rom(offset);
}

/* bulk erase of adjacent sectors */
static void flash_erase_sectors(uint32_t offset, uint32_t sectors,
                                bool progress)
{
    for (uint32_t i = 0; i < sectors; i++)
    {
        flash_erase_sector(offset + i * SECTOR_SIZE);

        /* display a progress report if requested */
        if (progress)
        {
            rb->lcd_putsf(0, 3, "Erasing... %u%%", (i + 1) * 100 / sectors);
            rb->lcd_update();
        }
    }
}

/* bulk program of bytes */
static void flash_program_bytes(uint32_t offset, const void* ptr,
                                uint32_t len, bool progress)
{
    const uint8_t* data = ptr;

    for (uint32_t i = 0; i < len; i += WORD_SIZE)
    {
        uint32_t j = i + 1;
        uint32_t k = ((j < len) ? j : i) + 1;
        uint16_t word = (data[i] << 8) | (j < len ? data[j] : 0xFF);

        flash_program_word(offset + i, word);

        /* display a progress report if requested */
        if (progress && ((i % SECTOR_SIZE) == 0 || k == len))
        {
            rb->lcd_putsf(0, 4, "Programming... %u%%", k * 100 / len);
            rb->lcd_update();
        }
    }
}

/* bulk verify of programmed bytes */
static bool flash_verify_bytes(uint32_t offset, const void* ptr,
                               uint32_t len, bool progress)
{
    const uint8_t* FB = flash(offset);
    const uint8_t* data = ptr;

    /* don't use memcmp so we can provide progress updates */
    for (uint32_t i = 0; i < len; i++)
    {
        uint32_t j = i + 1;

        if (FB[i] != data[i])
            return false;

        /* display a progress report if requested */
        if (progress && ((i % SECTOR_SIZE) == 0 || j == len))
        {
            rb->lcd_putsf(0, 5, "Verifying... %u%%", j * 100 / len);
            rb->lcd_update();
        }
    }

    return true;
}

/* print information about the flash chip */
static bool show_info(void)
{
    static const struct flash_info* fi = NULL;

    rb->lcd_clear_display();

    if (fi == NULL)
        flash_get_info(&fi);

    rb->lcd_putsf(0, 0, "Flash: V=%04x P=%04x", fi->vendor, fi->product);

    if (fi->size != 0)
    {
        rb->lcd_puts(0, 1, fi->name);
        rb->lcd_putsf(0, 2, "Size: %u KB", fi->size / 1024);
    }
    else
    {
        rb->lcd_puts(0, 1, "Unknown chip");
    }

    rb->lcd_update();

    if (fi->size == 0)
    {
        rb->splash(HZ * 3, "Sorry!");
        return false;
    }

    return true;
}

/* confirm a user's choice */
static bool confirm_choice(const char* msg)
{
    long button;
    rb->splashf(0, "%s ([PLAY] to CONFIRM)", msg);
    do
        button = rb->button_get(true);
    while (IS_SYSEVENT(button) || (button & BUTTON_REL));
    show_info();
    return (button == BUTTON_ON);
}

/* all-in-one firmware loader */
static bool load_firmware(const char* filename, enum firmware firmware,
                          const void** data, size_t* data_len)
{
    bool result = false;
    const char* msg = NULL;
    int fd = -1;
    off_t fd_len;
    uint8_t* buffer;
    size_t buffer_len;

    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
    {
        msg = "Aborting: open failure";
        goto bail;
    }

    /* get file and buffer lengths and acquire the buffer */
    fd_len = rb->filesize(fd);
    buffer = rb->plugin_get_audio_buffer(&buffer_len);

    /* ensure there's enough space in the buffer */
    if ((size_t) fd_len > buffer_len)
    {
        msg = "Aborting: out of memory";
        goto bail;
    }

    /* all known firmwares are less than or equal to FLASH_SIZE */
    if (fd_len > FLASH_SIZE)
    {
        msg = "Aborting: firmware too big";
        goto bail;
    }

    /* rockbox firmware specific code */
    if (firmware == FIRMWARE_ROCKBOX)
    {
        uint32_t checksum;
        uint8_t model[4];

        /* subtract the header length */
        fd_len -= sizeof(checksum) + sizeof(model);

        /* sanity check the length */
        if (fd_len < WORD_SIZE)
        {
            msg = "Aborting: firmware too small";
            goto bail;
        }

        /* read the various parts */
        if (
            rb->read(fd, &checksum, sizeof(checksum)) != sizeof(checksum) ||
            rb->read(fd, model, sizeof(model))        != sizeof(model)    ||
            rb->read(fd, buffer, fd_len)              != fd_len
        )
        {
            msg = "Aborting: read failure";
            goto bail;
        }

        /* verify the checksum */
        if (!verify_checksum(checksum, buffer, fd_len))
        {
            msg = "Aborting: checksum mismatch";
            goto bail;
        }

        /* verify the model */
        if (rb->memcmp(model, MODEL, sizeof(model)) != 0)
        {
            msg = "Aborting: model mismatch";
            goto bail;
        }
    }

    /* romdump specific code */
    if (firmware == FIRMWARE_ROMDUMP)
    {
        /* the romdump should be exactly the same size as the flash */
        if (fd_len != FLASH_SIZE)
        {
            msg = "Aborting: firmware size incorrect";
            goto bail;
        }

        /* exclude boot vector and boot loader regions */
        fd_len = BOOTLOADER_ENTRYPOINT - BOOT_VECTOR_SIZE;

        /* skip the boot vector */
        if (rb->lseek(fd, BOOT_VECTOR_SIZE, SEEK_SET) != BOOT_VECTOR_SIZE)
        {
            msg = "Aborting: lseek failure";
            goto bail;
        }

        /* read everything up to the boot loader */
        if (rb->read(fd, buffer, fd_len) != fd_len)
        {
            msg = "Aborting: read failure";
            goto bail;
        }
    }

    /* original firmware specific code */
    if (firmware == FIRMWARE_ORIGINAL)
    {
        uint32_t boot_vector[2];

        /* subtract the offset and the size of the boot vector */
        fd_len -= FIRMWARE_OFFSET + sizeof(boot_vector);

        /* sanity check the length */
        if (fd_len < WORD_SIZE)
        {
            msg = "Aborting: firmware too small";
            goto bail;
        }

        /* skip the leading bytes, whatever they are */
        if (rb->lseek(fd, FIRMWARE_OFFSET, SEEK_SET) != FIRMWARE_OFFSET)
        {
            msg = "Aborting: lseek failure";
            goto bail;
        }

        /* read the various parts */
        if (
            rb->read(fd, boot_vector, sizeof(boot_vector)) != sizeof(boot_vector) ||
            rb->read(fd, buffer, fd_len)                   != fd_len
        )
        {
            msg = "Aborting: read failure";
            goto bail;
        }

        /* verify the boot vector */
        if (boot_vector[0] != 0x10017ff0 || boot_vector[1] != 0x00000008)
        {
            msg = "Aborting: not an original firmware";
            goto bail;
        }
    }

    /* write the resulting buffer and length in the output parameters */
    *data = buffer;
    *data_len = fd_len;

    /* mark success */
    result = true;

bail: /* common exit code */
    if (fd >= 0)
        rb->close(fd);
    if (msg != NULL)
        rb->splash(HZ * 3, msg);
    return result;
}

/* prints fatal error if a critical failure occurs */
static void show_fatal_error(void)
{
    rb->splash(HZ * 30, "Disable idle poweroff, connect AC power and DON'T TURN PLAYER OFF!");
    rb->splash(HZ * 30, "Contact Rockbox developers as soon as possible!");
    rb->splash(HZ * 30, "Your device won't be bricked unless you turn off the power!");
    rb->splash(HZ * 30, "Don't use the device before further instructions from Rockbox developers!");
}

/* flash a bootloader */
static bool flash_bootloader(const char* filename)
{
    bool result = false;
    const char* msg = NULL;
    bool show_fatal = false;
    const void* data;
    size_t data_len;
    static uint8_t boot_sector[SECTOR_SIZE];

    /* load the firmware */
    if (!load_firmware(filename, FIRMWARE_ROCKBOX, &data, &data_len))
        goto bail;

    /* the bootloader can only be so big */
    if (data_len > BOOTLOADER_MAX_SIZE)
    {
        msg = "Aborting: bootloader too large";
        goto bail;
    }

    /* only support known bootloaders */
    if (!detect_valid_bootloader(data, data_len))
    {
        msg = "Aborting: bootloader is invalid";
        goto bail;
    }

    /* ask before doing anything dangerous */
    if (!confirm_choice("Update bootloader?"))
        goto bail;

    /* copy the original boot sector */
    rb->memcpy(boot_sector, flash(BOOT_SECTOR_OFFSET), SECTOR_SIZE);

    /* update the boot vector */
    rb->memcpy(boot_sector, data, BOOT_VECTOR_SIZE);

    /* erase the boot sector */
    flash_erase_sector(BOOT_SECTOR_OFFSET);

    /* erase the bootloader sectors */
    flash_erase_sectors(BOOTLOADER_ENTRYPOINT, BOOTLOADER_SECTORS, false);

    /* program the new boot sector */
    flash_program_bytes(BOOT_SECTOR_OFFSET, boot_sector, SECTOR_SIZE, false);

    /* program the new bootloader */
    flash_program_bytes(BOOTLOADER_ENTRYPOINT, data, data_len, false);

    /* verify the new boot sector */
    if (!flash_verify_bytes(BOOT_SECTOR_OFFSET, boot_sector, SECTOR_SIZE, false))
    {
        msg = "Boot sector corrupt!";
        show_fatal = true;
        goto bail;
    }

    /* verify the new bootloader */
    if (!flash_verify_bytes(BOOTLOADER_ENTRYPOINT, data, data_len, false))
    {
        msg = "Verify failed!";
        show_fatal = true;
        goto bail;
    }

    /* report success */
    rb->splash(HZ * 3, "Success!");

    /* mark success */
    result = true;

bail: /* common exit code */
    if (msg != NULL)
        rb->splash(HZ * 3, msg);
    if (show_fatal)
        show_fatal_error();
    return result;
}

/* flash a rockbox ram / rom image */
static bool flash_rockbox(const char* filename, uint32_t offset)
{
    bool result = false;
    const char* msg = NULL;
    const void* data;
    size_t data_len;
    struct flash_header header;

    /* load the firmware */
    if (!load_firmware(filename, FIRMWARE_ROCKBOX, &data, &data_len))
        goto bail;

    /* sanity check that the offset was set correctly */
    if (offset != FLASH_RAMIMAGE_ENTRY && offset != FLASH_ROMIMAGE_ENTRY)
    {
        msg = "Aborting: invalid image offset";
        goto bail;
    }

    /* ensure there's enough room for the ram / rom image */
    if (
        (offset == FLASH_RAMIMAGE_ENTRY && data_len > RAM_IMAGE_MAX_SIZE) ||
        (offset == FLASH_ROMIMAGE_ENTRY && data_len > ROM_IMAGE_MAX_SIZE)
    )
    {
        msg = "Aborting: ram / rom image too large";
        goto bail;
    }

    /* check for bootloader that can load rockbox from ram / rom */
    if (!detect_valid_bootloader(flash(BOOTLOADER_ENTRYPOINT), 0))
    {
        msg = "Aborting: incompatible bootloader";
        goto bail;
    }

    /* rom image specific checks */
    if (offset == FLASH_ROMIMAGE_ENTRY)
    {
        uint32_t relocation = *((const uint32_t*) data);

        /* sanity check of the image relocation */
        if (relocation != ROM_IMAGE_RELOCATION)
        {
            msg = "Aborting: invalid image relocation";
            goto bail;
        }
    }

    /* ask before doing anything dangerous */
    if (!rb->detect_original_firmware())
    {
        if (!confirm_choice("Update Rockbox flash image?"))
            goto bail;
    }
    else
    {
        if (!confirm_choice("Erase original firmware?"))
            goto bail;
    }

    /* erase all ram / rom image sectors */
    if (offset == FLASH_RAMIMAGE_ENTRY)
        flash_erase_sectors(offset, RAM_IMAGE_SECTORS, true);
    else if (offset == FLASH_ROMIMAGE_ENTRY)
        flash_erase_sectors(offset, ROM_IMAGE_SECTORS, true);

    /* prepare the header */
    header.magic = FLASH_MAGIC;
    header.length = data_len;
    rb->memset(&header.version, 0x00, sizeof(header.version));

    /* program the header */
    flash_program_bytes(offset, &header, sizeof(header), false);

    /* program the ram / rom image */
    flash_program_bytes(offset + sizeof(header), data, data_len, true);

    /* verify the header and ram / rom image */
    if (
        !flash_verify_bytes(offset, &header, sizeof(header), false)        ||
        !flash_verify_bytes(offset + sizeof(header), data, data_len, true)
    )
    {
        msg = "Verify failed!";
        /*
         * erase the ram / rom image header to prevent the bootloader
         * from trying to boot from it
         */
        flash_erase_sector(offset);
        goto bail;
    }

    /* report success */
    rb->splash(HZ * 3, "Success!");

    /* mark success */
    result = true;

bail: /* common exit code */
    if (msg != NULL)
        rb->splash(HZ * 3, msg);
    return result;
}

/* flash whole firmware; common code for romdump / original */
static bool flash_whole_firmware(const void* data, size_t data_len)
{
    bool result = false;
    const char* msg = NULL;
    bool show_fatal = false;
    uint8_t boot_vector[BOOT_VECTOR_SIZE];

    /* copy the original boot vector */
    rb->memcpy(boot_vector, flash(BOOT_SECTOR_OFFSET), BOOT_VECTOR_SIZE);

    /* erase everything except the bootloader */
    flash_erase_sectors(BOOT_SECTOR_OFFSET, WHOLE_FIRMWARE_SECTORS, true);

    /* program the original boot vector */
    flash_program_bytes(BOOT_SECTOR_OFFSET, boot_vector, BOOT_VECTOR_SIZE, false);

    /* program the whole firmware */
    flash_program_bytes(BOOT_SECTOR_OFFSET + BOOT_VECTOR_SIZE, data, data_len, true);

    /* verify the new boot vector */
    if (!flash_verify_bytes(BOOT_SECTOR_OFFSET, boot_vector, BOOT_VECTOR_SIZE, false))
    {
        msg = "Boot vector corrupt!";
        show_fatal = true;
        goto bail;
    }

    /* verify the new firmware */
    if (!flash_verify_bytes(BOOT_SECTOR_OFFSET + BOOT_VECTOR_SIZE, data, data_len, true))
    {
        msg = "Verify failed!";
        goto bail;
    }

    /* report success */
    rb->splash(HZ * 3, "Success!");

    /* mark success */
    result = true;

bail: /* common exit code */
    if (msg != NULL)
        rb->splash(HZ * 3, msg);
    if (show_fatal)
        show_fatal_error();
    return result;
}

/* flash rom dumps */
static bool flash_romdump(const char* filename)
{
    const void* data;
    size_t data_len;

    /* load the firmware */
    if (!load_firmware(filename, FIRMWARE_ROMDUMP, &data, &data_len))
        return false;

    /* ask before doing anything dangerous */
    if (!confirm_choice("Restore firmware section (bootloader will be kept)?"))
        return false;

    return flash_whole_firmware(data, data_len);
}

/* flash original firmware */
static bool flash_original(const char* filename)
{
    const void* data;
    size_t data_len;

    /* load the firmware */
    if (!load_firmware(filename, FIRMWARE_ORIGINAL, &data, &data_len))
        return false;

    /* ask before doing anything dangerous */
    if (!confirm_choice("Restore original firmware (bootloader will be kept)?"))
        return false;

    return flash_whole_firmware(data, data_len);
}

/* main function of plugin */
static void iriver_flash(const char* filename)
{
    /* refuse to run from ROM */
    const uint8_t* RB = (uint8_t*) rb;
    const uint8_t* FB = (uint8_t*) flash(0);
    if (RB >= FB && RB < FB + FLASH_SIZE)
    {
        rb->splash(HZ * 3, "Refusing to run from ROM");
        return;
    }

    /* refuse to run with low battery */
    if (!rb->battery_level_safe())
    {
        rb->splash(HZ * 3, "Refusing to run with low battery");
        return;
    }

    /* print information about flash; exit if not supported */
    if (!show_info())
        return;

    /* exit if no filename was provided */
    if (filename == NULL)
    {
        rb->splash(HZ * 3, "Please use this plugin with \"Open with...\"");
        return;
    }

    /* choose what to do with the file */
    if (rb->strcasestr(filename, "/bootloader.iriver") != NULL)
        flash_bootloader(filename);
    else if (rb->strcasestr(filename, "/rockbox.iriver") != NULL)
        flash_rockbox(filename, FLASH_RAMIMAGE_ENTRY);
    else if (rb->strcasestr(filename, "/rombox.iriver") != NULL)
        flash_rockbox(filename, FLASH_ROMIMAGE_ENTRY);
    else if (rb->strcasestr(filename, ROMDUMP) != NULL)
        flash_romdump(filename);
    else if (rb->strcasestr(filename, ORIGINAL) != NULL)
        flash_original(filename);
    else
        rb->splash(HZ * 3, "Unknown file type");
}

/* plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    /* need to disable memguard to write to flash */
    int mode = rb->system_memory_guard(MEMGUARD_NONE);

    /* setup LCD font */
    rb->lcd_setfont(FONT_SYSFIXED);

    /* don't let the backlight turn off or it might scare people */
    backlight_ignore_timeout();

    /* run the main entry function */
    iriver_flash(parameter);

    /* restore the original backlight settings */
    backlight_use_settings();

    /* restore LCD font */
    rb->lcd_setfont(FONT_UI);

    /* restore original memory guard setting */
    rb->system_memory_guard(mode);

    return PLUGIN_OK;
}
