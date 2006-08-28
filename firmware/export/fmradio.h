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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

/* returns the IN flag */
#define FMRADIO_IN_SCREEN(s)        ((s) & FMRADIO_IN_FLAG)
#define FMRADIO_STATUS_PLAYING(s)   ((s) & FMRADIO_PLAYING_OUT)
#define FMRADIO_STATUS_PAUSED(s)    ((s) & FMRADIO_PAUSED_OUT)

extern int  get_radio_status(void);

extern int fmradio_read(int addr);
extern void fmradio_set(int addr, int data);

#endif
