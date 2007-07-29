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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
