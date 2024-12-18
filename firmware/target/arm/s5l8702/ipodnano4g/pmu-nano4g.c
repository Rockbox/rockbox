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
    // TODO
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void pmu_set_cpu_voltage(bool high)
{
    // TODO
    (void) high;
}
#endif

#if (CONFIG_RTC == RTC_NANO4G)
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
    // TODO
    (void) fast_charge;
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
static unsigned char ints_msk[3];

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
#if CONFIG_CHARGING
    unsigned char status[3];
#else
    unsigned char status[2];
#endif

    pmu_read_multiple(D1759_REG_STATUSA, sizeof(status), status);

    if (status[0] & D1759_STATUSA_VBUS)
        usb_insert_int();
    else
        usb_remove_int();

#ifdef IPOD_ACCESSORY_PROTOCOL
    pmu_input_accessory = !!(status[0] & D1759_STATUSA_INPUT1);
#endif
    pmu_input_holdswitch = !(status[1] & D1759_STATUSB_INPUT2);
#if CONFIG_CHARGING
    pmu_input_firewire = !!(status[2] & D1759_STATUSC_VADAPTOR);
#endif
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
#if CONFIG_CHARGING
                pmu_write_multiple(D1759_REG_EVENTA, 3, "\xFF\xFF\xFF");
#else
                pmu_write_multiple(D1759_REG_EVENTA, 2, "\xFF\xFF");
#endif

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
    ints_msk[2] = 0xff;

    ints_msk[0] &= ~D1759_IRQMASKA_VBUS;      /* USB */
#ifdef IPOD_ACCESSORY_PROTOCOL
    ints_msk[0] &= ~D1759_IRQMASKA_INPUT1;    /* Accessory */
#endif
    ints_msk[1] &= ~D1759_IRQMASKB_INPUT2;    /* Holdswitch */
#if CONFIG_CHARGING
    ints_msk[2] &= ~D1759_IRQMASKC_VADAPTOR;  /* FireWire */
#endif

    pmu_write_multiple(D1759_REG_IRQMASKA, 3, ints_msk);

    /* clear all interrupts */
    pmu_write_multiple(D1759_REG_EVENTA, 3, "\xFF\xFF\xFF");

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
    pmu_wr(0x14, 0x13);
    pmu_wr(0x15, 0x0d);
    pmu_wr(0xb, 0x22);
    if (pmu_rd(0x7f) & 1)
        pmu_wr(0xd, (pmu_rd(0xd) & 0xdf));
//     if (unknown_boot_config & 1)                  // XXX: could it be disabling some LDO??? remember that in this type of PMU the LDO enable bits are
//         pmu_wr(0xd, (pmu_rd(0xd) & 0xdf));      //      located in other registers different from the voltage configuration???

    // TBC: LDOs ???
    pmu_wr(0x1f, 0x14);
    pmu_wr(0x1a, 0xb2);         // XXX: 0xA & 0x12, maybe it's an option to always ON, or ON in stby or something like that, could it be the ARM7 ???
    pmu_wr(0x1a, 0xb2);         // XXX: repeat, posibly a typo ??? ??? ???
    pmu_wr(0x19, 0x14);         // XXX: TBC: NAND (by similarity with nano3g)
    pmu_wr(0x21, 0x6);
    pmu_wr(0x1d, 0x12);
    pmu_wr(0x10, (pmu_rd(0x10) & 0x7f) | 0x60);

    pmu_wr(0x44, 0x72);                             // TBC: 0x40+ seems related to ADC
    pmu_wr(0x40, pmu_rd(0x40) | 0x40);

    pmu_wr(0x33, (pmu_rd(0x33) & 0x3) | 0x50);
    pmu_wr(0x34, (pmu_rd(0x34) & 0x80) | 0x54);     // XXX: see DA9030 page 110 charge_control REG 0x28
    pmu_wr(0x22, 0);

    /* configure and clear interrupts */
    pmu_wr_multiple(D1759_REG_IRQMASKA, 3, "\x50\xFE\x2B");
    pmu_wr_multiple(D1759_REG_EVENTA, 3, "\xFF\xFF\xFF");

    pmu_wr(D1759_REG_LEDOUT, 0x64); /* set backlight brightness to 40% */
    pmu_wr(D1759_REG_LEDCTL,
            pmu_rd(D1759_REG_LEDCTL) | D1759_LEDCTL_ENABLE); /* backlight on */

    pmu_wr(0xa, 0x70);
    pmu_wr(0x13, 0x02);
}

#ifdef BOOTLOADER
bool pmu_is_hibernated(void)
{
    return !!(pmu_rd(D1759_REG_EVENTB) & D1759_EVENTB_WARMBOOT);
}
#endif
