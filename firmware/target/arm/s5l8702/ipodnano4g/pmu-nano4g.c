/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#include "clocking-s5l8702.h"
#include "s5l87xx.h"


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
    unsigned char tmp = 0;
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
    /* 0x0d also holds the boot-config bit preinit may have cleared. */
    pmu_write(D1759_REG_WAKE, pmu_read(D1759_REG_WAKE) | condition);
}

void pmu_enter_standby(void)
{
    /* Apple's firmware powers off with two exact writes: backlight off,
     * then 0x0a = 1 on its own. OR-ing the request into the other bits of
     * 0x0a leaves the unit on until the hold switch is toggled. */
    pmu_write(D1759_REG_LEDCTL, 0x02);
    pmu_write(D1759_REG_SYSCTRL, D1759_SYSCTRL_STBY);
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void pmu_set_cpu_voltage(unsigned mv)
{
    /* D1759 core buck, register 0x14. code = (mV - 725) / 25.
     * 1200 mV is 0x13, 1100 mV is 0x0f. 532 and 266 MHz use 1200,
     * 133, 66.5 and 33.25 MHz use 1100.
     * Register 0x15 is standby and stays at the 0x0d preinit wrote. */
    pmu_write(D1759_REG_VCORE, (mv - 725) / 25);
}
#endif

#if (CONFIG_RTC == RTC_NANO4G)
void pmu_read_rtc(unsigned char* buffer)
{
    pmu_read_multiple(D1759_REG_RTCSEC, 7, buffer);
}

void pmu_write_rtc(unsigned char* buffer)
{
    pmu_write_multiple(D1759_REG_RTCSEC, 7, buffer);
}
#endif

void pmu_set_usblimit(bool fast_charge)
{
    unsigned char val = pmu_read(D1759_REG_ILIM) & ~D1759_ILIM_MASK;

    val |= fast_charge ? D1759_ILIM_500MA : D1759_ILIM_100MA;
    pmu_write(D1759_REG_ILIM, val);
}

/*
 * ADC
 */
static struct mutex pmu_adc_mutex;

/* converts raw value to millivolts */
unsigned short pmu_adc_raw2mv(
        const struct pmu_adc_channel *ch, unsigned short raw)
{
    (void) ch;
    return 2500 + (raw * 2000) / 1023;
}

/* returns raw value of the battery channel */
unsigned short pmu_read_adc(const struct pmu_adc_channel *ch)
{
    int raw, tries;

    (void) ch;
    mutex_lock(&pmu_adc_mutex);
    pmu_write(D1759_REG_ADCCTL, D1759_ADCCTL_EN | D1759_ADCCTL_START
                              | D1759_ADCCTL_MUX_VBAT);
    for (tries = 0; tries < 20; tries++)
    {
        udelay(50);
        if (!(pmu_read(D1759_REG_ADCCTL) & D1759_ADCCTL_START))
            break;
    }
    raw = (pmu_read(D1759_REG_ADCHI) << 2)
        | (pmu_read(D1759_REG_ADCLO) & 3);
    pmu_write(D1759_REG_ADCCTL, D1759_ADCCTL_EN);
    mutex_unlock(&pmu_adc_mutex);
    return raw;
}

/*
 * eINT
 */
#define Q_EINT  0

static long pmu_thread_stack[DEFAULT_STACK_SIZE/2/sizeof(long)];
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

#include "usb.h"

#if defined(BOOTLOADER) && !defined(HAVE_BOOTLOADER_USB_MODE)   // XXX: from usb-s5l8702.c
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
    unsigned char status[3] = { 0 };
#else
    unsigned char status[2] = { 0 };
#endif

    /* A failed read keeps the last state rather than reporting an unplug */
    if (pmu_read_multiple(D1759_REG_STATUSA, sizeof(status), status))
        return;

    /* Report only changes: every USB event restarts the idle poweroff
     * timer, and the inputs are polled. */
    if (status[0] & D1759_STATUSA_VBUS)
    {
        if (usb_detect() != USB_INSERTED)
            usb_insert_int();
    }
    else if (usb_detect() != USB_EXTRACTED)
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
        /* GPIO_EINT_PMU never fires on a 16 GB Nano 4G: unplugging USB and
         * moving the hold switch latch D1759 events, and none of the
         * candidate pins changes level. USB removal also latches only
         * EVENTC 0x14, which is masked. Poll the inputs instead. */
        queue_wait_w_tmo(&pmu_queue, &ev, HZ/5);
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
                pmu_read_inputs();
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
    unsigned char val = 0;
    pmu_rd_multiple(address, 1, &val);
    return val;
}

int pmu_wr(int address, unsigned char val)
{
    return pmu_wr_multiple(address, 1, &val);
}

#ifdef BOOTLOADER
/* emcore bootstub pmuwrite, with a spin limit. The Rockbox I2C driver
 * calls udelay(), and the usec timer is not running yet. */
static int i2c_spin(volatile uint32_t *reg, uint32_t mask, bool set)
{
    for (int i = 0; i < 100000; i++) {
        uint32_t v = *reg;
        if (set) {
            if (v & mask)
                return 0;
        } else if ((v & mask) == 0) {
            return 0;
        }
    }
    return -1;
}

static int pmu_wr_bounded(unsigned char addr, unsigned char data)
{
    if (i2c_spin(&IICUNK10(0), 0xffffffff, false))
        return -1;

    IICDS(0) = 0xe6;
    IICSTAT(0) = 0xf0;
    IICCON(0) = 0xb7;
    if (i2c_spin(&IICCON(0), 0x10, true))
        return -1;

    IICDS(0) = addr;
    IICCON(0) = 0xb7;
    if (i2c_spin(&IICCON(0), 0x10, true))
        return -1;

    IICDS(0) = data;
    IICCON(0) = 0xb7;
    if (i2c_spin(&IICCON(0), 0x10, true))
        return -1;

    IICSTAT(0) = 0xd0;
    IICCON(0) = 0xb7;
    return i2c_spin(&IICSTAT(0), 0x20, false);
}

void nano4g_backlight_now(void)
{
    clockgate_enable(CLOCKGATE_I2C0, true);
    clockgate_enable(CLOCKGATE_I2C0_2, true);

    /* I2C0 pins and controller setup, as in the emcore bootstub */
    PCON2 = (PCON2 & ~0xf0000000) | 0x20000000;
    PCON3 = (PCON3 & ~0x0000000f) | 0x00000002;
    if (i2c_spin(&IICUNK10(0), 0xffffffff, false))
        return;
    IICADD(0) = 0x40;
    IICUNK14(0) = 1;
    IICUNK18(0) = 0;
    IICSTAT(0) = 0x80;
    IICCON(0) = 0;
    IICSTAT(0) = 0;
    IICDS(0) = 0x40;
    IICCON(0) = 0x181;
    IICSTAT(0) = 0x10;

    /* bootstub final values: brightness 250, LEDCTL = 3 (on, no fade) */
    if (pmu_wr_bounded(D1759_REG_LEDOUT, 250))
        return;
    pmu_wr_bounded(D1759_REG_LEDCTL, 3);
}
#endif

/* Non-zero so it stays in .data: pmu_preinit() runs before SDRAM is up */
unsigned char pmu_chctl_boot = 0xff;

void pmu_preinit(void)
{
    pmu_chctl_boot = pmu_rd(D1759_REG_CHCTL);
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
