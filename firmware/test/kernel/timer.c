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
#include "sh7034.h"
#include "system.h"
#include "debug.h"

void tick_start(unsigned int interval_in_ms)
{
    unsigned int count;

    count = FREQ / 1000 / 8 * interval_in_ms;

    if(count > 0xffff)
    {
	debugf("Error! The tick interval is too long (%d ms)\n",
	       interval_in_ms);
	return;
    }
    
    /* We are using timer 0 */
    
    TSTR &= ~0x01; /* Stop the timer */
    TSNC &= ~0x01; /* No synchronization */
    TMDR &= ~0x01; /* Operate normally */

    TCNT0 = 0;   /* Start counting at 0 */
    GRA0 = 0xfff0;
    TCR0 = 0x23; /* Clear at GRA match, sysclock/8 */

    TSTR |= 0x01; /* Start timer 1 */

    /* Enable interrupt on level 1 */
    IPRC = (IPRC & ~0x00f0) | 0x0010;

    TIER0 |= 0x01; /* Enable GRA match interrupt */
    
    while(1)
    {
    }
}

#pragma interrupt
void IMIA0(void)
{
    TSR0 &= ~0x01;
    debugf("Yes\n");
}
