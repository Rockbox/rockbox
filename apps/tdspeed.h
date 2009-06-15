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

#define TDSPEED_OUTBUFSIZE 4096

void tdspeed_init(void);
bool tdspeed_config(int samplerate, bool stereo, int factor);
long tdspeed_est_output_size(void);
long tdspeed_est_input_size(long size);
int tdspeed_doit(int32_t *src[], int count);

#define SPEED_MAX 250
#define SPEED_MIN 35

#endif
