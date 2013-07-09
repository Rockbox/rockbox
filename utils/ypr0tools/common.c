/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 Lorenzo Miori
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

#include <openssl/md5.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <stdint.h>

void cyclic_xor(void *data, int datasize, void *xor, int xorsize)
{
    for(int i = 0; i < datasize; i++)
        *(uint8_t *)(data + i) ^= *(uint8_t *)(xor + (i % xorsize));
}

long get_filesize(FILE* handle)
{
    long size = 0;
    long old_pos = ftell(handle);
    fseek(handle, 0, SEEK_END);
    size = ftell(handle);
    fseek(handle, old_pos, SEEK_SET);
    return size;
}

void md5sum(char* md5sum_string, char* data, unsigned long size)
{
    uint8_t md5_checksum[MD5_DIGEST_LENGTH];
    MD5_CTX c;
    MD5_Init(&c);
    MD5_Update(&c, data, size);
    MD5_Final(md5_checksum, &c);
    memset(md5sum_string, 0, MD5_DIGEST_LENGTH*2+1);
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        sprintf(md5sum_string, "%02x", md5_checksum[i]);
        md5sum_string+=2;
    }
}