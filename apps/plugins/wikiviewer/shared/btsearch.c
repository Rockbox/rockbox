/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include "utf8_aux.h"
#include "btsearch.h"

#if 0
#define DEBUGF printf
#else
#define DEBUGF(...)
#endif

#define ROCKBOX_LITTLE_ENDIAN
#define KEY_MAXLEN 200

static inline unsigned short swap16(unsigned short value)
/*
   result[15..8] = value[ 7..0];
   result[ 7..0] = value[15..8];
 */
{
    return (value >> 8) | (value << 8);
}

static inline unsigned long swap32(unsigned long value)
/*
   result[31..24] = value[ 7.. 0];
   result[23..16] = value[15.. 8];
   result[15.. 8] = value[23..16];
   result[ 7.. 0] = value[31..24];
 */
{
    unsigned long hi = swap16(value >> 16);
    unsigned long lo = swap16(value & 0xffff);
    return (lo << 16) | hi;
}

#ifdef ROCKBOX_LITTLE_ENDIAN
#define letoh16(x) (x)
#define letoh32(x) (x)
#else
#define letoh16(x) swap16(x)
#define letoh32(x) swap32(x)
#endif

#ifdef BTSEARCH_MAIN
int main(int argc,char * argv[])
{
    if(argc<3)
        printf("Usage: btsearch <file> <key>\n");
    else
    {
        FILE *fd=fopen(argv[1],"r");
        uint32_t res_lo;
        uint32_t res_hi;
        fseek(fd,0,SEEK_SET);
        search_btree(fd, argv[2], strlen(argv[2]), 0, &res_lo, &res_hi, false);
        printf("Result: %d,%d\n",res_lo,res_hi);
    }

    return 0;
}

#endif

void search_btree(void* file, const char* key, uint16_t rkeylen, uint32_t globoffs,
                  uint32_t* res_lo, uint32_t* res_hi, const bool casesense)
{
    unsigned char nd_key[KEY_MAXLEN];
    uint8_t node_flags;
    uint16_t node_nr_active,i,keylen;
    uint32_t chldptr;
    uint64_t dtaptr_lo,dtaptr_hi;
    fread(&node_flags,sizeof(uint8_t),1,file);
    fread(&node_nr_active,sizeof(uint16_t),1,file);
    node_nr_active=letoh16(node_nr_active);
    if(node_nr_active<1)   /* error */
        goto err;

    for(i=0; i<node_nr_active; i++)
    {
        fread(&dtaptr_lo,sizeof(uint32_t),1,file);
        dtaptr_lo=letoh32(dtaptr_lo);
        fread(&dtaptr_hi,sizeof(uint32_t),1,file);
        dtaptr_hi=letoh32(dtaptr_hi);
        fread(&chldptr,sizeof(uint32_t),1,file);
        chldptr=letoh32(chldptr);
        fread(&keylen,sizeof(uint16_t),1,file);
        keylen=letoh32(keylen);
        fread(&nd_key,sizeof(unsigned char),(keylen<KEY_MAXLEN) ? keylen : KEY_MAXLEN,file);
        if(keylen-((keylen<KEY_MAXLEN) ? keylen : KEY_MAXLEN)>0)
            fseek(file,keylen-((keylen<KEY_MAXLEN) ? keylen : KEY_MAXLEN),SEEK_CUR);

        keylen=(keylen<KEY_MAXLEN) ? keylen : KEY_MAXLEN;

        nd_key[keylen]=0;
        DEBUGF("CMP: %s, %s\n", key, nd_key);
        if(utf8strcnmp(((const unsigned char*)key),((const unsigned char*)nd_key),rkeylen,keylen,casesense)>0)
            continue;

        if(utf8strcnmp(((const unsigned char*)key),((const unsigned char*)nd_key),rkeylen,keylen,casesense)==0)
        {
            DEBUGF("Found! %s\n", nd_key);
            *res_lo=dtaptr_lo;
            *res_hi=dtaptr_hi;
            return;
        }

        if(chldptr==0||node_flags==1)
            goto err;

        fseek(file,globoffs+chldptr,SEEK_SET);
        search_btree(file,key,rkeylen,globoffs,res_lo,res_hi,casesense);
        return;
    }

    if(node_flags!=1)   /* node not leaf */
    {
        fread(&chldptr,sizeof(uint32_t),1,file);
        chldptr=letoh32(chldptr);
        if(chldptr==0)   /* leaf */
            goto err;

        fseek(file,globoffs+chldptr,SEEK_SET);
        search_btree(file,key,rkeylen,globoffs,res_lo,res_hi,casesense);
    }

    return;
err:
    *res_lo=*res_hi=0;
}
