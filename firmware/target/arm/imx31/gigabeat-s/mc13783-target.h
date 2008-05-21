/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2008 by Michael Sevakis
 *
 * Gigabeat S mc13783 event descriptions
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef MC13783_TARGET_H
#define MC13783_TARGET_H

/* Declare event indexes in priority order in a packed array */
enum mc13783_event_ids
{
    MC13783_ADCDONE_EVENT = 0, /* ADC conversion complete */
    MC13783_ONOFD1_EVENT,      /* Power button */
#ifdef HAVE_HEADPHONE_DETECTION
    MC13783_ONOFD2_EVENT,      /* Headphone jack */
#endif
    MC13783_CHGDET_EVENT,      /* Charger detection */
    MC13783_USB4V4_EVENT,      /* USB insertion */
};

#endif /* MC13783_TARGET_H */
