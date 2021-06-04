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

/* #define LOGF_ENABLE */
#include "i2c-x1000.h"
#include "system.h"
#include "kernel.h"
#include "panic.h"
#include "logf.h"
#include "clk-x1000.h"
#include "irq-x1000.h"
#include "x1000/i2c.h"
#include "x1000/cpm.h"

#if I2C_ASYNC_BUS_COUNT != 3
# error "Wrong I2C_ASYNC_BUS_COUNT (should be 3)"
#endif

#define FIFO_SIZE       64  /* Size of data FIFOs */
#define FIFO_TX_THRESH  16  /* Wake up when TX FIFO gets to this level */
#define FIFO_RX_SLACK   2   /* Slack space to leave, avoids RX FIFO overflow */

typedef struct i2c_x1000_bus {
    /* Hardware channel, this is just equal to i2c-async bus number. */
    int chn;

    /* Buffer/count usage depends on what phase the bus is processing:
     *
     * - Phase1: writing out descriptor's buffer[0] for both READs and WRITEs.
     * - Phase2: writing out descriptor's buffer[1] for WRITEs, or issuing a
     *           series of read requests for READs.
     *
     * In phase1, buffer[1] and count[1] are equal to the descriptor's copy.
     * buffer[0] and count[0] get incremented/decremented as we send bytes.
     * Phase1 is only visited if we actually need to send bytes; if there
     * would be no data in this phase then __i2c_async_submit() sets up the
     * driver to go directly to phase2.
     *
     * Phase2 begins after phase1 writes out its last byte, or if phase1 was
     * skipped at submit time. For WRITEs phase2 is identical to phase1 so we
     * copy over buffer[1] and count[1] to buffer[0] and count[0], and zero
     * out buffer[1] and count[1].
     *
     * For READs phase2 sets buffer[0] to NULL and count[0] equal to count[1].
     * Now count[0] counts the number of bytes left to request, and count[1]
     * counts the number of bytes left to receive. i2c_x1000_fifo_write() sees
     * that buffer[0] is NULL and sends read requests instead of data bytes.
     * buffer[1] is advanced by i2c_x1000_fifo_read() we receive bytes.
     */
    unsigned char* buffer[2];
    int count[2];
    bool phase1;

    /* Copied fields from descriptor */
    uint8_t bus_cond;
    uint8_t tran_mode;

    /* Counter to keep track of when to send end conditions */
    int byte_cnt;
    int byte_cnt_end;

    /* Current bus frequency, used to calculate timeout durations */
    long freq;

    /* Timeout to reset the bus in case of buggy devices */
    struct timeout tmo;

    /* Flag used to indicate a reset is processing */
    int resetting;
} i2c_x1000_bus;

static i2c_x1000_bus i2c_x1000_busses[3];

static void i2c_x1000_fifo_write(i2c_x1000_bus* bus)
{
    int tx_free, tx_n;

    /* Get free space in FIFO */
    tx_free = FIFO_SIZE - REG_I2C_TXFLR(bus->chn);

  _again:
    /* Leave some slack space when reading. If we submit a full FIFO's worth
     * of read requests, there's a small chance that a byte "on the wire" is
     * unaccounted for and causes an RX FIFO overrun. Slack space is meant to
     * avoid this situation.
     */
    if(bus->tran_mode == I2C_READ) {
        tx_free -= FIFO_RX_SLACK;
        if(tx_free <= 0)
            goto _end;
    }

    /* Calculate number of bytes needed to send/request */
    tx_n = MIN(tx_free, bus->count[0]);

    /* Account for bytes we're about to send/request */
    bus->count[0] -= tx_n;
    tx_free -= tx_n;

    for(; tx_n > 0; --tx_n) {
        bus->byte_cnt += 1;

        /* Read data byte or set read request bit */
        uint32_t dc = bus->buffer[0] ? *bus->buffer[0]++ : jz_orm(I2C_DC, CMD);

        /* Check for first byte & apply RESTART.
         * Note the HW handles RESTART automatically when changing the
         * direction of the transfer, so we don't need to check for that.
         */
        if(bus->byte_cnt == 1 && (bus->bus_cond & I2C_RESTART))
            dc |= jz_orm(I2C_DC, RESTART);

        /* Check for last byte & apply STOP */
        if(bus->byte_cnt == bus->byte_cnt_end && (bus->bus_cond & I2C_STOP))
            dc |= jz_orm(I2C_DC, STOP);

        /* Add entry to FIFO */
        REG_I2C_DC(bus->chn) = dc;
    }

    /* FIFO full and current phase still has data to send.
     * Configure interrupt to fire when there's a good amount of free space.
     */
    if(bus->count[0] > 0) {
      _end:
        REG_I2C_TXTL(bus->chn) = FIFO_TX_THRESH;
        jz_writef(I2C_INTMSK(bus->chn), TXEMP(1), RXFL(0));
        return;
    }

    /* Advance to second phase if needed */
    if(bus->phase1 && bus->count[1] > 0) {
        bus->buffer[0] = bus->tran_mode == I2C_WRITE ? bus->buffer[1] : NULL;
        bus->count[0] = bus->count[1];
        bus->phase1 = false;

        /* Submit further data if possible; else wait for TX space */
        if(tx_free > 0)
            goto _again;
        else
            goto _end;
    }

    /* All phases are done. Now we just need to wake up when the whole
     * operation is complete, either by waiting for TX to drain or RX to
     * fill to the appropriate level. */
    if(bus->tran_mode == I2C_WRITE) {
        REG_I2C_TXTL(bus->chn) = 0;
        jz_writef(I2C_INTMSK(bus->chn), TXEMP(1), RXFL(0));
    } else {
        REG_I2C_RXTL(bus->chn) = bus->count[1] - 1;
        jz_writef(I2C_INTMSK(bus->chn), TXEMP(0), RXFL(1));
    }
}

