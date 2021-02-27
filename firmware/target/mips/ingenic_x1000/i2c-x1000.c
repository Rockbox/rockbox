/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "i2c-x1000.h"
#include "system.h"
#include "kernel.h"
#include "gpio-x1000.h"
#include "irq-x1000.h"
#include "x1000/i2c.h"
#include "x1000/cpm.h"

#define FIFO_SIZE   64  /* Max number of entries in RX/TX FIFO */
#define FIFO_TX_MIN 8   /* Minimum amount of entries to keep in TX FIFO */

struct i2c_gpio_data {
    int port;
    unsigned pins;
    int func;
};

struct i2c_channel {
    /* bus lock mutex, transfer completion semaphore */
    struct mutex mutex;
    struct semaphore sema;
    /* most recently set frequency */
    int freq;
    /* transfer parameters filled in by i2c_read/i2c_write */
    int write;
    int flags;
    int count;
    unsigned char* buf;
    /* these are initialized to "count" and count down to 0 */
    int bytes_rx;
    int bytes_tx;
    /* return code to caller */
    volatile int status;
};

static const struct i2c_gpio_data i2c_gpio_data[] = {
    {GPIO_B, 3 << 23, GPIO_DEVICE(0)},
    {GPIO_C, 3 << 26, GPIO_DEVICE(0)},
    /* Note: I2C1 is also on the following pins (normally used by LCD) */
    /* {GPIO_A, 3 <<  0, GPIO_DEVICE(2)}, */
    {GPIO_D, 3 <<  0, GPIO_DEVICE(1)},
};

static struct i2c_channel i2c_channels[3];

static void i2c_gate(int chn, int gate)
{
    switch(chn) {
    case 0: jz_writef(CPM_CLKGR, I2C0(gate)); break;
    case 1: jz_writef(CPM_CLKGR, I2C1(gate)); break;
    case 2: jz_writef(CPM_CLKGR, I2C2(gate)); break;
    default: break;
    }
}

static void i2c_enable(int chn)
{
    /* Enable controller */
    jz_writef(I2C_ENABLE(chn), ACTIVE(1));
    long deadline = current_tick + HZ/2;
    while(jz_readf(I2C_ENBST(chn), ACTIVE) == 0 &&
          TIME_BEFORE(current_tick, deadline))
        sleep(1);

    /* Enable IRQ */
    jz_write(I2C_INTMSK(chn), 0);
    system_enable_irq(IRQ_I2C(chn));
}

static void i2c_disable(int chn)
{
    /* Disable IRQ */
    system_disable_irq(IRQ_I2C(chn));

    /* Disable controller */
    jz_writef(I2C_ENABLE(chn), ACTIVE(0));
    long deadline = current_tick + HZ/2;
    while(jz_readf(I2C_ENBST(chn), ACTIVE) &&
          TIME_BEFORE(current_tick, deadline))
        sleep(1);
}

static void i2c_reset(int chn)
{
    /* Should clear any errors */
    REG_I2C_INTMSK(chn) = 0;
    REG_I2C_CINT(chn);
    i2c_disable(chn);
    mdelay(10);
    i2c_enable(chn);
}

static void i2c_interrupt(int chn)
{
    struct i2c_channel* c = &i2c_channels[chn];
    int intr = REG_I2C_INTST(chn);

    if(intr & jz_orm(I2C_INTST, TXABT)) {
        /* Bus problem */
        REG_I2C_CTXABT(chn);
        c->status = I2C_ERR_BUS;
        goto _err;
    }

    if(intr & jz_orm(I2C_INTST, RXUF, TXOF, RXOF)) {
        /* None of these should happen */
        REG_I2C_CTXOF(chn);
        REG_I2C_CRXOF(chn);
        REG_I2C_CRXUF(chn);
        c->status = I2C_ERR_DRIVER;
        goto _err;
    }

    /* Drain the RX queue */
    while(REG_I2C_RXFLR(chn)) {
        if(c->bytes_rx > 0) {
            *c->buf++ = jz_readf(I2C_DC(chn), DAT);
            c->bytes_rx -= 1;
        } else {
            /* Shouldn't happen */
            c->status = I2C_ERR_DRIVER;
            goto _err;
        }
    }

    if(c->bytes_tx > 0) {
        /* Find the amount of free space in the TX queue */
        int txflr = REG_I2C_TXFLR(chn);
        int to_tx = FIFO_SIZE - txflr;

        /* Leave 2 free when reading. One byte might be transmitting and thus
         * not accounted for in TXFLR or RXFLR. The second byte accounts for
         * the remote possibility that a byte was recieved in the time between
         * reading RXFLR as 0 and reading TXFLR above; in this case TXFLR may
         * have already been decremented. It's assumed only one byte could be
         * recieved in this time -- the CPU should be way faster than I2C.
         */
        if(!c->write)
            to_tx -= 2;

        /* Don't transmit more data than we have */
        if(c->bytes_tx < to_tx)
            to_tx = c->bytes_tx;

        /* Write data to TX queue */
        while(to_tx > 0) {
            unsigned dc;
            if(c->write)
                dc = *c->buf++;
            else
                dc = jz_orm(I2C_DC, CMD);

            /* Send RESTART on the first byte */
            if(c->bytes_tx == c->count && (c->flags & I2C_RESTART))
                dc |= jz_orm(I2C_DC, RESTART);

            /* Send STOP on the last byte */
            if(c->bytes_tx == 1 && (c->flags & I2C_STOP))
                dc |= jz_orm(I2C_DC, STOP);

            REG_I2C_DC(chn) = dc;
            c->bytes_tx -= 1;
            to_tx -= 1;
        }
    }

    /* Set RX FIFO interrupt */
    if(c->bytes_rx == 0)
        jz_writef(I2C_INTMSK(chn), RXFL(0));
    else if(c->bytes_rx >= FIFO_SIZE)
        REG_I2C_RXTL(chn) = FIFO_SIZE - 1;
    else
        REG_I2C_RXTL(chn) = c->bytes_rx - 1;

    /* Set TX FIFO interrupt */
    if(c->bytes_tx == 0)
        jz_writef(I2C_INTMSK(chn), TXEMP(0));
    else if(c->bytes_tx >= FIFO_SIZE - FIFO_TX_MIN)
        REG_I2C_TXTL(chn) = FIFO_TX_MIN;
    else
        REG_I2C_TXTL(chn) = FIFO_SIZE - c->bytes_tx;

    /* Check for completion */
    if(c->bytes_tx == 0 && c->bytes_rx == 0) {
      _err:
        jz_write(I2C_INTMSK(chn), 0);
        semaphore_release(&c->sema);
    }
}

