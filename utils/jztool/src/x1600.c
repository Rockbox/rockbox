/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 * Derived from src/x1000.c, Copyright (C) 2021 Aidan MacDonald
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

/* X1600 USB recovery uses a separate returning stage1 to initialise DRAM.
 * The flash SPL has a different link address and is not used by this path. */

#include "jztool.h"
#include "jztool_private.h"
#include "microtar-stdio.h"
#include <stdbool.h>
#include <string.h>

/* Result block written by usbstage1-x1600.c before returning to BootROM. */
#define X1600_STAGE1_RESULT_ADDR 0x8000f000
#define X1600_STAGE1_MAGIC       0xc0def00d

static uint32_t get_le32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/** \brief Load the Rockbox bootloader on an X1600 device
 * \param dev       USB device freshly returned by jz_usb_open()
 * \param type      Device type, used to pick the file extension
 * \param filename  Path to the "bootloader.<ext>" update package
 * \return either JZ_SUCCESS or an error code
 */
int jz_x1600_boot(jz_usbdev* dev, jz_device_type type, const char* filename)
{
    const jz_device_info* dev_info;
    const jz_cpu_info* cpu_info;
    char stage1_filename[32];
    uint8_t stage1_result[8];
    jz_buffer* stage1 = NULL, *bootloader = NULL, *info_file = NULL;
    mtar_t tar;
    int rc;

    dev_info = jz_get_device_info(type);
    if(!dev_info)
        return JZ_ERR_OTHER;

    cpu_info = jz_get_cpu_info(dev_info->cpu_type);
    if(!cpu_info)
        return JZ_ERR_OTHER;

    /* Load the raw USB stage1, without the flash SPL header. */
    sprintf(stage1_filename, "usbstage1.%s", dev_info->file_ext);

    rc = mtar_open(&tar, filename, "rb");
    if(rc != MTAR_ESUCCESS) {
        jz_log(dev->jz, JZ_LOG_ERROR, "cannot open file %s (tar error: %d)",
               filename, rc);
        return JZ_ERR_OPEN_FILE;
    }

    rc = jz_boot_get_file(dev->jz, &tar, stage1_filename, 0, &stage1);
    if(rc != JZ_SUCCESS) {
        jz_log(dev->jz, JZ_LOG_ERROR,
               "This update package has no %s member, so it cannot be USB "
               "booted. The X1600 needs a stage1 payload linked at 0x%08lx, "
               "separate from the flash SPL.",
               stage1_filename, (unsigned long)cpu_info->stage1_load_addr);
        goto error;
    }

    if(stage1->size > 20 * 1024) {
        jz_log(dev->jz, JZ_LOG_ERROR,
               "stage1 is %zu bytes; the BootROM's limit is 20 KiB (PM 34.9)",
               stage1->size);
        rc = JZ_ERR_BAD_FILE_FORMAT;
        goto error;
    }

    /* The second-stage image is linked at X1600_BOOT_LOAD_ADDR. */
    rc = jz_boot_get_file(dev->jz, &tar, "bootloader2.ucl", JZ_BOOT_DECOMPRESS, &bootloader);
    if(rc != JZ_SUCCESS)
        goto error;

    rc = jz_boot_get_file(dev->jz, &tar, "bootloader-info.txt", 0, &info_file);
    if(rc != JZ_SUCCESS)
        goto error;

    rc = jz_boot_show_version(dev->jz, info_file);
    if(rc != JZ_SUCCESS)
        goto error;

    /* Flush before upload so a previous stage1 is not executed from cache.
     * Stage1 initialises DRAM and returns to the BootROM. */
    rc = jz_usb_flush_caches(dev);
    if(rc != JZ_SUCCESS)
        goto error;

    rc = jz_usb_send(dev, cpu_info->stage1_load_addr, stage1->size, stage1->data);
    if(rc != JZ_SUCCESS)
        goto error;

    rc = jz_usb_start1(dev, cpu_info->stage1_exec_addr);
    if(rc != JZ_SUCCESS)
        goto error;

    /* Give stage1 time to finish DRAM training and return. */
    jz_sleepms(500);

    rc = jz_usb_recv(dev, X1600_STAGE1_RESULT_ADDR,
                     sizeof(stage1_result), stage1_result);
    if(rc != JZ_SUCCESS)
        goto error;

    if(get_le32(stage1_result) != X1600_STAGE1_MAGIC) {
        jz_log(dev->jz, JZ_LOG_ERROR, "Invalid stage1 result");
        rc = JZ_ERR_OTHER;
        goto error;
    }

    uint32_t status = get_le32(stage1_result + 4);
    if(status != 0) {
        jz_log(dev->jz, JZ_LOG_ERROR,
               "Stage1 DDR initialization failed (status 0x%08lx)",
               (unsigned long)status);
        rc = JZ_ERR_OTHER;
        goto error;
    }

    /* Flush before uploading and executing the bootloader. */
    rc = jz_usb_flush_caches(dev);
    if(rc != JZ_SUCCESS)
        goto error;

    rc = jz_usb_send(dev, cpu_info->stage2_load_addr,
                     bootloader->size, bootloader->data);
    if(rc != JZ_SUCCESS)
        goto error;

    rc = jz_usb_flush_caches(dev);
    if(rc != JZ_SUCCESS)
        goto error;

    rc = jz_usb_start2(dev, cpu_info->stage2_exec_addr);
    if(rc != JZ_SUCCESS)
        goto error;

    rc = JZ_SUCCESS;

  error:
    if(stage1)
        jz_buffer_free(stage1);
    if(bootloader)
        jz_buffer_free(bootloader);
    if(info_file)
        jz_buffer_free(info_file);
    mtar_close(&tar);
    return rc;
}
