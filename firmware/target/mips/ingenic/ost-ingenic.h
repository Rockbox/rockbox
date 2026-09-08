/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> >    <
 *   Player     |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#ifndef __OST_INGENIC_H__
#define __OST_INGENIC_H__

#include "ingenic-soc.h"

/* Start OST2 before delays; keep its prescaler consistent with system init. */
static inline void init_ost(void)
{
    jz_writef(CPM_CLKGR, OST(0));
    jz_writef(OST_CTRL, PRESCALE2_V(BY_4));
    jz_overwritef(OST_CLEAR, OST2(1));
    jz_write(OST_2CNTH, 0);
    jz_write(OST_2CNTL, 0);
    jz_setf(OST_ENABLE, OST2);
}

#endif /* __OST_INGENIC_H__ */
