/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "jztool.h"
#include "jztool_private.h"
#include "microtar-stdio.h"
#include <stdbool.h>
#include <string.h>

#define X1000_TCSM_BASE         0xf4000000

#define X1000_SPL_LOAD_ADDR     (X1000_TCSM_BASE + 0x1000)
#define X1000_SPL_EXEC_ADDR     (X1000_TCSM_BASE + 0x1800)

#define X1000_STANDARD_DRAM_BASE 0x80004000

#define HDR_BEGIN   128     /* header must begin within this many bytes */
#define HDR_LEN     256     /* header length cannot exceed this */

#define MIN(a,b) ((a) < (b) ? (a) : (b))

/* search for header value, label must be a 4-character string.
 * Returns the found value or 0 if the label wasn't found. */
static uint32_t search_header(const unsigned char* source, size_t length,
                              const char* label)
{
    size_t search_len = MIN(length, HDR_BEGIN);
    if(search_len < 8)
        return 0;
    search_len -= 7;

    /* find the beginning marker */
    size_t i;
    for(i = 8; i < search_len; i += 4)
        if(!memcmp(&source[i], "BEGINHDR", 8))
            break;
    if(i >= search_len)
        return 0;
    i += 8;

    /* search within the header */
    search_len = MIN(length, i + HDR_LEN) - 7;
    for(; i < search_len; i += 8) {
        if(!memcmp(&source[i], "ENDH", 4)) {
            break;
        } else if(!memcmp(&source[i], label, 4)) {
            i += 4;
            /* read little-endian value */
            uint32_t ret = source[i];
            ret |= source[i+1] << 8;
            ret |= source[i+2] << 16;
            ret |= source[i+3] << 24;
            return ret;
        }
    }

    return 0;
}

static int run_stage1(jz_usbdev* dev, jz_buffer* buf)
{
    int rc = jz_usb_send(dev, X1000_SPL_LOAD_ADDR, buf->size, buf->data);
    if(rc < 0)
        return rc;

    return jz_usb_start1(dev, X1000_SPL_EXEC_ADDR);
}

static int run_stage2(jz_usbdev* dev, jz_buffer* buf)
{
    uint32_t load_addr = search_header(buf->data, buf->size, "LOAD");
    if(!load_addr)
        load_addr = X1000_STANDARD_DRAM_BASE;

    int rc = jz_usb_send(dev, load_addr, buf->size, buf->data);
    if(rc < 0)
        return rc;

    rc = jz_usb_flush_caches(dev);
    if(rc < 0)
        return rc;

    return jz_usb_start2(dev, load_addr);
}

/** \brief Load the Rockbox bootloader on an X1000 device
 * \param dev       USB device freshly returned by jz_usb_open()
 * \param filename  Path to the "bootloader.target" file
 * \return either JZ_SUCCESS or an error code
 */
int jz_x1000_boot(jz_usbdev* dev, jz_device_type type, const char* filename)
{
    const jz_device_info* dev_info;
    char spl_filename[32];
    jz_buffer* spl = NULL, *bootloader = NULL, *info_file = NULL;
    mtar_t tar;
    int rc;

    /* In retrospect using a model-dependent archive format was not a good
     * idea, but it's not worth fixing just yet... */
    dev_info = jz_get_device_info(type);
    if(!dev_info)
        return JZ_ERR_OTHER;
    /* use of sprintf is safe since file_ext is short */
    sprintf(spl_filename, "spl.%s", dev_info->file_ext);

    /* Now open the archive */
    rc = mtar_open(&tar, filename, "rb");
    if(rc != MTAR_ESUCCESS) {
        jz_log(dev->jz, JZ_LOG_ERROR, "cannot open file %s (tar error: %d)", filename, rc);
        return JZ_ERR_OPEN_FILE;
    }

    /* Extract all necessary files */
    rc = jz_boot_get_file(dev->jz, &tar, spl_filename, false, &spl);
    if(rc != JZ_SUCCESS)
        goto error;

    /* - A bootloader2.ucl binary should carry the LOAD header to define its
     *   load address. This name must be used when the load address is not
     *   equal to 0x80004000 to ensure old jztools will not try to load it.
     *
     * - The bootloader.ucl name must only be used when the binary loads at
     *   0x80004000 and can be booted by old versions of jztool.
     */
    const char* bl_files[2] = {"bootloader2.ucl", "bootloader.ucl"};
    for(int i = 0; i < 2; ++i) {
        rc = jz_boot_get_file(dev->jz, &tar, bl_files[i],
                      JZ_BOOT_DECOMPRESS|JZ_BOOT_OPTIONAL, &bootloader);
        if(rc == JZ_SUCCESS)
            break;
        else if(rc != JZ_ERR_OPEN_FILE)
            goto error;
    }

    if(rc != JZ_SUCCESS) {
        jz_log(dev->jz, JZ_LOG_ERROR, "no bootloader binary found", filename);
        goto error;
    }

    rc = jz_boot_get_file(dev->jz, &tar, "bootloader-info.txt", false, &info_file);
    if(rc != JZ_SUCCESS)
        goto error;

    /* Display the version string */
    rc = jz_boot_show_version(dev->jz, info_file);
    if(rc != JZ_SUCCESS)
        goto error;

    /* Stage1 boot of SPL to set up hardware */
    rc = run_stage1(dev, spl);
    if(rc != JZ_SUCCESS)
        goto error;

    /* Need a bit of time for SPL to handle init */
    jz_sleepms(500);

    /* Stage2 boot into the bootloader's recovery menu
     * User has to take manual action from there */
    rc = run_stage2(dev, bootloader);
    if(rc != JZ_SUCCESS)
        goto error;

    rc = JZ_SUCCESS;

  error:
    if(spl)
        jz_buffer_free(spl);
    if(bootloader)
        jz_buffer_free(bootloader);
    if(info_file)
        jz_buffer_free(info_file);
    mtar_close(&tar);
    return rc;
}
