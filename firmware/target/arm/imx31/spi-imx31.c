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
#include "ccm-imx31.h"
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
static struct spi_module_desc
{
    struct cspi_map * const base;     /* CSPI module address */
    struct spi_transfer_desc *head;   /* Running job */
    struct spi_transfer_desc *tail;   /* Most recent job added */
    const struct spi_node *last_node; /* Last node used for module */
    void (*handler)(void);            /* Interrupt handler */
    int rxcount;                      /* Independent copy of txcount */
    int8_t enab;                      /* Enable count */
    int8_t byte_size;                 /* Size of transfers in bytes */
    int8_t cg;                        /* Clock-gating value */
    int8_t ints;                      /* AVIC vector number */
} spi_descs[SPI_NUM_CSPI] =
/* Init non-zero members */
{
#if (SPI_MODULE_MASK & USE_CSPI1_MODULE)
    {
        .base    = (struct cspi_map *)CSPI1_BASE_ADDR,
        .cg      = CG_CSPI1,
        .ints    = INT_CSPI1,
        .handler = CSPI1_HANDLER,
    },
#endif
#if (SPI_MODULE_MASK & USE_CSPI2_MODULE)
    {
        .base    = (struct cspi_map *)CSPI2_BASE_ADDR,
        .cg      = CG_CSPI2,
        .ints    = INT_CSPI2,
        .handler = CSPI2_HANDLER,
    },
#endif
#if (SPI_MODULE_MASK & USE_CSPI3_MODULE)
    {
        .base    = (struct cspi_map *)CSPI3_BASE_ADDR,
        .cg      = CG_CSPI3,
        .ints    = INT_CSPI3,
        .handler = CSPI3_HANDLER,
    },
#endif
};

/* Reset the module */
static void spi_reset(struct spi_module_desc * const desc)
{
    /* Reset by leaving it disabled */
    struct cspi_map * const base = desc->base;
    base->conreg &= ~CSPI_CONREG_EN;
}

/* Write the context for the node and remember it to avoid unneeded reconfigure */
static bool spi_set_context(struct spi_module_desc *desc,
                            struct spi_transfer_desc *xfer)
{
    const struct spi_node * const node = xfer->node; 
    struct cspi_map * const base = desc->base;

    if (desc->enab == 0)
        return false;

    if (node == desc->last_node)
        return true;

    /* Errata says CSPI should be disabled when writing PERIODREG. */
    base->conreg &= ~CSPI_CONREG_EN;

    /* Switch the module's node */
    desc->last_node = node;
    desc->byte_size = (((node->conreg >> 8) & 0x1f) + 1 + 7) / 8 - 1;

    /* Set the wait-states */
    base->periodreg = node->periodreg & 0xffff;

    /* Keep reserved and start bits cleared. Keep enabled bit. */
    base->conreg =
        (node->conreg & ~(0xfcc8e000 | CSPI_CONREG_XCH | CSPI_CONREG_SMC));
    return true;
}


/* Fill the TX fifo. Returns the number of remaining words. */
static int tx_fill_fifo(struct spi_module_desc * const desc,
                        struct cspi_map * const base,
                        struct spi_transfer_desc * const xfer)
{
    int count = xfer->count;
    int size = desc->byte_size;

    while ((base->statreg & CSPI_STATREG_TF) == 0)
    {
        uint32_t word = 0;

        switch (size & 3)
        {
        case 3:
            word  = *(unsigned char *)(xfer->txbuf + 3) << 24;
        case 2:
            word |= *(unsigned char *)(xfer->txbuf + 2) << 16;
        case 1:
            word |= *(unsigned char *)(xfer->txbuf + 1) << 8;
        case 0:
            word |= *(unsigned char *)(xfer->txbuf + 0);
        }

        xfer->txbuf += size + 1; /* Increment buffer */

        base->txdata = word;    /* Write to FIFO */

        if (--count == 0)
            break;
    }

    xfer->count = count;

    return count;
}

/* Start a transfer on the SPI */
static bool start_transfer(struct spi_module_desc * const desc,
                           struct spi_transfer_desc * const xfer)
{
    struct cspi_map * const base = desc->base;
    unsigned long intreg;

    if (!spi_set_context(desc, xfer))
        return false;

    base->conreg |= CSPI_CONREG_EN; /* Enable module */

    desc->rxcount = xfer->count;

    intreg = (xfer->count < 8) ?
        CSPI_INTREG_TCEN : /* Trans. complete: TX will run out in prefill */
        CSPI_INTREG_THEN;  /* INT when TX half-empty */

    intreg |= (xfer->count < 4) ?
        CSPI_INTREG_RREN : /* Must grab data on every word */
        CSPI_INTREG_RHEN;  /* Enough data to wait for half-full */

    tx_fill_fifo(desc, base, xfer);

    base->statreg = CSPI_STATREG_TC; /* Ack 'complete' */
    base->intreg = intreg;           /* Enable interrupts */
    base->conreg |= CSPI_CONREG_XCH; /* Begin transfer */

    return true;
}

