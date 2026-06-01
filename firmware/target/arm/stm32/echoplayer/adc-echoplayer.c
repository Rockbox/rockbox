/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Aidan MacDonald
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
#include "adc.h"
#include "mutex.h"
#include "kernel.h"
#include "fixedpoint.h"
#include "regs/stm32h743/adc.h"
#include "regs/stm32h743/rcc.h"

#define VREF_MV 3300

static volatile void *const adc3 = (void *)ITA_ADC3;
static struct mutex adc_mutex;

static void set_smptime(volatile void *adcbase, int channel, uint32_t smpval)
{
    uint32_t reg_off = ITO_ADC_SMPR1;
    if (channel > 10)
    {
        reg_off = ITO_ADC_SMPR2;
        channel -= 10;
    }

    volatile uint32_t *reg = adcbase + reg_off;
    uint32_t reg_val = *reg;

    reg_val &= ~(0x7 << (channel * 3));
    reg_val |= smpval << (channel * 3);

    *reg = reg_val;
}

void adc_init(void)
{
    mutex_init(&adc_mutex);
    reg_writef(RCC_AHB4ENR, ADC3EN(1));

    /* Power up ADC */
    reg_writelf(adc3, ADC_CR, DEEPPWD(0));
    reg_writelf(adc3, ADC_CR, ADVREGEN(1));
    while (!reg_readlf(adc3, ADC_ISR, LDORDY));

    /* Run calibration */
    reg_writelf(adc3, ADC_CR, ADCALDIF(0), ADCALLIN(1), ADCAL(1));
    while (reg_readlf(adc3, ADC_CR, ADCAL));

    /* Enable the ADC */
    reg_assignlf(adc3, ADC_ISR, ADRDY(1));
    reg_writelf(adc3, ADC_CR, ADEN(1));
    while (!reg_readlf(adc3, ADC_ISR, ADRDY));

    /* Set sampling times for each channel (387.5 cycles @ 6 MHz = 64.5us) */
    set_smptime(adc3, ADC_CHANNEL_VBUS,    BV_ADC_SMPR1_SMP0_387_5CYCLE);
    set_smptime(adc3, ADC_CHANNEL_BATTERY, BV_ADC_SMPR1_SMP0_387_5CYCLE);
}

/*
 * Converts raw ADC reading to millivolts, assuming the
 * ADC samples the output of a voltage divider as in the
 * following circuit:
 *
 *      [VIN]---[R1]
 *                |
 *                +----[ADC]
 *                |
 *              [R2]
 *                |
 *              [GND]
 *
 * The value returned is the voltage at the VIN node.
 */
static uint32_t scale_adc_val(uint32_t raw_val,
                              uint32_t vref_mV,
                              uint32_t r1_ohms,
                              uint32_t r2_ohms)
{
    /*
     * Finds the best fracbits value which doesn't overflow.
     * GCC will optimize this loop to a constant.
     */
    uint32_t fracbits = 16;
    uint32_t conv_fac;
    for (; fracbits > 0; --fracbits)
    {
        conv_fac = fp_div(r1_ohms + r2_ohms, r2_ohms, fracbits);
        if (conv_fac <= UINT16_MAX)
            break;
    }

    uint32_t conv_val = fp_mul(raw_val, conv_fac, fracbits);
    return fp_mul(vref_mV, conv_val, 16);
}

unsigned short adc_read(int channel)
{
    mutex_lock(&adc_mutex);

    /* Configure ADC to read the channel */
    reg_writelf(adc3, ADC_SQR1, L(1 - 1), SQ1(channel));
    reg_varl(adc3, ADC_PCSEL) = (1 << channel);

    /* Start conversion & wait until complete */
    reg_writelf(adc3, ADC_CR, ADSTART(1));
    while (reg_readlf(adc3, ADC_CR, ADSTART));

    uint32_t raw_val = reg_readl(adc3, ADC_DR);

    mutex_unlock(&adc_mutex);

    /* Convert to millivolts */
    switch (channel)
    {
    case ADC_CHANNEL_VBUS:
        return scale_adc_val(raw_val, VREF_MV, 174, 100);

    case ADC_CHANNEL_BATTERY:
        return scale_adc_val(raw_val, VREF_MV, 174, 390);

    default:
        return 0;
    }
}
