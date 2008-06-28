/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Miika Pekkarinen
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
#include <string.h>
#include <inttypes.h>
#include "structec.h"
#include "system.h"
#include "file.h"

#define MAX_STRUCT_SIZE 128

/**
 * Convert the struct endianess with the instructions provided.
 * 
 * For example:
 * struct test {
 *     long par1;
 *     short par2;
 *     short par3;
 * };
 * 
 * structec_convert(instance_of_test, "lss", sizeof(struct test), true);
 * 
 * Structures to be converted must be properly padded.
 * 
 * @param structure Pointer to the struct being converted.
 * @param ecinst Instructions how to do the endianess conversion.
 * @param count Number of structures to write
 * @param enable Conversion is not made unless this is true.
 */
void structec_convert(void *structure, const char *ecinst, 
                      long count, bool enable)
{
    const char *ecinst_ring = ecinst;
    char *buf = (char *)structure;
    
    if (!enable)
        return;
    
    while (count > 0)
    {
        switch (*ecinst_ring)
        {
            /* Swap nothing. */
            case 'c':
            {
                buf++;
                break;
            }
            
            /* Swap 2 bytes. */
            case 's':
            {
                uint16_t *data = (uint16_t *)buf;
                *data = swap16(*data);
                buf += 2;
                break;
            }

            /* Swap 4 bytes. */
            case 'l':
            {
                uint32_t *data = (uint32_t *)buf;
                *data = swap32(*data);
                buf += 4;
                break;
            }
            
            /* Skip N bytes, idea taken from metadata.c */
            default:
            {
                if (isdigit(*ecinst_ring))
                    buf += (*ecinst_ring - '0');

                break;
            }
        }
        
        ecinst_ring++;
        if (*ecinst_ring == '\0')
        {
            ecinst_ring = ecinst;
            count--;
        }
    }
}

/**
 * Determines the size of a struct in bytes by using endianess correction
 * string format.
 * 
 * @param ecinst endianess correction string.
 * @return length of the struct in bytes.
 */
static size_t structec_size(const char *ecinst)
{
    size_t size = 0;
    
    do
    {
        switch (*ecinst)
        {
            case 'c': size += 1; break;
            case 's': size += 2; break;
            case 'l': size += 4; break;
            default: 
                if (isdigit(*ecinst))
                    size += (*ecinst - '0');
        }
    } while (*(++ecinst) != '\0');
    
    return size;
}

/**
 * Reads endianess corrected structure members from the given file.
 * 
 * @param fd file descriptor of the file being read.
 * @param buf endianess corrected data is placed here.
 * @param scount the number of struct members to read.
 * @param ecinst endianess correction string.
 * @param ec if true, endianess correction is enabled.
 */
ssize_t ecread(int fd, void *buf, size_t scount, const char *ecinst, bool ec)
{
    ssize_t ret;
    size_t member_size = structec_size(ecinst);
    
    ret = read(fd, buf, scount * member_size);
    structec_convert(buf, ecinst, scount, ec);
    
    return ret;
}

/**
 * Writes endianess corrected structure members to the given file.
 * 
 * @param fd file descriptor of the file being written to.
 * @param buf endianess corrected data is read here.
 * @param scount the number of struct members to write.
 * @param ecinst endianess correction string.
 * @param ec if true, endianess correction is enabled.
 */
ssize_t ecwrite(int fd, const void *buf, size_t scount, 
                const char *ecinst, bool ec)
{
    char tmp[MAX_STRUCT_SIZE];
    size_t member_size = structec_size(ecinst);
    
    if (ec)
    {
        const char *p = (const char *)buf;
        int maxamount = (int)(MAX_STRUCT_SIZE / member_size);
        int i;
        
        for (i = 0; i < (long)scount; i += maxamount)
        {
            long amount = MIN((int)scount-i, maxamount);
            
            memcpy(tmp, p, member_size * amount);
            structec_convert(tmp, ecinst, amount, true);
            write(fd, tmp, amount * member_size);
            p += member_size * amount;
        }
        
        return scount * member_size;
    }
    
    return write(fd, buf, scount * member_size);
}

