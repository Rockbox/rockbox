/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#ifndef _POWER_H_
#define _POWER_H_

#ifdef HAVE_CHARGE_CTRL
extern bool charger_enabled;
#endif

void power_init(void);
bool charger_inserted(void);
void charger_enable(bool on);
void ide_power_enable(bool on);
bool ide_powered(void);
void power_off(void);

#ifdef HAVE_SPDIF_POWER
void spdif_power_enable(bool on);
#endif

#ifdef CONFIG_TUNER
/* status values */
#define FMRADIO_OFF     0 /* switched off */
#define FMRADIO_POWERED 1 /* left powered, but idle */
#define FMRADIO_PLAYING 2 /* actively in use */
extern void radio_set_status(int status);
extern int radio_get_status(void);
#endif

#endif
