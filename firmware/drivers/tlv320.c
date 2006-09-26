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
#include "audio.h"

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
    unsigned char data[2];

    /* The register address is the high 7 bits and the data the low 9 bits */
    data[0] = (reg << 1) | ((value >> 8) & 1);
    data[1] = value; 

    if (i2c_write(I2C_IFACE_0, TLV320_ADDR, data, 2) != 2)
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

    /* All ON except OUT, ADC, MIC and LINE */
    tlv320_write_reg(REG_PC, PC_OUT | PC_ADC | PC_MIC | PC_LINE);
    tlv320_set_recvol(0, 0, AUDIO_GAIN_MIC);
    tlv320_set_recvol(0, 0, AUDIO_GAIN_LINEIN);
    tlv320_mute(true);
    tlv320_write_reg(REG_AAP, AAP_DAC | AAP_MICM);
    tlv320_write_reg(REG_DAP, 0x00);  /* No deemphasis */
    tlv320_write_reg(REG_DAIF, DAIF_IWL_16 | DAIF_FOR_I2S);
    tlv320_write_reg(REG_DIA, DIA_ACT);
    tlv320_write_reg(REG_SRC, (1 << 5)); /* 44.1kHz */
    /* All ON except ADC, MIC and LINE */
    tlv320_write_reg(REG_PC, PC_ADC | PC_MIC | PC_LINE);
}

/**
 * Resets tlv320 to default values
 */
void tlv320_reset(void)
{
    tlv320_write_reg(REG_RR, RR_RESET);
}

/**
 * Sets left and right headphone volume
 *
 * Left & Right: 48 .. 121 .. 127 => Volume -73dB (mute) .. +0 dB .. +6 dB
 */
void tlv320_set_headphone_vol(int vol_l, int vol_r)
{
    unsigned value_dap = tlv320_regs[REG_DAP];
    unsigned value_dap_last = value_dap;
    unsigned value_l = LHV_LHV(vol_l);
    unsigned value_r = RHV_RHV(vol_r);

    /* keep track of current setting */
    tlv320.vol_l = vol_l;
    tlv320.vol_r = vol_r;

    if (value_l > HEADPHONE_MUTE || value_r > HEADPHONE_MUTE)
        value_dap &= ~DAP_DACM;
    else
        value_dap |= DAP_DACM;

    /* update */
    tlv320_write_reg(REG_LHV, LHV_LZC | value_l);
    tlv320_write_reg(REG_RHV, RHV_RZC | value_r);
    if (value_dap != value_dap_last)
        tlv320_write_reg(REG_DAP, value_dap);
}

/**
 * Set recording volume
 *
 * Line in   : 0 .. 31 => Volume -34.5 .. +12 dB
 * Mic (left): 0 ..  1 => Volume  +0,     +20 dB
 *
 */
void tlv320_set_recvol(int left, int right, int type)
{
    if (type == AUDIO_GAIN_MIC)
    {
        unsigned value_aap = tlv320_regs[REG_AAP];
        
        if (left)
            value_aap |= AAP_MICB;  /* Enable mic boost (20dB) */
        else
            value_aap &= ~AAP_MICB;

        tlv320_write_reg(REG_AAP, value_aap);            
        
    }
    else if (type == AUDIO_GAIN_LINEIN)
    {
        tlv320_write_reg(REG_LLIV, LLIV_LIV(left));
        tlv320_write_reg(REG_RLIV, RLIV_RIV(right));    
    }
}

/**
 * Mute (mute=true) or enable sound (mute=false)
 *
 */
void tlv320_mute(bool mute)
{
    unsigned value_dap = tlv320_regs[REG_DAP];
    unsigned value_l, value_r;

    if (mute)
    {
        value_l = LHV_LHV(HEADPHONE_MUTE);
        value_r = RHV_RHV(HEADPHONE_MUTE);
        value_dap |= DAP_DACM;
    }
    else
    {
        value_l = LHV_LHV(tlv320.vol_l);
        value_r = RHV_RHV(tlv320.vol_r);
        if (value_l > HEADPHONE_MUTE || value_r > HEADPHONE_MUTE)
            value_dap &= ~DAP_DACM;
    }

    tlv320_write_reg(REG_LHV, LHV_LZC | value_l);
    tlv320_write_reg(REG_RHV, RHV_RZC | value_r);
    tlv320_write_reg(REG_DAP, value_dap);
}

/* Nice shutdown of TLV320 codec */
void tlv320_close()
{
    tlv320_write_reg(REG_PC, PC_OFF | PC_CLK | PC_OSC | PC_OUT |
        PC_DAC | PC_ADC | PC_MIC | PC_LINE);  /* All OFF */
}

void tlv320_enable_recording(bool source_mic)
{
    unsigned value_daif = tlv320_regs[REG_DAIF];
    unsigned value_aap, value_pc;
    
    if (source_mic)
    {
        /* select MIC and enable mic boost (20 dB) */
        value_aap = AAP_DAC | AAP_INSEL | AAP_MICB;   
        value_pc = PC_LINE;                 /* power down LINE */
    }
    else
    {
        value_aap = AAP_DAC | AAP_MICM;
        value_pc = PC_MIC;                  /* power down MIC */
    }

    tlv320_write_reg(REG_PC, value_pc);
    tlv320_write_reg(REG_AAP, value_aap);

    /* Enable MASTER mode (start sending I2S data to the CPU) */
    value_daif |= DAIF_MS;                   
    tlv320_write_reg(REG_DAIF, value_daif);
}
  
void tlv320_disable_recording()
{
    unsigned value_pc = tlv320_regs[REG_PC];
    unsigned value_aap = tlv320_regs[REG_AAP];
    unsigned value_daif = tlv320_regs[REG_DAIF];

    value_daif &= ~DAIF_MS;                /* disable MASTER mode */
    tlv320_write_reg(REG_DAIF, value_daif);
    
    value_aap |= AAP_MICM;                 /* mute MIC */
    tlv320_write_reg(REG_PC, value_aap);

    value_pc |= PC_ADC | PC_MIC | PC_LINE; /* ADC, MIC and LINE off */
    tlv320_write_reg(REG_PC, value_pc);    
}

void tlv320_set_monitor(bool enable)
{
    unsigned value_aap, value_pc;

    if (enable)
    {
        value_aap = AAP_BYPASS | AAP_MICM;
        value_pc = PC_DAC | PC_ADC | PC_MIC; /* DAC, ADC and MIC off */
    }
    else
    {
        value_aap = AAP_DAC | AAP_MICM;
        value_pc = PC_ADC | PC_MIC | PC_LINE; /* ADC, MIC and LINE off */
    }

    tlv320_write_reg(REG_AAP, value_aap);
    tlv320_write_reg(REG_PC,  value_pc);
}
