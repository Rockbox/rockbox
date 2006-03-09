/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "string.h"
#include "adc.h"
#include "pcf50605.h"
#include "pcf50606.h"

#if CONFIG_CPU == SH7034
/**************************************************************************
 ** The A/D conversion is done every tick, in three steps:
 **
 ** 1) On the tick interrupt, the conversion of channels 0-3 is started, and
 **    the A/D interrupt is enabled.
 **
 ** 2) After the conversion is done (approx. 256*4 cycles later), an interrupt
 **    is generated at level 1, which is the same level as the tick interrupt
 **    itself. This interrupt will be pending until the tick interrupt is
 **    finished.
 **    When the A/D interrupt is finally served, it will read the results
 **    from the first conversion and start the conversion of channels 4-7.
 **
 ** 3) When the conversion of channels 4-7 is finished, the interrupt is
 **    triggered again, and the results are read. This time, no new
 **    conversion is started, it will be done in the next tick interrupt.
 **
 ** Thus, each channel will be updated HZ times per second.
 **
 *************************************************************************/

static int current_channel;
static unsigned short adcdata[NUM_ADC_CHANNELS];

static void adc_tick(void)
{
    /* Start a conversion of channel group 0. This will trigger an interrupt,
       and the interrupt handler will take care of group 1. */

    current_channel = 0;
    ADCSR = ADCSR_ADST | ADCSR_ADIE | ADCSR_SCAN | 3;
}

#pragma interrupt
void ADITI(void)
{
    if(ADCSR & ADCSR_ADF)
    {
        ADCSR = 0;

        if(current_channel == 0)
        {
            adcdata[0] = ADDRA >> 6;
            adcdata[1] = ADDRB >> 6;
            adcdata[2] = ADDRC >> 6;
            adcdata[3] = ADDRD >> 6;
            current_channel = 4;
            
            /* Convert the next group */
            ADCSR = ADCSR_ADST | ADCSR_ADIE | ADCSR_SCAN | 7;
        }
        else
        {
            adcdata[4] = ADDRA >> 6;
            adcdata[5] = ADDRB >> 6;
            adcdata[6] = ADDRC >> 6;
            adcdata[7] = ADDRD >> 6;
        }
    }
}

unsigned short adc_read(int channel)
{
    return adcdata[channel];
}

void adc_init(void)
{
    ADCR  = 0x7f; /* No external trigger; other bits should be 1 according
                     to the manual... */

    ADCSR = 0;
    
    current_channel = 0;

    /* Enable the A/D IRQ on level 1 */
    IPRE = (IPRE & 0xf0ff) | 0x0100;
    
    tick_add_task(adc_tick);
    
    sleep(2);    /* Ensure valid readings when adc_init returns */
}
#elif CONFIG_CPU == MCF5249
static unsigned char adcdata[NUM_ADC_CHANNELS];

#ifdef IRIVER_H300_SERIES
static int channelnum[] =
{
    5,   /* ADC_BUTTONS (ADCIN2) */
    6,   /* ADC_REMOTE  (ADCIN3) */
    0,   /* ADC_BATTERY (BATVOLT, resistive divider) */
    2,   /* ADC_REMOTEDETECT (ADCIN1, resistive divider) */
};

unsigned short adc_scan(int channel)
{
    unsigned char data;
    
    pcf50606_write(0x2f, 0x80 | (channelnum[channel] << 1) | 1);
    data = pcf50606_read(0x30);

    adcdata[channel] = data;

    return data;
}
#else

#define CS_LO  and_l(~0x80, &GPIO_OUT)
#define CS_HI  or_l(0x80, &GPIO_OUT)
#define CLK_LO and_l(~0x00400000, &GPIO_OUT)
#define CLK_HI or_l(0x00400000, &GPIO_OUT)
#define DO     (GPIO_READ & 0x80000000)
#define DI_LO  and_l(~0x00200000, &GPIO_OUT)
#define DI_HI  or_l(0x00200000, &GPIO_OUT)

/* delay loop */
#define DELAY   do { int _x; for(_x=0;_x<10;_x++);} while (0)

unsigned short adc_scan(int channel)
{
    unsigned char data = 0;
    int i;
    
    CS_LO;

    DI_HI;  /* Start bit */
    DELAY;
    CLK_HI;
    DELAY;
    CLK_LO;
    
    DI_HI;  /* Single channel */
    DELAY;
    CLK_HI;
    DELAY;
    CLK_LO;

    if(channel & 1) /* LSB of channel number */
        DI_HI;
    else
        DI_LO;
    DELAY;
    CLK_HI;
    DELAY;
    CLK_LO;
    
    if(channel & 2) /* MSB of channel number */
        DI_HI;
    else
        DI_LO;
    DELAY;
    CLK_HI;
    DELAY;
    CLK_LO;

    DELAY;

    for(i = 0;i < 8;i++) /* 8 bits of data */
    {
        CLK_HI;
        DELAY;
        CLK_LO;
        DELAY;
        data <<= 1;
        data |= DO?1:0;
    }
    
    CS_HI;

    adcdata[channel] = data;

    return data;
}
#endif

