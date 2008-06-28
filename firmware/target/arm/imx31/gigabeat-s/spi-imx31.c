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
#include "config.h"
#include "system.h"
#include "spi-imx31.h"
#include "avic-imx31.h"
#include "clkctl-imx31.h"
#include "debug.h"
#include "kernel.h"

/* Forward interrupt handler declarations */
#if (SPI_MODULE_MASK & USE_CSPI1_MODULE)
static __attribute__((interrupt("IRQ"))) void CSPI1_HANDLER(void);
#endif
#if (SPI_MODULE_MASK & USE_CSPI2_MODULE)
static __attribute__((interrupt("IRQ"))) void CSPI2_HANDLER(void);
#endif
#if (SPI_MODULE_MASK & USE_CSPI3_MODULE)
static __attribute__((interrupt("IRQ"))) void CSPI3_HANDLER(void);
#endif

/* State data associatated with each CSPI module */
static struct spi_module_descriptor
{
    struct cspi_map * const base;
    int enab;
    struct spi_node *last;
    enum IMX31_CG_LIST cg;
    enum IMX31_INT_LIST ints;
    int byte_size;
    void (*handler)(void);
    struct mutex m;
    struct wakeup w;
    struct spi_transfer *trans;
    int rxcount;
} spi_descs[SPI_NUM_CSPI] =
/* Init non-zero members */
{
#if (SPI_MODULE_MASK & USE_CSPI1_MODULE)
    {
        .base    = (struct cspi_map *)CSPI1_BASE_ADDR,
        .cg      = CG_CSPI1,
        .ints    = CSPI1,
        .handler = CSPI1_HANDLER,
    },
#endif
#if (SPI_MODULE_MASK & USE_CSPI2_MODULE)
    {
        .base    = (struct cspi_map *)CSPI2_BASE_ADDR,
        .cg      = CG_CSPI2,
        .ints    = CSPI2,
        .handler = CSPI2_HANDLER,
    },
#endif
#if (SPI_MODULE_MASK & USE_CSPI3_MODULE)
    {
        .base    = (struct cspi_map *)CSPI3_BASE_ADDR,
        .cg      = CG_CSPI3,
        .ints    = CSPI3,
        .handler = CSPI3_HANDLER,
    },
#endif
};

/* Common code for interrupt handlers */
static void spi_interrupt(enum spi_module_number spi)
{
    struct spi_module_descriptor *desc = &spi_descs[spi];
    struct cspi_map * const base = desc->base;
    struct spi_transfer *trans = desc->trans;
    int inc = desc->byte_size + 1;

    if (desc->rxcount > 0)
    {
        /* Data received - empty out RXFIFO */
        while ((base->statreg & CSPI_STATREG_RR) != 0)
        {
            uint32_t word = base->rxdata;

            switch (desc->byte_size & 3)
            {
            case 3:
                *(unsigned char *)(trans->rxbuf + 3) = word >> 24;
            case 2:
                *(unsigned char *)(trans->rxbuf + 2) = word >> 16;
            case 1:
                *(unsigned char *)(trans->rxbuf + 1) = word >> 8;
            case 0:
                *(unsigned char *)(trans->rxbuf + 0) = word;
            }

            trans->rxbuf += inc;

            if (--desc->rxcount < 4)
            {
                unsigned long intreg = base->intreg;

                if (desc->rxcount <= 0)
                {
                    /* No more to receive - stop RX interrupts */
                    intreg &= ~(CSPI_INTREG_RHEN | CSPI_INTREG_RREN);
                    base->intreg = intreg;
                    break;
                }
                else if (!(intreg & CSPI_INTREG_RREN))
                {
                    /* < 4 words expected - switch to RX ready */
                    intreg &= ~CSPI_INTREG_RHEN;
                    base->intreg = intreg | CSPI_INTREG_RREN;
                }
            }
        }
    }

    if (trans->count > 0)
    {
        /* Data to transmit - fill TXFIFO or write until exhausted */
        while ((base->statreg & CSPI_STATREG_TF) == 0)
        {
            uint32_t word = 0;

            switch (desc->byte_size & 3)
            {
            case 3:
                word  = *(unsigned char *)(trans->txbuf + 3) << 24;
            case 2:
                word |= *(unsigned char *)(trans->txbuf + 2) << 16;
            case 1:
                word |= *(unsigned char *)(trans->txbuf + 1) << 8;
            case 0:
                word |= *(unsigned char *)(trans->txbuf + 0);
            }

            trans->txbuf += inc;

            base->txdata = word;

            if (--trans->count <= 0)
            {
                /* Out of data - stop TX interrupts */
                base->intreg &= ~CSPI_INTREG_THEN;
                break;
            }
        }
    }

    /* If all interrupts have been remasked - we're done */
    if (base->intreg == 0)
    {
        base->statreg = CSPI_STATREG_TC | CSPI_STATREG_BO;
        wakeup_signal(&desc->w);
    }
}

