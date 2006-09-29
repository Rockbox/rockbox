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
#include "adc-target.h"


void adc_init(void) {
  /* Turn on the ADC PCLK */
  CLKCON |= (1<<15);

  /* Set channel 0, normal mode, disable "start by read" */
  ADCCON &= ~(0x3F);

  /* No start delay.  Use nromal conversion mode. */
  ADCDLY |= 0x1;

  /* Set and enable the prescaler */
  ADCCON = (ADCCON & ~(0xff<<6)) | (0x19<<6);
  ADCCON |= (1<<14);
}

unsigned short adc_read(int channel) {
  int i;

  /* Set the channel */
  ADCCON = (ADCCON & ~(0x7<<3)) | (channel<<3);

  /* Start the conversion process */
  ADCCON |= 0x1;

  /* Wait for a low Enable_start */
  i = 20000;
  while(i > 0) {
    if(ADCCON & 0x1) {
      i--;
    }
    else {
      break;
    }
  }
  if(i == 0) {
    /* Ran out of time */
    return(0);
  }

  /* Wait for high End_of_Conversion */
  i = 20000;
  while(i > 0) {
    if(ADCCON & (1<<15)) { 	
      break;
    }
    else {
      i--;
    }
  }
  if(i == 0) {
    /* Ran out of time */
    return(0);
  }

  return(ADCDAT0 & 0x3ff);
}
