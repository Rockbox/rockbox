/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: pmu-nano2g.c 27752 2010-08-08 10:49:32Z bertrik $
 *
 * Copyright © 2008 Rafaël Carré
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
#include "kernel.h"
#include "thread.h"

#include "pmu-target.h"
#include "adc-target.h"
#include "i2c-s5l8702.h"
#include "gpio-s5l8702.h"


int pmu_read_multiple(int address, int count, unsigned char* buffer)
{
    return i2c_read(0, 0xe6, address, count, buffer);
}

int pmu_write_multiple(int address, int count, unsigned char* buffer)
{
    return i2c_write(0, 0xe6, address, count, buffer);
}

unsigned char pmu_read(int address)
{
    unsigned char tmp;
    pmu_read_multiple(address, 1, &tmp);
    return tmp;
}

int pmu_write(int address, unsigned char val)
{
    return pmu_write_multiple(address, 1, &val);
}

#if 0
// TODO
void pmu_ldo_on_in_standby(unsigned int ldo, int onoff)
{
    if (ldo < 4)
    {
        unsigned char newval = pmu_read(0x3B) & ~(1 << (2 * ldo));
        if (onoff) newval |= 1 << (2 * ldo);
        pmu_write(0x3B, newval);
    }
    else if (ldo < 8)
    {
        unsigned char newval = pmu_read(0x3C) & ~(1 << (2 * (ldo - 4)));
        if (onoff) newval |= 1 << (2 * (ldo - 4));
        pmu_write(0x3C, newval);
    }
}

void pmu_ldo_set_voltage(unsigned int ldo, unsigned char voltage)
{
    if (ldo > 6) return;
    pmu_write(0x2d + (ldo << 1), voltage);
}

void pmu_hdd_power(bool on)
{
    pmu_write(0x1b, on ? 1 : 0);
}

void pmu_ldo_power_on(unsigned int ldo)
{
    if (ldo > 6) return;
    pmu_write(0x2e + (ldo << 1), 1);
}

void pmu_ldo_power_off(unsigned int ldo)
{
    if (ldo > 6) return;
    pmu_write(0x2e + (ldo << 1), 0);
}
#endif

void pmu_set_wake_condition(unsigned char condition)
{
    // TODO
    (void) condition;
}

