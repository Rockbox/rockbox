/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Will Robertson
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
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "power.h"
#include "panic.h"
#include "ata.h"
#include "ata-target.h"
#include "clkctl-imx31.h"

static const struct ata_pio_timings
{
    uint16_t time_2w;       /* t2 during write */
    uint16_t time_2r;       /* t2 during read */
    uint8_t  time_1;        /* t1 */
    uint8_t  time_pio_rdx;  /* trd */
    uint8_t  time_4;        /* t4 */
    uint8_t  time_9;        /* t9 */
} pio_timings[5] =
{
    [0] =  /* PIO mode 0 */
    {
        .time_1 = 70,
        .time_2w = 290,
        .time_2r = 290,
        .time_4 = 30,
        .time_9 = 20
    },
    [1] = /* PIO mode 1 */
    {
        .time_1 = 50,
        .time_2w = 290,
        .time_2r = 290,
        .time_4 = 20,
        .time_9 = 15
    },
    [2] = /* PIO mode 2 */
    {
        .time_1 = 30,
        .time_2w = 290,
        .time_2r = 290,
        .time_4 = 15,
        .time_9 = 10
    },
    [3] = /* PIO mode 3 */
    {
        .time_1 = 30,
        .time_2w = 80,
        .time_2r = 80,
        .time_4 = 10,
        .time_9 = 10
    },
    [4] = /* PIO mode 4 */
    {
        .time_1 = 25,
        .time_2w = 70,
        .time_2r = 70,
        .time_4 = 10,
        .time_9 = 10
    },
};

static int pio_mode = 0; /* Setup mode 0 by default */

/* Setup the timing for PIO mode */
void ata_set_pio_timings(int mode)
{
    while (!(ATA_INTERRUPT_PENDING & ATA_CONTROLLER_IDLE));

    const struct ata_pio_timings * const timings = &pio_timings[mode];
    /* T = period in nanoseconds */
    int T = 1000 * 1000 * 1000 / imx31_clkctl_get_ata_clk();

    pio_mode = mode;

    ATA_TIME_OFF = 3;
    ATA_TIME_ON = 3;

    ATA_TIME_1 = (timings->time_1 + T) / T;
    ATA_TIME_2W = (timings->time_2w + T) / T;
    ATA_TIME_2R = (timings->time_2r + T) / T;
    ATA_TIME_AX = (35 + T) / T; /* tA */
    ATA_TIME_PIO_RDX = 1;
    ATA_TIME_4 = (timings->time_4 + T) / T;
    ATA_TIME_9 = (timings->time_9 + T) / T;
}

void ata_reset(void)
{
    /* Be sure we're not busy */
    while (!(ATA_INTERRUPT_PENDING & ATA_CONTROLLER_IDLE));

    ATA_INTF_CONTROL &= ~ATA_ATA_RST;
    sleep(1);
    ATA_INTF_CONTROL |= ATA_ATA_RST;
    sleep(1);

    while (!(ATA_INTERRUPT_PENDING & ATA_CONTROLLER_IDLE));
}

/* This function is called before enabling the USB bus */
void ata_enable(bool on)
{
    (void)on;
}

bool ata_is_coldstart(void)
{
    return true;
}

void ata_device_init(void)
{
    ATA_INTF_CONTROL |= ATA_ATA_RST; /* Make sure we're not in reset mode */

    /* mode may be switched later once identify info is ready in which
     * case the main driver calls back */
    ata_set_pio_timings(pio_mode);
}
