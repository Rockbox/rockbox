/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Michael Sevakis
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
#include <stdlib.h>
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "avic-imx31.h"
#include "ccm-imx31.h"
#include "i2c-imx31.h"

/* Forward interrupt handler declarations */
#if (I2C_MODULE_MASK & USE_I2C1_MODULE)
static __attribute__((interrupt("IRQ"))) void I2C1_HANDLER(void);
#endif
#if (I2C_MODULE_MASK & USE_I2C2_MODULE)
static __attribute__((interrupt("IRQ"))) void I2C2_HANDLER(void);
#endif
#if (I2C_MODULE_MASK & USE_I2C3_MODULE)
static __attribute__((interrupt("IRQ"))) void I2C3_HANDLER(void);
#endif

#define IADR (0x00 / sizeof (unsigned short)) /* 00h */
#define IFDR (0x04 / sizeof (unsigned short)) /* 04h */
#define I2CR (0x08 / sizeof (unsigned short)) /* 08h */
#define I2SR (0x0c / sizeof (unsigned short)) /* 0ch */
#define I2DR (0x10 / sizeof (unsigned short)) /* 10h */

static struct i2c_module_descriptor
{
    volatile unsigned short * const base; /* Module base address */
    struct i2c_transfer_desc *head;       /* Running job */
    struct i2c_transfer_desc *tail;       /* Most recent job added */
    void (* const handler)(void);         /* Module interrupt handler */
    unsigned char addr;                   /* Composite address value */
    unsigned char busy;                   /* Have buffers been modified? */
    uint8_t enable;                       /* Enable count */
    const uint8_t cg;                     /* Clock gating index */
    const uint8_t ints;                   /* Module interrupt number */
} i2c_descs[I2C_NUM_I2C] =
{
#if (I2C_MODULE_MASK & USE_I2C1_MODULE)
    {
        .base    = (unsigned short *)I2C1_BASE_ADDR,
        .cg      = CG_I2C1,
        .ints    = INT_I2C1,
        .handler = I2C1_HANDLER,
    },
#endif
#if (I2C_MODULE_MASK & USE_I2C2_MODULE)
    {
        .base    = (unsigned short *)I2C2_BASE_ADDR,
        .cg      = CG_I2C2,
        .ints    = INT_I2C2,
        .handler = I2C2_HANDLER,
    },
#endif
#if (I2C_MODULE_MASK & USE_I2C3_MODULE)
    {
        .base    = (unsigned short *)I2C3_BASE_ADDR,
        .cg      = CG_I2C3,
        .ints    = INT_I2C3,
        .handler = I2C3_HANDLER,
    },
#endif
};


/* Actually begin the session at the queue head */
static bool start_transfer(struct i2c_module_descriptor * const desc,
                           struct i2c_transfer_desc * const xfer)
{
    volatile unsigned short * const base = desc->base;

    /* If interface isn't enabled or buffers have been used, the transfer
       cannot be started/restarted. */
    if (desc->enable == 0 || desc->busy != 0)
        return false;

    /* Set speed */
    base[IFDR] = xfer->node->ifdr;

    /* Clear status */
    base[I2SR] = 0;

    /* Enable module, enable Interrupt, master transmitter */
    unsigned short i2cr = I2C_I2CR_IEN | I2C_I2CR_IIEN | I2C_I2CR_MTX;

    unsigned char addr = xfer->node->addr & 0xfe;

    if (xfer->rxcount > 0)
    {
        addr |= 0x01; /* Slave address/rd */

        if (xfer->rxcount < 2)
        {
            /* Receiving less than two bytes - disable ACK generation */
            i2cr |= I2C_I2CR_TXAK;
        }
    }

    /* Remember composite address - contains info concerning RX or TX */
    desc->addr = addr;

    /* Set config, generate START. */
    base[I2CR] = i2cr | I2C_I2CR_MSTA;

    /* Address slave (first byte sent) and begin session. */
    base[I2DR] = addr;

    return true;
}

