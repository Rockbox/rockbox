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

#include <ctype.h>
#include "utf8_aux.h"

char utf8strcnmp(const unsigned char *s1, const unsigned char *s2,uint16_t n1,uint16_t n2, const bool casesense)
{
    unsigned short c1,c2;
    const unsigned char *s1p,*s2p;
    s1p=s1;
    s2p=s2;
    for(;; )
    {
        if(s1p-s1==n1)
        {
            if(n1==n2&&n2==s2p-s2) return 0;
            else return -1;
        }

        if(s2p-s2==n2)
        {
            /* printf("N1:%u,N2:%u,s1p-s1:%d\n",n1,n2,s1p-s1); */
            if(n1==n2&&n1==s1p-s1) return 0;
            else return 1;
        }

        s1p=utf8decode(s1p,&c1);
        s2p=utf8decode(s2p,&c2);
        if(c1==' ') c1='_';

        if(c2==' ') c2='_';

        /* if(s1p-s1==1&&s2p-s2==1&&c1<128&&c2<128){ */
        if(!casesense && c1<128&&c2<128)
        {
            /* printf("TLC\n); */
            c1=tolower(c1);
            c2=tolower(c2);
        }

        if(c1<c2) return -1;
        else if (c1>c2) return 1;
    }
    /* printf("CMPEND\n"); */
    return 0; /*won't happen*/
}

/* Decode 1 UTF-8 char and return a pointer to the next char. */
const unsigned char* utf8decode(const unsigned char *utf8, unsigned short *ucs)
{
    unsigned char c = *utf8++;
    unsigned long code;
    int tail = 0;

    if ((c <= 0x7f) || (c >= 0xc2))
    {
        /* Start of new character. */
        if (c < 0x80)          /* U-00000000 - U-0000007F, 1 byte */
            code = c;
        else if (c < 0xe0)     /* U-00000080 - U-000007FF, 2 bytes */
        {
            tail = 1;
            code = c & 0x1f;
        }
        else if (c < 0xf0)     /* U-00000800 - U-0000FFFF, 3 bytes */
        {
            tail = 2;
            code = c & 0x0f;
        }
        else if (c < 0xf5)     /* U-00010000 - U-001FFFFF, 4 bytes */
        {
            tail = 3;
            code = c & 0x07;
        }
        else
            /* Invalid size. */
            code = 0xfffd;

        while (tail-- && ((c = *utf8++) != 0))
        {
            if ((c & 0xc0) == 0x80)
                /* Valid continuation character. */
                code = (code << 6) | (c & 0x3f);
            else
            {
                /* Invalid continuation char */
                code = 0xfffd;
                utf8--;
                break;
            }
        }
    }
    else
        /* Invalid UTF-8 char */
        code = 0xfffd;

    /* currently we don't support chars above U-FFFF */
    *ucs = (code < 0x10000) ? code : 0xfffd;
    return utf8;
}
