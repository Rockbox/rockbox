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

/* INTERRUPT_STATUS0, INTERRUPT_MASK0, INTERRUPT_SENSE0 */
#define MC13783_ADCDONE     (1 << 0)  /* x */
#define MC13783_ADCBISDONE  (1 << 1)  /* x */
#define MC13783_TS          (1 << 2)  /* x */
#define MC13783_WHIGH       (1 << 3)  /* x */
#define MC13783_WLOW        (1 << 4)  /* x */
#define MC13783_CHGDET      (1 << 6)
#define MC13783_CHGOV       (1 << 7)
#define MC13783_CHGREV      (1 << 8)
#define MC13783_CHGSHORT    (1 << 9)
#define MC13783_CCCV        (1 << 10)
#define MC13783_CHGCURR     (1 << 11)
#define MC13783_BPONI       (1 << 12)
#define MC13783_LOBATL      (1 << 13)
#define MC13783_LOBATH      (1 << 14)
#define MC13783_UDP         (1 << 15)
#define MC13783_USB4V4      (1 << 16)
#define MC13783_USB2V0      (1 << 17)
#define MC13783_USB0V8      (1 << 18)
#define MC13783_IDFLOAT     (1 << 19)
#define MC13783_SE1         (1 << 21)
#define MC13783_CKDET       (1 << 22)
#define MC13783_UDM         (1 << 23)
/* x = no sense bit */

/* INTERRUPT_STATUS1, INTERRUPT_MASK1, INTERRUPT_SENSE1 */
#define MC13783_1HZ         (1 << 0)  /* x */
#define MC13783_TODA        (1 << 1)  /* x */
#define MC13783_ONOFD1      (1 << 3)  /* ON1B */
#define MC13783_ONOFD2      (1 << 4)  /* ON2B */
#define MC13783_ONOFD3      (1 << 5)  /* ON3B */
#define MC13783_SYSRST      (1 << 6)  /* x */
#define MC13783_RTCRST      (1 << 7)  /* x */
#define MC13783_PCI         (1 << 8)  /* x */
#define MC13783_WARM        (1 << 9)  /* x */
#define MC13783_MEMHLD      (1 << 10) /* x */
#define MC13783_PWRRDY      (1 << 11)
#define MC13783_THWARNL     (1 << 12)
#define MC13783_THWARNH     (1 << 13)
#define MC13783_CLK         (1 << 14)
#define MC13783_SEMAF       (1 << 15) /* x */
#define MC13783_MC2B        (1 << 17)
#define MC13783_HSDET       (1 << 18)
#define MC13783_HSL         (1 << 19)
#define MC13783_ALSPTH      (1 << 20)
#define MC13783_AHSSHORT    (1 << 21)
/* x = no sense bit */

/* POWER_UP_MODE_SENSE */

#define MC13783_ICTESTS     (1 << 0)
#define MC13783_CLKSELS     (1 << 1)
#define MC13783_PUMS1Sr(r)  (((r) >> 2) & 0x3)
#define MC13783_PUMS2S      (((r) >> 4) & 0x3)
#define MC13783_PUMS3S      (((r) >> 6) & 0x3)
    #define PUMS_LOW        0x0
    #define PUMS_OPEN       0x1
    #define PUMS_HIGH       0x2
#define MC13783_CHRGMOD0Sr(r) (((r) >> 8) & 0x3)
#define MC13783_CHRGMOD1Sr(r) (((r) >> 10) & 0x3)
    #define CHRGMOD_LOW     0x0
    #define CHRGMOD_OPEN    0x1
    #define CHRGMOD_HIGH    0x3
#define MC13783_UMODSr(r)   (((r) >> 12) & 0x3)
    #define UMODS0_LOW_UMODS1_LOW       0x0
    #define UMODS0_OPEN_UMODS1_LOW      0x1
    #define UMODS0_DONTCARE_UMODS1_HIGH 0x2
    #define UMODS0_HIGH_UMODS1_LOW      0x3
#define MC13783_USBEN       (1 << 14)
#define MC13783_SW1ABS      (1 << 15)
#define MC13783_SW2ABS      (1 << 16)

/* IDENTIFICATION */
/* SEMAPHORE */
/* ARBITRATION_PERIPHERAL_AUDIO */
/* ARBITRATION_SWITCHERS */
/* ARBITRATION_REGULATORS0 */
/* ARBITRATION_REGULATORS1 */

/* POWER_CONTROL0 */
#define MC13783_USEROFFSPI  (1 << 3)

