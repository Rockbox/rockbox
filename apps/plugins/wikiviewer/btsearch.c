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

#include "plugin.h"
#include "shared/btsearch.h"
#include <ctype.h>
#define KEY_MAXLEN 512

static size_t sf_read(int fd, void *buf, size_t count)
{
    ssize_t n;

    do {
        n = rb->read(fd, buf, count);
    } while (n < 0&&n!=-1);

    return n;
}

static signed char utf8strcnmp(const unsigned char *s1, const unsigned char *s2,
                               uint16_t n1,uint16_t n2,const bool casesense)
{
    unsigned short c1,c2;
    const unsigned char *s1p,*s2p;

    s1p=s1;
    s2p=s2;

    for(;; )
    {
        if(s1p-s1==n1)
        {
            if(n1==n2&&n2==s2p-s2)
                return 0;
            else
                return -1;
        }

        if(s2p-s2==n2)
        {
            if(n1==n2&&n1==s1p-s1)
                return 0;
            else
                return 1;
        }

        s1p=rb->utf8decode(s1p,&c1);
        s2p=rb->utf8decode(s2p,&c2);

        if(c1==' ') c1='_';

        if(c2==' ') c2='_';

        if(!casesense && c1<128&&c2<128)
        {
            c1=tolower(c1);
            c2=tolower(c2);
        }

        if(c1<c2)
            return -1;
        else if (c1>c2)
            return 1;
    }

    return 0;
}

void search_btree(void *fp,const char* key,uint16_t rkeylen, uint32_t globoffs,
                  uint32_t* res_lo,uint32_t* res_hi,const bool casesense)
{
    unsigned char nd_key[KEY_MAXLEN];
    uint8_t node_flags;
    uint16_t node_nr_active,i,keylen;
    uint32_t chldptr;
    uint32_t dtaptr_lo,dtaptr_hi;
    int file = (int)fp;

    sf_read(file,&node_flags,1);
    sf_read(file,&node_nr_active,2);
    node_nr_active=letoh16(node_nr_active);

    if(node_nr_active<1)   /* error */
    {
        *res_lo=*res_hi=0;
        return;
    }

    for(i=0; i<node_nr_active; i++)
    {
        sf_read(file, &dtaptr_lo,4);
        dtaptr_lo=letoh32(dtaptr_lo);
        sf_read(file, &dtaptr_hi,4);
        dtaptr_hi=letoh32(dtaptr_hi);
        sf_read(file, &chldptr,4);
        chldptr=letoh32(chldptr);
        sf_read(file, &keylen,2);
        keylen=letoh16(keylen);
        sf_read(file, &nd_key,(keylen<KEY_MAXLEN) ? keylen : KEY_MAXLEN);
        nd_key[keylen]=0;

        if(rb->strlen(nd_key)!=keylen)
        {
            *res_lo=*res_hi=0;
            LOGF("WrongKL\n");
            return;
        }

        if(keylen-KEY_MAXLEN>0)
            rb->lseek(file,keylen-((keylen<KEY_MAXLEN) ? keylen : KEY_MAXLEN),1);

        keylen=(keylen<KEY_MAXLEN) ? keylen : KEY_MAXLEN;
        nd_key[keylen]=0;
        LOGF("K:%d,%s",utf8strcnmp(((const unsigned char*)key),
                                   ((const unsigned char*)nd_key),rkeylen,
                                   keylen,casesense),nd_key);

        if(utf8strcnmp(((const unsigned char*)key),
                       ((const unsigned char*)nd_key),rkeylen,
                       keylen,casesense)>0)
            continue;

        if(utf8strcnmp(((const unsigned char*)key),
                       ((const unsigned char*)nd_key),rkeylen,
                       keylen,casesense)==0)
        {
            *res_lo=dtaptr_lo;
            *res_hi=dtaptr_hi;
            return;
        }

        if(chldptr==0||node_flags==1)
        {
            *res_lo=*res_hi=0;
            return;
        }

        rb->lseek(file,globoffs+chldptr,0);
        search_btree(fp,key,rkeylen,globoffs,res_lo,res_hi,casesense);
        return;
    }

    if(node_flags!=1)    /* node not leaf */
    {
        sf_read(file, &chldptr,4);
        chldptr=letoh32(chldptr);

        if(chldptr==0)   /*leaf */
        {
            *res_lo=*res_hi=0;
            return;
        }

        rb->lseek(file,globoffs+chldptr,0);
        search_btree(fp,key,rkeylen,globoffs,res_lo,res_hi,casesense);
        return;
    }

    *res_lo=*res_hi=0;
}
