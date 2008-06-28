/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Driver to control the (Samsung) tuner via port-banging SPI
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#ifndef FMRADIO_H
#define FMRADIO_H

/** declare some stuff here so powermgmt.c can properly tell if the radio is
    actually playing and not just paused. This break in heirarchy is allowed
    for audio_status(). **/

/* set when radio is playing or paused within fm radio screen */
#define FMRADIO_OFF         0x0
#define FMRADIO_PLAYING     0x1
#define FMRADIO_PAUSED      0x2

extern int  get_radio_status(void);

extern int fmradio_read(int addr);
extern void fmradio_set(int addr, int data);

#endif