void pmu_enter_standby(void)
{
    pmu_write(D1671_REG_SYSCTRLA, D1671_SYSCTRLA_GOSTDBY);
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void pmu_set_cpu_voltage(bool high)
{
    // TODO
    (void) high;
}
#endif

#if (CONFIG_RTC == RTC_NANO3G)
void pmu_read_rtc(unsigned char* buffer)
{
    // TODO
    (void) buffer;
}

void pmu_write_rtc(unsigned char* buffer)
{
    // TODO
    (void) buffer;
}
#endif

void pmu_set_usblimit(bool fast_charge)
{
    pmu_write(D1671_REG_CHCTL,
            (pmu_read(D1671_REG_CHCTL) & ~D1671_CHCTL_FASTCHRG) | fast_charge);
}

/*
 * ADC
 */
static struct mutex pmu_adc_mutex;

/* converts raw value to millivolts */
unsigned short pmu_adc_raw2mv(
        const struct pmu_adc_channel *ch, unsigned short raw)
{
    // TODO
    (void) ch;
    (void) raw;
    return 0;
}

/* returns raw value */
unsigned short pmu_read_adc(const struct pmu_adc_channel *ch)
{
    // TODO
    int raw = 0;
    mutex_lock(&pmu_adc_mutex);
    (void) ch;
    mutex_unlock(&pmu_adc_mutex);
    return raw;
}

/*
 * eINT
 */
#define Q_EINT  0

static char pmu_thread_stack[DEFAULT_STACK_SIZE/2];
static struct event_queue pmu_queue;
static unsigned char ints_msk[2];

static void pmu_eint_isr(struct eic_handler*);

static struct eic_handler pmu_eint =
{
    .gpio_n = GPIO_EINT_PMU,
    .type   = EIC_INTTYPE_LEVEL,
    .level  = EIC_INTLEVEL_LOW,
    .isr    = pmu_eint_isr,
};

static int pmu_input_holdswitch;

int pmu_holdswitch_locked(void)
{
   return pmu_input_holdswitch;
}

#ifdef IPOD_ACCESSORY_PROTOCOL
static int pmu_input_accessory;

int pmu_accessory_present(void)
{
   return pmu_input_accessory;
}
#endif

#if CONFIG_CHARGING
static int pmu_input_firewire;

int pmu_firewire_present(void)
{
   return pmu_input_firewire;
}
#endif

#if 1   // XXX: from usb-s5l8702.c
#include "usb.h"
static int usb_status = USB_EXTRACTED;

int usb_detect(void)
{
    return usb_status;
}

void usb_insert_int(void)
{
    usb_status = USB_INSERTED;
}

void usb_remove_int(void)
{
    usb_status = USB_EXTRACTED;
}
#endif

static void pmu_read_inputs(void)
{
    unsigned char status[2];

    pmu_read_multiple(D1671_REG_STATUSA, 2, status);

    if (status[0] & D1671_STATUSA_VBUS)
        usb_insert_int();
    else
        usb_remove_int();

#if CONFIG_CHARGING
    pmu_input_firewire = !!(status[0] & D1671_STATUSA_VADAPTOR);
#endif
#ifdef IPOD_ACCESSORY_PROTOCOL
    pmu_input_accessory = !!(status[0] & D1671_STATUSA_INPUT1);
#endif
    pmu_input_holdswitch = !(status[1] & D1671_STATUSB_INPUT2);
}

static void pmu_eint_isr(struct eic_handler *h)
{
     eint_unregister(h);
     queue_post(&pmu_queue, Q_EINT, 0);
}

static void NORETURN_ATTR pmu_thread(void)
{
    struct queue_event ev;

    while (true)
    {
        queue_wait_w_tmo(&pmu_queue, &ev, TIMEOUT_BLOCK);
        switch (ev.id)
        {
            case Q_EINT:
                /* clear all PMU interrupts, this will also raise
                   (disable) the PMU IRQ pin */
                pmu_write_multiple(D1671_REG_EVENTA, 2, "\xFF\xFF");

                /* get actual values */
                pmu_read_inputs();

                eint_register(&pmu_eint);
                break;

            case SYS_TIMEOUT:
                break;
        }
    }
}

/* main init */
void pmu_init(void)
{
    mutex_init(&pmu_adc_mutex);
    queue_init(&pmu_queue, false);

    create_thread(pmu_thread,
            pmu_thread_stack, sizeof(pmu_thread_stack), 0,
            "PMU" IF_PRIO(, PRIORITY_SYSTEM) IF_COP(, CPU));

    /* configure PMU interrutps */
    ints_msk[0] = 0xff;
    ints_msk[1] = 0xff;

    ints_msk[0] &= ~D1671_IRQMASKA_VBUS;      /* USB */
#if CONFIG_CHARGING
    ints_msk[0] &= ~D1671_IRQMASKA_VADAPTOR;  /* FireWire */
#endif
#ifdef IPOD_ACCESSORY_PROTOCOL
    ints_msk[0] &= ~D1671_IRQMASKA_INPUT1;    /* Accessory */
#endif
    ints_msk[1] &= ~D1671_IRQMASKB_INPUT2;    /* Holdswitch */

    pmu_write_multiple(D1671_REG_IRQMASKA, 2, ints_msk);

    /* clear all interrupts */
    pmu_write_multiple(D1671_REG_EVENTA, 2, "\xFF\xFF");

    /* get initial values */
    pmu_read_inputs();

    eint_register(&pmu_eint);
}

/*
 * preinit
 */
int pmu_rd_multiple(int address, int count, unsigned char* buffer)
{
    return i2c_rd(0, 0xe6, address, count, buffer);
}

int pmu_wr_multiple(int address, int count, unsigned char* buffer)
{
    return i2c_wr(0, 0xe6, address, count, buffer);
}

unsigned char pmu_rd(int address)
{
    unsigned char val;
    pmu_rd_multiple(address, 1, &val);
    return val;
}

int pmu_wr(int address, unsigned char val)
{
    return pmu_wr_multiple(address, 1, &val);
}

void pmu_preinit(void)
{
    // TBC: LDOs ???
    pmu_wr(0x1b, 0x14);
    pmu_wr(0x16, 0x14);
    pmu_wr(0x15, 0x14);     // TBC: Vnand = 2000 + val*50 = 3000 mV
    pmu_wr(0x18, 0x18);     // TBC TBC TBC: Vaccy = 3200 mV ???
    pmu_wr(0x10, (pmu_rd(0x10) & 0xdb) | 0x8);  // TBC: bit4 is related to NAND, LDO_0x15 on/off ???

                            // TBC: 0x30, 0x31 y 0x32 seems related to ADC (norboot)
    pmu_wr(0x34, 0x72);     // TBC: en DA9030: TBATHIGH (0-255, TBAT high temperature threshold
    pmu_wr(0x30, pmu_rd(0x30) | 0x20);  // TBC: TBATREF ON ???

    pmu_wr(0x21, 0x5c);     // TBC: CHRG_CTL (max. current and Vbat ???)
    pmu_wr(0xb, 0x6);
    pmu_wr(0x1d, 0);

    /* configure and clear interrupts */
    pmu_wr_multiple(D1671_REG_IRQMASKA, 2, "\x42\xBE");
    pmu_wr_multiple(D1671_REG_EVENTA, 2, "\xFF\xFF");

    /* backlight off */
    pmu_wr(D1671_REG_LEDCTL, pmu_rd(D1671_REG_LEDCTL) & ~D1671_LEDCTL_ENABLE);
}

#ifdef BOOTLOADER
bool pmu_is_hibernated(void)
{
    return !!(pmu_rd(D1671_REG_EVENTB) & D1671_EVENTB_WARMBOOT);
}
#endif
