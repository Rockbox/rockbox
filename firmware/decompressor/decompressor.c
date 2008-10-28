/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Jens Arnold
 *
 * Self-extracting firmware loader to work around the 200KB size limit
 * for archos player and recorder v1
 * Decompresses a built-in UCL-compressed image (method 2e) and executes it.
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

#include "uclimage.h"

#define ICODE_ATTR __attribute__ ((section (".icode")))

/* Symbols defined in the linker script */
extern char iramcopy[], iramstart[], iramend[];
extern char stackend[];
extern char loadaddress[], dramend[];

/* Prototypes */
extern void start(void);

void main(void) ICODE_ATTR;
int ucl_nrv2e_decompress_8(const unsigned char *src, unsigned char *dst,
                           unsigned long *dst_len) ICODE_ATTR;

/* Vector table */
void (*vbr[]) (void) __attribute__ ((section (".vectors"))) =
{
    start, (void *)stackend,
    start, (void *)stackend,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

/** All subsequent functions are executed from IRAM **/

#define ALIGNED_IMG_SIZE ((sizeof(image) + 3) & ~3)
/* This will never return */
void main(void)
{
    unsigned long dst_len; /* dummy */
    unsigned long *src = (unsigned long *)image;
    unsigned long *dst = (unsigned long *)(dramend - ALIGNED_IMG_SIZE);
    
    do
        *dst++ = *src++;
    while (dst < (unsigned long *)dramend);
    
    ucl_nrv2e_decompress_8(dramend - ALIGNED_IMG_SIZE, loadaddress, &dst_len);

    asm(
        "mov.l   @%0+,r0     \n"
        "jmp     @r0         \n"
        "mov.l   @%0+,r15    \n"
        : : "r"(loadaddress) : "r0"
    );
}
