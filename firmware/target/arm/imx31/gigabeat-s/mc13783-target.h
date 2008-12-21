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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
    MC13783_SE1_EVENT,         /* Main charger detection */
    MC13783_USB_EVENT,         /* USB insertion */
};

#endif /* MC13783_TARGET_H */
