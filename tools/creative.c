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
#include <elf.h>

#include "creative.h"
#include "hmac-sha1.h"

static const char null_key_v1[]  = "CTL:N0MAD|PDE0.SIGN.";
static const char null_key_v2[]  = "CTL:N0MAD|PDE0.DPMP.";
static const char null_key_v3[]  = "CTL:Z3N07|PDE0.DPMP.";
static const char null_key_v4[]  = "CTL:N0MAD|PDE0.DPFP.";

static const struct device_info devices[] =
{
    /* Creative Zen Vision:M */
    {"C\0r\0e\0a\0t\0i\0v\0e\0 \0Z\0e\0n\0 \0V\0i\0s\0i\0o\0n\0:\0M",             42, null_key_v2},
    /* Creative Zen Vision:M Go! */
    {"C\0r\0e\0a\0t\0i\0v\0e\0 \0Z\0e\0n\0 \0V\0i\0s\0i\0o\0n\0:\0M\0 \0G\0o\0!", 50, null_key_v2},
    /* Creative Zen Vision © TL */
    /* The "©" should be ANSI encoded or the device won't accept the firmware package. */
    {"C\0r\0e\0a\0t\0i\0v\0e\0 \0Z\0e\0n\0 \0V\0i\0s\0i\0o\0n\0 \0©\0T\0L",       46, null_key_v2},
    /* Creative ZEN V */
    {"C\0r\0e\0a\0t\0i\0v\0e\0 \0Z\0E\0N\0 \0V",                                  42, null_key_v4},
    /* Creative ZEN */
    {"C\0r\0e\0a\0t\0i\0v\0e\0 \0Z\0E\0N",                                        48, null_key_v3}
};

/*
Create a Zen Vision:M FRESCUE structure file
*/
extern void int2le(unsigned int val, unsigned char* addr);
extern unsigned int le2int(unsigned char* buf);


static int make_ciff_file(const unsigned char *inbuf, unsigned int length,
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
           25); /*Unicode encoded*/
    memset(&outbuf[0x78+25], 0, 32);
    memcpy(&outbuf[0x98], inbuf, length);
    memcpy(&outbuf[0x98+length], "LLUN", 4);
    int2le(20, &outbuf[0x98+length+4]);
    /* Do checksum */
    hmac_sha1((unsigned char *)devices[device].null, strlen(devices[device].null),
             outbuf, 0x98+length, key);
    memcpy(&outbuf[0x98+length+8], key, 20);
    return length+0x90+0x1C+8;
}

static int elf_convert(const unsigned char *inbuf, unsigned char *outbuf)
{
    Elf32_Ehdr *main_header;
    Elf32_Shdr *section_header;
    unsigned int i, j, sum;
    int startaddr;
    
    main_header = (Elf32_Ehdr*)inbuf;
    if( !( main_header->e_ident[0] == ELFMAG0 && main_header->e_ident[1] == ELFMAG1
        && main_header->e_ident[2] == ELFMAG2 && main_header->e_ident[3] == ELFMAG3 ) )
    {
        printf("Invalid ELF header!\n");
        return -1;
    }
    
    startaddr = (intptr_t)outbuf;
    
    for(i = 0; i < main_header->e_shnum; i++)
    {
        section_header = (Elf32_Shdr*)(inbuf+main_header->e_shoff+i*sizeof(Elf32_Shdr));
        
        if( (section_header->sh_flags & SHF_WRITE || section_header->sh_flags & SHF_ALLOC
             || section_header->sh_flags & SHF_EXECINSTR) && section_header->sh_size > 0 
             && section_header->sh_type != SHT_NOBITS                                     )
        {
            /* Address */
            int2le(section_header->sh_addr, outbuf);
            outbuf += 4;
            /* Size */
            int2le(section_header->sh_size, outbuf);
            outbuf += 4;
            /* Checksum */
            sum = 0;
            for(j=0; j<section_header->sh_size; j+= 4)
                sum += le2int((unsigned char*)(inbuf+section_header->sh_offset+j)) + (le2int((unsigned char*)(inbuf+section_header->sh_offset+j))>>16);
            int2le(sum, outbuf);
            outbuf += 2;
            memset(outbuf, 0, 2);
            outbuf += 2;
            /* Data */
            memcpy(outbuf, inbuf+section_header->sh_offset, section_header->sh_size);
            outbuf += section_header->sh_size;
        }
    }
    return (intptr_t)(outbuf - startaddr);
}

static int make_jrm_file(const unsigned char *inbuf, unsigned char *outbuf)
{
    int length;

    /* Clear the header area to zero */
    memset(outbuf, 0, 0x18);

    /* Header (EDOC) */
    memcpy(outbuf, "EDOC", 4);
    /* Total Size: temporarily set to 0 */
    memset(&outbuf[0x4], 0, 4);
    /* 4 bytes of zero */
    memset(&outbuf[0x8], 0, 4);
    
    length = elf_convert(inbuf, &outbuf[0xC]);
    if(length < 0)
        return -1;
    /* Now set the actual Total Size */
    int2le(4+length, &outbuf[0x4]);

    return 0xC+length;
}

int zvm_encode(const char *iname, const char *oname, int device)
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
        printf("Out of memory!\n");
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
        printf("Out of memory!\n");
        return -1;
    }
    length = make_jrm_file(buf, outbuf);
    free(buf);
    if(length < 0)
    {
        free(outbuf);
        printf("Error in making JRM file!\n");
        return -1;
    }
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