static void i2c_x1000_fifo_read(i2c_x1000_bus* bus)
{
    /* Get number of bytes in the RX FIFO */
    int rx_n = REG_I2C_RXFLR(bus->chn);

    /* Shouldn't happen, but check just in case */
    if(rx_n > bus->count[1]) {
        panicf("i2c_x1000(%d): expected %d bytes in RX fifo, got %d",
               bus->chn, bus->count[1], rx_n);
    }

    /* Fill buffer with data from FIFO */
    bus->count[1] -= rx_n;
    for(; rx_n != 0; --rx_n) {
        *bus->buffer[1]++ = jz_readf(I2C_DC(bus->chn), DAT);
    }
}

static void i2c_x1000_interrupt(i2c_x1000_bus* bus)
{
    int intr = REG_I2C_INTST(bus->chn);
    int status = I2C_STATUS_OK;

    /* Bus error; we can't prevent this from happening. As I understand
     * it, we cannot get a TXABT when the bus is idle, so it should be
     * safe to leave this interrupt unmasked all the time.
     */
    if(intr & jz_orm(I2C_INTST, TXABT)) {
        logf("i2c_x1000(%d): got TXABT (%08lx)",
             bus->chn, REG_I2C_ABTSRC(bus->chn));

        REG_I2C_CTXABT(bus->chn);
        status = I2C_STATUS_ERROR;
        goto _done;
    }

    /* FIFO errors shouldn't occur unless driver did something dumb */
    if(intr & jz_orm(I2C_INTST, RXUF, TXOF, RXOF)) {
#if 1
        panicf("i2c_x1000(%d): fifo error (%08x)", bus->chn, intr);
#else
        /* This is how the error condition would be cleared */
        REG_I2C_CTXOF(bus->chn);
        REG_I2C_CRXOF(bus->chn);
        REG_I2C_CRXUF(bus->chn);
        status = I2C_STATUS_ERROR;
        goto _done;
#endif
    }

    /* Read from FIFO on reads, and check if we have sent/received
     * the expected amount of data. If so, complete the descriptor. */
    if(bus->tran_mode == I2C_READ) {
        i2c_x1000_fifo_read(bus);
        if(bus->count[1] == 0)
            goto _done;
    } else if(bus->count[0] == 0) {
        goto _done;
    }

    /* Still need to send or request data -- issue commands to FIFO */
    i2c_x1000_fifo_write(bus);
    return;

  _done:
    jz_writef(I2C_INTMSK(bus->chn), TXEMP(0), RXFL(0));
    timeout_cancel(&bus->tmo);
    __i2c_async_complete_callback(bus->chn, status);
}

void I2C0(void)
{
    i2c_x1000_interrupt(&i2c_x1000_busses[0]);
}

void I2C1(void)
{
    i2c_x1000_interrupt(&i2c_x1000_busses[1]);
}

void I2C2(void)
{
    i2c_x1000_interrupt(&i2c_x1000_busses[2]);
}

static int i2c_x1000_bus_timeout(struct timeout* tmo)
{
    /* Buggy device is preventing the operation from completing, so we
     * can't do much except reset the bus and hope for the best. Device
     * drivers can aid us by detecting the TIMEOUT status we return and
     * resetting the device to get it out of a bugged state. */

    i2c_x1000_bus* bus = (i2c_x1000_bus*)tmo->data;
    switch(bus->resetting) {
    default:
        /* Start of reset. Disable the controller */
        REG_I2C_INTMSK(bus->chn) = 0;
        bus->resetting = 1;
        jz_writef(I2C_ENABLE(bus->chn), ACTIVE(0));
        return 1;
    case 1:
        /* Check if controller is disabled yet */
        if(jz_readf(I2C_ENBST(bus->chn), ACTIVE))
            return 1;

        /* Wait 10 ms after disabling to give time for bus to clear */
        bus->resetting = 2;
        return HZ/100;
    case 2:
        /* Re-enable the controller */
        bus->resetting = 3;
        jz_writef(I2C_ENABLE(bus->chn), ACTIVE(1));
        return 1;
    case 3:
        /* Check that controller is enabled */
        if(jz_readf(I2C_ENBST(bus->chn), ACTIVE) == 0)
            return 1;

        /* Reset complete */
        bus->resetting = 0;
        jz_overwritef(I2C_INTMSK(bus->chn), RXFL(0), TXEMP(0),
                      TXABT(1), TXOF(1), RXOF(1), RXUF(1));
        __i2c_async_complete_callback(bus->chn, I2C_STATUS_TIMEOUT);
        return 0;
    }
}

