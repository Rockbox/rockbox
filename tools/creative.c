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

#include "creative.h"
#include "hmac-sha1.h"
#include <stdio.h>
#include <stdbool.h>

/*
Create a Zen Vision:M FRESCUE structure file
*/


static int make_ciff_file(char *inbuf, int length, char *outbuf, int device)
{
    memcpy(outbuf, "FFIC", 4);
    int2le(length+90, &outbuf[4]);
    memcpy(&outbuf[8], "FNIC", 4);
    int2le(96, &outbuf[0xC]);
    memcpy(&outbuf[0x10], devices[device].cinf, devices[device].cinf_size);
    memset(&outbuf[0x10+devices[device].cinf_size], 0, 96 - devices[device].cinf_size);
    memcpy(&outbuf[0x70], "ATAD", 4);
    int2le(length+32, &outbuf[0x74]);
    memcpy(&outbuf[0x78], "H\0j\0u\0k\0e\0b\0o\0x\0\x32\0.\0j\0r\0m", 32); /*Unicode encoded*/
    memcpy(&outbuf[0x98], inbuf, length);
    memcpy(&outbuf[0x98+length], "LLUN", 4);
    int2le(20, &outbuf[0x98+length+4]);
    /* Do checksum */
    char key[20];
    hmac_sha((char*)devices[device].null, strlen(devices[device].null), outbuf, 0x98+length, key, 20);
    memcpy(&outbuf[0x98+length+8], key, 20);
    return length+0x90+0x1C+8;
}

static int make_jrm_file(char *inbuf, int length, char *outbuf)
{
    int i;
    unsigned int sum = 0;
    /* Calculate checksum for later use in header */
    for(i=0; i<length; i+= 4)
        sum += le2int(&inbuf[i]) + (le2int(&inbuf[i])>>16);

    /* Clear the header area to zero */
    memset(outbuf, 0, 0x18);

    /* Header (EDOC) */
    memcpy(outbuf, "EDOC", 4);
    /* Total Size */
    int2le(length+0x20, &outbuf[0x4]);
    /* 4 bytes of zero */

    /* Address = 0x900000 */
    int2le(0x900000, &outbuf[0xC]);
    /* Size */
    int2le(length, &outbuf[0x10]);
    /* Checksum */
    int2le(sum, &outbuf[0x14]);
    outbuf[0x16] = 0;
    outbuf[0x17] = 0;
    /* Data starts here... */
    memcpy(&outbuf[0x18], inbuf, length);

    /* Second block starts here ... */
    /* Address = 0x0 */
    /* Size */
    int2le(0x4, &outbuf[0x18+length+0x4]);
    /* Checksum */
    outbuf[0x18+length+0x8] = 0xA9;
    outbuf[0x18+length+0x9] = 0xD9;
    /* Data: MOV PC, 0x900000 */
    outbuf[0x18+length+0xC] = 0x09;
    outbuf[0x18+length+0xD] = 0xF6;
    outbuf[0x18+length+0xE] = 0xA0;
    outbuf[0x18+length+0xF] = 0xE3;

    return length+0x18+0x10;
}

int zvm_encode(char *iname, char *oname, int device)
{
    size_t len;
    int length;
    FILE *file;
    unsigned char *outbuf;
    unsigned char *buf;
    int i;

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
    if(len < length) {
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
    length = make_jrm_file(buf, len, outbuf);
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
    if(len < length) {
        free(buf);
        perror(oname);
        return -4;
    }

    free(buf);

    fclose(file);

    return 0;
}
