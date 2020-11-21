/***************************************************************************
 *
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * $Id$
 * 
 * Rockbox plugin copyright (C) 2009 Dave Chapman.
 * Based on encryption code (C) 2009 Michael Sparmann
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

/* 

   This viewer plugin is for the encryption/decryption of iPod Nano
   (2nd generation) firmware images using the hardware AES crypto unit
   in such devices.

   Encrypted images are stored with the modelname "nn2x" and extension
   ".ipodx" Unencrypted images use "nn2g" and ".ipod".

   Heavily based on Payloads/CryptFirmware/main.c from iBugger.

*/

#include "plugin.h"
#include "checksum.h"

static void aes_encrypt(void* data, uint32_t size)
{
    uint32_t ptr, i;
    uint32_t go = 1;
    PWRCONEXT &= ~0x400;
    AESTYPE = 1;
    AESUNKREG0 = 1;
    AESUNKREG0 = 0;
    AESCONTROL = 1;
    AESKEYLEN = 9;
    AESOUTSIZE = size;
    AESAUXSIZE = 0x10;
    AESINSIZE = 0x10;
    AESSIZE3 = 0x10;
    for (ptr = 0; ptr < (size >> 2); ptr += 4)
    {
        AESOUTADDR = (uint32_t)data + (ptr << 2);
        AESINADDR = (uint32_t)data + (ptr << 2);
        AESAUXADDR = (uint32_t)data + (ptr << 2);
        if (ptr != 0)
            for (i = 0; i < 4; i++)
                ((uint32_t*)data)[ptr + i] ^= ((uint32_t*)data)[ptr + i - 4];
        AESSTATUS = 6;
        AESGO = go;
        go = 3;
        while ((AESSTATUS & 6) == 0);
    }
    AESCONTROL = 0;
    PWRCONEXT |= 0x400;
}

static void aes_decrypt(void* data, uint32_t size)
{
    uint32_t ptr, i;
    uint32_t go = 1;
    PWRCONEXT &= ~0x400;
    AESTYPE = 1;
    AESUNKREG0 = 1;
    AESUNKREG0 = 0;
    AESCONTROL = 1;
    AESKEYLEN = 8;
    AESOUTSIZE = size;
    AESAUXSIZE = 0x10;
    AESINSIZE = 0x10;
    AESSIZE3 = 0x10;
    for (ptr = (size >> 2) - 4; ; ptr -= 4)
    {
        AESOUTADDR = (uint32_t)data + (ptr << 2);
        AESINADDR = (uint32_t)data + (ptr << 2);
        AESAUXADDR = (uint32_t)data + (ptr << 2);
        AESSTATUS = 6;
        AESGO = go;
        go = 3;
        while ((AESSTATUS & 6) == 0);
        if (ptr == 0) break;
        for (i = 0; i < 4; i++)
            ((uint32_t*)data)[ptr + i] ^= ((uint32_t*)data)[ptr + i - 4];
    }
    AESCONTROL = 0;
    PWRCONEXT |= 0x400;
}

static void calc_hash(uint32_t* data, uint32_t size, uint32_t* result)
{
    uint32_t ptr, i;
    uint32_t ctrl = 2;

    PWRCONEXT &= ~0x4;

    for (ptr = 0; ptr < (size >> 2); ptr += 0x10)
    {
      for (i = 0; i < 0x10; i++) HASHDATAIN[i] = data[ptr + i];
      HASHCTRL = ctrl;
      ctrl = 0xA;
      while ((HASHCTRL & 1) != 0);
    }
    for (i = 0; i < 5; i ++) result[i] = HASHRESULT[i];

    PWRCONEXT |= 0x4;
}

