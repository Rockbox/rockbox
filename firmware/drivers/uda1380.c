/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Andy Young
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "lcd.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "power.h"
#include "debug.h"
#include "system.h"
#include "sprintf.h"
#include "button.h"
#include "string.h"
#include "file.h"
#include "buffer.h"

#include "i2c-h100.h"
#include "uda1380.h"

/* ------------------------------------------------- */
/* Local functions and variables */
/* ------------------------------------------------- */

int uda1380_write_reg(unsigned char reg, unsigned short value);
unsigned short uda1380_regs[0x30];

/* Definition of a good (?) configuration to start with */
/* Not enabling ADC for now.. */

#define NUM_DEFAULT_REGS 13
unsigned short uda1380_defaults[2*NUM_DEFAULT_REGS] = 
{
        REG_0,        EN_DAC | EN_INT | EN_DEC | SYSCLK_256FS | WSPLL_25_50,
        REG_I2S,      I2S_IFMT_IIS,
        REG_PWR,      PON_PLL | PON_HP | PON_DAC | EN_AVC | PON_AVC | PON_BIAS,
        REG_AMIX,     AMIX_RIGHT(0x10) | AMIX_LEFT(0x10), /* 00=max, 3f=mute */
        REG_MASTER_VOL, MASTER_VOL_LEFT(0x7f) | MASTER_VOL_RIGHT(0x7f), /* 00=max, ff=mute */
        REG_MIX_VOL,    MIX_VOL_CHANNEL_1(0) | MIX_VOL_CHANNEL_2(0xff), /* 00=max, ff=mute */
        REG_EQ,         0,
        REG_MUTE,       MUTE_CH2, /* Mute channel 2 (digital decimation filter) */
        REG_MIX_CTL,    0,
        REG_DEC_VOL,    0,
        REG_PGA,        MUTE_ADC,
        REG_ADC,        SKIP_DCFIL,
        REG_AGC,        0
};

/* Returns 0 if register was written or -1 if write failed */
int uda1380_write_reg(unsigned char reg, unsigned short value)
{
    unsigned char data[4];

    data[0] = UDA1380_ADDR;
    data[1] = reg;
    data[2] = value >> 8;
    data[3] = value & 0xff;

    if (i2c_write(1, data, 4) != 4)
    {
        DEBUGF("uda1380 error reg=0x%x", reg);
        return -1;
    } 

    uda1380_regs[reg] = value;

    return 0;
}

/**
 * Sets the master volume
 *
 * \param vol Range [0..255] 0=max, 255=mute
 *
 */
int uda1380_setvol(int vol)
{
    return uda1380_write_reg(REG_MASTER_VOL,
                             MASTER_VOL_LEFT(vol) | MASTER_VOL_RIGHT(vol));
}

/**
 * Mute (mute=1) or enable sound (mute=0)
 *
 */
int uda1380_mute(int mute)
{
    unsigned int value = uda1380_regs[REG_MUTE];

    if (mute)
        value = value | MUTE_MASTER;
    else
        value = value & ~MUTE_MASTER;

    return uda1380_write_reg(REG_MUTE, value);
}

/* Returns 0 if successful or -1 if some register failed */
int uda1380_set_regs(void)
{
    int i;
    memset(uda1380_regs, 0, sizeof(uda1380_regs));

    /* Initialize all registers */
    for (i=0; i<NUM_DEFAULT_REGS; i++)
    {
        unsigned char reg = uda1380_defaults[i*2+0];
        unsigned short value = uda1380_defaults[i*2+1];

        if (uda1380_write_reg(reg, value) == -1)
            return -1;
    }

    return 0;
}

/* Initialize UDA1380 codec with default register values (uda1380_defaults) */
int uda1380_init(void)
{
    if (uda1380_set_regs() == -1)
        return -1;
    
    return 0;
}

/* Nice shutdown of UDA1380 codec */
void uda1380_close(void)
{
    uda1380_write_reg(REG_PWR, 0);  /* Disable power    */
    uda1380_write_reg(REG_0, 0);    /* Disable codec    */
}
