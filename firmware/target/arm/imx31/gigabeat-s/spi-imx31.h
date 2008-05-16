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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

struct spi_transfer
{
    const void *txbuf;
    void       *rxbuf;
    int         count;
};

/* One-time init of SPI driver */
void spi_init(void);

/* Enable the specified module for the node */
void spi_enable_module(struct spi_node *node);

/* Disabled the specified module for the node */
void spi_disable_module(struct spi_node *node);

/* Lock module mutex */
void spi_lock(struct spi_node *node);

/* Unlock module mutex */
void spi_unlock(struct spi_node *node);

/* Send and/or receive data on the specified node */
int spi_transfer(struct spi_node *node, struct spi_transfer *trans);

#endif /* SPI_IMX31_H */
