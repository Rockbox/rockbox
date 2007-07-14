/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Tuner header for the Sanyo LV24020LP
 *
 * Copyright (C) 2007 Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _LV24020LP_H_
#define _LV24020LP_H_

/* Define additional tuner messages here */
#define HAVE_RADIO_REGION

#define LV24020LP_CTRL_STAT (RADIO_GET_CHIP_FIRST+0)
#define LV24020LP_REG_STAT  (RADIO_GET_CHIP_FIRST+1)
#define LV24020LP_MSS_FM    (RADIO_GET_CHIP_FIRST+2)
#define LV24020LP_MSS_IF    (RADIO_GET_CHIP_FIRST+3)
#define LV24020LP_MSS_SD    (RADIO_GET_CHIP_FIRST+4)
#define LV24020LP_IF_SET    (RADIO_GET_CHIP_FIRST+5)
#define LV24020LP_SD_SET    (RADIO_GET_CHIP_FIRST+6)

struct lv24020lp_region_data
{
    unsigned char deemphasis;
} __attribute__((packed));

const unsigned char lv24020lp_region_data[TUNER_NUM_REGIONS];

int lv24020lp_set(int setting, int value);
int lv24020lp_get(int setting);
void lv24020lp_power(bool status);

#ifndef CONFIG_TUNER_MULTI
#define tuner_set lv24020lp_set
#define tuner_get lv24020lp_get
#endif

#endif /* _LV24020LP_H_ */
