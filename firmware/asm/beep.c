/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2011 Michael Sevakis
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
#if defined(CPU_ARM)
#include "arm/beep.c"
#elif defined (CPU_COLDFIRE)
#include "m68k/beep.c"
#else /* Generic */

/* Actually output samples into beep_buf */
static FORCE_INLINE void beep_generate(int16_t *out, unsigned long count,
                                       uint32_t *phase, uint32_t step)
{
    uint32_t ph = *phase;

    do
    {
        int16_t amp = ph > UINT32_MAX / 2 ? -INT16_MAX : INT16_MAX;

        *out++ = amp;
        *out++ = amp;

        ph += step;
    }
    while (--count);

    *phase = ph;
}

#define BEEP_GENERIC
#endif /* CPU_* */