static void i2c_interrupt(struct i2c_module_descriptor * const desc)
{
    volatile unsigned short * const base = desc->base;
    unsigned short const i2sr = base[I2SR];
    struct i2c_transfer_desc *xfer = desc->head;

    base[I2SR] = 0; /* Clear IIF */

    if (i2sr & I2C_I2SR_IAL)
    {
        /* Bus arbitration was lost - retry current transfer until it succeeds */
        /* Best guess as to why: at high CPU speeds, voltage hasn't had time to
         * stabilize to register STOP if a new transfer is started very soon
         * after a previous one has completed. AFAICT, this might happen once at
         * most on a transfer. Switching to repeated START could be a better way
         * to handle the cases where multiple transfers are already queued. */
        if (start_transfer(desc, xfer))
            return;

        /* Restart failed - STOP */
        base[I2CR] &= ~(I2C_I2CR_MSTA | I2C_I2CR_IIEN);
    }
    else if (xfer->txcount >= 0 && (base[I2CR] & I2C_I2CR_MTX))
    {
        /* ADDR cycle or TX */
        if ((i2sr & I2C_I2SR_RXAK) != 0)
        {
            /* NACK */
        }
        else if (xfer->txcount > 0)
        {
            desc->busy = 1;
            base[I2DR] = *xfer->txdata++; /* Send next TX byte */
            xfer->txcount--;
            return;
        }
        else if (desc->addr & 0x01)
        {
            /* ADDR cycle is complete */
            base[I2CR] &= ~I2C_I2CR_MTX; /* Switch to RX mode */
            base[I2DR];                  /* Dummy read */
            return;
        }

        /* Transfer complete or NACK - STOP */
        base[I2CR] &= ~(I2C_I2CR_MSTA | I2C_I2CR_IIEN);
    }
    else if (xfer->rxcount > 0)
    {
        /* RX */
        desc->busy = 1;

        if (--xfer->rxcount > 0)
        {
            if (xfer->rxcount == 1)
            {
                /* 2nd to Last byte - NACK */
                base[I2CR] |= I2C_I2CR_TXAK;
            }

            *xfer->rxdata++ = base[I2DR]; /* Read data from I2DR and store */
            return;
        }

        /* Generate STOP signal before reading final byte */
        base[I2CR] &= ~(I2C_I2CR_MSTA | I2C_I2CR_IIEN);
        *xfer->rxdata++ = base[I2DR]; /* Read data from I2DR and store */
    }

    /* Start next transfer, if any */
    for (;;)
    {
        struct i2c_transfer_desc *next = xfer->next;
        i2c_transfer_cb_fn_type callback = xfer->callback;
        xfer->next = NULL;

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
            /* Queue next job */
            desc->head = next;

            if (callback != NULL)
                callback(xfer);

            desc->busy = 0;
            if (!start_transfer(desc, next))
            {
                xfer = next;
                continue;
            }
        }

        break;
    }
}

#if (I2C_MODULE_MASK & USE_I2C1_MODULE)
static __attribute__((interrupt("IRQ"))) void I2C1_HANDLER(void)
{
    i2c_interrupt(&i2c_descs[I2C1_NUM]);
}
#endif
#if (I2C_MODULE_MASK & USE_I2C2_MODULE)
static __attribute__((interrupt("IRQ"))) void I2C2_HANDLER(void)
{
    i2c_interrupt(&i2c_descs[I2C2_NUM]);
}
#endif
#if (I2C_MODULE_MASK & USE_I2C3_MODULE)
static __attribute__((interrupt("IRQ"))) void I2C3_HANDLER(void)
{
    i2c_interrupt(&i2c_descs[I2C3_NUM]);
}
#endif

/* Send and/or receive data on the specified node asynchronously */
bool i2c_transfer(struct i2c_transfer_desc *xfer)
{
    if (xfer == NULL || xfer->next != NULL || xfer->node == NULL ||
        xfer->rxcount < 0 || xfer->txcount < 0 ||
        (xfer->rxcount <= 0 && xfer->txcount <= 0))
    {
        /* Can't pass a busy descriptor, requires a node, < 0 sizes
           or all-0 sizes not permitted. */
        return false;
    }

    bool retval = true;
    struct i2c_module_descriptor * const desc = &i2c_descs[xfer->node->num];
    unsigned long cpsr = disable_irq_save();

    if (desc->head == NULL)
    {
        /* No transfers in progress; start interface. */
        desc->busy = 0;
        retval = start_transfer(desc, xfer);

        if (retval)
        {
            /* Start ok: actually put it in the queue. */
            desc->head = xfer;
            desc->tail = xfer;
            xfer->next = xfer; /* First, self-reference terminate */
        }
    }
    else
    {
        /* Already running: simply add to end and the final INT on the
         * running transfer will pick it up. */
        desc->tail->next = xfer; /* Add to tail */
        desc->tail       = xfer; /* New tail */
        xfer->next       = xfer; /* Self-reference terminate */
    }

    restore_irq(cpsr);

    return retval;
}

