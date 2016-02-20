/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Bertrik Sikken
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

/*
    Provides access to the codec/charger/rtc/adc part of the as3525.
    This part is on address 0x46 of the internal i2c bus in the as3525.
    Registers in the codec part seem to be nearly identical to the registers
    in the AS3514 (used in the "v1" versions of the sansa c200 and e200).

    I2C register description:
    * I2C2_CNTRL needs to be set to 0x51 for transfers to work at all.
      bit 0: ? possibly related to using ACKs during transfers
      bit 1: direction of transfer (0 = write, 1 = read)
      bit 2: use 2-byte slave address
    * I2C2_IMR, I2C2_RIS, I2C2_MIS, I2C2_INT_CLR interrupt bits:
      bit 2: byte read interrupt
      bit 3: byte write interrupt
      bit 4: ? possibly some kind of error status
      bit 7: ACK error
    * I2C2_SR (status register) indicates in bit 0 if a transfer is busy.
    * I2C2_SLAD0 contains the i2c slave address to read from / write to.
    * I2C2_CPSR0/1 is the divider from the peripheral clock to the i2c clock.
    * I2C2_DACNT sets the number of bytes to transfer and actually starts it.

    When a transfer is attempted to a non-existing i2c slave address,
    interrupt bit 7 is raised and DACNT is not decremented after the transfer.
 */

#include "ascodec.h"
#include "clock-target.h"
#include "kernel.h"
#include "system.h"
#include "as3525.h"
#include "i2c.h"
#include "logf.h"

#define I2C2_DATA       *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x00))
#define I2C2_SLAD0      *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x04))
#define I2C2_CNTRL      *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x0C))
#define I2C2_DACNT      *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x10))
#define I2C2_CPSR0      *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x1C))
#define I2C2_CPSR1      *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x20))
#define I2C2_IMR        *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x24))
#define I2C2_RIS        *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x28))
#define I2C2_MIS        *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x2C))
#define I2C2_SR         *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x30))
#define I2C2_INT_CLR    *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x40))
#define I2C2_SADDR      *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x44))

#define I2C2_CNTRL_MASTER     0x01
#define I2C2_CNTRL_READ       0x02
#define I2C2_CNTRL_WRITE      0x00
#define I2C2_CNTRL_RESET      0x10
#define I2C2_CNTRL_REPSTARTEN 0x40

#define I2C2_CNTRL_DEFAULT (I2C2_CNTRL_MASTER|I2C2_CNTRL_REPSTARTEN|I2C2_CNTRL_RESET)

#define I2C2_IRQ_TXEMPTY      0x04
#define I2C2_IRQ_RXFULL       0x08
#define I2C2_IRQ_RXOVER       0x10
#define I2C2_IRQ_ACKTIMEO     0x80


static struct mutex as_mtx;

#if CONFIG_CHARGING
static bool chg_status = false;
static bool endofch = false;
#endif

/* returns true when busy */
static inline bool i2c_busy(void)
{
    return (I2C2_SR & 1);
}

void i2c_init(void)
{
}

static void i2c2_init(void)
{
    int prescaler;

    /* prescaler for i2c clock */
    prescaler = AS3525_I2C_PRESCALER;
    I2C2_CPSR0 = prescaler & 0xFF;          /* 8 lsb */
    I2C2_CPSR1 = (prescaler >> 8) & 0x3;    /* 2 msb */

    /* set i2c slave address of codec part */
    I2C2_SLAD0 = AS3514_I2C_ADDR << 1;

    I2C2_CNTRL = I2C2_CNTRL_DEFAULT;

}

/* initialises the internal i2c bus and prepares for transfers to the codec */
void ascodec_init(void)
{

    mutex_init(&as_mtx);

    /* enable clock */
    bitset32(&CGU_PERI, CGU_I2C_AUDIO_MASTER_CLOCK_ENABLE);

    i2c2_init();

    /* Generate irq for usb+charge status change */
    ascodec_write(AS3514_IRQ_ENRD0,
#if CONFIG_CHARGING /* m200v4 can't charge */
        IRQ_CHGSTAT | IRQ_ENDOFCH |
#endif
        IRQ_USBSTAT);

#if CONFIG_CPU == AS3525v2
    /* XIRQ = IRQ, active low reset signal, 6mA push-pull output */
    ascodec_write_pmu(0x1a, 3, (1<<2)|3); /* 1A-3 = Out_Cntr3 register */
    /* reset for compatible with old bootloader */
    ascodec_write(AS3514_IRQ_ENRD2, 0x0);
#else
    /* Generate irq for push-pull, active high, irq on rtc+adc change */
    ascodec_write(AS3514_IRQ_ENRD2, IRQ_PUSHPULL | IRQ_HIGHACTIVE);
#endif

    VIC_INT_ENABLE = INTERRUPT_AUDIO;

    /* detect if USB was connected at startup since there is no transition */
    int data = ascodec_read(AS3514_IRQ_ENRD0);

    if(data & USB_STATUS)
        usb_insert_int();

#if CONFIG_CHARGING
    chg_status = data & CHG_STATUS;
#endif
}

