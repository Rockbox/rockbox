/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
*
* Copyright (C) 2009 by Michael Sevakis
*
* Miscellaneous compiler support routines.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied
*
****************************************************************************/
#include "plugin.h"

#if defined(ARM_NEED_DIV0)
void __attribute__((naked)) __div0(void)
{
    asm volatile("bx %0" : : "r"(rb->__div0));
}

#if defined(CPU_ARM_MICRO)
void __aeabi_idiv0(void) __attribute__((alias("__div0")));
void __aeabi_ldiv0(void) __attribute__((alias("__div0")));
#endif
#endif

#if defined(USE_STACK_PROTECTOR)
const uint32_t __stack_chk_guard = 0x3BADC0DE;

void __stack_chk_fail(void)
{
    rb->panicf("plugin smashed stack");
}
#endif

void *memcpy(void *dest, const void *src, size_t n)
{
    return rb->memcpy(dest, src, n);
}

void *memset(void *dest, int c, size_t n)
{
    return rb->memset(dest, c, n);
}

void *memmove(void *dest, const void *src, size_t n)
{
    return rb->memmove(dest, src, n);
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    return rb->memcmp(s1, s2, n);
}
