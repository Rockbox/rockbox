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
    void (* const handler)(void);         /* Module interrupt handler */
    struct mutex m;                       /* Node mutual-exclusion */
    struct wakeup w;                      /* I2C done signal */
    unsigned char *addr_data;             /* Additional addressing data */
    int addr_count;                       /* Addressing byte count */
    unsigned char *data;                  /* TX/RX buffer (actual data) */
    int data_count;                       /* TX/RX byte count */
    unsigned char addr;                   /* Address + r/w bit */
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

static void i2c_interrupt(enum i2c_module_number i2c)
{
    struct i2c_module_descriptor * const desc = &i2c_descs[i2c];
    volatile unsigned short * const base = desc->base;
    unsigned short i2sr = base[I2SR];

    base[I2SR] = 0; /* Clear IIF */

    if (desc->addr_count >= 0)
    {
        /* ADDR cycle - either done or more to send */
        if ((i2sr & I2C_I2SR_RXAK) != 0)
        {
            goto i2c_stop; /* problem */
        }

        if (--desc->addr_count < 0)
        {
            /* Switching to data cycle */
            if (desc->addr & 0x1)
            {
                base[I2CR] &= ~I2C_I2CR_MTX; /* Switch to RX mode */
                base[I2DR];                  /* Dummy read */
                return;
            }
            /* else remaining data is TX - handle below */
            goto i2c_transmit;
        }
        else
        {
            base[I2DR] = *desc->addr_data++; /* Send next addressing byte */
            return;
        }
    }

    if (base[I2CR] & I2C_I2CR_MTX)
    {
        /* Transmitting data */
        if ((i2sr & I2C_I2SR_RXAK) == 0)
        {
i2c_transmit:
            if (desc->data_count > 0)
            {
                /* More bytes to send, got ACK from previous byte */
                base[I2DR] = *desc->data++;
                desc->data_count--;
                return;
            }
        }
        /* else done or no ACK received */
    }
    else
    {
        /* Receiving data */
        if (--desc->data_count > 0)
        {
            if (desc->data_count == 1)
            {
                /* 2nd to Last byte - NACK */
                base[I2CR] |= I2C_I2CR_TXAK;
            }

            *desc->data++ = base[I2DR]; /* Read data from I2DR and store */
            return;
        }
        else
        {
            /* Generate STOP signal before reading data */
            base[I2CR] &= ~(I2C_I2CR_MSTA | I2C_I2CR_IIEN);
            *desc->data++ = base[I2DR]; /* Read data from I2DR and store */
            goto i2c_done;
        }
    }

i2c_stop:
    /* Generate STOP signal */
    base[I2CR] &= ~(I2C_I2CR_MSTA | I2C_I2CR_IIEN);
i2c_done:
    /* Signal thread we're done */
    wakeup_signal(&desc->w);
}

#if (I2C_MODULE_MASK & USE_I2C1_MODULE)
static __attribute__((interrupt("IRQ"))) void I2C1_HANDLER(void)
{
    i2c_interrupt(I2C1_NUM);
}
#endif
#if (I2C_MODULE_MASK & USE_I2C2_MODULE)
static __attribute__((interrupt("IRQ"))) void I2C2_HANDLER(void)
{
    i2c_interrupt(I2C2_NUM);
}
#endif
#if (I2C_MODULE_MASK & USE_I2C3_MODULE)
static __attribute__((interrupt("IRQ"))) void I2C3_HANDLER(void)
{
    i2c_interrupt(I2C3_NUM);
}
#endif

static int i2c_transfer(struct i2c_node * const node,
                        struct i2c_module_descriptor *const desc)
{
    volatile unsigned short * const base = desc->base;
    int count = desc->data_count;
    uint16_t i2cr;

    /* Make sure bus is idle. */
    while (base[I2SR] & I2C_I2SR_IBB);

    /* Set speed */
    base[IFDR] = node->ifdr;

    /* Enable module */
    base[I2CR] = I2C_I2CR_IEN;

