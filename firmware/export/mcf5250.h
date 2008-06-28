/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Christian Gmeiner
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
#ifndef __MCF5250_H__
#define __MCF5250_H__

#include "mcf5249.h"

/* here we remove stuff, which is not included in mfc5250 */
#undef DACR1
#undef DMR1
#undef INTERRUPTSTAT3
#undef INTERRUPTCLEAR3
#undef INTERRUPTEN3
#undef IPERRORADR

/* here we define some new stuff */

#define CSAR4 (*(volatile unsigned long *)(MBAR + 0x0b0))   /* Chip Select Address Register Bank 4  */
#define CSMR4 (*(volatile unsigned long *)(MBAR + 0x0b4))   /* Chip Select Mask Register Bank 4     */
#define CSCR4 (*(volatile unsigned long *)(MBAR + 0x0b8))   /* Chip Select Control Register Bank 4  */

#endif
