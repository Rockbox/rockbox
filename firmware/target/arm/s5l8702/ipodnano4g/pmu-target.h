/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright © 2009 Michael Sparmann
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

#ifndef __PMU_TARGET_H__
#define __PMU_TARGET_H__

#include <stdint.h>
#include <stdbool.h>
#include "config.h"


/* undocummented PMU registers */
enum d1759_regs {
    D1759_REG_EVENTA        = 0x01,
    D1759_REG_EVENTB        = 0x02,
    D1759_REG_EVENTC        = 0x03,
    D1759_REG_STATUSA       = 0x04,
    D1759_REG_STATUSB       = 0x05,
    D1759_REG_STATUSC       = 0x06,
    D1759_REG_IRQMASKA      = 0x07,
    D1759_REG_IRQMASKB      = 0x08,
    D1759_REG_IRQMASKC      = 0x09,
    D1759_REG_SYSCTRL       = 0x0a, /* preinit writes 0x70 */
    D1759_REG_WAKE          = 0x0d, /* preinit gates bit 5 from PMU 0x7f */
    D1759_REG_VCORE         = 0x14, /* active core buck, code = (mV - 725) / 25 */
    D1759_REG_LEDOUT        = 0x30,
    D1759_REG_LEDCTL        = 0x31,
    D1759_REG_ILIM          = 0x33, /* preinit keeps bits 0..1, sets 0x50 */
    D1759_REG_CHCTL         = 0x34, /* OF keeps bit 7 and writes 0x54 */
    D1759_REG_ADCCTL        = 0x40,
    D1759_REG_ADCLO         = 0x41, /* result bits 0..1 */
    D1759_REG_ADCHI         = 0x42, /* result bits 2..9 */
    D1759_REG_RTCSEC        = 0x50, /* 7-byte BCD, same order as Nano 3G */
};

#if 1
enum d1759_reg_eventa {
    D1759_EVENTA_VBUS       = 0x08,     /* USB: 0 -> not present */
    D1759_EVENTA_INPUT1     = 0x20,     /* accessory: 0 -> not present */
    D1759_EVENTA_UNK6       = 0x40,     // ???
};
enum d1759_reg_eventb {
    D1759_EVENTB_INPUT2     = 0x01,     /* hold switch: 0 -> locked */
    D1759_EVENTB_WARMBOOT   = 0x80,     /* TBC */
};
enum d1759_reg_eventc {
    D1759_EVENTC_VADAPTOR   = 0x08,     /* FireWire: 0 -> not present */
};

enum d1759_reg_statusa {
    D1759_STATUSA_VBUS      = 0x08,
    D1759_STATUSA_INPUT1    = 0x20,
};
enum d1759_reg_statusb {
    D1759_STATUSB_INPUT2    = 0x01,
    D1759_STATUSB_WARMBOOT  = 0x80,
};
enum d1759_reg_statusc {
    D1759_STATUSC_VADAPTOR  = 0x08,
};

enum d1759_reg_irqmaska {
    D1759_IRQMASKA_VBUS     = 0x08,
    D1759_IRQMASKA_INPUT1   = 0x20,
};
enum d1759_reg_irqmaskb {
    D1759_IRQMASKB_INPUT2   = 0x01,
    D1759_IRQMASKB_WARMBOOT = 0x80,
};
enum d1759_reg_irqmaskc {
    D1759_IRQMASKC_VADAPTOR = 0x08,
};
#else // TODO?
/* PMU interrupts */
#define D1759_EVENTA_VBUS       (1<<3)  /* USB: 0 -> not present */
#define D1759_EVENTA_INPUT1     (1<<5)  /* accessory: 0 -> not present */
#define D1759_EVENTA_UNK6       (1<<6)  // ???

#define D1759_EVENTB_INPUT2     (1<<0)  /* hold switch: 0 -> locked */
#define D1759_EVENTB_WARMBOOT   (1<<7)  /* TBC */

#define D1759_EVENTC_VADAPTOR   (1<<3)  /* FireWire: 0 -> not present */
#endif

