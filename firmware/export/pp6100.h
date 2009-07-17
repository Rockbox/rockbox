#ifndef __PP6100_H__
#define __PP6100_H__
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Robert Keevil
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

/* We believe this is quite similar to the 5020 and for now we just use that
   completely and redefine any minor differences */
#include "pp5020.h"

#undef DRAM_START
#define DRAM_START       0x10f00000

#define GPIOM_ENABLE     (*(volatile unsigned long *)(0x6000d180))
#define GPIOM_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d190))
#define GPIOM_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d1a0))
#define GPIOM_INPUT_VAL  (*(volatile unsigned long *)(0x6000d1b0))
#define GPIOM_INT_STAT   (*(volatile unsigned long *)(0x6000d1c0))
#define GPIOM_INT_EN     (*(volatile unsigned long *)(0x6000d1d0))
#define GPIOM_INT_LEV    (*(volatile unsigned long *)(0x6000d1e0))
#define GPIOM_INT_CLR    (*(volatile unsigned long *)(0x6000d1f0))

#define GPIOM 12

#endif
