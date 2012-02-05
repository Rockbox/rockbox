/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * tuner for the ipod fm remote and other ipod remote tuners
 *
 * Copyright (C) 2009 Laurent Gautier
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
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "kernel.h"
#include "iap.h"
#include "tuner.h" /* tuner abstraction interface */
#include "adc.h"
#include "settings.h"
#include "power.h"

static unsigned char tuner_param = 0x00, old_tuner_param = 0xFF;
/* temp var for tests to avoid looping execution in submenus settings*/
static int mono_mode = -1, old_region = -1;

int radio_present = 0;

static int tuner_frequency = 0;
static int tuner_signal_power = 0;
static bool radio_tuned = false;
static bool rds_event = false;

static char rds_radioname[9];
static char rds_radioinfo[65];

static void rmt_tuner_signal_power(unsigned char value)
{
    tuner_signal_power = (int)(value);
}

void rmt_tuner_freq(unsigned int len, const unsigned char *buf)
{
    /* length currently unused */
    (void)len;

    unsigned int khz = (buf[2] << 24) | (buf[3] << 16) |
                       (buf[4] << 8) | buf[5];
    tuner_frequency = khz *1000 ;
    radio_tuned = true;
    rmt_tuner_signal_power(buf[6]);
}

static void rmt_tuner_set_freq(int curr_freq)
{
    if (curr_freq != tuner_frequency)
    {
        radio_tuned = false;
        tuner_signal_power = 0;
        /* clear rds name and info */
        memset(rds_radioname,' ',sizeof(rds_radioname));
        memset(rds_radioinfo,' ',sizeof(rds_radioinfo));
        /* ex: 00 01 63 14 = 90.9MHz */
        unsigned char data[] = {0x07, 0x0B, 0x00, 0x01, 0x63, 0x14};
        
        if (curr_freq != 0)
        {
            unsigned int khz = curr_freq / 1000;
            data[2] = (khz >> 24) & 0xFF;
            data[3] = (khz >> 16) & 0xFF;
            data[4] = (khz >>  8) & 0xFF;
            data[5] = (khz >>  0) & 0xFF;
            iap_send_pkt(data, sizeof(data));
        }
    }
}

static void rmt_tuner_sleep(int state)
{
    if (state == 0)
    {
        tuner_param = 0x00;
        old_tuner_param = 0xFF;
        mono_mode = -1;
        old_region = -1;
        tuner_frequency = 0;
        radio_tuned = false;
        
        /* tuner HW on */
        const unsigned char data[] = {0x07, 0x05, 0x01};
        iap_send_pkt(data, sizeof(data));
        /* boost gain */
        const unsigned char data1[] = {0x07, 0x24, 0x06 };
        iap_send_pkt(data1, sizeof(data1));
        /* set volume */
        unsigned char data2[] = {0x03, 0x09, 0x04, 0x00, 0x00 };
        data2[4] = (char)((global_settings.volume+58) * 4);
        iap_send_pkt(data2, sizeof(data2));
        /* set rds on */
        const unsigned char data3[] = {0x07, 0x20, 0x40, 0x00, 0x00, 0x10 };
        iap_send_pkt(data3, sizeof(data3));
    }
    else
    {
        /* unbooste gain */
        const unsigned char data[] = {0x07, 0x24, 0x00};
        iap_send_pkt(data, sizeof(data));
        /* set rds off */
        const unsigned char data1[] = {0x07, 0x20, 0x00, 0x00, 0x00, 0x00 };
        iap_send_pkt(data1, sizeof(data1)); 
        /* stop tuner HW */
        const unsigned char data2[] = {0x07, 0x05, 0x00};
        iap_send_pkt(data2, sizeof(data2));
    }
}

static void rmt_tuner_scan(int param)
{
    const unsigned char data[] = {0x07, 0x11, 0x08};  /* RSSI level */
    unsigned char updown = 0x00;
    radio_tuned = false;
    iap_send_pkt(data, sizeof(data));

    if (param == 1)
    {
        updown = 0x07;  /* scan up */
    }
    else if (param == -1)
    {
        updown = 0x08;  /* scan down */
    }
    else if (param == 10)
    {
        updown = 0x01;  /* scan up starting from beginning of the band */
    }
    unsigned char data1[] = {0x07, 0x12, updown};
    iap_send_pkt(data1, sizeof(data1));
}

static void rmt_tuner_mute(int value)
{
    /* mute flag off (play) */
    unsigned char data[] = {0x03, 0x09, 0x03, 0x01};
    if (value)
    {
        /* mute flag on (pause) */
        data[3] = 0x02;
    }
    iap_send_pkt(data, sizeof(data));
}

static void rmt_tuner_region(int region)
{
    if (region != old_region)
    {
        const struct fm_region_data *rd = &fm_region_data[region];
        unsigned char data[] = {0x07, 0x08, 0x00};
        if (rd->freq_min == 76000000)
        {
            data[2] = 0x02; /* japan band */
        }
        else
        {
            data[2] = 0x01; /* us eur band */
        }
        iap_send_pkt(data, sizeof(data));
        sleep(HZ/100);
        old_region = region;
    }
}

/* set stereo/mono, deemphasis, delta freq... */
static void rmt_tuner_set_param(unsigned char tuner_param)
{
    if(tuner_param != old_tuner_param)
    {
        unsigned char data[] = {0x07, 0x0E, 0x00};

        data[2] = tuner_param;
        iap_send_pkt(data, sizeof(data));
        old_tuner_param = tuner_param;
    }
}

