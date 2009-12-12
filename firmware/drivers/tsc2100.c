/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jonathan Gordon
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
#include "cpu.h"
#include "system.h"
#include "spi.h"
#include "tsc2100.h"

/* adc_data contains the last readings from the tsc2100 */
static short adc_data[10];
static short adc_status;

void tsc2100_read_data(void)
{
    int page = 0, address = 0;
    unsigned int i;
    unsigned short command = 0x8000|(page << 11)|(address << 5);
    unsigned char out[] = {command >> 8, command & 0xff};
    unsigned char *p_adc_data=(unsigned char *)&adc_data;
    
    adc_status|=tsc2100_readreg(TSSTAT_PAGE, TSSTAT_ADDRESS);
    
    spi_block_transfer(SPI_target_TSC2100,
                       out, sizeof(out), (char *)adc_data, sizeof(adc_data));
                       
    for(i=0; i<sizeof(adc_data); i+=2)
        adc_data[i>>1]=(short)(p_adc_data[i]<<8|p_adc_data[i+1]);
}

/* Read X, Y, Z1, Z2 touchscreen coordinates. */
bool tsc2100_read_touch(short *x, short* y, short *z1, short *z2)
{
    if( adc_status&(3<<9) ) {
        *x      = adc_data[0];
        *y      = adc_data[1];
        *z1     = adc_data[2];
        *z2     = adc_data[3];
        
        adc_status&=~(3<<9);
        
        return true;
    } else {
        return false;
    }
}

bool tsc2100_read_volt(short *bat1, short *bat2, short *aux)
{
    if( adc_status&(7<<4) ) {
        *bat1   = adc_data[5];
        *bat2   = adc_data[6];
        *aux    = adc_data[7];
        
        adc_status&=~(7<<4);
        return true;
    } else {
        return false;
    }
}

void tsc2100_set_mode(bool poweron, unsigned char scan_mode)
{
    short tsadc=(scan_mode<<TSADC_ADSCM_SHIFT)| /* mode */
                 (0x3<<TSADC_RESOL_SHIFT)| /* 12 bit resolution */
                 (0x2<<TSADC_ADCR_SHIFT )| /* 2 MHz internal clock */
                 (0x2<<TSADC_PVSTC_SHIFT);
                 
    if(!poweron)
    {
        tsadc|=TSADC_ADST;
    }
                 
    if(scan_mode<6)
        tsadc|=TSADC_PSTCM;
        
    tsc2100_writereg(TSADC_PAGE, TSADC_ADDRESS, tsadc);
}

void tsc2100_adc_init(void)
{
    /* Set the TSC2100 to read touchscreen */
    tsc2100_set_mode(true, 0x02);

    tsc2100_writereg(TSSTAT_PAGE, TSSTAT_ADDRESS, 
                     (0x1<<TSSTAT_PINTDAV_SHIFT) /* Data available only */
                     );

    /* An additional 2 mA can be saved here by powering down vref between
     * conversions, but it adds a click to the audio on the M:Robe 500
     */
    tsc2100_writereg(TSREF_PAGE, TSREF_ADDRESS,
                     TSREF_VREFM|TSREF_IREFV);
}

short tsc2100_readreg(int page, int address)
{
    unsigned short command = 0x8000|(page << 11)|(address << 5);
    unsigned char out[] = {command >> 8, command & 0xff};
    unsigned char in[2];
    spi_block_transfer(SPI_target_TSC2100, out, sizeof(out), in, sizeof(in));
    return (in[0]<<8)|in[1];
}

void tsc2100_writereg(int page, int address, short value)
{
    unsigned short command = (page << 11)|(address << 5);
    unsigned char out[4] = {command >> 8, command & 0xff,
                            value >> 8,   value & 0xff};
    spi_block_transfer(SPI_target_TSC2100, out, sizeof(out), NULL, 0);
}

void tsc2100_keyclick(void)
{
    // 1100 0100 0001 0000
    //short val = 0xC410;
    tsc2100_writereg(TSAC2_PAGE, TSAC2_ADDRESS, tsc2100_readreg(TSAC2_PAGE, TSAC2_ADDRESS)&0x8000);
}

#ifdef HAVE_HARDWARE_BEEP

void pcmbuf_beep(unsigned int frequency, size_t duration, int amplitude)
{
    tsc2100_keyclick();
}
#endif
