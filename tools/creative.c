/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "creative.h"
#include "hmac-sha1.h"

static const char null_key_v1[]  = "CTL:N0MAD|PDE0.SIGN.";
static const char null_key_v2[]  = "CTL:N0MAD|PDE0.DPMP.";
static const char null_key_v3[]  = "CTL:Z3N07|PDE0.DPMP.";
static const char null_key_v4[]  = "CTL:N0MAD|PDE0.DPFP.";

static const unsigned char bootloader_v1[] =
{
    0xD3, 0xF0, 0x29, 0xE3, /* MSR     CPSR_cf, #0xD3     */
    0x09, 0xF6, 0xA0, 0xE3  /* MOV     PC, #0x900000      */
};

static const unsigned char bootloader_v2[] =
{
    0xD3, 0xF0, 0x29, 0xE3, /* MSR     CPSR_cf, #0xD3     */
    0x09, 0xF6, 0xA0, 0xE3  /* MOV     PC, #0x40000000      */
};

static const unsigned char bootloader_v3[] =
{
    0 /* Unknown */
};

static const struct device_info devices[] =
{
    /* Creative Zen Vision:M */
    {"C\0r\0e\0a\0t\0i\0v\0e\0 \0Z\0e\0n\0 \0V\0i\0s\0i\0o\0n\0:\0M",             42, null_key_v2, bootloader_v1, sizeof(bootloader_v1), 0x00900000},
    /* Creative Zen Vision:M Go! */
    {"C\0r\0e\0a\0t\0i\0v\0e\0 \0Z\0e\0n\0 \0V\0i\0s\0i\0o\0n\0:\0M\0 \0G\0o\0!", 50, null_key_v2, bootloader_v1, sizeof(bootloader_v1), 0x00900000},
    /* Creative Zen Vision © TL */
    /* The "©" should be ANSI encoded or the device won't accept the firmware package. */
    {"C\0r\0e\0a\0t\0i\0v\0e\0 \0Z\0e\0n\0 \0V\0i\0s\0i\0o\0n\0 \0©\0T\0L",       48, null_key_v2, bootloader_v1, sizeof(bootloader_v1), 0x00900000},
    /* Creative ZEN V */
    {"C\0r\0e\0a\0t\0i\0v\0e\0 \0Z\0E\0N\0 \0V",                                  42, null_key_v4, bootloader_v3, sizeof(bootloader_v3), 0x00000000},
    /* Creative ZEN */
    {"C\0r\0e\0a\0t\0i\0v\0e\0 \0Z\0E\0N",                                        48, null_key_v3, bootloader_v2, sizeof(bootloader_v2), 0x40000000}
};

/*
Create a Zen Vision:M FRESCUE structure file
*/
extern void int2le(unsigned int val, unsigned char* addr);
extern unsigned int le2int(unsigned char* buf);


static int make_ciff_file(unsigned char *inbuf, unsigned int length,
                          unsigned char *outbuf, int device)
{
    unsigned char key[20];
    memcpy(outbuf, "FFIC", 4);
    int2le(length+90, &outbuf[4]);
    memcpy(&outbuf[8], "FNIC", 4);
    int2le(96, &outbuf[0xC]);
    memcpy(&outbuf[0x10], devices[device].cinf, devices[device].cinf_size);
    memset(&outbuf[0x10+devices[device].cinf_size], 0,
           96 - devices[device].cinf_size);
    memcpy(&outbuf[0x70], "ATAD", 4);
    int2le(length+32, &outbuf[0x74]);
    memcpy(&outbuf[0x78], "H\0j\0u\0k\0e\0b\0o\0x\0\x32\0.\0j\0r\0m",
           32); /*Unicode encoded*/
    memcpy(&outbuf[0x98], inbuf, length);
    memcpy(&outbuf[0x98+length], "LLUN", 4);
    int2le(20, &outbuf[0x98+length+4]);
    /* Do checksum */
    hmac_sha1((unsigned char *)devices[device].null, strlen(devices[device].null),
             outbuf, 0x98+length, key);
    memcpy(&outbuf[0x98+length+8], key, 20);
    return length+0x90+0x1C+8;
}

