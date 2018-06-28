/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Linus Nielsen Feltzing
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
#include "config.h"

#if CONFIG_CPU == SH7034
#include "sh7034.h"
#endif
#if CONFIG_CPU == MCF5249
#include "mcf5249.h"
#endif
#if CONFIG_CPU == MCF5250
#include "mcf5250.h"
#endif
#if (CONFIG_CPU == PP5020) || (CONFIG_CPU == PP5022)
#include "pp5020.h"
#endif
#if CONFIG_CPU == PP5002
#include "pp5002.h"
#endif
#if CONFIG_CPU == PP5024
#include "pp5024.h"
#endif
#if CONFIG_CPU == PP6100
#include "pp6100.h"
#endif
#if CONFIG_CPU == PNX0101
#include "pnx0101.h"
#endif
#if CONFIG_CPU == S3C2440
#include "s3c2440.h"
#endif
#if CONFIG_CPU == DM320
#include "dm320.h"
#endif
#if CONFIG_CPU == IMX31L
#include "imx31l.h"
#endif
#ifdef CPU_TCC77X
#include "tcc77x.h"
#endif
#ifdef CPU_TCC780X
#include "tcc780x.h"
#endif
#if CONFIG_CPU == S5L8700 || CONFIG_CPU == S5L8701
#include "s5l8700.h"
#endif
#if CONFIG_CPU == S5L8702
#include "s5l8702.h"
#endif
#if CONFIG_CPU == JZ4732
#include "jz4740.h"
#endif
#if CONFIG_CPU == JZ4760B
#include "jz4760b.h"
#endif
#if CONFIG_CPU == AS3525
#include "as3525.h"
#endif
#if CONFIG_CPU == AS3525v2
#include "as3525v2.h"
#endif
#if CONFIG_CPU == IMX233
#include "imx233.h"
#endif
#if CONFIG_CPU == RK27XX
#include "rk27xx.h"
#endif
