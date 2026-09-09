/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> >    <
 *   Player     |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Bounded spinning for X1600 hardware waits.
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
#ifndef __SPIN_X1600_H__
#define __SPIN_X1600_H__

#include <stdint.h>

/* Bound register polling by iterations so waits can terminate without a
 * working timebase. */
#define X1600_SPIN_GUARD 2000000u

/* Spin while cond is true. Returns nonzero if the iteration budget
 * expires. */
#define x1600_spin_while(cond)                                              \
    __extension__ ({                                                        \
        uint32_t _spin_guard = X1600_SPIN_GUARD;                            \
        while((cond) && --_spin_guard)                                      \
            ;                                                               \
        _spin_guard == 0;                                                   \
    })

/* Spin until `cond` becomes true. Non-zero if the guard expired. */
#define x1600_spin_until(cond) x1600_spin_while(!(cond))

/* Same, but run `body` each pass, for waits that must yield to the kernel.
 * Only usable once the scheduler is running -- not in the SPL. */
#define x1600_spin_while_do(cond, body)                                     \
    __extension__ ({                                                        \
        uint32_t _spin_guard = X1600_SPIN_GUARD;                            \
        while((cond) && --_spin_guard)                                      \
            { body; }                                                       \
        _spin_guard == 0;                                                   \
    })

#endif /* __SPIN_X1600_H__ */