static uint32_t get_uint32be(unsigned char* buf)
{
    return (uint32_t)((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
}

static void put_uint32be(unsigned char* buf, uint32_t x)
{
    buf[0] = (x & 0xff000000) >> 24;
    buf[1] = (x & 0xff0000) >> 16;
    buf[2] = (x & 0xff00) >> 8;
    buf[3] = x & 0xff;
}

enum plugin_status plugin_start(const void* parameter)
{
    int fd;
    int length;
    int n;
    ssize_t buf_size;
    uint32_t* buf;
    int size;
    uint32_t sum;
    uint32_t hash[0x200];
    char outputfilename[MAX_PATH];

    fd = rb->open(parameter,O_RDONLY);

    if (fd < 0) {
        rb->splash(HZ*2, "Cannot open file");
        return PLUGIN_ERROR;
    }

    length = rb->filesize(fd);

    if (length < 12) {
        rb->splash(HZ*2, "File too small");
        return PLUGIN_ERROR;
    }

    /* Get the audio buffer */
    buf = rb->plugin_get_audio_buffer((size_t *)&buf_size);

    /* Use uncached alias for buf - equivalent to buf |= 0x40000000 */
    buf += 0x10000000;

    if (length > buf_size) {
        rb->splash(HZ*2, "File too big");
        return PLUGIN_ERROR;
    }

    n = rb->read(fd, buf, length);
    if (n < length) {
        rb->splash(HZ*2, "Cannot read file");
        return PLUGIN_ERROR;
    }
    rb->close(fd);

    size = length - 8;  /* Size of firmware image */

    if (calc_checksum(MODEL_NUMBER, (unsigned char*)(buf + 2), size) != 
        get_uint32be((unsigned char*)buf)) {
        rb->splash(HZ*2, "Bad checksum in input file");
        return PLUGIN_ERROR;
    }

    n = rb->strlen(parameter);
    if (memcmp(buf+1,"nn2g",4)==0) {
        /* Encrypting - Input file should be .ipod, output file is .ipodx */

        if ((n < 6) || (rb->strcmp(parameter+n-5,".ipod") != 0)) {
            rb->splash(HZ*2, "Input filename must be .ipod");
            return PLUGIN_ERROR;
        }

        if (n + 2 > MAX_PATH) {
            rb->splash(HZ*2, "Filename too long");
            return PLUGIN_ERROR;
        }

        size = (size + 0x3f) & ~0x3f;  /* Pad to multiple of 64 bytes */
        if (size > (length - 8)) {
            rb->memset(&buf[length/4], 0, size - (length - 8));
        }

        rb->strlcpy(outputfilename, parameter, MAX_PATH);
        outputfilename[n] = 'x';
        outputfilename[n+1] = 0;

        /* Everything is OK, now do the encryption */

        /* 1 - Calculate hashes */

        rb->memset(hash, 0, sizeof(hash));

        hash[1] = 2;
        hash[2] = 1;
        hash[3] = 0x40;
        hash[5] = size;

        calc_hash(buf + 2, size, &hash[7]);
        calc_hash(hash, 0x200, &hash[0x75]);

        /* 2 - Do the encryption */

        rb->splash(0, "Encrypting...");
        aes_encrypt(buf + 2, size);

        /* 3 - Update the Rockbox header */

        sum = calc_checksum(MODEL_NUMBER, (unsigned char*)hash, sizeof(hash));
        sum = calc_checksum(sum, (unsigned char*)(buf + 2), size); 
        put_uint32be((unsigned char*)buf, sum);
        memcpy(buf + 1, "nn2x", 4);

        /* 4 - Write to disk */
        fd = rb->open(outputfilename,O_WRONLY|O_CREAT|O_TRUNC, 0666);

        if (fd < 0) {
            rb->splash(HZ*2, "Could not open output file");
            return PLUGIN_ERROR;
        }

    n = rb->write(fd, buf, 8);
    n = rb->write(fd, hash, sizeof(hash));
    n = rb->write(fd, buf + 2, size);

        rb->close(fd);
    } else if (memcmp(buf + 1,"nn2x",4)==0) {
        /* Decrypting - Input file should be .ipodx, output file is .ipod */

        if ((n < 7) || (rb->strcmp(parameter+n-6,".ipodx") != 0)) {
            rb->splash(HZ*2, "Input filename must be .ipodx");
            return PLUGIN_ERROR;
        }

        rb->strlcpy(outputfilename, parameter, MAX_PATH);
        outputfilename[n-1] = 0;  /* Remove "x" at end of filename */

        /* Everything is OK, now do the decryption */

        size -= 0x800;  /* Remove hash size from firmware size */

        /* 1 - Decrypt */

        rb->splash(0, "Decrypting...");

        aes_decrypt(&buf[0x202], size);

        /* 2 - Calculate hashes to verify decryption */

        rb->lcd_clear_display();
        rb->splash(0, "Calculating hash...");

        rb->memset(hash, 0, sizeof(hash));

        hash[1] = 2;
        hash[2] = 1;
        hash[3] = 0x40;
        hash[5] = size;

        calc_hash(&buf[0x202], size, &hash[7]);
        calc_hash(hash, 0x200, &hash[0x75]);

        if ((memcmp(hash + 7, buf + 9, 20) != 0) ||
            (memcmp(hash + 75, buf + 77, 20) != 0)) {
            rb->splash(HZ*2, "Decryption failed - bad hash");
            return PLUGIN_ERROR;
        }

        /* 3 - Update the Rockbox header */

        sum = calc_checksum(MODEL_NUMBER, (unsigned char*)(&buf[0x202]), size); 
        put_uint32be((unsigned char*)buf, sum);
        memcpy(buf + 1, "nn2g", 4);

        /* 4 - Write to disk */
        fd = rb->open(outputfilename,O_WRONLY|O_CREAT|O_TRUNC, 0666);

        if (fd < 0) {
            rb->splash(HZ*2, "Could not open output file");
            return PLUGIN_ERROR;
        }

        n = rb->write(fd, buf, 8);
        n = rb->write(fd, &buf[0x202], size);

        rb->close(fd);
    } else {
        rb->splash(HZ*2,"Invalid input file");
        return PLUGIN_ERROR;
    }

    return PLUGIN_OK;
}
