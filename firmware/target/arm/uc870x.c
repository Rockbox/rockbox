/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Cástor Muñoz
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
#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "system.h"

#include "uart-target.h"
#include "uc870x.h"


/*
 * UC870x: UART controller for s5l870x
 */

/* Rx related masks */
#if CONFIG_CPU == S5L8700
#define UTRSTAT_RX_RELATED_INTS (UTRSTAT_RX_INT_BIT | UTRSTAT_ERR_INT_BIT)
#define UCON_RX_RELATED_INTS    (UCON_RX_INT_BIT | UCON_ERR_INT_BIT)

#elif CONFIG_CPU == S5L8701
#define UTRSTAT_RX_RELATED_INTS \
        (UTRSTAT_RX_INT_BIT | UTRSTAT_ERR_INT_BIT | UTRSTAT_AUTOBR_INT_BIT)
#define UCON_RX_RELATED_INTS \
        (UCON_RX_INT_BIT | UCON_ERR_INT_BIT | UCON_AUTOBR_INT_BIT)

#else /* CONFIG_CPU == S5L8702 */
#define UTRSTAT_RX_RELATED_INTS \
        (UTRSTAT_RX_INT_BIT | UTRSTAT_ERR_INT_BIT | \
        UTRSTAT_AUTOBR_INT_BIT | UTRSTAT_RX_TOUT_INT_BIT)
#define UCON_RX_RELATED_INTS \
        (UCON_RX_INT_BIT | UCON_ERR_INT_BIT | \
        UCON_AUTOBR_INT_BIT | UCON_RX_TOUT_INT_BIT)
#endif

#define UART_PORT_BASE(u,i)     (((u)->baddr) + (u)->port_off * (i))

/* Initialization */
static void uartc_reset_port_id(const struct uartc* uartc, int port_id)
{
    uart_target_disable_irq(uartc->id, port_id);
    uart_target_disable_gpio(uartc->id, port_id);

    /* set port registers to default reset values */
    uint32_t baddr = UART_PORT_BASE(uartc, port_id);
    UCON(baddr) = 0;
    ULCON(baddr) = 0;
    UMCON(baddr) = 0;
    UFCON(baddr) = UFCON_RX_FIFO_RST_BIT | UFCON_TX_FIFO_RST_BIT;
    UTRSTAT(baddr) = ~0; /* clear all interrupts */
    UBRDIV(baddr) = 0;
#if CONFIG_CPU == S5L8702
    UBRCONTX(baddr) = 0;
    UBRCONRX(baddr) = 0;
#endif

    uartc->port_l[port_id] = (void*)0;
}

static void uartc_reset(const struct uartc* uartc)
{
    for (int port_id = 0; port_id < uartc->n_ports; port_id++)
        uartc_reset_port_id(uartc, port_id);
}

void uartc_open(const struct uartc *uartc)
{
    uart_target_enable_clocks(uartc->id);
    uartc_reset(uartc);
}

void uartc_close(const struct uartc *uartc)
{
    uartc_reset(uartc);
    uart_target_disable_clocks(uartc->id);
}

