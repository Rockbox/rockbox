/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "x1000-codec.h"
#include "audiohw.h"
#include "pcm_sampr.h"
#include "kernel.h"
#include "x1000/aic.h"

#define CAPS (SAMPR_CAP_192 | SAMPR_CAP_176 | \
              SAMPR_CAP_96  | SAMPR_CAP_88  | SAMPR_CAP_48 | \
              SAMPR_CAP_44  | SAMPR_CAP_32  | SAMPR_CAP_24 | \
              SAMPR_CAP_22  | SAMPR_CAP_16  | SAMPR_CAP_12 | \
              SAMPR_CAP_11  | SAMPR_CAP_8)

/* future proofing */
#if (defined(HAVE_X1000_ICODEC_PLAY) && (HW_SAMPR_CAPS & ~CAPS) != 0) || \
    (defined(HAVE_X1000_ICODEC_REC) && (REC_SAMPR_CAPS & ~CAPS) != 0)
# error "bad SAMPR_CAPS"
#endif

static const uint8_t play_fsel_to_hw[] = {
    HW_HAVE_192_(12,)
    HW_HAVE_176_(11,)
    HW_HAVE_96_(10,)
    HW_HAVE_88_(9,)
    HW_HAVE_48_(8,)
    HW_HAVE_44_(7,)
    HW_HAVE_32_(6,)
    HW_HAVE_24_(5,)
    HW_HAVE_22_(4,)
    HW_HAVE_16_(3,)
    HW_HAVE_12_(2,)
    HW_HAVE_11_(1,)
    HW_HAVE_8_(0,)
};

static const uint8_t rec_fsel_to_hw[] = {
    /* REC_HAVE_192_(12,)
       REC_HAVE_176_(11,) */
    REC_HAVE_96_(10,)
    REC_HAVE_88_(9,)
    REC_HAVE_48_(8,)
    REC_HAVE_44_(7,)
    REC_HAVE_32_(6,)
    REC_HAVE_24_(5,)
    REC_HAVE_22_(4,)
    REC_HAVE_16_(3,)
    REC_HAVE_12_(2,)
    REC_HAVE_11_(1,)
    REC_HAVE_8_(0,)
};

void x1000_icodec_open(void)
{
    /* Ingenic does not specify any timing constraints for reset,
     * let's do a 1ms delay for fun */
    jz_writef(AIC_RGADW, ICRST(1));
    mdelay(1);
    jz_writef(AIC_RGADW, ICRST(0));

    /* Power-up and initial config sequence */
    static const uint8_t init_config[] = {
        JZCODEC_CR_VIC,   0x00,
#if defined(X1000_EXCLK_24MHZ)
        JZCODEC_CR_CK,    0x40,
#else
        JZCODEC_CR_CK,    0x42,
#endif
        JZCODEC_AICR_DAC, 0x13,
        JZCODEC_AICR_ADC, 0x13,
#ifdef HAVE_RECORDING
        JZCODEC_CR_DMIC,  0x80,
        JZCODEC_CR_MIC1,  0x00,
#endif
    };

    for(size_t i = 0; i < ARRAYLEN(init_config); i += 2)
        x1000_icodec_write(init_config[i], init_config[i+1]);
}

void x1000_icodec_close(void)
{
    /* TODO: how to close ? */
}

int x1000_icodec_read(int reg)
{
    jz_writef(AIC_RGADW, ADDR(reg));
    return jz_readf(AIC_RGDATA, DATA);
}

void x1000_icodec_write(int reg, int value)
{
    jz_writef(AIC_RGADW, ADDR(reg), DATA(value));
    jz_writef(AIC_RGADW, RGWR(1));
    while(jz_readf(AIC_RGADW, RGWR));

    /* FIXME: ingenic suggests reading back to verify... why??? */
}

void x1000_icodec_update(int reg, int mask, int value)
{
    value |= (x1000_icodec_read(reg) & ~mask);
    x1000_icodec_write(reg, value);
}

void x1000_icodec_set_dac_frequency(int fsel)
{
    x1000_icodec_update(JZCODEC_FCR_DAC, 0x0f, play_fsel_to_hw[fsel]);
}

void x1000_icodec_set_adc_frequency(int fsel)
{
    x1000_icodec_update(JZCODEC_FCR_ADC, 0x0f, play_fsel_to_hw[fsel]);
}

void x1000_icodec_set_adc_highpass_filter(bool en)
{
    x1000_icodec_update(JZCODEC_FCR_ADC, 0x40, en ? 0x40 : 0x00);
}
