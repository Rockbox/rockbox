/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * $Id$
 *
 * Tuner header for the Silicon Labs SI4700
 *
 * Copyright (C) 2008 Dave Chapman
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

#ifndef _SI4700_H_
#define _SI4700_H_
#include <stdint.h>

#define HAVE_RADIO_REGION
#define HAVE_RADIO_RSSI

struct si4700_dbg_info
{
    uint16_t regs[16];  /* Read registers */
};

bool si4700_detect(void);
void si4700_init(void);
int si4700_set(int setting, int value);
int si4700_get(int setting);
void si4700_dbg_info(struct si4700_dbg_info *nfo);
/* For interrupt-based mono/stereo indicator */
bool si4700_st(void);

/** RDS support **/
void si4700_rds_init(void);
/* Radio is fully powered up or about to be powered down */
void si4700_rds_powerup(bool on);
#ifdef RDS_ISR_PROCESSING
/* Read raw RDS info for processing - asynchronously */
void si4700_read_raw_async(int count); /* implemented by target */
void si4700_rds_read_raw_async(void);
void si4700_rds_read_raw_async_complete(unsigned char *regbuf,
                                        uint16_t data[4]);
#else /* ndef RDS_ISR_PROCESSING */
/* Read raw RDS info for processing */
bool si4700_rds_read_raw(uint16_t data[4]);
#endif /* RDS_ISR_PROCESSING */
/* Obtain specified string */
char* si4700_get_rds_info(int setting);
/* Set the event flag */
void si4700_rds_set_event(void);

#ifndef CONFIG_TUNER_MULTI
#define tuner_set si4700_set
#define tuner_get si4700_get
#define tuner_get_rds_info si4700_get_rds_info
#endif

#endif /* _SI4700_H_ */