/** Synchronous transfers support **/
static void i2c_sync_xfer_complete_cb(struct i2c_transfer_desc *xfer)
{
    semaphore_release(&((struct i2c_sync_transfer_desc *)xfer)->sema);
}

static inline bool i2c_sync_wait_for_xfer_complete(struct i2c_sync_transfer_desc *xfer)
{
    return semaphore_wait(&xfer->sema, TIMEOUT_BLOCK) == OBJ_WAIT_SUCCEEDED;
}

int i2c_read(struct i2c_node *node, int addr, unsigned char *data,
             int data_count)
{
    struct i2c_sync_transfer_desc xfer;

    xfer.xfer.node = node;

    unsigned char ad;

    if (addr >= 0)
    {
        /* Sub-address */
        ad = addr;
        xfer.xfer.txdata = &ad;
        xfer.xfer.txcount = 1;
    }
    else
    {
        /* Raw read from slave */
        xfer.xfer.txcount = 0;
    }

    xfer.xfer.rxdata = data;
    xfer.xfer.rxcount = data_count;
    xfer.xfer.callback = i2c_sync_xfer_complete_cb;
    xfer.xfer.next = NULL;

    semaphore_init(&xfer.sema, 1, 0);

    if (i2c_transfer(&xfer.xfer) && i2c_sync_wait_for_xfer_complete(&xfer))
    {
        return data_count - xfer.xfer.rxcount;
    }

    return -1;
}

int i2c_write(struct i2c_node *node, const unsigned char *data,
              int data_count)
{
    struct i2c_sync_transfer_desc xfer;

    xfer.xfer.node = node;
    xfer.xfer.txdata = data;
    xfer.xfer.txcount = data_count;
    xfer.xfer.rxcount = 0;
    xfer.xfer.callback = i2c_sync_xfer_complete_cb;
    xfer.xfer.next = NULL;

    semaphore_init(&xfer.sema, 1, 0);

    if (i2c_transfer(&xfer.xfer) && i2c_sync_wait_for_xfer_complete(&xfer))
    {
        return data_count - xfer.xfer.txcount;
    }

    return -1;
}

void INIT_ATTR i2c_init(void)
{
    /* Do one-time inits for each module that will be used - leave
     * module disabled and unclocked until something wants it. */
    for (int i = 0; i < I2C_NUM_I2C; i++)
    {
        struct i2c_module_descriptor * const desc = &i2c_descs[i];
        ccm_module_clock_gating(desc->cg, CGM_ON_RUN_WAIT);
        desc->base[I2CR] = 0;
        ccm_module_clock_gating(desc->cg, CGM_OFF);
    }
}

/* Enable or disable the node - modules will be switched on/off accordingly. */
void i2c_enable_node(struct i2c_node *node, bool enable)
{
    struct i2c_module_descriptor * const desc = &i2c_descs[node->num];

    if (enable)
    {
        if (++desc->enable == 1)
        {
            /* First enable */
            ccm_module_clock_gating(desc->cg, CGM_ON_RUN_WAIT);
            desc->base[I2CR] = I2C_I2CR_IEN;
            desc->base[I2SR] = 0;
            avic_enable_int(desc->ints, INT_TYPE_IRQ, INT_PRIO_DEFAULT,
                            desc->handler);
        }
    }
    else
    {
        if (desc->enable > 0 && --desc->enable == 0)
        {
            /* Last enable */

            /* Wait for last tranfer */
            while (*(void ** volatile)&desc->head != NULL);

            /* Wait for STOP */
            while (desc->base[I2SR] & I2C_I2SR_IBB);

            desc->base[I2CR] &= ~I2C_I2CR_IEN;
            avic_disable_int(desc->ints);
            ccm_module_clock_gating(desc->cg, CGM_OFF);
        }
    }
}