/* POWER_CONTROL1 */
/* POWER_CONTROL2 */
/* REGEN_ASSIGNMENT */
/* MEMORYA */
/* MEMORYB */
/* RTC_TIME */
/* RTC_ALARM */
/* RTC_DAY */
/* RTC_DAY_ALARM */
/* SWITCHERS0 */
/* SWITCHERS1 */
/* SWITCHERS2 */
/* SWITCHERS3 */
/* SWITCHERS4 */
/* SWITCHERS5 */
/* REGULATOR_SETTING0 */
/* REGULATOR_SETTING1 */
/* REGULATOR_MODE0 */
/* REGULATOR_MODE1 */
/* POWER_MISCELLANEOUS */
/* AUDIO_RX0 */
/* AUDIO_RX1 */
/* AUDIO_TX */
/* SSI_NETWORK */
/* AUDIO_CODEC */
/* AUDIO_STEREO_CODEC */

/* ADC0 */
#define MC13783_LICELLCON   (1 << 0)
#define MC13783_CHRGICON    (1 << 1)
#define MC13783_BATICON     (1 << 2)
#define MC13783_RTHEN       (1 << 3)
#define MC13783_DTHEN       (1 << 4)
#define MC13783_UIDEN       (1 << 5)
#define MC13783_ADOUTEN     (1 << 6)
#define MC13783_ADOUTPER    (1 << 7)
#define MC13783_ADREFEN     (1 << 10)
#define MC13783_ADREFMODE   (1 << 11)
#define MC13783_TSMODw(w)   ((w) << 12)
#define MC13783_TSMODr(r)   (((r) >> 12) & 0x3)
#define MC13783_CHRGRAWDIV  (1 << 15)
#define MC13783_ADINC1      (1 << 16)
#define MC13783_ADINC2      (1 << 17)
#define MC13783_WCOMP       (1 << 18)
#define MC13783_ADCBIS0     (1 << 23)

/* ADC1 */
#define MC13783_ADEN        (1 << 0)
#define MC13783_RAND        (1 << 1)
#define MC13783_ADSEL       (1 << 3)
#define MC13783_TRIGMASK    (1 << 4)
#define MC13783_ADA1w(w)    ((w) << 5)
#define MC13783_ADA1r(r)    (((r) >> 5) & 0x3)
#define MC13783_ADA2w(w)    ((w) << 8)
#define MC13783_ADA2r(r)    (((r) >> 8) & 0x3)
#define MC13783_ATOw(w)     ((w) << 11)
#define MC13783_ATOr(r)     (((r) >> 11) & 0xff)
#define MC13783_ATOX        (1 << 19)
#define MC13783_ASC         (1 << 20)
#define MC13783_ADTRIGIGN   (1 << 21)
#define MC13783_ADONESHOT   (1 << 22)
#define MC13783_ADCBIS1     (1 << 23)

/* ADC2 */
#define MC13783_ADD1r(r)    (((r) >> 2) & 0x3ff)
#define MC13783_ADD2r(r)    (((r) >> 14) & 0x3ff)

/* ADC3 */
#define MC13783_WHIGHw(w)   ((w) << 0)
#define MC13783_WHIGHr(r)   ((r) & 0x3f)
#define MC13783_ICIDr(r)    (((r) >> 6) & 0x3)
#define MC13783_WLOWw(w)    ((w) << 9)
#define MC13783_WLOWr(r)    (((r) >> 9) & 0x3f)
#define MC13783_ADCBIS2     (1 << 23)

/* ADC4 */
#define MC13783_ADCBIS1r(r) (((r) >> 2) & 0x3ff)
#define MC13783_ADCBIS2r(r) (((r) >> 14) & 0x3ff)

/* CHARGER */
/* USB0 */
/* CHARGER_USB1 */

/* LED_CONTROL0 */
#define MC13783_LEDEN       (1 << 0)

/* LED_CONTROL1 */
/* LED_CONTROL2 */
/* LED_CONTROL3 */
/* LED_CONTROL4 */
/* LED_CONTROL5 */
/* TRIM0 */
/* TRIM1 */
/* TEST0 */
/* TEST1 */
/* TEST2 */
/* TEST3 */

void mc13783_init(void);
uint32_t mc13783_set(unsigned address, uint32_t bits);
uint32_t mc13783_clear(unsigned address, uint32_t bits);
int mc13783_write(unsigned address, uint32_t data);
int mc13783_write_multiple(unsigned start, const uint32_t *buffer, int count);
int mc13783_write_regset(const unsigned char *regs, const uint32_t *data, int count);
uint32_t mc13783_read(unsigned address);
int mc13783_read_multiple(unsigned start, uint32_t *buffer, int count);
int mc13783_read_regset(const unsigned char *regs, uint32_t *buffer, int count);
void mc1378_alarm_start(void);

#endif /* _MC13783_H_ */
