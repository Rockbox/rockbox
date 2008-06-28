/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: irivertools.h
 *
 * Copyright (C) 2007 Dominik Wenger
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

#ifndef CHECKSUMS_H
#define CHECKSUMS_H

/* precalculated checksums for H110/H115 */
static struct sumpairs h100pairs[] = {
#include "h100sums.h"
};

/* precalculated checksums for H120/H140 */
static struct sumpairs h120pairs[] = {
#include "h120sums.h"
};

/* precalculated checksums for H320/H340 */
static struct sumpairs h300pairs[] = {
#include "h300sums.h"
};

#endif