static long i2c_calc_xfer_tmo(int freq, int count)
{
    /* 100ms base delay plus data-dependent amount */
    return (HZ/10) + (HZ * 8 * count / freq);
}

static int i2c_xfer(int write, int chn, int addr, int flag,
                    int count, unsigned char* data)
{
    /* Do nothing */
    if(count == 0)
        return 0;

    /* Fill driver parameters */
    struct i2c_channel* c = &i2c_channels[chn];
    c->write = write;
    c->flags = flag;
    c->buf = data;
    c->count = count;
    c->bytes_rx = write ? 0 : count;
    c->bytes_tx = count;
    c->status = 0;

    /* Program registers */
    REG_I2C_CINT(chn);
    REG_I2C_TAR(chn) = addr & (0x3ff|I2C_10BITADDR);
    REG_I2C_RXTL(chn) = 0;
    REG_I2C_TXTL(chn) = 0;
    jz_writef(I2C_INTMSK(chn),
              TXABT(1), RXOF(1), RXUF(1),
              TXOF(1), RXFL(1), TXEMP(1));

    /* Wait for completion */
    long tmo_dur = i2c_calc_xfer_tmo(c->freq, count);
    if(semaphore_wait(&c->sema, tmo_dur) == OBJ_WAIT_TIMEDOUT) {
        i2c_reset(chn);
        return I2C_ERR_TIMEOUT;
    }

    /* TODO: Avoid resetting the bus if we can recover some other way */
    if(c->status != 0)
        i2c_reset(chn);

    return c->status;
}

/* Public API */
void i2c_init(void)
{
    for(int i = 0; i < 3; ++i) {
        mutex_init(&i2c_channels[i].mutex);
        semaphore_init(&i2c_channels[i].sema, 1, 0);
    }
}

void i2c_set_freq(int chn, int freq)
{
    /* Store frequency */
    i2c_channels[chn].freq = freq;

    /* Disable the channel if previously active */
    i2c_gate(chn, 0);
    if(jz_readf(I2C_ENBST(chn), ACTIVE) == 1)
        i2c_disable(chn);

    /* Request to shut down the channel */
    if(freq == 0) {
        i2c_gate(chn, 1);
        return;
    }

    /* Calculate timing parameters */
    unsigned pclk = clk_get_pclk();
    unsigned t_SU_DAT = pclk / (freq * 8);
    unsigned t_HD_DAT = pclk / (freq * 12);
    unsigned t_LOW    = pclk / (freq * 2);
    unsigned t_HIGH   = pclk / (freq * 2);
    if(t_SU_DAT > 1)      t_SU_DAT -= 1;
    if(t_SU_DAT > 255)    t_SU_DAT = 255;
    if(t_SU_DAT == 0)     t_SU_DAT = 0;
    if(t_HD_DAT > 0xffff) t_HD_DAT = 0xfff;
    if(t_LOW < 8)         t_LOW = 8;
    if(t_HIGH < 6)        t_HIGH = 6;

    /* Control register setting */
    unsigned reg = jz_orf(I2C_CON, SLVDIS(1), RESTART(1), MD(1));
    if(freq <= I2C_FREQ_100K)
        reg |= jz_orf(I2C_CON, SPEED_V(100K));
    else
        reg |= jz_orf(I2C_CON, SPEED_V(400K));

    /* Write the new controller settings */
    jz_write(I2C_CON(chn), reg);
    jz_write(I2C_SDASU(chn), t_SU_DAT);
    jz_write(I2C_SDAHD(chn), t_HD_DAT);
    if(freq <= I2C_FREQ_100K) {
        jz_write(I2C_SLCNT(chn), t_LOW);
        jz_write(I2C_SHCNT(chn), t_HIGH);
    } else {
        jz_write(I2C_FLCNT(chn), t_LOW);
        jz_write(I2C_FHCNT(chn), t_HIGH);
    }

    /* Claim pins */
    gpio_config(i2c_gpio_data[chn].port,
                i2c_gpio_data[chn].pins,
                i2c_gpio_data[chn].func);

    /* Enable the controller */
    i2c_enable(chn);
}

void i2c_lock(int chn)
{
    mutex_lock(&i2c_channels[chn].mutex);
}

void i2c_unlock(int chn)
{
    mutex_unlock(&i2c_channels[chn].mutex);
}

int i2c_read(int chn, int addr, int flag,
             int count, unsigned char* data)
{
    return i2c_xfer(0, chn, addr, flag, count, data);
}

int i2c_write(int chn, int addr, int flag,
              int count, const unsigned char* data)
{
    return i2c_xfer(1, chn, addr, flag, count, (unsigned char*)data);
}

void I2C0(void)
{
    i2c_interrupt(0);
}

void I2C1(void)
{
    i2c_interrupt(1);
}

void I2C2(void)
{
    i2c_interrupt(2);
}
