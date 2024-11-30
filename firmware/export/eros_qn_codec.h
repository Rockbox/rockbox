/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 *
 * Copyright (c) 2021 Andrew Ryabinin, Dana Conrad
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

#ifndef _EROS_QN_CODEC_H
#define _EROS_QN_CODEC_H

/*
 * Note: Maximum volume is set one step below unity in order to
 *       avoid overflowing pcm samples due to our DC Offset.
 *
 *       The DAC's output is hot enough this should not be an issue.
 */
#define PCM5102A_VOLUME_MIN -740
#define PCM5102A_VOLUME_MAX -20

/* a small DC offset prevents play/pause clicking due to the PCM5102A DAC auto-muting */
#define PCM_DC_OFFSET_VALUE -1

AUDIOHW_SETTING(VOLUME, "dB", 0, 2, PCM5102A_VOLUME_MIN/10, PCM5102A_VOLUME_MAX/10, 0)

/* flag indicating whether this is a new revision unit with the ES9018K2M DAC */
extern int es9018k2m_present_flag;

/* Switch the output sink. 0 - headphones, 1 - line out */
void eros_qn_switch_output(int select);

/* Record last volume setting for switching between headphones/line out */
void eros_qn_set_last_vol(long int vol_l, long int vol_r);

/* this just calls audiohw_set_volume() with the last (locally) known volume,
 * used for switching to/from fixed line out volume. */
void eros_qn_set_outputs(void);

/* returns (global_settings.volume_limit * 10) */
int eros_qn_get_volume_limit(void);

/* powers on the dac and tries to speak over i2c.
 * will return 1 if es9018k2m, 0 if non-i2c dac.
 * if pwr_after_discovery = 1, leave dac powered up.
 * if 0, power down dac. */
bool eros_qn_discover_dac(bool pwr_after_discovery);

#endif
