/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Nicolas Pitre <nico@cam.org>
 * Copyright (C) 2006-2007 by Stéphane Doyon <s.doyon@videotron.ca>
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

#ifndef _TDSPEED_H
#define _TDSPEED_H

/* some #define functions to get the pitch, stretch and speed values based
 * upon two known values.  Remember that params are alphabetical. */
#define GET_SPEED(pitch, stretch) \
    ((pitch * stretch + PITCH_SPEED_100 / 2L) / PITCH_SPEED_100)
#define GET_PITCH(speed, stretch) \
    ((speed * PITCH_SPEED_100 + stretch / 2L) / stretch)
#define GET_STRETCH(pitch, speed) \
    ((speed * PITCH_SPEED_100 + pitch   / 2L) / pitch)

#define STRETCH_MAX (250L * PITCH_SPEED_PRECISION) /* 250% */
#define STRETCH_MIN (35L  * PITCH_SPEED_PRECISION) /* 35%  */

void dsp_timestretch_enable(bool enable);
void dsp_set_timestretch(int32_t percent);
int32_t dsp_get_timestretch(void);
bool dsp_timestretch_available(void);
void tdspeed_move(int i, void* current, void* new);

#endif /* _TDSPEED_H */
