/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Amaury Pouly
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
#include "samsung.h"
#include <openssl/md5.h>
#include <string.h>
#include <stdlib.h>

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

static uint8_t g_yp_key[128] =
{
    0xa3, 0x04, 0xb9, 0xcd, 0x34, 0x13, 0x4a, 0x19, 0x19, 0x35, 0xdf, 0xbb, 0x8f, 0x3d, 0x7f, 0x09,
    0x42, 0x3c, 0x96, 0xc6, 0x41, 0xa9, 0x95, 0xf1, 0xd0, 0xac, 0x16, 0x37, 0x57, 0x1f, 0x28, 0xe7,
    0x0b, 0xc2, 0x12, 0x09, 0x39, 0x42, 0xd2, 0x96, 0xf5, 0x00, 0xd2, 0x23, 0xa4, 0x24, 0xe2, 0x8e,
    0x50, 0x3c, 0x6e, 0x23, 0xeb, 0x68, 0xed, 0x31, 0xb7, 0xee, 0xc0, 0xc7, 0x09, 0xf8, 0x0e, 0x9d,
    0x51, 0xed, 0x17, 0x95, 0x64, 0x09, 0xe0, 0xf9, 0xf0, 0xef, 0x86, 0xc0, 0x04, 0x46, 0x89, 0x8a,
    0x6e, 0x27, 0x69, 0xde, 0xc7, 0x9d, 0x1e, 0xee, 0x3c, 0x3f, 0x17, 0x05, 0x44, 0xbb, 0xbb, 0x1d,
    0x3d, 0x5d, 0x6e, 0xf2, 0xcf, 0x15, 0xd6, 0x3c, 0xcc, 0x7d, 0x67, 0x1a, 0xb8, 0xd2, 0x1b, 0x54,
    0x97, 0xa2, 0x58, 0x58, 0xf7, 0x4e, 0x5e, 0x50, 0x42, 0x69, 0xdc, 0xe7, 0x3a, 0x87, 0x2e, 0x22
};

static void cyclic_xor(void *data, int datasize, void *xor, int xorsize)
{
    for(int i = 0; i < datasize; i++)
        *(uint8_t *)(data + i) ^= *(uint8_t *)(xor + (i % xorsize));
}

struct samsung_firmware_t *samsung_read(samsung_read_t read,
    samsung_printf_t printf, void *user, enum samsung_error_t *err)
{
    struct yp_header_t yp_hdr;
    struct yp_md5_t yp_md5;

    /* read and check header */
    if(read(user, 0, &yp_hdr, sizeof(yp_hdr)) != sizeof(yp_hdr))
    {
        printf(user, true, "Cannot read YP header\n");
        *err = SAMSUNG_READ_ERROR;
        return NULL;
    }
    if(strncmp(yp_hdr.signature, YP_SIGNATURE, sizeof(yp_hdr.signature)) != 0)
    {
        printf(user, true, "Bad YP signature\n");
        *err = SAMSUNG_FORMAT_ERROR;
        return NULL;
    }

    printf(user, false, "Model: %s\n", yp_hdr.model);
    printf(user, false, "Version: %s %s %s\n", yp_hdr.version, yp_hdr.region, yp_hdr.extra);

    /* allocate and read data */
    struct samsung_firmware_t *fw = malloc(sizeof(struct samsung_firmware_t));
    memset(fw, 0, sizeof(struct samsung_firmware_t));
    fw->data_size = yp_hdr.datasize;
    fw->data = malloc(fw->data_size);
    strncpy(fw->version, yp_hdr.version, sizeof(yp_hdr.version));
    strncpy(fw->region, yp_hdr.region, sizeof(yp_hdr.region));
    strncpy(fw->extra, yp_hdr.extra, sizeof(yp_hdr.extra));
    strncpy(fw->model, yp_hdr.model, sizeof(yp_hdr.model));

    if(read(user, sizeof(yp_hdr), fw->data, fw->data_size) != fw->data_size)
    {
        printf(user, true, "Cannot read data\n");
        *err = SAMSUNG_READ_ERROR;
        samsung_free(fw);
        return NULL;
    }

    /* read and check MD5 */
    if(read(user, yp_hdr.datasize + sizeof(yp_hdr), &yp_md5, sizeof(yp_md5)) != sizeof(yp_md5))
    {
        printf(user, true, "Cannot read MD5 sum\n");
        *err = SAMSUNG_READ_ERROR;
        samsung_free(fw);
        return NULL;
    }

    uint8_t actual_md5[MD5_DIGEST_LENGTH];
    MD5_CTX c;
    MD5_Init(&c);
    MD5_Update(&c, fw->data, fw->data_size);
    MD5_Final(actual_md5, &c);

    if(memcmp(actual_md5, yp_md5.md5, MD5_DIGEST_LENGTH) != 0)
    {
        printf(user, true, "MD5 mismatch\n");
        *err = SAMSUNG_MD5_ERROR;
        samsung_free(fw);
        return NULL;
    }

    /* decrypt */
    cyclic_xor(fw->data, fw->data_size, g_yp_key, sizeof(g_yp_key));

    /* success ! */
    *err = SAMSUNG_SUCCESS;
    return fw;
}

enum samsung_error_t samsung_write(samsung_write_t write, samsung_printf_t printf,
    void *user, struct samsung_firmware_t *fw)
{
    struct yp_header_t yp_hdr;
    struct yp_md5_t yp_md5;

    // write header
    strncpy(yp_hdr.signature, YP_SIGNATURE, sizeof(yp_hdr.signature));
    strncpy(yp_hdr.version, fw->version, sizeof(yp_hdr.version));
    strncpy(yp_hdr.region, fw->region, sizeof(yp_hdr.region));
    strncpy(yp_hdr.extra, fw->extra, sizeof(yp_hdr.extra));
    strncpy(yp_hdr.model, fw->model, sizeof(yp_hdr.model));
    yp_hdr.datasize = fw->data_size;

    printf(user, false, "Model: %s\n", yp_hdr.model);
    printf(user, false, "Version: %s %s %s\n", yp_hdr.version, yp_hdr.region, yp_hdr.extra);

    if(write(user, 0, &yp_hdr, sizeof(yp_hdr)) != sizeof(yp_hdr))
    {
        printf(user, true, "Cannot write header\n");
        return SAMSUNG_WRITE_ERROR;
    }

    // encrypt data
    cyclic_xor(fw->data, fw->data_size, g_yp_key, sizeof(g_yp_key));
    // compute MD5
    MD5_CTX c;
    MD5_Init(&c);
    MD5_Update(&c, fw->data, fw->data_size);
    MD5_Final(yp_md5.md5, &c);

    // write data
    if(write(user, sizeof(yp_hdr), fw->data, fw->data_size) != fw->data_size)
    {
        // decrypt data so that the firmware data is the same after the call
        cyclic_xor(fw->data, fw->data_size, g_yp_key, sizeof(g_yp_key));
        printf(user, true, "Cannot write data\n");
        return SAMSUNG_WRITE_ERROR;
    }
    // decrypt data so that the firmware data is the same after the call
    cyclic_xor(fw->data, fw->data_size, g_yp_key, sizeof(g_yp_key));
    // write md5
    if(write(user, sizeof(yp_hdr) + fw->data_size, &yp_md5, sizeof(yp_md5)) != sizeof(yp_md5))
    {
        printf(user, true, "Cannot write md5\n");
        return SAMSUNG_WRITE_ERROR;
    }

    return SAMSUNG_SUCCESS;
}

void samsung_free(struct samsung_firmware_t *fw)
{
    if(fw)
    {
        free(fw->data);
        free(fw);
    }
}