void uartc_port_open(struct uartc_port *port)
{
    const struct uartc *uartc = port->uartc;
    uint32_t baddr = UART_PORT_BASE(uartc, port->id);

    uart_target_enable_gpio(uartc->id, port->id);

    /* disable Tx/Rx and mask all interrupts */
    UCON(baddr) = 0;

    /* clear all interrupts */
    UTRSTAT(baddr) = ~0;

    /* configure registers */
    UFCON(baddr) = UFCON_FIFO_ENABLE_BIT
         | UFCON_RX_FIFO_RST_BIT
         | UFCON_TX_FIFO_RST_BIT
         | ((port->rx_trg & UFCON_RX_FIFO_TRG_MASK) << UFCON_RX_FIFO_TRG_POS)
         | ((port->tx_trg & UFCON_TX_FIFO_TRG_MASK) << UFCON_TX_FIFO_TRG_POS);

    UMCON(baddr) = UMCON_RTS_BIT; /* activate nRTS (low level) */

    UCON(baddr) = (UCON_MODE_DISABLED << UCON_RX_MODE_POS)
                | (UCON_MODE_DISABLED << UCON_TX_MODE_POS)
                | ((port->clksel & UCON_CLKSEL_MASK) << UCON_CLKSEL_POS)
                | (port->rx_cb ? UCON_RX_RELATED_INTS|UCON_RX_TOUT_EN_BIT : 0)
                | (port->tx_cb ? UCON_TX_INT_BIT : 0);

    /* init and register port struct */
    port->baddr = baddr;
    port->utrstat_int_mask = (port->rx_cb ? UTRSTAT_RX_RELATED_INTS : 0)
                           | (port->tx_cb ? UTRSTAT_TX_INT_BIT : 0);
#if CONFIG_CPU != S5L8700
    port->abr_aborted = 0;
#endif
    uartc->port_l[port->id] = port;

    /* enable interrupts */
    uart_target_clear_irq(uartc->id, port->id);
    /*if (port->utrstat_int_mask)*/
    uart_target_enable_irq(uartc->id, port->id);
}

void uartc_port_close(struct uartc_port *port)
{
    uartc_reset_port_id(port->uartc, port->id);
}

/* Configuration */
void uartc_port_config(struct uartc_port *port,
                uint8_t data_bits, uint8_t parity, uint8_t stop_bits)
{
    ULCON(port->baddr) = ((parity & ULCON_PARITY_MASK) << ULCON_PARITY_POS)
                | ((stop_bits & ULCON_STOP_BITS_MASK) << ULCON_STOP_BITS_POS)
                | ((data_bits & ULCON_DATA_BITS_MASK) << ULCON_DATA_BITS_POS);
}

/* set bitrate using precalculated values */
void uartc_port_set_bitrate_raw(struct uartc_port *port, uint32_t brdata)
{
    uint32_t baddr = port->baddr;
    UBRDIV(baddr) = brdata & 0xff;
#if CONFIG_CPU == S5L8702
    UBRCONRX(baddr) = brdata >> 8;
    UBRCONTX(baddr) = brdata >> 8;
#endif
}

#if 0
/* calculate values to set real bitrate as close as possible to the
   requested speed */
void uartc_port_set_bitrate(struct uartc_port *port, unsigned int speed)
{
    int uclk = port->clkhz;

    /* Real baud width in UCLK/16 ticks: trunc(UCLK/(16*speed) + 0.5) */
    int brdiv = (uclk + (speed << 3)) / (speed << 4);

    uint32_t brdata = brdiv - 1;

#if CONFIG_CPU == S5L8702
    /* Fine adjust:
     *
     * Along the whole frame, insert/remove "jittered" bauds when needed
     * to minimize frame lenght accumulated error.
     *
     * jitter_width: "jittered" bauds are 1/16 wider/narrower than normal
     * bauds, so step is 1/16 of real baud width = brdiv (in UCLK ticks)
     *
     * baud_err_width: it is the difference between theoric width and real
     * width = CLK/speed - brdiv*16 (in UCLK ticks)
     *
     * Previous widths are scaled by 'speed' factor to simplify operations
     * and preserve precision using integer operations.
     */
    int jitter_width = brdiv * speed;
    int baud_err_width = uclk - (jitter_width << 4);

    int jitter_incdec = UBRCON_JITTER_INC;
    if (baud_err_width < 0) {
        baud_err_width = -baud_err_width;
        jitter_incdec = UBRCON_JITTER_DEC;
    }

    int err_width = 0;
    uint32_t brcon = 0;
    for (int bit = 0; bit < UC_FRAME_MAX_LEN; bit++) {
        err_width += baud_err_width;
        /* adjust to the nearest width */
        if (jitter_width < (err_width << 1)) {
            brcon |= jitter_incdec << UBRCON_JITTER_POS(bit);
            err_width -= jitter_width;
        }
    }

    brdata |= (brcon << 8);
#endif /* CONFIG_CPU == S5L8702 */

    uartc_port_set_rawbr(port, brdata);
}
#endif

