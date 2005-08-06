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
#include "debug.h"
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

/* Definition of a playback configuration to start with */
#define NUM_DEFAULT_REGS 10
unsigned tlv320_defaults[2*NUM_DEFAULT_REGS] =
{
    REG_PC,         PC_ON | PC_OSC | PC_CLK | PC_DAC | ~PC_OUT,     /* do we need to enable osciliator and clock? */
    REG_LLIV,       LLIV_LIM,                                       /* mute adc input */
    REG_RLIV,       RLIV_RIM,                                       /* mute adc input */
    REG_LHV,        LHV_LHV(HEADPHONE_MUTE),                        /* mute headphone */
    REG_RHV,        RHV_RHV(HEADPHONE_MUTE),                        /* mute headphone */
    REG_AAP,        AAP_MICM,                                       /* mute microphone */
    REG_DAP,        DAP_DEEMP_DIS,                                  /* de-emphasis control: disabled */
    REG_DAIF,       DAIF_FOR_I2S | DAIF_IWL_24 | ~DAIF_MS,          /* i2s with 24 bit data len and slave mode */
    REG_SRC,        0,                                              /* ToDo */
    REG_DIA,        DIA_ACT,                                        /* activate digital interface */
};
unsigned tlv320_regs[0xf];

void tlv320_write_reg(unsigned reg, unsigned value)
{
    unsigned data[3];

    data[0] = TLV320_ADDR;
    data[1] = reg << 1;
    data[2] = value & 0xff; 

    if (i2c_write(1, data, 3) != 3)
    {
        DEBUGF("tlv320 error reg=0x%x", reg);
        return;
    } 

    tlv320_regs[reg] = value;
}

/* Returns 0 if successful or -1 if some register failed */
void tlv320_set_regs()
{
    int i;
    memset(tlv320_regs, 0, sizeof(tlv320_regs));

    /* Initialize all registers */
    for (i=0; i<NUM_DEFAULT_REGS; i++)
    {
        unsigned reg = tlv320_defaults[i*2+0];
        unsigned value = tlv320_defaults[i*2+1];

        tlv320_write_reg(reg, value);
    }
}

/* public functions */

/**
 * Init our tlv with default values
 */
void tlv320_init()
{
    tlv320_reset();
    tlv320_set_regs();
}

/**
 * Resets tlv320 to default values
 */
void tlv320_reset()
{
    tlv320_write_reg(REG_RR, RR_RESET):
}

void tlv320_enable_output(bool enable)
{
    unsigned value = tlv320regs[REG_PC];

    if (enable)
        value |= PC_OUT;
    else
        value &= ~PC_OUT;

    tlv320_write_reg(REG_PC, value);
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
    value_l |= LHV_LHV(vol_l);
    value_r |= LHV_LHV(vol_r);

    /* update */
    tlv320_write_reg(REG_LHV, value_l);
    tlv320_write_reg(REG_RHV, value_r);
}

/**
 * Sets left and right linein volume  (31(max) to 0(muted))
 */
void tlv320_set_linein_vol(int vol_l, int vol_r)
{
    unsigned value_l = tlv320regs[REG_LLIV];
    unsigned value_r = tlv320regs[REG_RLIV];

    value_l |= LLIV_LHV(vol_l);
    value_r |= RLIV_RHV(vol_r);

    tlv320_write_reg(REG_LLIV, value_l);
    tlv320_write_reg(REG_RLIV, value_r);
}

/**
 * Mute (mute=true) or enable sound (mute=false)
 *
 */
void tlv320_mute(bool mute)
{
    unsigned value_l = tlv320regs[REG_LHV];
    unsigned value_r = tlv320regs[REG_RHV];

    if (mute)
    {
        value_l |= LHV_LHV(HEADPHONE_MUTE);
        value_r |= RHV_RHV(HEADPHONE_MUTE);
    }
    else
    {
        value_l |= LHV_LHV(tlv320.vol_l);
        value_r |= RHV_RHV(tlv320.vol_r);
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
    unsigned value_pc = tlv320regs[REG_PC];
    unsigned value_aap = tlv320regs[REG_AAP];

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
    unsigned value = tlv320regs[REG_PC];

    /* powerdown mic, linein and adc */
    value &= ~(PC_MIC | PC_LINE | PC_ADC);

    /* powerdown mic, linein and adc */
    tlv320_write_reg(REG_PC, value);
}