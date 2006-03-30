/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Christian Gmeiner
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
#include "logf.h"
#include "system.h"
#include "sprintf.h"
#include "button.h"
#include "string.h"
#include "file.h"
#include "buffer.h"

#include "i2c-coldfire.h"
#include "tlv320.h"

/* local functions and definations */
#define TLV320_ADDR 0x34

struct tlv320_info
{
    int vol_l;
    int vol_r;
} tlv320;

/* Shadow registers */
unsigned tlv320_regs[0xf];

void tlv320_write_reg(unsigned reg, unsigned value)
{
    unsigned char data[3];

    data[0] = TLV320_ADDR;
    /* The register address is the high 7 bits and the data the low 9 bits */
    data[1] = (reg << 1) | ((value >> 8) & 1);
    data[2] = value & 0xff; 

    if (i2c_write(1, data, 3) != 3)
    {
        logf("tlv320 error reg=0x%x", reg);
        return;
    } 

    tlv320_regs[reg] = value;
}

/* public functions */

/**
 * Init our tlv with default values
 */
void tlv320_init(void)
{
    memset(tlv320_regs, 0, sizeof(tlv320_regs));

    /* Initialize all registers */

    tlv320_write_reg(REG_PC, 0x00); /* All ON */
    tlv320_set_linein_vol(0, 0);
    tlv320_mute(true);
    tlv320_write_reg(REG_AAP, AAP_DAC|AAP_MICM);
    tlv320_write_reg(REG_DAP, 0x00);  /* No deemphasis */
    tlv320_write_reg(REG_DAIF, DAIF_IWL_16|DAIF_FOR_I2S);
    tlv320_set_headphone_vol(0, 0);
    tlv320_write_reg(REG_DIA, DIA_ACT);
    tlv320_write_reg(REG_SRC, (8 << 2)); /* 44.1kHz */
}

/**
 * Resets tlv320 to default values
 */
void tlv320_reset(void)
{
    tlv320_write_reg(REG_RR, RR_RESET);
}

/**
 * Sets left and right headphone volume  (127(max) to 48(muted))
 */
void tlv320_set_headphone_vol(int vol_l, int vol_r)
{
    unsigned value_l = tlv320_regs[REG_LHV];
    unsigned value_r = tlv320_regs[REG_RHV];

    /* keep track of current setting */
    tlv320.vol_l = vol_l;
    tlv320.vol_r = vol_r;

    /* set new values in local register holders */
    value_l = LHV_LHV(vol_l);
    value_r = RHV_RHV(vol_r);

    /* update */
    tlv320_write_reg(REG_LHV, value_l);
    tlv320_write_reg(REG_RHV, value_r);
}

/**
 * Sets left and right linein volume  (31(max) to 0(muted))
 */
void tlv320_set_linein_vol(int vol_l, int vol_r)
{
    unsigned value_l = tlv320_regs[REG_LLIV];
    unsigned value_r = tlv320_regs[REG_RLIV];

    value_l |= LLIV_LIV(vol_l);
    value_r |= RLIV_RIV(vol_r);

    tlv320_write_reg(REG_LLIV, value_l);
    tlv320_write_reg(REG_RLIV, value_r);
}

/**
 * Mute (mute=true) or enable sound (mute=false)
 *
 */
void tlv320_mute(bool mute)
{
    unsigned value_l = tlv320_regs[REG_LHV];
    unsigned value_r = tlv320_regs[REG_RHV];

    if (mute)
    {
        value_l = LHV_LHV(HEADPHONE_MUTE);
        value_r = RHV_RHV(HEADPHONE_MUTE);
    }
    else
    {
        value_l = LHV_LHV(tlv320.vol_l);
        value_r = RHV_RHV(tlv320.vol_r);
    }

    tlv320_write_reg(REG_LHV, value_r);
    tlv320_write_reg(REG_RHV, value_r);
}

void tlv320_close()
{
    /* todo */
}

void tlv320_enable_recording(bool source_mic)
{
    unsigned value_pc = tlv320_regs[REG_PC];
    unsigned value_aap = tlv320_regs[REG_AAP];

    /* select source*/
    if (source_mic)
    {
        value_aap &= ~AAP_INSEL;
        value_pc |= PC_MIC;
    }
    else
    {
        value_aap |= AAP_INSEL;
        value_pc |= PC_LINE;
    }

    /* poweron adc */
    value_pc |= PC_ADC;

    tlv320_write_reg(REG_AAP, value_aap);
    tlv320_write_reg(REG_PC, value_pc);
}

void tlv320_disable_recording()
{
    unsigned value = tlv320_regs[REG_PC];

    /* powerdown mic, linein and adc */
    value &= ~(PC_MIC | PC_LINE | PC_ADC);

    /* powerdown mic, linein and adc */
    tlv320_write_reg(REG_PC, value);
}