/* returns false if transfer incomplete */
static bool i2c2_transfer(void)
{
    static int try = 0;

    /* wait for transfer*/
    int i = 10000;
    while (I2C2_DACNT != 0 && i--);

    if (!i) {
        if (try == 5)
            panicf("I2C2 reset failed");

        logf("reset I2C2 %d", try);

        i2c2_init();

        try++;
        return false;
    }

    try = 0;
    return true;
}

void ascodec_write(unsigned int index, unsigned int value)
{
    ascodec_lock();

#ifndef HAVE_AS3543
    if (index == AS3514_CVDD_DCDC3) /* prevent setting of the LREG_CP_not bit */
        value &= ~(1 << 5);
#endif

    do {
        /* wait if still busy */
        while (i2c_busy());

        /* start transfer */
        I2C2_SADDR = index;
        I2C2_CNTRL &= ~(1 << 1);
        I2C2_DATA = value;
        I2C2_DACNT = 1;

    } while (!i2c2_transfer());

    ascodec_unlock();
}

int ascodec_read(unsigned int index)
{
    int data;

    ascodec_lock();

    do {
        /* wait if still busy */
        while (i2c_busy());

        /* start transfer */
        I2C2_SADDR = index;
        I2C2_CNTRL |= (1 << 1);
        I2C2_DACNT = 1;

    } while (!i2c2_transfer());

    data = I2C2_DATA;

    ascodec_unlock();

    return data;
}

void ascodec_readbytes(unsigned int index, unsigned int len, unsigned char *data)
{
    unsigned int i;

    for(i = 0; i < len; i++)
        data[i] = ascodec_read(index+i);
}

#if CONFIG_CPU == AS3525v2
void ascodec_write_pmu(unsigned int index, unsigned int subreg,
                       unsigned int value)
{
    ascodec_lock();
    /* we submit consecutive requests to make sure no operations happen on the
     * i2c bus between selecting the sub register and writing to it */
    ascodec_write(AS3543_PMU_ENABLE, 8 | subreg);
    ascodec_write(index, value);

    ascodec_unlock();
}

int ascodec_read_pmu(unsigned int index, unsigned int subreg)
{
    ascodec_lock();
    /* we submit consecutive requests to make sure no operations happen on the
     * i2c bus between selecting the sub register and reading it */
    ascodec_write(AS3543_PMU_ENABLE, subreg);
    int ret = ascodec_read(index);

    ascodec_unlock();

    return ret;
}
#endif /* CONFIG_CPU == AS3525v2 */

void INT_AUDIO(void)
{
    int oldstatus = disable_irq_save();
    int data = ascodec_read(AS3514_IRQ_ENRD0);

#if CONFIG_CHARGING
    if (data & CHG_ENDOFCH) { /* chg finished */
        endofch = true;
    }

    chg_status = data & CHG_STATUS;
#endif

    if (data & USB_CHANGED) { /* usb status changed */
        if (data & USB_STATUS) {
            usb_insert_int();
        } else {
            usb_remove_int();
        }
    }

    restore_irq(oldstatus);
}

#if CONFIG_CHARGING
bool ascodec_endofch(void)
{
    int oldstatus = disable_irq_save();
    bool ret = endofch;
    endofch = false;
    restore_irq(oldstatus);

    return ret;
}

bool ascodec_chg_status(void)
{
    return chg_status;
}

void ascodec_monitor_endofch(void)
{
    /* already enabled */
}

void ascodec_write_charger(int value)
{
#if CONFIG_CPU == AS3525
    ascodec_write(AS3514_CHARGER, value);
#else
    ascodec_write_pmu(AS3543_CHARGER, 1, value);
#endif
}

int ascodec_read_charger(void)
{
#if CONFIG_CPU == AS3525
    return ascodec_read(AS3514_CHARGER);
#else
    return ascodec_read_pmu(AS3543_CHARGER, 1);
#endif
}
#endif /* CONFIG_CHARGING */

void ascodec_lock(void)
{
    mutex_lock(&as_mtx);
}

void ascodec_unlock(void)
{
    mutex_unlock(&as_mtx);
}
