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
#ifndef __UC870X_H__
#define __UC870X_H__

#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "system.h"
#include "uart-target.h"


/*
 * UC870x: UART controller for s5l870x
 *
 * This UART is similar to the UART described in s5l8700 datasheet,
 * (see also s3c2416 and s3c6400 datasheets). On s5l8701/2 the UC870x
 * includes autobauding, and fine tunning for Tx/Rx on s5l8702.
 */

/*
 * Controller registers
 */
#define REG32_PTR_T volatile uint32_t *

#define ULCON(ba)       (*((REG32_PTR_T)((ba) + 0x00))) /* line control */
#define UCON(ba)        (*((REG32_PTR_T)((ba) + 0x04))) /* control */
#define UFCON(ba)       (*((REG32_PTR_T)((ba) + 0x08))) /* FIFO control */
#define UMCON(ba)       (*((REG32_PTR_T)((ba) + 0x0C))) /* modem control */
#define UTRSTAT(ba)     (*((REG32_PTR_T)((ba) + 0x10))) /* Tx/Rx status */
#define UERSTAT(ba)     (*((REG32_PTR_T)((ba) + 0x14))) /* Rx error status */
#define UFSTAT(ba)      (*((REG32_PTR_T)((ba) + 0x18))) /* FIFO status */
#define UMSTAT(ba)      (*((REG32_PTR_T)((ba) + 0x1C))) /* modem status */
#define UTXH(ba)        (*((REG32_PTR_T)((ba) + 0x20))) /* transmission hold */
#define URXH(ba)        (*((REG32_PTR_T)((ba) + 0x24))) /* receive buffer */
#define UBRDIV(ba)      (*((REG32_PTR_T)((ba) + 0x28))) /* baud rate divisor */
#if CONFIG_CPU != S5L8700
#define UABRCNT(ba)     (*((REG32_PTR_T)((ba) + 0x2c))) /* autobaud counter */
#define UABRSTAT(ba)    (*((REG32_PTR_T)((ba) + 0x30))) /* autobaud status */
#endif
#if CONFIG_CPU == S5L8702
#define UBRCONTX(ba)    (*((REG32_PTR_T)((ba) + 0x34))) /* Tx frame config */
#define UBRCONRX(ba)    (*((REG32_PTR_T)((ba) + 0x38))) /* Rx frame config */
#endif

/* ULCON register */
#define ULCON_DATA_BITS_MASK        0x3
#define ULCON_DATA_BITS_POS         0
#define ULCON_DATA_BITS_5           0
#define ULCON_DATA_BITS_6           1
#define ULCON_DATA_BITS_7           2
#define ULCON_DATA_BITS_8           3

#define ULCON_STOP_BITS_MASK        0x1
#define ULCON_STOP_BITS_POS         2
#define ULCON_STOP_BITS_1           0
#define ULCON_STOP_BITS_2           1

#define ULCON_PARITY_MASK           0x7
#define ULCON_PARITY_POS            3
#define ULCON_PARITY_NONE           0
#define ULCON_PARITY_ODD            4
#define ULCON_PARITY_EVEN           5
#define ULCON_PARITY_FORCE_1        6
#define ULCON_PARITY_FORCE_0        7

#define ULCON_INFRARED_EN_BIT       (1 << 6)

/* UCON register */
#define UCON_RX_MODE_MASK           0x3
#define UCON_RX_MODE_POS            0

#define UCON_TX_MODE_MASK           0x3
#define UCON_TX_MODE_POS            2

#define UCON_MODE_DISABLED          0
#define UCON_MODE_INTREQ            1   /* INT request or polling mode */
#define UCON_MODE_UNDEFINED         2   /* Not defined, DMAREQ signal 1 ??? */
#define UCON_MODE_DMAREQ            3   /* DMA request (signal 0) */

#define UCON_SEND_BREAK_BIT         (1 << 4)
#define UCON_LOOPBACK_BIT           (1 << 5)
#define UCON_RX_TOUT_EN_BIT         (1 << 7)    /* Rx timeout enable */