static void set_deltafreq(int delta)
{
    tuner_param &= 0xFC;
    switch (delta)
    {
        case 1:
        {
            /* 100KHz */
            tuner_param |= 0x01;
            break;
        }
        case 2:
        {
            /* 50KHz */
            tuner_param |= 0x02;
        break;
        }

        default:
        {
            /* 200KHz */
            tuner_param |= 0x00;
            break;
        }
    }
}

static void set_deemphasis(int deemphasis)
{
    tuner_param &= 0xBF;
    switch (deemphasis)
    {
        case 1:
        {
            tuner_param |= 0x40;
            break;
        }
        default:
        {
            tuner_param |= 0x00;
            break;
        }
    }
}

static void set_mono(int value)
{
    tuner_param &= 0xEF;

    if (value != mono_mode)
    {
        tuner_param &= 0xEF;
        if (value == 1)
            tuner_param |= 0x10;
        rmt_tuner_set_param(tuner_param);
        sleep(HZ/100);
        mono_mode = value;
    }
}

static bool reply_timeout(void)
{
    int timeout = 0;
    
    sleep(HZ/50);
    do
    {
        iap_handlepkt();
        sleep(HZ/50);
        timeout++;
    }
    while((ipod_rmt_tuner_get(RADIO_TUNED) == 0) && (timeout < TIMEOUT_VALUE));
    
    return (timeout >= TIMEOUT_VALUE);
}

void rmt_tuner_rds_data(unsigned int len, const unsigned char *buf)
{
    if (buf[2] == 0x1E)
    {
        strlcpy(rds_radioname,buf+4,8);
    }
    else if(buf[2] == 0x04)
    {
        strlcpy(rds_radioinfo,buf+4,len-4);
    }
    rds_event = true;
}
    
/* tuner abstraction layer: set something to the tuner */
int ipod_rmt_tuner_set(int setting, int value)
{
    switch(setting)
    {
        case RADIO_SLEEP:
        {
            rmt_tuner_sleep(value);
            sleep(HZ/10);
            if(value)
            {
                tuner_frequency = 0;
            }
            break;
        }

        case RADIO_FREQUENCY:
        {
            rmt_tuner_set_freq(value);
            if (reply_timeout())
                return 0;
            break;
        }

        case RADIO_SCAN_FREQUENCY:
        {
            const struct fm_region_data * const fmr =
            &fm_region_data[global_settings.fm_region];

            /* case: scan for presets, back to beginning of the band */
            if (radio_tuned && (value == fmr->freq_min))
            {
                tuner_set(RADIO_FREQUENCY,value);
            }

            /* scan through frequencies */
            if (radio_tuned)
            {
                if ((tuner_frequency <= fmr->freq_min)
                    && (tuner_frequency >= fmr->freq_max))
                {
                    tuner_set(RADIO_FREQUENCY,value);
                }
                /* scan down */
                if(value < tuner_frequency)
                    rmt_tuner_scan(-1);
                /* scan up */
                else
                    rmt_tuner_scan(1);
                    
                sleep(HZ/10);
                if (reply_timeout())
                {
                    tuner_set(RADIO_FREQUENCY,value);
                    rmt_tuner_scan(1);
                    if (reply_timeout() == true)
                        return 0;
                }
                radio_tuned = false;
            }    

            if (tuner_frequency == value)
            {
                radio_tuned = true;
                return 1;
            }
            else
            {
                radio_tuned = false;
                return 0;
            }
        }

        case RADIO_MUTE:
        {
            /* mute flag sent to accessory */
            rmt_tuner_mute(value);
            break;
        }

        case RADIO_REGION:
        {
            const struct fm_region_data *rd = &fm_region_data[value];
            int band = (rd->freq_min == 76000000) ? 2 : 0;
            int spacing = (100000 / rd->freq_step);
            int deemphasis = (rd->deemphasis == 50) ? 1 : 0;

            rmt_tuner_region(band);
            set_deltafreq(spacing);
            set_deemphasis(deemphasis);
            rmt_tuner_set_param(tuner_param);
            break;
        }

        case RADIO_FORCE_MONO:
        {
            set_mono(value);
            break;
        }

        default:
            return -1;
    }
    return 1;
}

/* tuner abstraction layer: read something from the tuner */
int ipod_rmt_tuner_get(int setting)
{
    int val = -1; /* default for unsupported query */

    switch(setting)
    {
        case RADIO_PRESENT:
            val = radio_present;
            if (val)
            {
                /* if accessory disconnected */
                if(adc_read(ADC_ACCESSORY) >= 10)
                {
                    radio_present = 0;
                    val = 0;
                }
            }
            break;

        /* radio tuned: yes no */
        case RADIO_TUNED:
            val = 0;
            if (radio_tuned)
                val = 1;
            break;

        /* radio is always stereo */
        /* we can't know when it's in mono mode, depending of signal quality */
        /* except if it is forced in mono mode */
        case RADIO_STEREO:
            val = true;
            break;
            
        case RADIO_EVENT:
            if (rds_event)
            {
                val = 1;
                rds_event = false;
            }
            break;
    }
    return val;
}

char* ipod_get_rds_info(int setting)
{
    char *text = NULL;
    
    switch(setting)
    {
        case RADIO_RDS_NAME:
            text = rds_radioname;
            break;

        case RADIO_RDS_TEXT:
            text = rds_radioinfo;
            break;
    }
    return text;
}
