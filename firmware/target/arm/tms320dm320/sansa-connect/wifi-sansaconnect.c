/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2021 by Tomasz Moń
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

#include "kernel.h"
#include "system.h"
#include "spi.h"
#include "avr-sansaconnect.h"
#include "libertas/if_spi_drv.h"

#define IO_SERIAL0_XMIT         (0x100)

void libertas_spi_init(void)
{
    IO_GIO_DIR0 &= ~((1 << 4) /* CS */ | (1 << 3) /* reset */);
    libertas_spi_reset(1);
    libertas_spi_cs(1);

    /* Enable the clock */
    bitset16(&IO_CLK_MOD2, CLK_MOD2_SIF0);

    /* Disable transmitter */
    IO_SERIAL0_TX_ENABLE = 0x0001;

    /* SELSDEN = 0, SLVEN = 0, SIOCLR = 0, SCLKM = 1, MSB = 1, MSSEL = 0,
     * RATE = 1 (Bit rate = ARM clock / 2*(RATE + 1)))
     * Boosted 148.5 MHz / 4 = 37.125 MHz
     * Default 74.25 MHz / 4 = 18.5625 MHz
     */
    IO_SERIAL0_MODE = 0x0601;

    /* Disable the clock */
    bitclr16(&IO_CLK_MOD2, CLK_MOD2_SIF0);

    /* Make sure the SPI clock is not inverted */
    bitclr16(&IO_CLK_INV, CLK_INV_SIF0);
}

void libertas_spi_reset(int high)
{
    if (high)
    {
        IO_GIO_BITSET0 = (1 << 3);
    }
    else
    {
        IO_GIO_BITCLR0 = (1 << 3);
    }
}

void libertas_spi_pd(int high)
{
    avr_hid_wifi_pd(high);
}

void libertas_spi_cs(int high)
{
    if (high)
    {
        IO_GIO_BITSET0 = (1 << 4);
    }
    else
    {
        IO_GIO_BITCLR0 = (1 << 4);
    }
}

void libertas_spi_tx(const uint8_t *buf, int len)
{
    /* Enable the clock */
    bitset16(&IO_CLK_MOD2, CLK_MOD2_SIF0);
    IO_SERIAL0_TX_ENABLE = 0x0001;

    while (len > 0)
    {
        IO_SERIAL0_TX_DATA = *(buf + 1);
        while (IO_SERIAL0_RX_DATA & IO_SERIAL0_XMIT) {};
        IO_SERIAL0_TX_DATA = *buf;
        while (IO_SERIAL0_RX_DATA & IO_SERIAL0_XMIT) {};

        buf += 2;
        len -= 2;
    }

    IO_SERIAL0_TX_ENABLE = 0x0000;

    /* Disable the clock */
    bitclr16(&IO_CLK_MOD2, CLK_MOD2_SIF0);
}

void libertas_spi_rx(uint8_t *buf, int len)
{
    /* Enable the clock */
    bitset16(&IO_CLK_MOD2, CLK_MOD2_SIF0);
    IO_SERIAL0_TX_ENABLE = 0x0001;

    while (len > 0)
    {
        uint16_t data;
        IO_SERIAL0_TX_DATA = 0;
        while ((data = IO_SERIAL0_RX_DATA) & IO_SERIAL0_XMIT) {};
        *(buf + 1) = data & 0xFF;
        IO_SERIAL0_TX_DATA = 0;
        while ((data = IO_SERIAL0_RX_DATA) & IO_SERIAL0_XMIT) {};
        *buf = data & 0xFF;

        buf += 2;
        len -= 2;
    }

    IO_SERIAL0_TX_ENABLE = 0x0000;

    /* Disable the clock */
    bitclr16(&IO_CLK_MOD2, CLK_MOD2_SIF0);
}
