/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "pcf50606.h"

/* These voltages were determined by measuring the output of the PCF50606
   on a running H300, and verified by disassembling the original firmware */
static void set_voltages(void)
{
    static const unsigned char buf[5] =
    {
        0xf4,   /* IOREGC  = 2.9V, ON in all states  */
        0xef,   /* D1REGC  = 2.4V, ON in all states  */
        0x18,   /* D2REGC  = 3.3V, OFF in all states */
        0xf0,   /* D3REGC  = 2.5V, ON in all states  */
        0xef,   /* LPREGC1 = 2.4V, ON in all states  */
    };

    pcf50606_write_multiple(0x23, buf, 5);
}

void pcf50606_init(void)
{
    pcf50606_i2c_init();

    set_voltages();

    pcf50606_write(0x08, 0x60); /* Wake on USB and charger insertion */
    pcf50606_write(0x09, 0x05); /* USB and ON key debounce: 14ms */
    pcf50606_write(0x29, 0x1C); /* Disable the unused MBC module */

    pcf50606_write(0x35, 0x13); /* Backlight PWM = 512Hz 50/50 */
    pcf50606_write(0x3a, 0x3b); /* PWM output on GPOOD1 */
}
