/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

/* TODO: these functions could be refactored to be CPU-agnostic */
static int run_stage1(jz_usbdev* dev, jz_buffer* buf)
{
    int rc = jz_usb_send(dev, 0xf4001000, buf->size, buf->data);
    if(rc < 0)
        return rc;

    return jz_usb_start1(dev, 0xf4001800);
}

static int run_stage2(jz_usbdev* dev, jz_buffer* buf)
{
    int rc = jz_usb_send(dev, 0x80004000, buf->size, buf->data);
    if(rc < 0)
        return rc;

    rc = jz_usb_flush_caches(dev);
    if(rc < 0)
        return rc;

    return jz_usb_start2(dev, 0x80004000);
}

static int get_file(jz_context* jz, mtar_t* tar, const char* file,
                    bool decompress, jz_buffer** buf)
{
    jz_buffer* buffer = NULL;
    const mtar_header_t* h;
    int rc;

    rc = mtar_find(tar, file);
    if(rc != MTAR_ESUCCESS) {
        jz_log(jz, JZ_LOG_ERROR, "can't find %s in boot file, tar error %d", file, rc);
        return JZ_ERR_BAD_FILE_FORMAT;
    }

    h = mtar_get_header(tar);
    buffer = jz_buffer_alloc(h->size, NULL);
    if(!buffer)
        return JZ_ERR_OUT_OF_MEMORY;

    rc = mtar_read_data(tar, buffer->data, buffer->size);
    if(rc < 0 || (unsigned)rc != buffer->size) {
        jz_buffer_free(buffer);
        jz_log(jz, JZ_LOG_ERROR, "can't read %s in boot file, tar error %d", file, rc);
        return JZ_ERR_BAD_FILE_FORMAT;
    }

    if(decompress) {
        uint32_t dst_len;
        jz_buffer* nbuf = jz_ucl_unpack(buffer->data, buffer->size, &dst_len);
        jz_buffer_free(buffer);
        if(!nbuf) {
            jz_log(jz, JZ_LOG_ERROR, "error decompressing %s in boot file", file);
            return JZ_ERR_BAD_FILE_FORMAT;
        }

        /* for simplicity just forget original size of buffer */
        nbuf->size = dst_len;
        buffer = nbuf;
    }

    *buf = buffer;
    return JZ_SUCCESS;
}

static int show_version(jz_context* jz, jz_buffer* info_file)
{
    /* Extract the version string and log it for informational purposes */
    char* boot_version = (char*)info_file->data;
    char* endpos = memchr(boot_version, '\n', info_file->size);
    if(!endpos) {
        jz_log(jz, JZ_LOG_ERROR, "invalid metadata in boot file");
        return JZ_ERR_BAD_FILE_FORMAT;
    }

    *endpos = 0;
    jz_log(jz, JZ_LOG_NOTICE, "Rockbox bootloader version: %s", boot_version);
    return JZ_SUCCESS;
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
    rc = get_file(dev->jz, &tar, spl_filename, false, &spl);
    if(rc != JZ_SUCCESS)
        goto error;

    rc = get_file(dev->jz, &tar, "bootloader.ucl", true, &bootloader);
    if(rc != JZ_SUCCESS)
        goto error;

    rc = get_file(dev->jz, &tar, "bootloader-info.txt", false, &info_file);
    if(rc != JZ_SUCCESS)
        goto error;

    /* Display the version string */
    rc = show_version(dev->jz, info_file);
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