unsigned short adc_read(int channel)
{
    return adcdata[channel];
}

static int adc_counter;

static void adc_tick(void)
{
    if(++adc_counter == HZ)
    {
        adc_counter = 0;
        adc_scan(ADC_BATTERY);
        adc_scan(ADC_REMOTEDETECT); /* Temporary. Remove when the remote
                                       detection feels stable. */
    }
}

void adc_init(void)
{
#ifndef IRIVER_H300_SERIES
    or_l(0x80600080, &GPIO_FUNCTION); /* GPIO7:  CS
                                         GPIO21: Data In (to the ADC)
                                         GPIO22: CLK
                                         GPIO31: Data Out (from the ADC) */
    or_l(0x00600080, &GPIO_ENABLE);
    or_l(0x80, &GPIO_OUT);          /* CS high */
    and_l(~0x00400000, &GPIO_OUT);  /* CLK low */
#endif

    adc_scan(ADC_BATTERY);
    
    tick_add_task(adc_tick);
}

#elif CONFIG_CPU == TCC730


/**************************************************************************
 **
 ** Each channel will be updated HZ/CHANNEL_ORDER_SIZE times per second.
 **
 *************************************************************************/

static int current_channel;
static int current_channel_idx;
static unsigned short adcdata[NUM_ADC_CHANNELS];

#define CHANNEL_ORDER_SIZE 2
static int channel_order[CHANNEL_ORDER_SIZE] = {6,7};

static void adc_tick(void)
{
    if (ADCON & (1 << 3)) {
        /* previous conversion finished? */
        adcdata[current_channel] = ADDATA >> 6;
        if (++current_channel_idx >= CHANNEL_ORDER_SIZE)
            current_channel_idx = 0;
        current_channel = channel_order[current_channel_idx];
        int adcon = (current_channel << 4) | 1;
        ADCON = adcon;
    }
}

unsigned short adc_read(int channel)
{
    return adcdata[channel];
}

void adc_init(void)
{
    current_channel_idx = 0;
    current_channel = channel_order[current_channel_idx];
  
    ADCON = (current_channel << 4) | 1;
  
    tick_add_task(adc_tick);
  
    sleep(2);    /* Ensure valid readings when adc_init returns */
}

#elif CONFIG_CPU == PP5020 || (CONFIG_CPU == PP5002)

struct adc_struct {
    long timeout;
    void (*conversion)(unsigned short *data);
    short channelnum;
    unsigned short data;
};

static struct adc_struct adcdata[NUM_ADC_CHANNELS] IDATA_ATTR;

static unsigned short _adc_read(struct adc_struct *adc)
{
    if (adc->timeout < current_tick) {
        unsigned char data[2];
        unsigned short value;
        /* 5x per 2 seconds */
        adc->timeout = current_tick + (HZ * 2 / 5);

        /* ADCC1, 10 bit, start */
        pcf50605_write(0x2f, (adc->channelnum << 1) | 0x1);
        pcf50605_read_multiple(0x30, data, 2); /* ADCS1, ADCS2 */
        value   = data[0];
        value <<= 2;
        value  |= data[1] & 0x3;

        if (adc->conversion) {
            adc->conversion(&value);
        }
        adc->data = value;
        return value;
    } else {
        return adc->data;
    }
}

/* Force an ADC scan _now_ */
unsigned short adc_scan(int channel) {
    struct adc_struct *adc = &adcdata[channel];
    adc->timeout = 0;
    return _adc_read(adc);
}

/* Retrieve the ADC value, only does a scan periodically */
unsigned short adc_read(int channel) {
    return _adc_read(&adcdata[channel]);
}

void adc_init(void)
{
    struct adc_struct *adc_battery = &adcdata[ADC_BATTERY];
    adc_battery->channelnum = 0x2; /* ADCVIN1, resistive divider */
    adc_battery->timeout = 0;
    _adc_read(adc_battery);
}

#elif CONFIG_CPU == PNX0101

static unsigned short adcdata[NUM_ADC_CHANNELS];

unsigned short adc_read(int channel)
{
    return adcdata[channel];
}

static void adc_tick(void)
{
    if (ADCST & 0x10) {
        adcdata[0] = ADCCH0 & 0x3ff;
        adcdata[1] = ADCCH1 & 0x3ff;
        adcdata[2] = ADCCH2 & 0x3ff;
        adcdata[3] = ADCCH3 & 0x3ff;
        adcdata[4] = ADCCH4 & 0x3ff;
        ADCST = 0xa;
    }
}

void adc_init(void)
{
    ADCR24 = 0xaaaaa;
    ADCR28 = 0;
    ADCST = 2;
    ADCST = 0xa;

    while (!(ADCST & 0x10));
    adc_tick();
  
    tick_add_task(adc_tick);
}

#endif
