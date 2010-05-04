/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2007 Will Robertson
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
#ifndef SPI_IMX31_H
#define SPI_IMX31_H

#define USE_CSPI1_MODULE (1 << 0)
#define USE_CSPI2_MODULE (1 << 1)
#define USE_CSPI3_MODULE (1 << 2)

/* SPI_MODULE_MASK is set in target's config */
enum spi_module_number
{
    __CSPI_NUM_START = -1, /* The first will be 0 */
#if (SPI_MODULE_MASK & USE_CSPI1_MODULE)
    CSPI1_NUM,
#endif
#if (SPI_MODULE_MASK & USE_CSPI2_MODULE)
    CSPI2_NUM,
#endif
#if (SPI_MODULE_MASK & USE_CSPI3_MODULE)
    CSPI3_NUM,
#endif
    SPI_NUM_CSPI,
};

struct cspi_map
{
    volatile uint32_t rxdata;       /* 00h */
    volatile uint32_t txdata;       /* 04h */
    volatile uint32_t conreg;       /* 08h */
    volatile uint32_t intreg;       /* 0Ch */
    volatile uint32_t dmareg;       /* 10h */
    volatile uint32_t statreg;      /* 14h */
    volatile uint32_t periodreg;    /* 18h */
    volatile uint32_t skip1[0x69];  /* 1Ch */
    volatile uint32_t testreg;      /* 1C0h */
};

struct spi_node
{
    enum spi_module_number num; /* Module number (CSPIx_NUM) */
    unsigned long conreg;       /* CSPI conreg setup */
    unsigned long periodreg;    /* CSPI periodreg setup */
};

struct spi_transfer_desc;

typedef void (*spi_transfer_cb_fn_type)(struct spi_transfer_desc *);

struct spi_transfer_desc
{
    const struct spi_node *node;      /* node for this transfer */
    const void *txbuf;                /* buffer to transmit */
    void       *rxbuf;                /* buffer to receive */
    int         count;                /* number of elements */
    spi_transfer_cb_fn_type callback; /* function to call when done */
    struct spi_transfer_desc *next;   /* next transfer queued,
                                         spi layer sets this */
};

/* NOTE: SPI updates the descrptor during the operation. Do not write
 * to it until completion notification is received. If no callback is
 * specified, the caller must find a way to ensure integrity.
 *
 * -1 will be written to 'count' if an error occurs, otherwise it will
 * be zero when completed.
 */

/* One-time init of SPI driver */
void spi_init(void);

/* Enable the specified module for the node */
void spi_enable_module(const struct spi_node *node);

/* Disabled the specified module for the node */
void spi_disable_module(const struct spi_node *node);

/* Send and/or receive data on the specified node (asychronous) */
bool spi_transfer(struct spi_transfer_desc *xfer);

#endif /* SPI_IMX31_H */