/* Select Tx/Rx modes: disabling Tx/Rx resets HW, including
   FIFOs and shift registers */
void uartc_port_set_rx_mode(struct uartc_port *port, uint32_t mode)
{
    UCON(port->baddr) = (mode << UCON_RX_MODE_POS) |
        (_UCON_RD(port->baddr) & ~(UCON_RX_MODE_MASK << UCON_RX_MODE_POS));
}

void uartc_port_set_tx_mode(struct uartc_port *port, uint32_t mode)
{
    UCON(port->baddr) = (mode << UCON_TX_MODE_POS) |
        (_UCON_RD(port->baddr) & ~(UCON_TX_MODE_MASK << UCON_TX_MODE_POS));
}

/* Transmit */
bool uartc_port_tx_ready(struct uartc_port *port)
{
    return (UTRSTAT(port->baddr) & UTRSTAT_TXBUF_EMPTY_BIT);
}

void uartc_port_tx_byte(struct uartc_port *port, uint8_t ch)
{
    UTXH(port->baddr) = ch;
#ifdef UC870X_DEBUG
    port->n_tx_bytes++;
#endif
}

void uartc_port_send_byte(struct uartc_port *port, uint8_t ch)
{
    /* wait for transmit buffer empty */
    while (!uartc_port_tx_ready(port));
    uartc_port_tx_byte(port, ch);
}

/* Receive */
bool uartc_port_rx_ready(struct uartc_port *port)
{
    /* test receive buffer data ready */
    return (UTRSTAT(port->baddr) & UTRSTAT_RXBUF_RDY_BIT);
}

uint8_t uartc_port_rx_byte(struct uartc_port *port)
{
    return URXH(port->baddr) & 0xff;
}

uint8_t uartc_port_read_byte(struct uartc_port *port)
{
    while (!uartc_port_rx_ready(port));
    return uartc_port_rx_byte(port);
}

#if CONFIG_CPU != S5L8700
/* Autobauding */
static inline int uartc_port_abr_status(struct uartc_port *port)
{
    return UABRSTAT(port->baddr) & UABRSTAT_STATUS_MASK;
}

void uartc_port_abr_start(struct uartc_port *port)
{
    port->abr_aborted = 0;
    UCON(port->baddr) = _UCON_RD(port->baddr) | UCON_AUTOBR_START_BIT;
}

void uartc_port_abr_stop(struct uartc_port *port)
{
    if (uartc_port_abr_status(port) == UABRSTAT_STATUS_COUNTING)
        /* There is no known way to stop the HW once COUNTING is
         * in progress.
         * If we disable AUTOBR_START_BIT now, COUNTING is not
         * aborted, instead the HW will launch interrupts for
         * every new rising edge detected while AUTOBR_START_BIT
         * remains disabled.
         * If AUTOBR_START_BIT is enabled, the HW will stop by
         * itself when a rising edge is detected.
         * So, do not disable AUTOBR_START_BIT and wait for the
         * next rising edge.
         */
        port->abr_aborted = 1;
    else
        UCON(port->baddr) = _UCON_RD(port->baddr) & ~UCON_AUTOBR_START_BIT;
}
#endif /* CONFIG_CPU != S5L8700 */