/* Interrupt handlers for each CSPI module */
#if (SPI_MODULE_MASK & USE_CSPI1_MODULE)
static __attribute__((interrupt("IRQ"))) void CSPI1_HANDLER(void)
{
    spi_interrupt(CSPI1_NUM);
}
#endif

#if (SPI_MODULE_MASK & USE_CSPI2_MODULE)
static __attribute__((interrupt("IRQ"))) void CSPI2_HANDLER(void)
{
    spi_interrupt(CSPI2_NUM);
}
#endif

#if (SPI_MODULE_MASK & USE_CSPI3_MODULE)
static __attribute__((interrupt("IRQ"))) void CSPI3_HANDLER(void)
{
    spi_interrupt(CSPI3_NUM);
}
#endif

/* Write the context for the node and remember it to avoid unneeded reconfigure */
static bool spi_set_context(struct spi_node *node,
                            struct spi_module_descriptor *desc)
{
    struct cspi_map * const base = desc->base;

    if ((base->conreg & CSPI_CONREG_EN) == 0)
        return false;

    if (node != desc->last)
    {
        /* Switch the module's node */
        desc->last = node;
        desc->byte_size = (((node->conreg >> 8) & 0x1f) + 1 + 7) / 8 - 1;

        /* Keep reserved and start bits cleared. Keep enabled bit. */
        base->conreg =
            (node->conreg & ~(0xfcc8e000 | CSPI_CONREG_XCH | CSPI_CONREG_SMC))
            | CSPI_CONREG_EN;
        /* Set the wait-states */
        base->periodreg = node->periodreg & 0xffff;
        /* Clear out any spuriously-pending interrupts */
        base->statreg = CSPI_STATREG_TC | CSPI_STATREG_BO;
    }

    return true;
}

/* Initialize each of the used SPI descriptors */
void spi_init(void)
{
    int i;

    for (i = 0; i < SPI_NUM_CSPI; i++)
    {
        struct spi_module_descriptor * const desc = &spi_descs[i];
        mutex_init(&desc->m);
        wakeup_init(&desc->w);
    }
}

/* Get mutually-exclusive access to the node */
void spi_lock(struct spi_node *node)
{
    mutex_lock(&spi_descs[node->num].m);
}

/* Release mutual exclusion */
void spi_unlock(struct spi_node *node)
{
    mutex_unlock(&spi_descs[node->num].m);
}

/* Enable the specified module for the node */
void spi_enable_module(struct spi_node *node)
{
    struct spi_module_descriptor * const desc = &spi_descs[node->num];

    mutex_lock(&desc->m);

    if (++desc->enab == 1)
    {
        /* First enable for this module */
        struct cspi_map * const base = desc->base;

        /* Enable clock-gating register */
        imx31_clkctl_module_clock_gating(desc->cg, CGM_ON_ALL);
        
        /* Reset */
        base->conreg &= ~CSPI_CONREG_EN;
        base->conreg |= CSPI_CONREG_EN;
        base->intreg = 0;
        base->statreg = CSPI_STATREG_TC | CSPI_STATREG_BO;

        /* Enable interrupt at controller level */
        avic_enable_int(desc->ints, IRQ, 6, desc->handler);
    }

    mutex_unlock(&desc->m);
}

/* Disabled the specified module for the node */
void spi_disable_module(struct spi_node *node)
{
    struct spi_module_descriptor * const desc = &spi_descs[node->num];

    mutex_lock(&desc->m);

    if (desc->enab > 0 && --desc->enab == 0)
    {
        /* Last enable for this module */
        struct cspi_map * const base = desc->base;

        /* Disable interrupt at controller level */
        avic_disable_int(desc->ints);

        /* Disable interface */
        base->conreg &= ~CSPI_CONREG_EN;

        /* Disable interface clock */
        imx31_clkctl_module_clock_gating(desc->cg, CGM_OFF);
    }

    mutex_unlock(&desc->m);
}

/* Send and/or receive data on the specified node */
int spi_transfer(struct spi_node *node, struct spi_transfer *trans)
{
    struct spi_module_descriptor * const desc = &spi_descs[node->num];
    int retval;

    if (trans->count <= 0)
        return true;

    mutex_lock(&desc->m);

    retval = spi_set_context(node, desc);

    if (retval)
    {
        struct cspi_map * const base = desc->base;
        unsigned long intreg;

        desc->trans = trans;
        desc->rxcount = trans->count;

        /* Enable needed interrupts - FIFOs will start filling */
        intreg = CSPI_INTREG_THEN;

        intreg |= (trans->count < 4) ?
            CSPI_INTREG_RREN : /* Must grab data on every word */
            CSPI_INTREG_RHEN;  /* Enough data to wait for half-full */

        base->intreg = intreg;

        /* Start transfer */
        base->conreg |= CSPI_CONREG_XCH;

        if (wakeup_wait(&desc->w, HZ) != OBJ_WAIT_SUCCEEDED)
        {
            base->intreg = 0;
            base->conreg &= ~CSPI_CONREG_XCH;
            retval = false;
        }
    }

    mutex_unlock(&desc->m);

    return retval;
}