#define UCON_CLKSEL_MASK            0x1
#define UCON_CLKSEL_POS             10
#define UCON_CLKSEL_PCLK            0   /* internal */
#define UCON_CLKSEL_ECLK            1   /* external */

#if CONFIG_CPU == S5L8702
#define UCON_RX_TOUT_INT_BIT        (1 << 11)   /* Rx timeout INT enable */
#endif
#define UCON_RX_INT_BIT             (1 << 12)   /* Rx INT enable */
#define UCON_TX_INT_BIT             (1 << 13)   /* Tx INT enable */
#define UCON_ERR_INT_BIT            (1 << 14)   /* Rx error INT enable */
#define UCON_MODEM_INT_BIT          (1 << 15)   /* modem INT enable (TBC) */
#if CONFIG_CPU != S5L8700
#define UCON_AUTOBR_INT_BIT         (1 << 16)   /* autobauding INT enable */
#define UCON_AUTOBR_START_BIT       (1 << 17)   /* autobauding start/stop */
#endif

#if CONFIG_CPU == S5L8701
/* WTF! ABR bits are swapped on reads, so don't forget to
   always use this workaround to read the UCON register. */
static inline uint32_t _UCON_RD(uint32_t ba)
{
    uint32_t ucon = UCON(ba);
    return ((ucon & 0xffff) |
           ((ucon & UCON_AUTOBR_INT_BIT) << 1) |
           ((ucon & UCON_AUTOBR_START_BIT) >> 1));
}
#else
#define _UCON_RD(ba) UCON(ba)
#endif

/* UFCON register */
#define UFCON_FIFO_ENABLE_BIT       (1 << 0)
#define UFCON_RX_FIFO_RST_BIT       (1 << 1)
#define UFCON_TX_FIFO_RST_BIT       (1 << 2)

#define UFCON_RX_FIFO_TRG_MASK      0x3
#define UFCON_RX_FIFO_TRG_POS       4
#define UFCON_RX_FIFO_TRG_4         0
#define UFCON_RX_FIFO_TRG_8         1
#define UFCON_RX_FIFO_TRG_12        2
#define UFCON_RX_FIFO_TRG_16        3

#define UFCON_TX_FIFO_TRG_MASK      0x3
#define UFCON_TX_FIFO_TRG_POS       6
#define UFCON_TX_FIFO_TRG_EMPTY     0
#define UFCON_TX_FIFO_TRG_4         1
#define UFCON_TX_FIFO_TRG_8         2
#define UFCON_TX_FIFO_TRG_12        3

/* UMCON register */
#define UMCON_RTS_BIT               (1 << 0)
#define UMCON_AUTO_FLOW_CTRL_BIT    (1 << 4)

/* UTRSTAT register */
#define UTRSTAT_RXBUF_RDY_BIT       (1 << 0)
#define UTRSTAT_TXBUF_EMPTY_BIT     (1 << 1)
#define UTRSTAT_TX_EMPTY_BIT        (1 << 2)
#if CONFIG_CPU == S5L8702
#define UTRSTAT_RX_TOUT_INT_BIT     (1 << 3)    /* Rx timeout INT status */
#endif
#define UTRSTAT_RX_INT_BIT          (1 << 4)
#define UTRSTAT_TX_INT_BIT          (1 << 5)
#define UTRSTAT_ERR_INT_BIT         (1 << 6)
#define UTRSTAT_MODEM_INT_BIT       (1 << 7)    /* modem INT status */
#if CONFIG_CPU != S5L8700
#define UTRSTAT_AUTOBR_INT_BIT      (1 << 8)    /* autobauding INT status */
#endif

/* UERSTAT register */
#define UERSTAT_OVERRUN_BIT         (1 << 0)
#define UERSTAT_PARITY_ERR_BIT      (1 << 1)
#define UERSTAT_FRAME_ERR_BIT       (1 << 2)
#define UERSTAT_BREAK_DETECT_BIT    (1 << 3)

