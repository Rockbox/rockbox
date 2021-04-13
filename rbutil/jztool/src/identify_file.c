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
#include <string.h>

/* Following is copied from mkspl-x1000, basically */
struct x1000_spl_header {
    uint8_t magic[8];
    uint8_t type;
    uint8_t crc7;
    uint8_t ppb;
    uint8_t bpp;
    uint32_t length;
};

static const uint8_t x1000_spl_header_magic[8] =
    {0x06, 0x05, 0x04, 0x03, 0x02, 0x55, 0xaa, 0x55};

static const size_t X1000_SPL_HEADER_SIZE = 2 * 1024;

static uint8_t crc7(const uint8_t* buf, size_t len)
{
    /* table-based computation of CRC7 */
    static const uint8_t t[256] = {
        0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
        0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
        0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
        0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
        0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
        0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
        0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
        0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
        0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
        0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
        0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
        0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
        0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
        0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
        0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
        0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
        0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
        0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
        0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
        0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
        0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
        0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
        0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
        0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
        0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
        0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
        0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
        0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
        0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
        0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
        0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
        0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
    };

    uint8_t crc = 0;
    while(len--)
        crc = t[(crc << 1) ^ *buf++];
    return crc;
}

int jz_identify_x1000_spl(const void* data, size_t len)
{
    /* Use <= check because a header-only file is not really valid,
     * it should have at least one byte in it... */
    if(len <= X1000_SPL_HEADER_SIZE)
        return JZ_IDERR_WRONG_SIZE;

    /* Look for header magic bytes */
    const struct x1000_spl_header* header = (const struct x1000_spl_header*)data;
    if(memcmp(header->magic, x1000_spl_header_magic, 8))
        return JZ_IDERR_BAD_HEADER;

    /* Length stored in the header should equal the length of the file */
    if(header->length != len)
        return JZ_IDERR_WRONG_SIZE;

    /* Compute the CRC7 checksum; it only covers the SPL code */
    const uint8_t* dat = (const uint8_t*)data;
    uint8_t sum = crc7(&dat[X1000_SPL_HEADER_SIZE], len - X1000_SPL_HEADER_SIZE);
    if(header->crc7 != sum)
        return JZ_IDERR_BAD_CHECKSUM;

    return JZ_SUCCESS;

}

static const struct scramble_model_info {
    const char* name;
    int model_num;
} scramble_models[] = {
    {"fiio", 114},
    {NULL, 0},
};

int jz_identify_scramble_image(const void* data, size_t len)
{
    /* 4 bytes checksum + 4 bytes player model */
    if(len < 8)
        return JZ_IDERR_WRONG_SIZE;

    /* Look up the model number */
    const uint8_t* dat = (const uint8_t*)data;
    const struct scramble_model_info* model_info = &scramble_models[0];
    for(; model_info->name != NULL; ++model_info)
        if(!memcmp(&dat[4], model_info->name, 4))
            break;

    if(model_info->name == NULL)
        return JZ_IDERR_UNRECOGNIZED_MODEL;

    /* Compute the checksum */
    uint32_t sum = model_info->model_num;
    for(size_t i = 8; i < len; ++i)
        sum += dat[i];

    /* Compare with file's checksum, it's stored in big-endian form */
    uint32_t fsum = (dat[0] << 24) | (dat[1] << 16) | (dat[2] << 8) | dat[3];
    if(sum != fsum)
        return JZ_IDERR_BAD_CHECKSUM;

    return JZ_SUCCESS;
}

int jz_identify_fiiom3k_bootimage(const void* data, size_t len)
{
    /* The bootloader image is simply a dump of the first NAND eraseblock,
     * so it has a fixed 128 KiB size */
    if(len != 128*1024)
        return JZ_IDERR_WRONG_SIZE;

    /* We'll verify the embedded SPL, but we have to drag out the correct
     * length from the header. Length should be more than 12 KiB, due to
     * limitations of the hardware */
    const struct x1000_spl_header* spl_header;
    spl_header = (const struct x1000_spl_header*)data;
    if(spl_header->length > 12 * 1024)
        return JZ_IDERR_BAD_HEADER;

    int rc = jz_identify_x1000_spl(data, spl_header->length);
    if(rc < 0)
        return rc;

    const uint8_t* dat = (const uint8_t*)data;

    /* Check the partition table is present */
    if(memcmp(&dat[0x3c00], "nand", 4))
        return JZ_IDERR_OTHER;

    /* Check first bytes of PDMA firmware. It doesn't change
     * between OF versions, and Rockbox doesn't modify it. */
    static const uint8_t pdma_fw[] = {0x54, 0x25, 0x42, 0xb3, 0x70, 0x25, 0x42, 0xb3};
    if(memcmp(&dat[0x4000], pdma_fw, sizeof(pdma_fw)))
       return JZ_IDERR_OTHER;

    return JZ_SUCCESS;
}
