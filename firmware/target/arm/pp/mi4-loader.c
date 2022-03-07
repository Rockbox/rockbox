/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2006 by Barry Wardell
 * Copyright (C) 2020 by William Wilgus [MULTIBOOT]
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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

#include <stdbool.h>
#include <stdio.h>
#include "config.h"
#include "mi4-loader.h"
#include "loader_strerror.h"
#include "crc32.h"
#include "file.h"
#if defined(HAVE_BOOTDATA)
#include "multiboot.h"
#endif /* HAVE_BOOTDATA */

static inline unsigned int le2int(unsigned char* buf)
{
   int32_t res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

   return res;
}

static inline void int2le(unsigned int val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
    addr[2] = (val >> 16) & 0xff;
    addr[3] = (val >> 24) & 0xff;
}

static struct tea_key tea_keytable[] = {
  { "default" ,          { 0x20d36cc0, 0x10e8c07d, 0xc0e7dcaa, 0x107eb080 } },
  { "sansa",             { 0xe494e96e, 0x3ee32966, 0x6f48512b, 0xa93fbb42 } },
  { "sansa_gh",          { 0xd7b10538, 0xc662945b, 0x1b3fce68, 0xf389c0e6 } },
  { "sansa_103",         { 0x1d29ddc0, 0x2579c2cd, 0xce339e1a, 0x75465dfe } },
  { "rhapsody",          { 0x7aa9c8dc, 0xbed0a82a, 0x16204cc7, 0x5904ef38 } },
  { "p610",              { 0x950e83dc, 0xec4907f9, 0x023734b9, 0x10cfb7c7 } },
  { "p640",              { 0x220c5f23, 0xd04df68e, 0x431b5e25, 0x4dcc1fa1 } },
  { "virgin",            { 0xe83c29a1, 0x04862973, 0xa9b3f0d4, 0x38be2a9c } },
  { "20gc_eng",          { 0x0240772c, 0x6f3329b5, 0x3ec9a6c5, 0xb0c9e493 } },
  { "20gc_fre",          { 0xbede8817, 0xb23bfe4f, 0x80aa682d, 0xd13f598c } },
  { "elio_p722",         { 0x6af3b9f8, 0x777483f5, 0xae8181cc, 0xfa6d8a84 } },
  { "c200",              { 0xbf2d06fa, 0xf0e23d59, 0x29738132, 0xe2d04ca7 } },
  { "c200_103",          { 0x2a7968de, 0x15127979, 0x142e60a7, 0xe49c1893 } },
  { "c200_106",          { 0xa913d139, 0xf842f398, 0x3e03f1a6, 0x060ee012 } },
  { "view",              { 0x70e19bda, 0x0c69ea7d, 0x2b8b1ad1, 0xe9767ced } },
  { "sa9200",            { 0x33ea0236, 0x9247bdc5, 0xdfaedf9f, 0xd67c9d30 } },
  { "hdd1630/hdd63x0",   { 0x04543ced, 0xcebfdbad, 0xf7477872, 0x0d12342e } },
  { "vibe500",           { 0xe3a66156, 0x77c6b67a, 0xe821dca5, 0xca8ca37c } },
};

/*

tea_decrypt() from http://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm

"Following is an adaptation of the reference encryption and decryption
routines in C, released into the public domain by David Wheeler and
Roger Needham:"

*/

/* NOTE: The mi4 version of TEA uses a different initial value to sum compared
         to the reference implementation and the main loop is 8 iterations, not
         32.
*/

static void tea_decrypt(uint32_t* v0, uint32_t* v1, uint32_t* k) {
    uint32_t sum=0xF1BBCDC8, i;                    /* set up */
    uint32_t delta=0x9E3779B9;                     /* a key schedule constant */
    uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */
    for(i=0; i<8; i++) {                               /* basic cycle start */
        *v1 -= ((*v0<<4) + k2) ^ (*v0 + sum) ^ ((*v0>>5) + k3);
        *v0 -= ((*v1<<4) + k0) ^ (*v1 + sum) ^ ((*v1>>5) + k1);
        sum -= delta;                                   /* end cycle */
    }
}

/* mi4 files are encrypted in 64-bit blocks (two little-endian 32-bit
   integers) and the key is incremented after each block
 */

static void tea_decrypt_buf(unsigned char* src,
                            unsigned char* dest,
                            size_t n, uint32_t * key)
{
    uint32_t v0, v1;
    unsigned int i;

    for (i = 0; i < (n / 8); i++) {
        v0 = le2int(src);
        v1 = le2int(src+4);

        tea_decrypt(&v0, &v1, key);

        int2le(v0, dest);
        int2le(v1, dest+4);

        src += 8;
        dest += 8;

        /* Now increment the key */
        key[0]++;
        if (key[0]==0) {
            key[1]++;
            if (key[1]==0) {
                key[2]++;
                if (key[2]==0) {
                    key[3]++;
                }
            }
        }
    }
}

