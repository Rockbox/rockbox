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

#include "s5l8700.h"
#include "uc870x.h"

/* Define LOGF_ENABLE to enable logf output in this file */
#define LOGF_ENABLE
#include "logf.h"


/* shall include serial HW configuracion for specific target */
#define NANO2G_UART_CLK_HZ      24000000  /* external OSC0 ??? */

/* This values below are valid with a UCLK of 24MHz */
#define BRDATA_9600         (155)  /* 9615   */
#define BRDATA_19200        (77)   /* 19231  */
#define BRDATA_28800        (51)   /* 28846  */
#define BRDATA_38400        (38)   /* 38462  */
#define BRDATA_57600        (25)   /* 57692  */
#define BRDATA_115200       (12)   /* 115385 */


extern const struct uartc s5l8701_uartc0;

struct uartc_port ser_port IDATA_ATTR =
{
    /* location */
    .uartc = &s5l8701_uartc0,
    .id = 0,

    /* configuration */
    .rx_trg = UFCON_RX_FIFO_TRG_4,
    .tx_trg = UFCON_TX_FIFO_TRG_EMPTY,
    .clksel = UCON_CLKSEL_ECLK,
    .clkhz = NANO2G_UART_CLK_HZ,

    /* interrupt callbacks */
    .rx_cb = NULL,
    .tx_cb = NULL,  /* polling */
};

/*
 * serial driver API
 */
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

    logf("[%lu] "MODEL_NAME" port %d ready!",
                (uint32_t)USEC_TIMER, ser_port.id);
}

int tx_rdy(void)
{
    return uartc_port_tx_ready(&ser_port) ? 1 : 0;
}

void tx_writec(unsigned char c)
{
    uartc_port_tx_byte(&ser_port, c);
}
