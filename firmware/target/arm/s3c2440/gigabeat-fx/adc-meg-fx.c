/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Wade Brown
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "cpu.h"
#include "system.h"
#include "adc-target.h"
#include "kernel.h"



static unsigned short adc_readings[NUM_ADC_CHANNELS];

/* prototypes */
static unsigned short __adc_read(int channel);
static void adc_tick(void);



void adc_init(void) 
{
    int i;

    /* Turn on the ADC PCLK */
    s3c_regset(&CLKCON, 1<<15);

    /* Set channel 0, normal mode, disable "start by read" */
    ADCCON &= ~(0x3F);

    /* No start delay.  Use normal conversion mode. */
    ADCDLY = 0x1;

    /* Set and enable the prescaler */
    ADCCON = (ADCCON & ~(0xff<<6)) | (0x19<<6);
    ADCCON |= (1<<14);

    /* prefill the adc channels */
    for (i = 0; i < NUM_ADC_CHANNELS; i++)
    {
        adc_readings[i] = __adc_read(i);
    }

    /* start at zero so when the tick starts it is at zero */
    adc_readings[0] = __adc_read(0);

    /* attach the adc reading to the tick */
    tick_add_task(adc_tick);

    
}



/* Called to get the recent ADC reading */
inline unsigned short adc_read(int channel)
{
    return adc_readings[channel];
}



/** 
  * Read the ADC by polling
  * @param channel The ADC channel to read
  * @return 10bit reading from ADC channel or ADC_READ_ERROR if timeout
  */
static unsigned short __adc_read(int channel) 
{
  int i;

  /* Set the channel */
  ADCCON = (ADCCON & ~(0x7<<3)) | (channel<<3);

  /* Start the conversion process */
  ADCCON |= 0x1;

  /* Wait for a low Enable_start */
  for (i = 20000;;) {
    if(0 == (ADCCON & 0x1)) {
      break;
    }
    else {
      i--;
      if (0 == i) {
        /* Ran out of time */
        return ADC_READ_ERROR;
      }
    }
  }

  /* Wait for high End_of_Conversion */
  for(i = 20000;;) {
    if(ADCCON & (1<<15)) {
      break;
    }
    else {
      i--;
      if(0 == i) {
        /* Ran out of time */
        return ADC_READ_ERROR;
      }
    }
  }

  return (ADCDAT0 & 0x3ff);
}



/* add this to the tick so that the ADC converts are done in the background */
static void adc_tick(void)
{
    static unsigned channel;

    /* Check if the End Of Conversion is set */
    if (ADCCON & (1<<15)) 
    {
        adc_readings[channel] = (ADCDAT0 & 0x3FF);
        if (++channel >= NUM_ADC_CHANNELS)
        {
            channel = 0;
        }

        /* setup the next conversion and start it*/
        ADCCON = (ADCCON & ~(0x7<<3)) | (channel<<3) | 0x01;
    }
}



  
