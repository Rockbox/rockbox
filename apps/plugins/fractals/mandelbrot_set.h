/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Tomer Shalev
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
#ifndef _MANDELBROT_SET_H
#define _MANDELBROT_SET_H

#include "fractal_sets.h"

/* CPU stuff */
#if CONFIG_CPU == SH7034
#include "cpu_sh7043.h"
#elif defined CPU_COLDFIRE
#include "cpu_coldfire.h"
#elif defined CPU_ARM
#include "cpu_arm.h"
#endif

#if CONFIG_CPU == SH7034
#define MULS16_ASR10(a, b) muls16_asr10(a, b)
#define MULS32_ASR26(a, b) muls32_asr26(a, b)
#elif defined CPU_COLDFIRE
/* Needs the EMAC initialised to fractional mode w/o rounding and saturation */
#define MULS32_INIT() coldfire_set_macsr(EMAC_FRACTIONAL)
#define MULS16_ASR10(a, b) muls16_asr10(a, b)
#define MULS32_ASR26(a, b) muls32_asr26(a, b)
#elif defined CPU_ARM
#define MULS32_ASR26(a, b) muls32_asr26(a, b)
#endif

/* default macros */
#ifndef MULS16_ASR10
#define MULS16_ASR10(a, b) ((short)(((long)(a) * (long)(b)) >> 10))
#endif
#ifndef MULS32_ASR26
#define MULS32_ASR26(a, b) ((long)(((long long)(a) * (long long)(b)) >> 26))
#endif
#ifndef MULS32_INIT
#define MULS32_INIT()
#endif

extern struct fractal_ops mandelbrot_ops;

#endif
