/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include "sh7034.h"
#include "led.h"
#include "system.h"

void led(bool on)
{
#ifdef ASM_IMPLEMENTATION
    if ( on )
        asm("or.b"  "\t" "%0,@(r0,gbr)" : : "I"(0x40), "z"(PBDR_ADDR+1));
    else
        asm("and.b" "\t" "%0,@(r0,gbr)" : : "I"(~0x40), "z"(PBDR_ADDR+1));
#else
    if ( on )
    {
        __set_bit_constant(6, &PBDRL);
    }
    else
    {
        __clear_bit_constant(6, &PBDRL);
    }
#endif
}