static int make_jrm_file(unsigned char *inbuf, unsigned int length,
                         unsigned char *outbuf, int device)
{
    unsigned int i;
    unsigned int sum = 0;

    /* Clear the header area to zero */
    memset(outbuf, 0, 0x18);

    /* Header (EDOC) */
    memcpy(outbuf, "EDOC", 4);
    /* Total Size */
    #define SIZEOF_BOOTLOADER_CODE  devices[device].bootloader_size
    int2le(4+0xC+SIZEOF_BOOTLOADER_CODE+0xC+length, &outbuf[0x4]);
    /* 4 bytes of zero */
    memset(&outbuf[0x8], 0, 0x4);
    
    /* First block starts here ... */
    /* Address = 0x0 */
    memset(&outbuf[0xC], 0, 0x4);
    /* Size */
    int2le(SIZEOF_BOOTLOADER_CODE, &outbuf[0x10]);
    /* Checksum */
    for(i=0; i<SIZEOF_BOOTLOADER_CODE; i+= 4)
        sum += le2int((unsigned char*)&devices[device].bootloader[i]) + (le2int((unsigned char*)&devices[device].bootloader[i])>>16);
    int2le(sum, &outbuf[0x14]);
    outbuf[0x16] = 0;
    outbuf[0x17] = 0;
    /*Data */
    memcpy(&outbuf[0x18], devices[device].bootloader, SIZEOF_BOOTLOADER_CODE);
    
    /* Second block starts here ... */
    /* Address = depends on target */
    #define SB_START    (0x18+SIZEOF_BOOTLOADER_CODE)
    int2le(devices[device].memory_address, &outbuf[SB_START]);
    /* Size */
    int2le(length, &outbuf[SB_START+0x4]);
    /* Checksum */
    sum = 0;
    for(i=0; i<length; i+= 4)
        sum += le2int(&inbuf[i]) + (le2int(&inbuf[i])>>16);
    int2le(sum, &outbuf[SB_START+0x8]);
    outbuf[SB_START+0xA] = 0;
    outbuf[SB_START+0xB] = 0;
    /* Data */
    memcpy(&outbuf[SB_START+0xC], inbuf, length);
    
    return SB_START+0xC+length;
}

int zvm_encode(char *iname, char *oname, int device)
{
    size_t len;
    int length;
    FILE *file;
    unsigned char *outbuf;
    unsigned char *buf;

    file = fopen(iname, "rb");
    if (!file) {
        perror(iname);
        return -1;
    }
    fseek(file, 0, SEEK_END);
    length = ftell(file);

    fseek(file, 0, SEEK_SET);

    buf = (unsigned char*)malloc(length);
    if ( !buf ) {
        printf("out of memory!\n");
        return -1;
    }

    len = fread(buf, 1, length, file);
    if(len < (size_t)length) {
        perror(iname);
        return -2;
    }
    fclose(file);

    outbuf = (unsigned char*)malloc(length+0x300);
    if ( !outbuf ) {
        free(buf);
        printf("out of memory!\n");
        return -1;
    }
    length = make_jrm_file(buf, len, outbuf, device);
    free(buf);
    buf = (unsigned char*)malloc(length+0x200);
    memset(buf, 0, length+0x200);
    length = make_ciff_file(outbuf, length, buf, device);
    free(outbuf);

    file = fopen(oname, "wb");
    if (!file) {
        free(buf);
        perror(oname);
        return -3;
    }

    len = fwrite(buf, 1, length, file);
    if(len < (size_t)length) {
        free(buf);
        perror(oname);
        return -4;
    }

    free(buf);

    fclose(file);

    return 0;
}