/* UFSTAT register */
#define UFSTAT_RX_FIFO_CNT_MASK     0xf
#define UFSTAT_RX_FIFO_CNT_POS      0

#define UFSTAT_TX_FIFO_CNT_MASK     0xf
#define UFSTAT_TX_FIFO_CNT_POS      4

#define UFSTAT_RX_FIFO_FULL_BIT     (1 << 8)
#define UFSTAT_TX_FIFO_FULL_BIT     (1 << 9)
#define UFSTAT_RX_FIFO_ERR_BIT      (1 << 10)   /* clears when reading UERSTAT
                                                   for the last pending error */
/* UMSTAT register */
#define UMSTAT_CTS_ACTIVE_BIT       (1 << 0)
#define UMSTAT_CTS_DELTA_BIT        (1 << 4)


#if CONFIG_CPU == S5L8702
/* Bitrate:
 *
 * Master UCLK clock is divided by 16 to serialize data, UBRDIV is
 * used to configure nominal bit width, NBW = (UBRDIV+1)*16 in UCLK
 * clock ticks.
 *
 * Fine tuning works shrining/expanding each individual bit of each
 * frame. Each bit width can be incremented/decremented by 1/16 of
 * nominal bit width, it seems UCLK is divided by 17 for expanded
 * bits and divided by 15 for compressed bits. A whole frame of N
 * bits can be shrined or expanded up to (NBW * N / 16) UCLK clock
 * ticks (in 1/16 steps).
 */
/* UBRCONx register */
#define UC_FRAME_MAX_LEN            12  /* 1 start + 8 data + 1 par + 2 stop */
#define UBRCON_JITTER_MASK          0x3
#define UBRCON_JITTER_POS(bit)      ((bit) << 1)  /* 0..UC_FRAME_MAX_LEN-1 */

#define UBRCON_JITTER_NONE          0   /* no jitter for this bit */
#define UBRCON_JITTER_INC           1   /* increment 1/16 bit width */
#define UBRCON_JITTER_UNUSED        2   /* does nothing */
#define UBRCON_JITTER_DEC           3   /* decremet 1/16 bit width */
#endif /* CONFIG_CPU == S5L8702 */


#if CONFIG_CPU != S5L8700
/* Autobauding:
 *
 * Initial UABRSTAT is NOT_INIT, it goes to READY when either of
 * UCON_AUTOBR bits are enabled for the first time.
 *
 * Interrupts are enabled/disabled using UCON_AUTOBR_INT_BIT and
 * checked using UTRSTAT_AUTOBR_INT_BIT, writing this bit cleans the
 * interrupt.
 *
 * When UCON_AUTOBR_START_BIT is enabled, autobauding starts and the
 * hardware waits for a low pulse on RX line.
 *
 * Once autobauding is started, when a falling edge is detected on
 * the RX line, UABRSTAT changes to COUNTING status, an internal
 * counter starts incrementing at UCLK clock frequency. During
 * COUNTING state, UABRCNT reads as the value of the previous ABR
 * count, not the value of the current internal count.
 *
 * Count finish when a rising edge is detected on the line, at this
 * moment internal counter stops and it can be read using UABRCNT
 * register, UABRSTAT goes to READY, AUTOBR_START_BIT is disabled,
 * and an interrupt is raised if UCON_AUTOBR_INT_BIT is enabled.
 */
/* UABRSTAT register */
#define UABRSTAT_STATUS_MASK        0x3
#define UABRSTAT_STATUS_POS         0

#define UABRSTAT_STATUS_NOT_INIT    0   /* initial status */
#define UABRSTAT_STATUS_READY       1   /* machine is ready */
#define UABRSTAT_STATUS_COUNTING    2   /* count in progress */
#endif /* CONFIG_CPU != S5L8700 */


/*
 * other HW definitions
 */
#define UART_FIFO_SIZE      16


/*
 * structs
 */
