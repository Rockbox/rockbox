/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "cpu.h"
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "ata-target.h"
#include "hwcompat.h"

volatile unsigned char* ata_control;
int ata_io_address; /* 0x300 or 0x200 */

void ata_reset(void)
{
    /* state HRR0 */
    and_b(~0x02, &PADRH); /* assert _RESET */
    sleep(1); /* > 25us */

    /* state HRR1 */
    or_b(0x02, &PADRH); /* negate _RESET */
    sleep(1); /* > 2ms */
}

void ata_address_detect(void)
{
    if (read_hw_mask() & ATA_ADDRESS_200)
    {
        ata_io_address = 0x200; /* For debug purposes only */
        ata_control = ATA_CONTROL1;
    }
    else
    {
        ata_io_address = 0x300; /* For debug purposes only */
        ata_control = ATA_CONTROL2;
    }
}

void ata_enable(bool on)
{
    if(on)
        and_b(~0x80, &PADRL); /* enable ATA */
    else
        or_b(0x80, &PADRL); /* disable ATA */

    or_b(0x80, &PAIORL);
}

void ata_device_init(void)
{
    or_b(0x02, &PAIORH); /* output for ATA reset */
    or_b(0x02, &PADRH);  /* release ATA reset */
    PACR2 &= 0xBFFF; /* GPIO function for PA7 (IDE enable) */
}

bool ata_is_coldstart(void)
{
    return (PACR2 & 0x4000) != 0;
}
