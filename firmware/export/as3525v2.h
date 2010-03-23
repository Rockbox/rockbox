/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2009 Rafaël Carré
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

#ifndef __AS3525V2_H__
#define __AS3525V2_H__

#include "as3525.h"

/* insert differences here */

#ifndef IRAM_SIZE   /* protect in case the define name changes */
#   error IRAM_SIZE not defined !
#endif
#undef IRAM_SIZE
#define IRAM_SIZE 0x100000

#define CGU_SDSLOT         (*(volatile unsigned long *)(CGU_BASE + 0x3C))

#endif /* __AS3525V2_H__ */
