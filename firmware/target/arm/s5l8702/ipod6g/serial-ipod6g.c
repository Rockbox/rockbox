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
#include "cpu.h"
#include "system.h"
#include "serial.h"

#include "s5l8702.h"
#include "uc870x.h"

/* Define LOGF_ENABLE to enable logf output in this file */
#define LOGF_ENABLE
#include "logf.h"


/* shall include serial HW configuracion for specific target */
#define IPOD6G_UART_CLK_HZ      12000000  /* external OSC0 ??? */

/* This values below are valid with a UCLK of 12MHz */
#define BRDATA_9600         (77)                    /* 9615   */
#define BRDATA_19200        (38)                    /* 19231  */
#define BRDATA_28800        (25)                    /* 28846  */
#define BRDATA_38400        (19 | (0xc330c << 8))   /* 38305  */
#define BRDATA_57600        (12)                    /* 57692  */
#define BRDATA_115200       (6 | (0xffffff << 8))   /* 114286 */


extern const struct uartc s5l8702_uartc;
#ifdef IPOD_ACCESSORY_PROTOCOL
static void iap_rx_isr(int, char*, char*, uint32_t);
#endif

struct uartc_port ser_port IDATA_ATTR =
{
    /* location */
    .uartc = &s5l8702_uartc,
    .id = 0,

    /* configuration */
    .rx_trg = UFCON_RX_FIFO_TRG_4,
    .tx_trg = UFCON_TX_FIFO_TRG_EMPTY,
    .clksel = UCON_CLKSEL_ECLK,
    .clkhz = IPOD6G_UART_CLK_HZ,

    /* interrupt callbacks */
#ifdef IPOD_ACCESSORY_PROTOCOL
    .rx_cb = iap_rx_isr,
#else
    .rx_cb = NULL,
#endif
    .tx_cb = NULL,  /* polling */
};

/*
 * serial driver API
 */
int tx_rdy(void)
{
    return uartc_port_tx_ready(&ser_port) ? 1 : 0;
}

void tx_writec(unsigned char c)
{
    uartc_port_tx_byte(&ser_port, c);
}

#ifndef IPOD_ACCESSORY_PROTOCOL
void serial_setup(void)
{
    uartc_port_open(&ser_port);

    /* set a default configuration, Tx and Rx modes are
       disabled when the port is initialized */
    uartc_port_config(&ser_port, ULCON_DATA_BITS_8,
                        ULCON_PARITY_NONE, ULCON_STOP_BITS_1);
    uartc_port_set_bitrate_raw(&ser_port, BRDATA_115200);

    /* enable Tx interrupt request or POLLING mode */
    uartc_port_set_tx_mode(&ser_port, UCON_MODE_INTREQ);

    logf("[%lu] "MODEL_NAME" port %d ready!", USEC_TIMER, ser_port.id);
}


#else /* IPOD_ACCESSORY_PROTOCOL */
#include "kernel.h"
#include "pmu-target.h"
#include "iap.h"

static enum {
    ABR_STATUS_LAUNCHED,    /* ST_SYNC */
    ABR_STATUS_SYNCING,     /* ST_SOF */
    ABR_STATUS_DONE
} abr_status;

static int bitrate = 0;
static bool acc_plugged = false;

static void serial_acc_tick(void)
{
    bool plugged = pmu_accessory_present();
    if (acc_plugged != plugged)
    {
        acc_plugged = plugged;
        if (acc_plugged)
        {
            uartc_open(ser_port.uartc);
            uartc_port_open(&ser_port);
            /* set a default configuration, Tx and Rx modes are
               disabled when the port is initialized */
            uartc_port_config(&ser_port, ULCON_DATA_BITS_8,
                                ULCON_PARITY_NONE, ULCON_STOP_BITS_1);
            uartc_port_set_tx_mode(&ser_port, UCON_MODE_INTREQ);
            serial_bitrate(bitrate);
        }
        else
        {
            uartc_port_close(&ser_port);
            uartc_close(ser_port.uartc);
        }
    }
}

