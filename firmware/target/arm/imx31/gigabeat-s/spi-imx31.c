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
#include "cpu.h"
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
    volatile unsigned long *base;
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
        .base    = (unsigned long *)CSPI1_BASE_ADDR,
        .cg      = CG_CSPI1,
        .ints    = CSPI1,
        .handler = CSPI1_HANDLER,
    },
#endif
#if (SPI_MODULE_MASK & USE_CSPI2_MODULE)
    {
        .base    = (unsigned long *)CSPI2_BASE_ADDR,
        .cg      = CG_CSPI2,
        .ints    = CSPI2,
        .handler = CSPI2_HANDLER,
    },
#endif
#if (SPI_MODULE_MASK & USE_CSPI3_MODULE)
    {
        .base    = (unsigned long *)CSPI3_BASE_ADDR,
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
    volatile unsigned long * const base = desc->base;
    struct spi_transfer *trans = desc->trans;
    int inc = desc->byte_size + 1;

    if (desc->rxcount > 0)
    {
        /* Data received - empty out RXFIFO */
        while ((base[CSPI_STATREG_I] & CSPI_STATREG_RR) != 0)
        {
            uint32_t word = base[CSPI_RXDATA_I];

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
                unsigned long intreg = base[CSPI_INTREG_I];

                if (desc->rxcount <= 0)
                {
                    /* No more to receive - stop RX interrupts */
                    intreg &= ~(CSPI_INTREG_RHEN | CSPI_INTREG_RREN);
                    base[CSPI_INTREG_I] = intreg;
                    break;
                }
                else if (!(intreg & CSPI_INTREG_RREN))
                {
                    /* < 4 words expected - switch to RX ready */
                    intreg &= ~CSPI_INTREG_RHEN;
                    base[CSPI_INTREG_I] = intreg | CSPI_INTREG_RREN;
                }
            }
        }
    }

    if (trans->count > 0)
    {
        /* Data to transmit - fill TXFIFO or write until exhausted */
        while ((base[CSPI_STATREG_I] & CSPI_STATREG_TF) == 0)
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

            base[CSPI_TXDATA_I] = word;

            if (--trans->count <= 0)
            {
                /* Out of data - stop TX interrupts */
                base[CSPI_INTREG_I] &= ~CSPI_INTREG_THEN;
                break;
            }
        }
    }

    /* If all interrupts have been remasked - we're done */
    if (base[CSPI_INTREG_I] == 0)
    {
        base[CSPI_STATREG_I] = CSPI_STATREG_TC | CSPI_STATREG_BO;
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
    volatile unsigned long * const base = desc->base;

    if ((base[CSPI_CONREG_I] & CSPI_CONREG_EN) == 0)
        return false;

    if (node != desc->last)
    {
        /* Switch the module's node */
        desc->last = node;
        desc->byte_size = (((node->conreg >> 8) & 0x1f) + 1 + 7) / 8 - 1;

        /* Keep reserved and start bits cleared. Keep enabled bit. */
        base[CSPI_CONREG_I] =
            (node->conreg & ~(0xfcc8e000 | CSPI_CONREG_XCH | CSPI_CONREG_SMC))
            | CSPI_CONREG_EN;
        /* Set the wait-states */
        base[CSPI_PERIODREG_I] = node->periodreg & 0xffff;
        /* Clear out any spuriously-pending interrupts */
        base[CSPI_STATREG_I] = CSPI_STATREG_TC | CSPI_STATREG_BO;
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
        volatile unsigned long * const base = desc->base;

        /* Enable clock-gating register */
        imx31_clkctl_module_clock_gating(desc->cg, CGM_ON_ALL);
        
        /* Reset */
        base[CSPI_CONREG_I] &= ~CSPI_CONREG_EN;
        base[CSPI_CONREG_I] |= CSPI_CONREG_EN;
        base[CSPI_INTREG_I] = 0;
        base[CSPI_STATREG_I] = CSPI_STATREG_TC | CSPI_STATREG_BO;

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
        volatile unsigned long * const base = desc->base;

        /* Disable interrupt at controller level */
        avic_disable_int(desc->ints);

        /* Disable interface */
        base[CSPI_CONREG_I] &= ~CSPI_CONREG_EN;

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
        volatile unsigned long * const base = desc->base;
        unsigned long intreg;

        desc->trans = trans;
        desc->rxcount = trans->count;

        /* Enable needed interrupts - FIFOs will start filling */
        intreg = CSPI_INTREG_THEN;

        intreg |= (trans->count < 4) ?
            CSPI_INTREG_RREN : /* Must grab data on every word */
            CSPI_INTREG_RHEN;  /* Enough data to wait for half-full */

        base[CSPI_INTREG_I] = intreg;

        /* Start transfer */
        base[CSPI_CONREG_I] |= CSPI_CONREG_XCH;

        if (wakeup_wait(&desc->w, HZ) != OBJ_WAIT_SUCCEEDED)
        {
            base[CSPI_INTREG_I] = 0;
            base[CSPI_CONREG_I] &= ~CSPI_CONREG_XCH;
            retval = false;
        }
    }

    mutex_unlock(&desc->m);

    return retval;
}
