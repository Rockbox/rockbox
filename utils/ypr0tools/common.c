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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <stdint.h>
#include "common.h"
#include "../../tools/fwpatcher/md5.h"

uint8_t g_yp_key[] =
{
  0xa3, 0x04, 0xb9, 0xcd, 0x34, 0x13, 0x4a, 0x19, 0x19, 0x31, 0xdf, 0xbb,
  0x8f, 0x3d, 0x7f, 0x09, 0x42, 0x3c, 0x96, 0x33, 0x41, 0xa9, 0x95, 0xf1,
  0xd0, 0xac, 0x16, 0x37, 0x57, 0x35, 0x28, 0xe7, 0x0b, 0xc2, 0x12, 0x09,
  0x39, 0x42, 0xd2, 0x96, 0xf5, 0x00, 0xd2, 0x23, 0x37, 0x24, 0xe2, 0x8e,
  0x50, 0x3c, 0x6e, 0x23, 0xeb, 0x68, 0xed, 0x31, 0xb7, 0xee, 0xc0, 0xc7,
  0x09, 0xf8, 0x39, 0x9d, 0x51, 0xed, 0x17, 0x95, 0x64, 0x09, 0xe0, 0xf9,
  0xf0, 0xef, 0x86, 0xc0, 0x04, 0x46, 0x89, 0x8a, 0x6e, 0x27, 0x69, 0xde,
  0xc7, 0x31, 0x1e, 0xee, 0x3c, 0x3f, 0x17, 0x05, 0x44, 0xbb, 0xbb, 0x1d,
  0x3d, 0x5d, 0x6e, 0xf2, 0x78, 0x15, 0xd6, 0x3c, 0xcc, 0x7d, 0x67, 0x1a,
  0xb8, 0xd2, 0x79, 0x54, 0x97, 0xa2, 0x58, 0x58, 0xf7, 0x4e, 0x5e, 0x50,
  0x42, 0x69, 0xdc, 0xe7, 0x3a, 0x87, 0x2e, 0x22
};

char* firmware_components[] = {"MBoot", "Linux", "RootFS", "Sysdata"};
char* firmware_filenames[] = {"MBoot.bin", "zImage", "cramfs-fsl.rom", "SYSDATA.bin"};

void cyclic_xor(void *data, int datasize, void *xor, int xorsize)
{
    for(int i = 0; i < datasize; i++)
        *(uint8_t *)(data + i) ^= *(uint8_t *)(xor + (i % xorsize));
}

size_t get_filesize(FILE* handle)
{
    long size = 0;
    long old_pos = ftell(handle);
    fseek(handle, 0, SEEK_END);
    size = ftell(handle);
    fseek(handle, old_pos, SEEK_SET);
    return size;
}

/* A very rough implementation... */
void join_path(char* destination, char* first, char* second)
{
    memset(destination, 0, MAX_PATH);
    if (first != NULL && strlen(first) > 0)
    {
        strcpy(destination, first);
        if (destination[strlen(destination) - 1] != DIR_SEPARATOR)
        {
             int l = strlen(destination);
             destination[l] = DIR_SEPARATOR;
             destination[l + 1] = '\0';
        }
    }
    strcat(destination, second);
}

void md5sum(char* md5sum_string, char* data, unsigned long size)
{
    uint8_t md5_checksum[16];
    md5_context c;
    md5_starts(&c);
    md5_update(&c, (unsigned char*)data, size);
    md5_finish(&c, md5_checksum);
    memset(md5sum_string, 0, MD5_DIGEST_LENGTH*2+1);
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        sprintf(md5sum_string, "%02x", md5_checksum[i]);
        md5sum_string+=2;
    }
}