static int tea_find_key(struct mi4header_t *mi4header, unsigned char* buf)
{
    unsigned int i;
    uint32_t key[4];
    uint32_t keyinc;
    unsigned char magic_dec[8];
    int key_found = -1;
    unsigned int magic_location = mi4header->length-4;
    int unaligned = 0;

    if ( (magic_location % 8) != 0 )
    {
        unaligned = 1;
        magic_location -= 4;
    }

    for (i=0; i < NUM_KEYS && (key_found<0) ; i++) {
        key[0] = tea_keytable[i].key[0];
        key[1] = tea_keytable[i].key[1];
        key[2] = tea_keytable[i].key[2];
        key[3] = tea_keytable[i].key[3];

        /* Now increment the key */
        keyinc = (magic_location-mi4header->plaintext)/8;
        if ((key[0]+keyinc) < key[0]) key[1]++;
        key[0] += keyinc;
        if (key[1]==0) key[2]++;
        if (key[2]==0) key[3]++;

        /* Decrypt putative magic */
        tea_decrypt_buf(&buf[magic_location], magic_dec, 8, key);

        if (le2int(&magic_dec[4*unaligned]) == 0xaa55aa55)
        {
            key_found = i;
        }
    }

    return key_found;
}

/* Load mi4 format firmware image from a FULLY QUALIFIED PATH */
static int load_mi4_filename(unsigned char* buf,
                                const char* filename,
                               unsigned int buffer_size)
{
    int fd;
    struct mi4header_t mi4header;
    int rc;
    unsigned long sum;

    fd = open(filename, O_RDONLY);

    if(fd < 0)
        return EFILE_NOT_FOUND;

    read(fd, &mi4header, MI4_HEADER_SIZE);

    /* MI4 file size */
    if ((mi4header.mi4size-MI4_HEADER_SIZE) > buffer_size)
    {
        close(fd);
        return EFILE_TOO_BIG;
    }

    /* Load firmware file */
    lseek(fd, MI4_HEADER_SIZE, SEEK_SET);
    rc = read(fd, buf, mi4header.mi4size-MI4_HEADER_SIZE);
    close(fd);
    if(rc < (int)mi4header.mi4size-MI4_HEADER_SIZE)
        return EREAD_IMAGE_FAILED;

    /* Check CRC32 to see if we have a valid file */
    sum = crc_32r (buf, mi4header.mi4size - MI4_HEADER_SIZE, 0);

    if(sum != mi4header.crc32)
        return EBAD_CHKSUM;

    if( (mi4header.plaintext + MI4_HEADER_SIZE) != mi4header.mi4size)
    {
        /* Load encrypted firmware */
        int key_index = tea_find_key(&mi4header, buf);

        if (key_index < 0)
            return EINVALID_FORMAT;

        /* Plaintext part is already loaded */
        buf += mi4header.plaintext;

        /* Decrypt in-place */
        tea_decrypt_buf(buf, buf,
                        mi4header.mi4size-(mi4header.plaintext+MI4_HEADER_SIZE),
                        tea_keytable[key_index].key);

        /* Check decryption was successfull */
        if(le2int(&buf[mi4header.length-mi4header.plaintext-4]) != 0xaa55aa55)
            return EREAD_IMAGE_FAILED;
    }

    return mi4header.mi4size - MI4_HEADER_SIZE;
}

/* Load mi4 format firmware image */
int load_mi4(unsigned char* buf, const char* firmware, unsigned int buffer_size)
{

    int ret = EFILE_NOT_FOUND;
    char filename[MAX_PATH+2];
    /* only filename passed */
    if (firmware[0] != '/')
    {

#ifdef HAVE_MULTIBOOT /* defined by config.h */
        /* checks <volumes> highest index to lowest for redirect file
         * 0 is the default boot volume, it is not checked here
         * if found <volume>/rockbox_main.<playername> and firmware
         * has a bootdata region this firmware will be loaded */
        for (unsigned int i = NUM_VOLUMES - 1; i > 0 && ret < 0; i--)
        {
            if (get_redirect_dir(filename, sizeof(filename), i,
                                 BOOTDIR, firmware) > 0)
            {
                ret = load_mi4_filename(buf, filename, buffer_size);
            /* if firmware has no boot_data don't load from external drive */
                if (write_bootdata(buf, ret, i) <= 0)
                    ret = EKEY_NOT_FOUND;
            }
            /* if ret is valid breaks from loop to continue loading */
        }
#endif

        if (ret < 0) /* Check default volume, no valid firmware file loaded yet */
        {
            /* First check in BOOTDIR */
            snprintf(filename, sizeof(filename), BOOTDIR "/%s",firmware);

            ret = load_mi4_filename(buf, filename, buffer_size);

            if (ret < 0)
            {
                /* Check in root dir */
                snprintf(filename, sizeof(filename),"/%s",firmware);
                ret = load_mi4_filename(buf, filename, buffer_size);
            }
#ifdef HAVE_BOOTDATA
                /* 0 is the default boot volume */
                write_bootdata(buf, ret, 0);
#endif
        }
    }
    else /* full path passed ROLO etc.*/
        ret = load_mi4_filename(buf, firmware, buffer_size);

    return ret;
}
