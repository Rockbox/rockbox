/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2008 by Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _MC13783_H_
#define _MC13783_H_

enum mc13783_regs_enum
{
    MC13783_INTERRUPT_STATUS0 = 0x00,
    MC13783_INTERRUPT_MASK0,
    MC13783_INTERRUPT_SENSE0,
    MC13783_INTERRUPT_STATUS1,
    MC13783_INTERRUPT_MASK1,
    MC13783_INTERRUPT_SENSE1,
    MC13783_POWER_UP_MODE_SENSE,
    MC13783_IDENTIFICATION,
    MC13783_SEMAPHORE,
    MC13783_ARBITRATION_PERIPHERAL_AUDIO,
    MC13783_ARBITRATION_SWITCHERS,
    MC13783_ARBITRATION_REGULATORS0,
    MC13783_ARBITRATION_REGULATORS1,
    MC13783_POWER_CONTROL0,
    MC13783_POWER_CONTROL1,
    MC13783_POWER_CONTROL2,
    MC13783_REGEN_ASSIGNMENT,
    MC13783_CONTROL_SPARE,
    MC13783_MEMORYA,
    MC13783_MEMORYB,
    MC13783_RTC_TIME,
    MC13783_RTC_ALARM,
    MC13783_RTC_DAY,
    MC13783_RTC_DAY_ALARM,
    MC13783_SWITCHERS0,
    MC13783_SWITCHERS1,
    MC13783_SWITCHERS2,
    MC13783_SWITCHERS3,
    MC13783_SWITCHERS4,
    MC13783_SWITCHERS5,
    MC13783_REGULATOR_SETTING0,
    MC13783_REGULATOR_SETTING1,
    MC13783_REGULATOR_MODE0,
    MC13783_REGULATOR_MODE1,
    MC13783_POWER_MISCELLANEOUS,
    MC13783_POWER_SPARE,
    MC13783_AUDIO_RX0,
    MC13783_AUDIO_RX1,
    MC13783_AUDIO_TX,
    MC13783_SSI_NETWORK,
    MC13783_AUDIO_CODEC,
    MC13783_AUDIO_STEREO_CODEC,
    MC13783_AUDIO_SPARE,
    MC13783_ADC0,
    MC13783_ADC1,
    MC13783_ADC2,
    MC13783_ADC3,
    MC13783_ADC4,
    MC13783_CHARGER,
    MC13783_USB0,
    MC13783_CHARGER_USB1,
    MC13783_LED_CONTROL0,
    MC13783_LED_CONTROL1,
    MC13783_LED_CONTROL2,
    MC13783_LED_CONTROL3,
    MC13783_LED_CONTROL4,
    MC13783_LED_CONTROL5,
    MC13783_SPARE,
    MC13783_TRIM0,
    MC13783_TRIM1,
    MC13783_TEST0,
    MC13783_TEST1,
    MC13783_TEST2,
    MC13783_TEST3,
    MC13783_NUM_REGS,
};

void mc13783_init(void);
void mc13783_set(unsigned address, uint32_t bits);
void mc13783_clear(unsigned address, uint32_t bits);
int mc13783_write(unsigned address, uint32_t data);
int mc13783_write_multiple(unsigned start, const uint32_t *buffer, int count);
uint32_t mc13783_read(unsigned address);
int mc13783_read_multiple(unsigned start, uint32_t *buffer, int count);

#endif /* _MC13783_H_ */
