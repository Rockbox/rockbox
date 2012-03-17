/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2012 by Amaury Pouly
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
#include "symbols.h"
#include "safe_read.h"

/* See -mpoke-function-name description
 * Limit function name search in the following ways:
 * - define the maximal function size (to limit backward search)
 * - define the min/max function name size (to limit sanity checks)
 */
#define MAX_FUNCTION_SIZE   32768
#define MIN_FUNCTION_NAME   4
#define MAX_FUNCTION_NAME   64

#define MAGIC_MAGIC(x)  ((x) & 0xff000000)
#define MAGIC_SIZE(x)   ((x) & 0x00ffffff)
#define MAGIC_VALUE     0xff000000

#ifdef POKE_FUNCTION_NAME
// return true if character c is valid at position idx in a function name
static bool is_valid_function_char(char c, int idx)
{
    return (idx > 0 && c >= '0' && c<= '9') || (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') || c == '_';
}
#endif

const char *sym_get_function_name(uint32_t addr)
{
#ifdef POKE_FUNCTION_NAME
    // align address
    addr = addr & ~3;
    // search backward until failure or success
    for(unsigned offset = 0; offset < MAX_FUNCTION_SIZE; offset += 4)
    {
        uint32_t magic;
        // stop if wrap
        if(offset > addr)
            break;
        // try to read
        if(!safe_read32((void *)(addr - offset), &magic))
            break;
        // check if it's magic
        if(MAGIC_MAGIC(magic) != MAGIC_VALUE)
            continue;
        unsigned size = MAGIC_SIZE(magic);
        // check wrap
        if(size > addr - offset)
            continue;
        /* size is rounded up to the next word so it's not the actual
         * string length but can't be more than length + '0' + 3 */
        if(size > MAX_FUNCTION_NAME + 4 || size < MIN_FUNCTION_NAME)
            continue;
        // read string and stop on any strange character, compute actual length
        char *p = (char *)(addr - offset - size);
        bool zero_found = false;
        unsigned length = 0;
        while(length < size)
        {
            uint8_t c;
            if(!safe_read8(&p[length], &c))
                goto Lbad_read;
            // zero ?
            if(c == 0)
            {
                zero_found = true;
                break;
            }
            // bad character ?
            if(!is_valid_function_char(c, length))
                goto Lbad_name;
            // goto next one
            length++;
        }
        // we must have found a zero
        if(!zero_found)
            continue;
        /* name looks correct, but check that the size is coherent with announced
         * one: the difference is at most 4 ('0' + 3) */
        if(size - length > 4)
            continue;
        // really check size now
        if(length < MIN_FUNCTION_NAME || length > MAX_FUNCTION_NAME)
            continue;
        // apparently we found it
        return p;

        Lbad_read:// bad read, skip (perhaps this is a false negative)
        Lbad_name:// bad string, skip
        continue;
    }
#endif
    return "";

}