/* Common code for interrupt handlers */
static void spi_interrupt(enum spi_module_number spi)
{
    struct spi_module_desc *desc = &spi_descs[spi];
    struct cspi_map * const base = desc->base;
    unsigned long intreg = base->intreg;
    struct spi_transfer_desc *xfer = desc->head;
    int inc = desc->byte_size + 1;

    /* Data received - empty out RXFIFO */
    while ((base->statreg & CSPI_STATREG_RR) != 0)
    {
        uint32_t word = base->rxdata;

        if (desc->rxcount <= 0)
            continue;

        if (xfer->rxbuf != NULL)
        {
            /* There is a receive buffer */
            switch (desc->byte_size & 3)
            {
            case 3:
                *(unsigned char *)(xfer->rxbuf + 3) = word >> 24;
            case 2:
                *(unsigned char *)(xfer->rxbuf + 2) = word >> 16;
            case 1:
                *(unsigned char *)(xfer->rxbuf + 1) = word >> 8;
            case 0:
                *(unsigned char *)(xfer->rxbuf + 0) = word;
            }

            xfer->rxbuf += inc;
        }

        if (--desc->rxcount < 4)
        {
            if (desc->rxcount == 0)
            {
                /* No more to receive - stop RX interrupts */
                intreg &= ~(CSPI_INTREG_RHEN | CSPI_INTREG_RREN);
                base->intreg = intreg;
            }
            else if (intreg & CSPI_INTREG_RHEN)
            {
                /* < 4 words expected - switch to RX ready */
                intreg &= ~CSPI_INTREG_RHEN;
                intreg |= CSPI_INTREG_RREN;
                base->intreg = intreg;
            }
        }
    }

    if (xfer->count > 0)
    {
        /* Data to transmit - fill TXFIFO or write until exhausted. */
        if (tx_fill_fifo(desc, base, xfer) != 0)
            return;

        /* Out of data - stop TX interrupts, enable TC interrupt. */
        intreg &= ~CSPI_INTREG_THEN;
        intreg |= CSPI_INTREG_TCEN;
        base->intreg = intreg;
    }

    if ((intreg & CSPI_INTREG_TCEN) && (base->statreg & CSPI_STATREG_TC))
    {
        /* Outbound transfer is complete. */
        intreg &= ~CSPI_INTREG_TCEN;
        base->intreg = intreg;
        base->statreg = CSPI_STATREG_TC; /* Ack 'complete' */
    }

    if (intreg != 0)
        return;

    /* All interrupts are masked; we're done with current transfer. */
    for (;;)
    {
        struct spi_transfer_desc *next = xfer->next;
        spi_transfer_cb_fn_type callback = xfer->callback;
        xfer->next = NULL;

        base->conreg &= ~CSPI_CONREG_EN; /* Disable module */

        if (next == xfer)
        {
            /* Last job on queue */
            desc->head = NULL;

            if (callback != NULL)
                callback(xfer);

            /* Callback may have restarted transfers. */
        }
        else
        {
            /* Queue next job. */
            desc->head = next;

            if (callback != NULL)
                callback(xfer);

            if (!start_transfer(desc, next))
            {
                xfer = next;
                xfer->count = -1;
                continue; /* Failed: try next */
            }
        }

        break;
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

/* Initialize the SPI driver */
void spi_init(void)
{
    unsigned i;
    for (i = 0; i < SPI_NUM_CSPI; i++)
    {
        struct spi_module_desc * const desc = &spi_descs[i];
        ccm_module_clock_gating(desc->cg, CGM_ON_RUN_WAIT);
        spi_reset(desc);
        ccm_module_clock_gating(desc->cg, CGM_OFF);
    }
}

/* Enable the specified module for the node */
void spi_enable_module(const struct spi_node *node)
{
    struct spi_module_desc * const desc = &spi_descs[node->num];

    if (++desc->enab == 1)
    {
        /* Enable clock-gating register */
        ccm_module_clock_gating(desc->cg, CGM_ON_RUN_WAIT);
        /* Reset */
        spi_reset(desc);
        desc->last_node = NULL;
        /* Enable interrupt at controller level */
        avic_enable_int(desc->ints, INT_TYPE_IRQ, INT_PRIO_DEFAULT,
                        desc->handler);
    }
}

/* Disable the specified module for the node */
void spi_disable_module(const struct spi_node *node)
{
    struct spi_module_desc * const desc = &spi_descs[node->num];

    if (desc->enab > 0 && --desc->enab == 0)
    {
        /* Last enable for this module */
        struct cspi_map * const base = desc->base;

        /* Wait for outstanding transactions */
        while (*(void ** volatile)&desc->head != NULL);

        /* Disable interrupt at controller level */
        avic_disable_int(desc->ints);

        /* Disable interface */
        base->conreg &= ~CSPI_CONREG_EN;

        /* Disable interface clock */
        ccm_module_clock_gating(desc->cg, CGM_OFF);
    }
}

/* Send and/or receive data on the specified node */
bool spi_transfer(struct spi_transfer_desc *xfer)
{
    bool retval;
    struct spi_module_desc * desc;
    int oldlevel;

    if (xfer->count == 0)
        return true;  /* No data? No problem. */

    if (xfer->count < 0 || xfer->next != NULL || xfer->node == NULL)
    {
        /* Can't pass a busy descriptor, requires a node and negative size
         * is invalid to pass. */
        return false;
    }

    oldlevel = disable_irq_save();

    desc = &spi_descs[xfer->node->num];

    if (desc->head == NULL)
    {
        /* No transfers in progress; start interface. */
        retval = start_transfer(desc, xfer);

        if (retval)
        {
            /* Start ok: actually put it in the queue. */
            desc->head = xfer;
            desc->tail = xfer;
            xfer->next = xfer; /* First, self-reference terminate */
        }
        else
        {
            xfer->count = -1; /* Signal error */
        }
    }
    else
    {
        /* Already running: simply add to end and the final INT on the
         * running transfer will pick it up. */
        desc->tail->next = xfer; /* Add to tail */
        desc->tail       = xfer; /* New tail */
        xfer->next       = xfer; /* Self-reference terminate */
        retval = true;
    }

    restore_irq(oldlevel);

    return retval;
}
