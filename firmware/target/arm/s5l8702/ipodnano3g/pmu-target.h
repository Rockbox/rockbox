/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: pmu-target.h 24721 2010-02-17 15:54:48Z theseven $
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
enum d1671_regs {
    D1671_REG_EVENTA        = 0x02,
    D1671_REG_EVENTB        = 0x03,
    D1671_REG_STATUSA       = 0x04,
    D1671_REG_STATUSB       = 0x05,
    D1671_REG_IRQMASKA      = 0x06,
    D1671_REG_IRQMASKB      = 0x07,
    D1671_REG_SYSCTRLA      = 0x08,
    D1671_REG_LEDCTL        = 0x20,
    D1671_REG_CHCTL         = 0x21,     // TBC
    D1671_REG_RTCSEC        = 0x40,     // TBC
};

enum d1671_reg_eventa {
    D1671_EVENTA_VBUS       = 0x08,     /* USB: 0 -> not present */
    D1671_EVENTA_VADAPTOR   = 0x10,     /* FireWire: 0 -> not present */
    D1671_EVENTA_INPUT1     = 0x20,     /* accessory: 0 -> not present */
    D1671_EVENTA_UNK6       = 0x40,     // ???
};
enum d1671_reg_eventb {
    D1671_EVENTB_INPUT2     = 0x01,     /* hold switch: 0 -> locked */
    D1671_EVENTB_WARMBOOT   = 0x80,     // TBC
};

enum d1671_reg_statusa {
    D1671_STATUSA_VBUS      = 0x08,
    D1671_STATUSA_VADAPTOR  = 0x10,
    D1671_STATUSA_INPUT1    = 0x20,
};
enum d1671_reg_statusb {
    D1671_STATUSB_INPUT2    = 0x01,
    D1671_STATUSB_WARMBOOT  = 0x80,
};

enum d1671_reg_irqmaska {
    D1671_IRQMASKA_VBUS     = 0x08,
    D1671_IRQMASKA_VADAPTOR = 0x10,
    D1671_IRQMASKA_INPUT1   = 0x20,
};
enum d1671_reg_irqmaskb {
    D1671_IRQMASKB_INPUT2   = 0x01,
    D1671_IRQMASKB_WARMBOOT = 0x80,
};

enum d1671_reg_sysctrla {
    D1671_SYSCTRLA_GOSTDBY  = 0x01,
    D1671_SYSCTRLA_GOUNK    = 0x02,     /* enter unknown state (shutdown?) */
};

enum d1671_reg_ledctl {
    D1671_LEDCTL_UNKNOWN    = 0x40,
    D1671_LEDCTL_ENABLE     = 0x80,
};
#define D1671_LEDCTL_OUT_POS    0
#define D1671_LEDCTL_OUT_MASK   0x1f

enum d1671_reg_chctl {
    D1671_CHCTL_FASTCHRG    = 0x01,     /* 100/500mA USB limit */
};

/* GPIO for external PMU interrupt */
#define GPIO_EINT_PMU   0x7b

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
unsigned char pmu_rd(int address);
int pmu_wr(int address, unsigned char val);
int pmu_rd_multiple(int address, int count, unsigned char* buffer);
int pmu_wr_multiple(int address, int count, unsigned char* buffer);
bool pmu_is_hibernated(void);
#endif

// void pmu_ldo_on_in_standby(unsigned int ldo, int onoff);
// void pmu_ldo_set_voltage(unsigned int ldo, unsigned char voltage);
// void pmu_ldo_power_on(unsigned int ldo);
// void pmu_ldo_power_off(unsigned int ldo);
void pmu_set_wake_condition(unsigned char condition);
void pmu_enter_standby(void);
// void pmu_nand_power(bool on);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void pmu_set_cpu_voltage(bool high);
#endif
#if (CONFIG_RTC == RTC_NANO3G)
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
