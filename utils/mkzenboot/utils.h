/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#ifndef __UTILS__
#define __UTILS__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "mkzenboot.h"
#include "hmac-sha1.h"

int filesize(FILE* fd);
unsigned int le2int(unsigned char* buf);
unsigned int be2int(unsigned char* buf);
void int2le(unsigned int val, unsigned char* addr);
const char* find_firmware_key(const unsigned char* buffer, size_t len);
uint32_t find_firmware_offset(unsigned char* buffer, size_t len);
bool crypt_firmware(const char* key, unsigned char* buffer, size_t len);
bool inflate_to_buffer(const unsigned char *buffer, size_t len,
    unsigned char* out_buffer, size_t out_len, char** err_msg);
int cenc_decode(unsigned char* src, int srclen, unsigned char* dst, int dstlen);
bool bf_cbc_decrypt(const unsigned char* key, size_t keylen, unsigned char* data,
    size_t datalen, const unsigned char* iv);
uint32_t swap(uint32_t val);
enum zen_error_t compute_md5sum(const char *file, uint8_t file_md5sum[16]);
enum zen_error_t compute_md5sum_buf(void *buf, size_t sz, uint8_t file_md5sum[16]);
enum zen_error_t read_file(const char *file, void **buffer, size_t *size);
enum zen_error_t write_file(const char *file, void *buffer, size_t size);
enum zen_error_t find_pe_data(void *fw, size_t fw_size, uint32_t *data_ptr, uint32_t *data_size);
int convxdigit(char digit, uint8_t *val);

#endif /* __UTILS__ */