/* ISR */
void ICODE_ATTR uartc_callback(const struct uartc* uartc, int port_id)
{
    struct uartc_port *port = uartc->port_l[port_id];
    uint32_t baddr = port->baddr;

    /* filter registered interrupts */
    uint32_t ints = UTRSTAT(baddr) & port->utrstat_int_mask;

    /* clear interrupts, events ocurring while processing
       this ISR will be processed in the next call */
    UTRSTAT(baddr) = ints;

    if (ints & UTRSTAT_RX_RELATED_INTS)
    {
        int len = 0;
#if CONFIG_CPU != S5L8700
        uint32_t abr_cnt = 0;

        if (ints & UTRSTAT_AUTOBR_INT_BIT)
        {
            if (uartc_port_abr_status(port) == UABRSTAT_STATUS_COUNTING) {
                #ifdef UC870X_DEBUG
                if (_UCON_RD(baddr) & UCON_AUTOBR_START_BIT) port->n_abnormal0++;
                else port->n_abnormal1++;
                #endif
                /* try to fix abnormal situations */
                UCON(baddr) = _UCON_RD(baddr) | UCON_AUTOBR_START_BIT;
            }
            else if (!port->abr_aborted)
                abr_cnt = UABRCNT(baddr);
        }

        if (ints & (UTRSTAT_RX_RELATED_INTS ^ UTRSTAT_AUTOBR_INT_BIT))
#endif /* CONFIG_CPU != S5L8700 */
        {
            /* get FIFO count */
            uint32_t ufstat = UFSTAT(baddr);
            len = (ufstat & UFSTAT_RX_FIFO_CNT_MASK) |
                        ((ufstat & UFSTAT_RX_FIFO_FULL_BIT) >> (8 - 4));

            for (int i = 0; i < len; i++) {
                /* must read error status first, then data */
                port->rx_err[i] = UERSTAT(baddr);
                port->rx_data[i] = URXH(baddr);
            }
        }

        /* 'len' might be zero due to RX_TOUT interrupts are
         * raised by the hardware even when RX FIFO is empty.
         * When overrun, it is marked on the first error:
         * overrun = len ? (rx_err[0] & UERSTAT_OVERRUN_BIT) : 0
         */
#if CONFIG_CPU == S5L8700
        port->rx_cb(len, port->rx_data, port->rx_err);
#else
        /* 'abr_cnt' is zero when no ABR interrupt exists */
        port->rx_cb(len, port->rx_data, port->rx_err, abr_cnt);
#endif

#ifdef UC870X_DEBUG
        if (len) {
            port->n_rx_bytes += len;
            if (port->rx_err[0] & UERSTAT_OVERRUN_BIT)
                port->n_ovr_err++;
            for (int i = 0; i < len; i++) {
                if (port->rx_err[i] & UERSTAT_PARITY_ERR_BIT)
                    port->n_parity_err++;
                if (port->rx_err[i] & UERSTAT_FRAME_ERR_BIT)
                    port->n_frame_err++;
                if (port->rx_err[i] & UERSTAT_BREAK_DETECT_BIT)
                    port->n_break_detect++;
            }
        }
#endif
    }

#if 0
    /* not used and not tested */
    if (ints & UTRSTAT_TX_INT_BIT)
    {
        port->tx_cb(UART_FIFO_SIZE - ((UFSTAT(baddr) & \
                    UFSTAT_TX_FIFO_CNT_MASK) >> UFSTAT_TX_FIFO_CNT_POS));
    }
#endif
}


#ifdef UC870X_DEBUG
/*#define LOGF_ENABLE*/
#include "logf.h"

#if CONFIG_CPU == S5L8702
static int get_bitrate(int uclk, int brdiv, int brcon, int frame_len)
{
    logf("get_bitrate(%d, %d, 0x%08x, %d)", uclk, brdiv, brcon, frame_len);

    int avg_speed;
    int speed_sum = 0;
    unsigned int frame_width = 0; /* in UCLK clock ticks */

    /* calculate resulting speed for every frame len */
    for (int bit = 0; bit < frame_len; bit++)
    {
        frame_width += brdiv * 16;

        int incdec = ((brcon >> UBRCON_JITTER_POS(bit)) & UBRCON_JITTER_MASK);
        if (incdec == UBRCON_JITTER_INC) frame_width += brdiv;
        else if (incdec == UBRCON_JITTER_DEC) frame_width -= brdiv;

        /* speed = truncate((UCLK / (real_frame_width / NBITS)) + 0.5)
           XXX: overflows for big UCLK */
        int speed = (((uclk*(bit+1))<<1) + frame_width) / (frame_width<<1);
        speed_sum += speed;
        logf("  %d: %c %d", bit, ((incdec == UBRCON_JITTER_INC) ? 'i' :
                        ((incdec == UBRCON_JITTER_DEC) ? 'd' : '.')), speed);
    }

    /* average of the speed for all frame lengths */
    avg_speed = speed_sum / frame_len;
    logf("  avg speed = %d", avg_speed);

    return avg_speed;
}
#endif /* CONFIG_CPU == S5L8702 */

