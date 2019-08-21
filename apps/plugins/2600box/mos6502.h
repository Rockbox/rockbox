/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2019 Sebastian Leonhardt
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

#ifndef CPU_H
#define CPU_H

#include "rbconfig.h"

#define VERSION_CPU    "1.3"

struct cpu {
    /* Registers are kept in native int size for better performance. The emulator
       code must ensure that the registers never exceed 8 bit (or 16 for PC) */
    unsigned program_counter;
    unsigned accumulator;
    unsigned x_register;
    unsigned y_register;
    unsigned stack_pointer;
    /* Space to save the 6502's flags.
       NEVER access the flags directly, ALWAYS use the provided flag handling
       defines, because they are saved and handled in an optimized individual way! */
    /* Note that we don't save the status register, it's prepared on-the-fly
       from the flags. See flag handling macros below */
    BYTE carry_flag;
    BYTE zero_flag;
    BYTE interrupt_flag;
    BYTE decimal_flag;
    BYTE overflow_flag;
    BYTE negative_flag;
    /* Currently not used; space to save e.g. interrupt/break/halt etc. state */
    unsigned state;
};

extern struct cpu cpu;

void do_cpu(void);
void init_cpu(void);

#endif  /* CPU_H */