enum d1759_reg_ledctl {
    D1759_LEDCTL_ENABLE     = 0x01,
};

enum d1759_reg_sysctrl {
    D1759_SYSCTRL_STBY      = 0x01, /* OR into 0x0a; do not replace 0x70 */
};

enum d1759_reg_chctl {
    D1759_CHCTL_ACTIVE      = 0x80, /* bit OF preserves */

    /* USB input current in D1759_REG_ILIM bits 0..1, as Apple's DxeD1759
     * picks it: 100 mA while USB is not configured, 500 mA once it is,
     * 3 for an Apple charger. (Bit 0 of CHCTL resets the SoC.) */
    D1759_ILIM_MASK         = 0x03,
    D1759_ILIM_100MA        = 0x00,
    D1759_ILIM_500MA        = 0x02,
};

/* From the OF ADC routine: ADCCTL = EN | START | mux, poll START, then read
 * the 10-bit result; mV = 2500 + raw * 2000 / 1023, as on the D1671. */
enum d1759_reg_adcctl {
    D1759_ADCCTL_MUX_VBAT   = 0x04,
    D1759_ADCCTL_START      = 0x10, /* cleared when the conversion is done */
    D1759_ADCCTL_EN         = 0x40, /* also the register's value at rest */
};


/* GPIO for external PMU interrupt */
// #define GPIO_EINT_PMU   0x7b                // TODO: I think it is not this one, or it does not work well, also this GPIO does not exist in s5l8720

// #define GPIO_EINT_PMU   5   // 0.5
// #define GPIO_EINT_PMU   14  // 1.6
#define GPIO_EINT_PMU   46  // 5.6
// #define GPIO_EINT_PMU   76  // 9.4
// #define GPIO_EINT_PMU   91  // B.3
// #define GPIO_EINT_PMU   97  // C.1
// #define GPIO_EINT_PMU   98  // C.2


struct pmu_adc_channel
{
    const char *name;
    // TODO
};

void pmu_preinit(void);
void pmu_init(void);
unsigned char pmu_read(int address);
int pmu_write(int address, unsigned char val);
int pmu_read_multiple(int address, int count, unsigned char* buffer);
int pmu_write_multiple(int address, int count, unsigned char* buffer);
#ifdef BOOTLOADER
void nano4g_backlight_now(void);
unsigned char pmu_rd(int address);
int pmu_wr(int address, unsigned char val);
int pmu_rd_multiple(int address, int count, unsigned char* buffer);
int pmu_wr_multiple(int address, int count, unsigned char* buffer);
bool pmu_is_hibernated(void);
/* D1759_REG_CHCTL as the BootROM left it, before pmu_preinit() */
extern unsigned char pmu_chctl_boot;
#endif

// void pmu_ldo_on_in_standby(unsigned int ldo, int onoff);
// void pmu_ldo_set_voltage(unsigned int ldo, unsigned char voltage);
// void pmu_ldo_power_on(unsigned int ldo);
// void pmu_ldo_power_off(unsigned int ldo);
void pmu_set_wake_condition(unsigned char condition);
void pmu_enter_standby(void);
// void pmu_nand_power(bool on);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void pmu_set_cpu_voltage(unsigned mv);
#endif
#if (CONFIG_RTC == RTC_NANO4G)
void pmu_read_rtc(unsigned char* buffer);
void pmu_write_rtc(unsigned char* buffer);
#endif
void pmu_set_usblimit(bool fast_charge);

unsigned short pmu_read_adc(const struct pmu_adc_channel *ch);
unsigned short pmu_adc_raw2mv(
        const struct pmu_adc_channel *ch, unsigned short raw);

int pmu_holdswitch_locked(void);
#if CONFIG_CHARGING
int pmu_firewire_present(void);
#endif
#ifdef IPOD_ACCESSORY_PROTOCOL
int pmu_accessory_present(void);
#endif

#endif /* __PMU_TARGET_H__ */