void uartc_port_get_line_info(struct uartc_port *port,
                int *tx_status, int *rx_status,
                int *tx_speed, int *rx_speed, char *line_cfg)
{
    uint32_t baddr = port->baddr;

    uint32_t ucon = _UCON_RD(baddr);
    if (tx_status)
        *tx_status = ((ucon >> UCON_TX_MODE_POS) & UCON_TX_MODE_MASK) ? 1 : 0;
    if (rx_status)
        *rx_status = ((ucon >> UCON_RX_MODE_POS) & UCON_RX_MODE_MASK) ? 1 : 0;

    uint32_t ulcon = ULCON(baddr);
    int n_data = ((ulcon >> ULCON_DATA_BITS_POS) & ULCON_DATA_BITS_MASK) + 5;
    int n_stop = ((ulcon >> ULCON_STOP_BITS_POS) & ULCON_STOP_BITS_MASK) + 1;
    int parity = (ulcon >> ULCON_PARITY_POS) & ULCON_PARITY_MASK;

    uint32_t brdiv = UBRDIV(baddr) + 1;
#if CONFIG_CPU == S5L8702
    int frame_len = 1 + n_data + (parity ? 1 : 0) + n_stop;
    if (tx_speed)
        *tx_speed = get_bitrate(port->clkhz, brdiv, UBRCONTX(baddr), frame_len);
    if (rx_speed)
        *rx_speed = get_bitrate(port->clkhz, brdiv, UBRCONRX(baddr), frame_len);
#else
    /* speed = truncate(UCLK/(16*brdiv) + 0.5) */
    int speed = (port->clkhz + (brdiv << 3)) / (brdiv << 4);
    if (tx_speed) *tx_speed = speed;
    if (rx_speed) *rx_speed = speed;
#endif

    if (line_cfg) {
        line_cfg[0] = '0' + n_data;
        line_cfg[1] = ((parity == ULCON_PARITY_NONE)    ? 'N' :
                      ((parity == ULCON_PARITY_EVEN)    ? 'E' :
                      ((parity == ULCON_PARITY_ODD)     ? 'O' :
                      ((parity == ULCON_PARITY_FORCE_1) ? 'M' :
                      ((parity == ULCON_PARITY_FORCE_0) ? 'S' : '?')))));
        line_cfg[2] = '0' + n_stop;
        line_cfg[3] = '\0';
    }
}

#if CONFIG_CPU != S5L8700
/* Autobauding */
int uartc_port_get_abr_info(struct uartc_port *port, uint32_t *abr_cnt)
{
    int status;
    uint32_t abr_status;

    int flags = disable_irq_save();

    abr_status = uartc_port_abr_status(port);

    if (_UCON_RD(port->baddr) & UCON_AUTOBR_START_BIT) {
        if (abr_status == UABRSTAT_STATUS_COUNTING)
            status = ABR_INFO_ST_COUNTING;  /* waiting for rising edge */
        else
            status = ABR_INFO_ST_LAUNCHED;  /* waiting for falling edge */
    }
    else {
        if (abr_status == UABRSTAT_STATUS_COUNTING)
            status = ABR_INFO_ST_ABNORMAL;
        else
            status = ABR_INFO_ST_IDLE;
    }

    if (abr_cnt)
        *abr_cnt = UABRCNT(port->baddr);

    restore_irq(flags);

    return status;
}
#endif /* CONFIG_CPU != S5L8700 */
#endif /* UC870X_DEBUG */
