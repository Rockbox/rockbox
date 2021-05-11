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
#include "microtar.h"
#include "ucl/ucl.h"
#include <stdbool.h>
#include <string.h>

static uint32_t xread32(const uint8_t* d)
{
    uint32_t r = 0;
    r |= d[0] << 24;
    r |= d[1] << 16;
    r |= d[2] <<  8;
    r |= d[3] <<  0;
    return r;
}

/* adapted from firmware/common/ucl_decompress.c */
static jz_buffer* ucl_unpack(const uint8_t* src, uint32_t src_len,
                             uint32_t* dst_len)
{
    static const uint8_t magic[8] =
        {0x00, 0xe9, 0x55, 0x43, 0x4c, 0xff, 0x01, 0x1a};

    jz_buffer* buffer = NULL;

    /* make sure there are enough bytes for the header */
    if(src_len < 18)
        goto error;

    /* avoid memcmp for reasons of code size */
    for(size_t i = 0; i < sizeof(magic); ++i)
        if(src[i] != magic[i])
            goto error;

    /* read the other header fields */
    /* uint32_t flags = xread32(&src[8]); */
    uint8_t method = src[12];
    /* uint8_t level = src[13]; */
    uint32_t block_size = xread32(&src[14]);

    /* check supported compression method */
    if(method != 0x2e)
        goto error;

    /* validate */
    if(block_size < 1024 || block_size > 8*1024*1024)
        goto error;

    src += 18;
    src_len -= 18;

    /* Calculate amount of space that we might need & allocate a buffer:
     * - subtract 4 to account for end of file marker
     * - each block is block_size bytes + 8 bytes of header
     * - add one to nr_blocks to account for case where file size < block size
     * - total size =  max uncompressed size of block * nr_blocks
     */
    uint32_t nr_blocks = (src_len - 4) / (8 + block_size) + 1;
    uint32_t max_size = nr_blocks * (block_size + block_size/8 + 256);
    buffer = jz_buffer_alloc(max_size, NULL);
    if(!buffer)
        goto error;

    /* perform the decompression */
    uint32_t dst_ilen = buffer->size;
    uint8_t* dst = buffer->data;
    while(1) {
        if(src_len < 4)
            goto error;

        uint32_t out_len = xread32(src); src += 4, src_len -= 4;
        if(out_len == 0)
            break;

        if(src_len < 4)
            goto error;

        uint32_t in_len = xread32(src); src += 4, src_len -= 4;
        if(in_len > block_size || out_len > block_size ||
           in_len == 0 || in_len > out_len)
            goto error;

        if(src_len < in_len)
            goto error;

        if(in_len < out_len) {
            uint32_t actual_out_len = dst_ilen;
            int rc = ucl_nrv2e_decompress_safe_8(src, in_len, dst, &actual_out_len, NULL);
            if(rc != UCL_E_OK)
                goto error;
            if(actual_out_len != out_len)
                goto error;
        } else {
            for(size_t i = 0; i < in_len; ++i)
                dst[i] = src[i];
        }

        src += in_len;
        src_len -= in_len;
        dst += out_len;
        dst_ilen -= out_len;
    }

    /* subtract leftover number of bytes to get size of compressed output */
    *dst_len = buffer->size - dst_ilen;
    return buffer;

  error:
    jz_buffer_free(buffer);
    return NULL;
}

static int m3k_stage1(jz_usbdev* dev, jz_buffer* buf)
{
    int rc = jz_usb_send(dev, 0xf4001000, buf->size, buf->data);
    if(rc < 0)
        return rc;

    return jz_usb_start1(dev, 0xf4001800);
}

static int m3k_stage2(jz_usbdev* dev, jz_buffer* buf)
{
    int rc = jz_usb_send(dev, 0x80004000, buf->size, buf->data);
    if(rc < 0)
        return rc;

    rc = jz_usb_flush_caches(dev);
    if(rc < 0)
        return rc;

    return jz_usb_start2(dev, 0x80004000);
}

static int m3k_get_file(jz_context* jz, mtar_t* tar, const char* file,
                        bool decompress, jz_buffer** buf)
{
    jz_buffer* buffer = NULL;
    mtar_header_t h;
    int rc;

    rc = mtar_find(tar, file, &h);
    if(rc != MTAR_ESUCCESS) {
        jz_log(jz, JZ_LOG_ERROR, "can't find %s in boot file, tar error %d", file, rc);
        return JZ_ERR_BAD_FILE_FORMAT;
    }

    buffer = jz_buffer_alloc(h.size, NULL);
    if(!buffer)
        return JZ_ERR_OUT_OF_MEMORY;

    rc = mtar_read_data(tar, buffer->data, buffer->size);
    if(rc != MTAR_ESUCCESS) {
        jz_buffer_free(buffer);
        jz_log(jz, JZ_LOG_ERROR, "can't read %s in boot file, tar error %d", file, rc);
        return JZ_ERR_BAD_FILE_FORMAT;
    }

    if(decompress) {
        uint32_t dst_len;
        jz_buffer* nbuf = ucl_unpack(buffer->data, buffer->size, &dst_len);
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

static int m3k_show_version(jz_context* jz, jz_buffer* info_file)
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

/** \brief Load the Rockbox bootloader on the FiiO M3K
 * \param dev       USB device freshly returned by jz_usb_open()
 * \param filename  Path to the "bootloader.m3k" file
 * \return either JZ_SUCCESS or an error code
 */
int jz_fiiom3k_boot(jz_usbdev* dev, const char* filename)
{
    jz_buffer* spl = NULL, *bootloader = NULL, *info_file = NULL;
    mtar_t tar;
    int rc;

    rc = mtar_open(&tar, filename, "r");
    if(rc != MTAR_ESUCCESS) {
        jz_log(dev->jz, JZ_LOG_ERROR, "cannot open file %s (tar error: %d)", filename, rc);
        return JZ_ERR_OPEN_FILE;
    }

    /* Extract all necessary files */
    rc = m3k_get_file(dev->jz, &tar, "spl.m3k", false, &spl);
    if(rc != JZ_SUCCESS)
        goto error;

    rc = m3k_get_file(dev->jz, &tar, "bootloader.ucl", true, &bootloader);
    if(rc != JZ_SUCCESS)
        goto error;

    rc = m3k_get_file(dev->jz, &tar, "bootloader-info.txt", false, &info_file);
    if(rc != JZ_SUCCESS)
        goto error;

    /* Display the version string */
    rc = m3k_show_version(dev->jz, info_file);
    if(rc != JZ_SUCCESS)
        goto error;

    /* Stage1 boot of SPL to set up hardware */
    rc = m3k_stage1(dev, spl);
    if(rc != JZ_SUCCESS)
        goto error;

    /* Need a bit of time for SPL to handle init */
    jz_sleepms(500);

    /* Stage2 boot into the bootloader's recovery menu
     * User has to take manual action from there */
    rc = m3k_stage2(dev, bootloader);
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
