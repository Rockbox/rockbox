/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: rtc_as3514.c 12131 2007-01-27 20:48:48Z dan_a $
 *
 * Copyright (C) 2007 by Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "spi.h"
#include "rtc.h"
#include <stdbool.h>

void rtc_init(void)
{
}

int rtc_read_datetime(unsigned char* buf)
{
    char command = 0x04; /* burst read from the start of the time/date reg */
    spi_block_transfer(SPI_target_RX5X348AB,
                       &command, 1, buf, 7);
    return 1;
}
