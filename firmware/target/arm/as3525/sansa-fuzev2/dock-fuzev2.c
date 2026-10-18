/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Sansa Fuze v2 dock: detection, line out volume
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
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "button.h"
#include "sound.h"
#include "generic_i2c.h"
#include "as3514.h"
#include "dock-target.h"

/* Everything here follows the OF (02.03.33):
 *
 * - Dock presence is sensed on the SSP RX line: the OF clocks a 4-bit frame
 *   out of the (otherwise unused) SSP and reads back 0 when a dock is
 *   attached. Four equal samples in a row change the state.
 * - The dock has a volume control at I2C address 0x9a on a bit-banged bus
 *   (B6 = SCL, B7 = SDA). It takes a single byte, 0xa0 | level (0..31).
 * - While docked, the codec headphone amp is switched to line out mode at a
 *   fixed level, see as3514.c.
 *
 * The dock also reports remote key presses (A1 irq, byte read from 0x60);
 * that is not implemented yet. */

#define DOCK_I2C_ADDR       0x9a
#define DOCK_VOLUME_CMD     0xa0
#define DOCK_VOLUME_MAX     31

#define DOCK_SCL_PIN        6
#define DOCK_SDA_PIN        7

#define DOCK_DEBOUNCE_MASK  0xf

#define SSP_SR_RNE          (1<<2)
#define SSP_SR_BSY          (1<<4)

static int dock_i2c_bus;
static uint8_t dock_samples;
static bool dock_present = false;

static void dock_scl_dir(bool out)
{
    if (out)
        GPIOB_DIR |= 1 << DOCK_SCL_PIN;
    else
        GPIOB_DIR &= ~(1 << DOCK_SCL_PIN);
}

static void dock_sda_dir(bool out)
{
    if (out)
        GPIOB_DIR |= 1 << DOCK_SDA_PIN;
    else
        GPIOB_DIR &= ~(1 << DOCK_SDA_PIN);
}

static void dock_scl_out(bool level)
{
    GPIOB_PIN(DOCK_SCL_PIN) = level ? 1 << DOCK_SCL_PIN : 0;
}

static void dock_sda_out(bool level)
{
    GPIOB_PIN(DOCK_SDA_PIN) = level ? 1 << DOCK_SDA_PIN : 0;
}

static bool dock_scl_in(void)
{
    return GPIOB_PIN(DOCK_SCL_PIN);
}

static bool dock_sda_in(void)
{
    return GPIOB_PIN(DOCK_SDA_PIN);
}

static void dock_delay(int delay)
{
    if (delay != 0)
        udelay(delay);
}

static const struct i2c_interface dock_i2c_interface = {
    .scl_out = dock_scl_out,
    .scl_dir = dock_scl_dir,
    .sda_out = dock_sda_out,
    .sda_dir = dock_sda_dir,
    .sda_in = dock_sda_in,
    .scl_in = dock_scl_in,
    .delay = dock_delay,

    /* the OF waits about 4us per phase */
    .delay_hd_sta = 4,
    .delay_hd_dat = 4,
    .delay_su_dat = 4,
    .delay_su_sto = 4,
    .delay_su_sta = 4,
    .delay_thigh = 4
};

/* Returns true if the dock pulls the SSP RX line low */
static bool dock_sample(void)
{
    int timeout = 1000;

    while (SSP_SR & SSP_SR_RNE)
        (void)SSP_DATA;

    SSP_DATA = 0;
    while ((SSP_SR & SSP_SR_BSY) && --timeout)
        ;

    if (!(SSP_SR & SSP_SR_RNE))
        return false;

    return (SSP_DATA & 0xf) == 0;
}

static void dock_tick(void)
{
    dock_samples = (dock_samples << 1) | dock_sample();

    if ((dock_samples & DOCK_DEBOUNCE_MASK) == DOCK_DEBOUNCE_MASK)
        dock_present = true;
    else if ((dock_samples & DOCK_DEBOUNCE_MASK) == 0)
        dock_present = false;
}

bool lineout_inserted(void)
{
    return dock_present;
}

/* Called by the codec driver while line out is in use, volume in tenth dB */
void as3543_lineout_volume(int vol_l, int vol_r)
{
    int vol = MAX(vol_l, vol_r);
    int min = sound_min(SOUND_VOLUME) * 10;
    int max = sound_max(SOUND_VOLUME) * 10;
    int level = 0;
    unsigned char cmd;

    if (vol > min)
        level = ((vol - min) * DOCK_VOLUME_MAX + (max - min) / 2) / (max - min);
    if (level > DOCK_VOLUME_MAX)
        level = DOCK_VOLUME_MAX;

    cmd = DOCK_VOLUME_CMD | level;
    i2c_write_data(dock_i2c_bus, DOCK_I2C_ADDR, -1, &cmd, 1);
}

void dock_init(void)
{
    int i;

    /* bus idle: both lines driven high */
    dock_scl_dir(true);
    dock_sda_dir(true);
    dock_scl_out(1);
    dock_sda_out(1);
    dock_i2c_bus = i2c_add_node(&dock_i2c_interface);

    /* SSP: 4-bit frames, only used to sample the RX line */
    bitset32(&CGU_PERI, CGU_SSP_CLOCK_ENABLE);
    SSP_CR1 = 0;
    SSP_CPSR = 2;
    SSP_CR0 = 3;
    SSP_CR1 = 2;

    for (i = 0; i < 4; i++)
        dock_tick();

    tick_add_task(dock_tick);
}
