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
 * Gigabeat S GPIO interrupt event descriptions header
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef GPIO_TARGET_H
#define GPIO_TARGET_H

/* MC13783 GPIO pin info for this target */
#define MC13783_GPIO_NUM    GPIO1_NUM   
#define MC13783_GPIO_ISR    GPIO1_ISR
#define MC13783_GPIO_LINE   31

/* Declare event indexes in priority order in a packed array */
enum gpio_event_ids
{
    /* GPIO1 event IDs */
    MC13783_EVENT_ID = GPIO1_EVENT_FIRST,
    /* GPIO2 event IDs */
    /* none defined */
    /* GPIO3 event IDs */
    /* none defined */
};

void mc13783_event(void);

#endif /* GPIO_TARGET_H */