struct uartc
{
    /* static configuration */
    uint8_t id;
    uint8_t n_ports;
    uint16_t port_off;
    uint32_t baddr;
    struct uartc_port **port_l;
};

struct uartc_port
{
    /* static configuration */
    const struct uartc * const uartc;
    const uint8_t id;                   /* port number */
    const uint8_t rx_trg;               /* UFCON_RX_FIFO_TRG_xxx */
    const uint8_t tx_trg;               /* UFCON_TX_FIFO_TRG_xxx */
    const uint8_t clksel;               /* UFCON_CLKSEL_xxx */
    const uint32_t clkhz;               /* UCLK (PCLK or ECLK) frequency */
    void (* const tx_cb) (int len);     /* ISRs */
#if CONFIG_CPU != S5L8700
    void (* const rx_cb) (int len, char *data, char *err, uint32_t abr_cnt);
#else
    void (* const rx_cb) (int len, char *data, char *err);
#endif

    /* private */
    uint32_t baddr;
    uint32_t utrstat_int_mask;
    uint8_t rx_data[UART_FIFO_SIZE];    /* data buffer for rx_cb */
    uint8_t rx_err[UART_FIFO_SIZE];     /* error buffer for rx_cb */
#if CONFIG_CPU != S5L8700
    bool abr_aborted;
#endif

#ifdef UC870X_DEBUG
    uint32_t n_tx_bytes;
    uint32_t n_rx_bytes;
    uint32_t n_ovr_err;
    uint32_t n_parity_err;
    uint32_t n_frame_err;
    uint32_t n_break_detect;
#if CONFIG_CPU != S5L8700
    /* autobauding */
    uint32_t n_abnormal0;
    uint32_t n_abnormal1;
#endif
#endif
};


/*
 * uc870x low level API
 */

/* Initialization */
void uartc_open(const struct uartc* uartc);
void uartc_close(const struct uartc* uartc);
void uartc_port_open(struct uartc_port *port);
void uartc_port_close(struct uartc_port *port);
void uartc_port_rx_onoff(struct uartc_port *port, bool onoff);
void uartc_port_tx_onoff(struct uartc_port *port, bool onoff);

/* Configuration */
void uartc_port_config(struct uartc_port *port,
                uint8_t data_bits, uint8_t parity, uint8_t stop_bits);
void uartc_port_set_bitrate_raw(struct uartc_port *port, uint32_t brdata);
void uartc_port_set_bitrate(struct uartc_port *port, unsigned int speed);
void uartc_port_set_rx_mode(struct uartc_port *port, uint32_t mode);
void uartc_port_set_tx_mode(struct uartc_port *port, uint32_t mode);

/* Transmit */
bool uartc_port_tx_ready(struct uartc_port *port);
void uartc_port_tx_byte(struct uartc_port *port, uint8_t ch);
void uartc_port_send_byte(struct uartc_port *port, uint8_t ch);

/* Receive */
bool uartc_port_rx_ready(struct uartc_port *port);
uint8_t uartc_port_rx_byte(struct uartc_port *port);
uint8_t uartc_port_read_byte(struct uartc_port *port);

#if CONFIG_CPU != S5L8700
/* Autobauding */
void uartc_port_abr_start(struct uartc_port *port);
void uartc_port_abr_stop(struct uartc_port *port);
#endif

/* ISR */
void uartc_callback(const struct uartc *uartc, int port_id);

/* Debug */
#ifdef UC870X_DEBUG
void uartc_port_get_line_info(struct uartc_port *port,
                int *tx_status, int *rx_status,
                int *tx_speed, int *rx_speed, char *line_cfg);

#if CONFIG_CPU != S5L8700
enum {
    ABR_INFO_ST_IDLE,
    ABR_INFO_ST_LAUNCHED,
    ABR_INFO_ST_COUNTING,
    ABR_INFO_ST_ABNORMAL
};

int uartc_port_get_abr_info(struct uartc_port *port, uint32_t *abr_cnt);
#endif
#endif /* UC870X_DEBUG */

#endif /* __UC870X_H__ */