void serial_setup(void)
{
    uartc_close(ser_port.uartc);
    tick_add_task(serial_acc_tick);
}

void serial_bitrate(int rate)
{
    bitrate = rate;
    if (!acc_plugged)
        return;

    logf("[%lu] serial_bitrate(%d)", USEC_TIMER, rate);

    if (rate == 0) {
        /* Using auto-bitrate (ABR) to detect accessory Tx speed:
         *
         * + Here:
         *   - Disable Rx logic to clean the FIFO and the shift
         *     register, thus no Rx data interrupts are generated.
         *   - Launch ABR and wait for a low pulse in Rx line.
         *
         * + In ISR, when a low pulse is detected (ideally it is the
         *   start bit of 0xff):
         *   - Calculate and configure detected speed.
         *   - Enable Rx to verify that the next received data frame
         *     is 0x55 or 0xff:
         *     - If so, it's assumed bit rate is correctly detected,
         *       it will not be modified until speed is changed using
         *       RB options menu.
         *     - If not, reset iAP state machine and launch a new ABR.
         */
        uartc_port_set_rx_mode(&ser_port, UCON_MODE_DISABLED);
        uartc_port_abr_start(&ser_port);
        abr_status = ABR_STATUS_LAUNCHED;
    }
    else {
        uint32_t brdata;
        if      (rate == 57600) brdata = BRDATA_57600;
        else if (rate == 38400) brdata = BRDATA_38400;
        else if (rate == 19200) brdata = BRDATA_19200;
        else brdata = BRDATA_9600;
        uartc_port_abr_stop(&ser_port); /* abort ABR if already launched */
        uartc_port_set_bitrate_raw(&ser_port, brdata);
        uartc_port_set_rx_mode(&ser_port, UCON_MODE_INTREQ);
        abr_status = ABR_STATUS_DONE;
    }
}

static void iap_rx_isr(int len, char *data, char *err, uint32_t abr_cnt)
{
    /* ignore Rx errors, upper layer will discard bad packets */
    (void) err;

    static int sync_retry;

    if (abr_status == ABR_STATUS_LAUNCHED) {
        /* autobauding */
        if (abr_cnt) {
            #define BR2CNT(s) (IPOD6G_UART_CLK_HZ / (unsigned)(s))
            if (abr_cnt < BR2CNT(57600*1.1) || abr_cnt > BR2CNT(9600*0.9)) {
                /* detected speed out of range, relaunch ABR */
                uartc_port_abr_start(&ser_port);
                return;
            }
            /* valid speed detected, select it */
            uint32_t brdata;
            if      (abr_cnt < BR2CNT(48000)) brdata = BRDATA_57600;
            else if (abr_cnt < BR2CNT(33600)) brdata = BRDATA_38400;
            else if (abr_cnt < BR2CNT(24000)) brdata = BRDATA_28800;
            else if (abr_cnt < BR2CNT(14400)) brdata = BRDATA_19200;
            else brdata = BRDATA_9600;

            /* set detected speed */
            uartc_port_set_bitrate_raw(&ser_port, brdata);
            uartc_port_set_rx_mode(&ser_port, UCON_MODE_INTREQ);

            /* enter SOF state */
            iap_getc(0xff);

            abr_status = ABR_STATUS_SYNCING;
            sync_retry = 2; /* we are expecting [0xff] 0x55 */
        }
    }

    /* process received data */
    while (len--)
    {
        bool sync_done = !iap_getc(*data++);

        if (abr_status == ABR_STATUS_SYNCING)
        {
            if (sync_done) {
                abr_status = ABR_STATUS_DONE;
            }
            else if (--sync_retry == 0) {
                /* invalid speed detected, relaunch ABR
                   discarding remaining data (if any) */
                serial_bitrate(0);
                break;
            }
        }
    }
}
#endif /* IPOD_ACCESSORY_PROTOCOL */