void __i2c_async_submit(int busnr, i2c_descriptor* desc)
{
    i2c_x1000_bus* bus = &i2c_x1000_busses[busnr];

    if(desc->tran_mode == I2C_READ) {
        if(desc->count[0] > 0) {
            /* Handle initial write as phase1 */
            bus->buffer[0] = desc->buffer[0];
            bus->count[0] = desc->count[0];
            bus->phase1 = true;
        } else {
            /* No initial write, skip directly to phase2 */
            bus->buffer[0] = NULL;
            bus->count[0] = desc->count[1];
            bus->phase1 = false;
        }

        /* Set buffer/count for phase2 read */
        bus->buffer[1] = desc->buffer[1];
        bus->count[1] = desc->count[1];
    } else {
        /* Writes always have to have buffer[0] populated; buffer[1] is
         * allowed to be NULL (and thus count[1] = 0). This matches our
         * phase logic so no need for anything special
         */
        bus->buffer[0] = desc->buffer[0];
        bus->count[0] = desc->count[0];
        bus->buffer[1] = desc->buffer[1];
        bus->count[1] = desc->count[1];
        bus->phase1 = true;
    }

    /* Save bus condition and transfer mode */
    bus->bus_cond = desc->bus_cond;
    bus->tran_mode = desc->tran_mode;

    /* Byte counter is used to check for first and last byte and apply
     * the requested end mode */
    bus->byte_cnt = 0;
    bus->byte_cnt_end = desc->count[0] + desc->count[1];

    /* Ensure interrupts are cleared */
    REG_I2C_CINT(busnr);

    /* Program target address */
    jz_overwritef(I2C_TAR(busnr), ADDR(desc->slave_addr & ~I2C_10BIT_ADDR),
                  10BITS(desc->slave_addr & I2C_10BIT_ADDR ? 1 : 0));

    /* Do the initial FIFO fill; this sets up the needed interrupts. */
    i2c_x1000_fifo_write(bus);

    /* Software timeout to deal with buggy slave devices that pull the bus
     * high forever and leave us hanging. Use 100ms + whatever time should
     * be needed to handle data transmission. Account for 9 bits per byte
     * because of the ACKs necessary after each byte.
     */
    long ticks = (HZ/10) + (HZ * 9 * bus->byte_cnt_end / bus->freq);
    timeout_register(&bus->tmo, i2c_x1000_bus_timeout, ticks, (intptr_t)bus);
}

void i2c_init(void)
{
    /* Initialize core */
    __i2c_async_init();

    /* Initialize our bus data structures */
    for(int i = 0; i < 3; ++i) {
        i2c_x1000_busses[i].chn = i;
        i2c_x1000_busses[i].freq = 0;
        i2c_x1000_busses[i].resetting = 0;
    }
}

/* Stuff only required during initialization is below, basically the same as
 * the old driver except for how the IRQs are initially set up. */

static void i2c_x1000_gate(int chn, int gate)
{
    switch(chn) {
    case 0: jz_writef(CPM_CLKGR, I2C0(gate)); break;
    case 1: jz_writef(CPM_CLKGR, I2C1(gate)); break;
    case 2: jz_writef(CPM_CLKGR, I2C2(gate)); break;
    default: break;
    }
}

static void i2c_x1000_enable(int chn)
{
    /* Enable controller */
    jz_writef(I2C_ENABLE(chn), ACTIVE(1));
    while(jz_readf(I2C_ENBST(chn), ACTIVE) == 0);

    /* Set up interrutpts */
    jz_overwritef(I2C_INTMSK(chn), RXFL(0), TXEMP(0),
                  TXABT(1), TXOF(1), RXOF(1), RXUF(1));
    system_enable_irq(IRQ_I2C(chn));
}

static void i2c_x1000_disable(int chn)
{
    /* Disable interrupts */
    system_disable_irq(IRQ_I2C(chn));
    REG_I2C_INTMSK(chn) = 0;

    /* Disable controller */
    jz_writef(I2C_ENABLE(chn), ACTIVE(0));
    while(jz_readf(I2C_ENBST(chn), ACTIVE));
}

void i2c_x1000_set_freq(int chn, int freq)
{
    /* Store frequency */
    i2c_x1000_busses[chn].freq = freq;

    /* Disable the channel if previously active */
    i2c_x1000_gate(chn, 0);
    if(jz_readf(I2C_ENBST(chn), ACTIVE) == 1)
        i2c_x1000_disable(chn);

    /* Request to shut down the channel */
    if(freq == 0) {
        i2c_x1000_gate(chn, 1);
        return;
    }

    /* Calculate timing parameters */
    unsigned pclk = clk_get(X1000_CLK_PCLK);
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

    /* Enable the controller */
    i2c_x1000_enable(chn);
}
