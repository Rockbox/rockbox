/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Michal Sevakis
 * Based on the work of Thom Johansen
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
#ifndef SPDIF_H
#define SPDIF_H

#ifdef HAVE_SPDIF_POWER
#define IF_SPDIF_POWER_(...)    __VA_ARGS__
#else
#define IF_SPDIF_POWER_(...)
#endif

/* Initialize the S/PDIF driver */
void spdif_init(void);
/* Return the S/PDIF frequency in herz - unrounded */
unsigned long spdif_measure_frequency(void);
#ifdef HAVE_SPDIF_OUT
/* Set the S/PDIF audio feed - Use AUDIO_SRC_* values -
   will be off if not powered or !on */
void spdif_set_output_source(int source IF_SPDIF_POWER_(, bool on));
/* Return the last set S/PDIF audio source - literally the last value passed
   to spdif_set_monitor regardless of power state */
int spdif_get_output_source(bool *src_on);
#endif /* HAVE_SPDIF_OUT */

#endif /* SPDIF_H */