    /* Enable Interrupt, Master */
    i2cr = I2C_I2CR_IEN | I2C_I2CR_IIEN | I2C_I2CR_MTX;

    if ((desc->addr & 0x1) && desc->data_count < 2)
    {
        /* Receiving less than two bytes - disable ACK generation */
        i2cr |= I2C_I2CR_TXAK;
    }

    /* Set config */
    base[I2CR] = i2cr;

    /* Generate START */
    base[I2CR] = i2cr | I2C_I2CR_MSTA;

    /* Address slave (first byte sent) and begin session. */
    base[I2DR] = desc->addr;

    /* Wait for transfer to complete */
    if (wakeup_wait(&desc->w, HZ) == OBJ_WAIT_SUCCEEDED)
    {
        count -= desc->data_count;
    }
    else
    {
        /* Generate STOP if timeout */
        base[I2CR] &= ~(I2C_I2CR_MSTA | I2C_I2CR_IIEN);
        count = -1;
    }

    desc->addr_count = 0;

    return count;
}

int i2c_read(struct i2c_node *node, int reg,
             unsigned char *data, int data_count)
{
    struct i2c_module_descriptor *const desc = &i2c_descs[node->num];
    unsigned char ad[1];

    mutex_lock(&desc->m);

    desc->addr = (node->addr & 0xfe) | 0x1;  /* Slave address/rd */

    if (reg >= 0)
    {
        /* Sub-address */
        desc->addr_count = 1;
        desc->addr_data = ad;
        ad[0] = reg;
    }
    /* else raw read from slave */

    desc->data = data;
    desc->data_count = data_count;

    data_count = i2c_transfer(node, desc);

    mutex_unlock(&desc->m);

    return data_count;
}

int i2c_write(struct i2c_node *node, const unsigned char *data, int data_count)
{
    struct i2c_module_descriptor *const desc = &i2c_descs[node->num];

    mutex_lock(&desc->m);

    desc->addr = node->addr & 0xfe; /* Slave address/wr */
    desc->data = (unsigned char *)data;
    desc->data_count = data_count;

    data_count = i2c_transfer(node, desc);

    mutex_unlock(&desc->m);

    return data_count;
}

void i2c_init(void)
{
    int i;

    /* Do one-time inits for each module that will be used - leave
     * module disabled and unclocked until something wants it */
    for (i = 0; i < I2C_NUM_I2C; i++)
    {
        struct i2c_module_descriptor *const desc = &i2c_descs[i];
        ccm_module_clock_gating(desc->cg, CGM_ON_RUN_WAIT);
        mutex_init(&desc->m);
        wakeup_init(&desc->w);
        desc->base[I2CR] = 0;
        ccm_module_clock_gating(desc->cg, CGM_OFF);
    }
}

void i2c_enable_node(struct i2c_node *node, bool enable)
{
    struct i2c_module_descriptor *const desc = &i2c_descs[node->num];

    mutex_lock(&desc->m);

    if (enable)
    {
        if (++desc->enable == 1)
        {
            /* First enable */
            ccm_module_clock_gating(desc->cg, CGM_ON_RUN_WAIT);
            avic_enable_int(desc->ints, INT_TYPE_IRQ, INT_PRIO_DEFAULT,
                            desc->handler);
        }
    }
    else
    {
        if (desc->enable > 0 && --desc->enable == 0)
        {
            /* Last enable */
            while (desc->base[I2SR] & I2C_I2SR_IBB); /* Wait for STOP */
            desc->base[I2CR] &= ~I2C_I2CR_IEN;
            avic_disable_int(desc->ints);
            ccm_module_clock_gating(desc->cg, CGM_OFF);
        }
    }

    mutex_unlock(&desc->m);
}

void i2c_lock_node(struct i2c_node *node)
{
    struct i2c_module_descriptor *const desc = &i2c_descs[node->num];
    mutex_lock(&desc->m);
}

void i2c_unlock_node(struct i2c_node *node)
{
    struct i2c_module_descriptor *const desc = &i2c_descs[node->num];
    mutex_unlock(&desc->m);
}
