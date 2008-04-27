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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "clkctl-imx31.h"
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

static struct i2c_module_descriptor
{
    struct i2c_map *base;     /* Module base address */
    enum IMX31_CG_LIST cg;    /* Clock gating index */
    enum IMX31_INT_LIST ints; /* Module interrupt number */
    int enable;               /* Enable count */
    void (*handler)(void);    /* Module interrupt handler */
    struct mutex m;           /* Node mutual-exclusion */
    struct wakeup w;          /* I2C done signal */
    unsigned char *addr_data; /* Additional addressing data */
    int addr_count;           /* Addressing byte count */
    unsigned char *data;      /* TX/RX buffer (actual data) */
    int data_count;           /* TX/RX byte count */
    unsigned char addr;       /* Address + r/w bit */
} i2c_descs[I2C_NUM_I2C] =
{
#if (I2C_MODULE_MASK & USE_I2C1_MODULE)
    {
        .base    = (struct i2c_map *)I2C1_BASE_ADDR,
        .cg      = CG_I2C1,
        .ints    = I2C1,
        .handler = I2C1_HANDLER,
    },
#endif
#if (I2C_MODULE_MASK & USE_I2C2_MODULE)
    {
        .base    = (struct i2c_map *)I2C2_BASE_ADDR,
        .cg      = CG_I2C2,
        .ints    = I2C2,
        .handler = I2C2_HANDLER,
    },
#endif
#if (I2C_MODULE_MASK & USE_I2C3_MODULE)
    {
        .base    = (struct i2c_map *)I2C3_BASE_ADDR,
        .cg      = CG_I2C3,
        .ints    = I2C3,
        .handler = I2C3_HANDLER,
    },
#endif
};

static void i2c_interrupt(enum i2c_module_number i2c)
{
    struct i2c_module_descriptor *const desc = &i2c_descs[i2c];
    struct i2c_map * const base = desc->base;
    uint16_t i2sr = base->i2sr;

    base->i2sr = i2sr & ~I2C_I2SR_IIF; /* Clear IIF */

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
                base->i2cr &= ~I2C_I2CR_MTX; /* Switch to RX mode */
                base->i2dr;                  /* Dummy read */
                return;
            }
            /* else remaining data is TX - handle below */
            goto i2c_transmit;
        }
        else
        {
            base->i2dr = *desc->addr_data++; /* Send next addressing byte */
            return;
        }
    }

    if (base->i2cr & I2C_I2CR_MTX)
    {
i2c_transmit:
        /* Transmitting data */
        if ((i2sr & I2C_I2SR_RXAK) == 0 && desc->data_count > 0)
        {
            /* More bytes to send, got ACK from previous byte */
            base->i2dr = *desc->data++;
            desc->data_count--;
            return;
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
                base->i2cr |= I2C_I2CR_TXAK;
            }

            *desc->data++ = base->i2dr; /* Read data from I2DR and store */
            return;
        }
        else
        {
            /* Generate STOP signal before reading data */
            base->i2cr &= ~(I2C_I2CR_MSTA | I2C_I2CR_IIEN);
            *desc->data++ = base->i2dr; /* Read data from I2DR and store */
            goto i2c_done;
        }
    }

i2c_stop:
    /* Generate STOP signal */
    base->i2cr &= ~(I2C_I2CR_MSTA | I2C_I2CR_IIEN);
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
    struct i2c_map * const base = desc->base;
    int count = desc->data_count;
    uint16_t i2cr;

    /* Set speed */
    base->ifdr = node->ifdr;

    /* Enable module */
    base->i2cr = I2C_I2CR_IEN;

    /* Enable Interrupt, Master */
    i2cr = I2C_I2CR_IEN | I2C_I2CR_IIEN | I2C_I2CR_MTX;

    if ((desc->addr & 0x1) && desc->data_count < 2)
    {
        /* Receiving less than two bytes - disable ACK generation */
        i2cr |= I2C_I2CR_TXAK;
    }

    /* Set config */
    base->i2cr = i2cr;

    /* Generate START */
    base->i2cr |= I2C_I2CR_MSTA;

    /* Address slave (first byte sent) and begin session. */
    base->i2dr = desc->addr;

    /* Wait for transfer to complete */
    count = (wakeup_wait(&desc->w, HZ) != OBJ_WAIT_SUCCEEDED) ?
                -1 : (count - desc->data_count);
    
    /* Disable module - generate STOP if timeout */
    base->i2cr = 0;

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

    desc->addr_count = 0; /* To eliminate zeroing elsewhere */

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
        imx31_clkctl_module_clock_gating(desc->cg, CGM_ON_ALL);
        mutex_init(&desc->m);
        wakeup_init(&desc->w);
        desc->base->i2cr = 0;
        imx31_clkctl_module_clock_gating(desc->cg, CGM_OFF);
    }

#if 0
    /* Pad config set up by OF bootloader doesn't agree with manual but
     * TX works at the moment - probably would't do this here either */
    uint32_t reg = SW_PAD_CTL_CSI_PIXCLK_I2C_CLK_I2C_DAT;
    reg &= ~0xfffff;
    reg |= (1 << 19) | (3 << 17) | (1 << 15) | (1 << 14) |
           (1 << 13) | (0 << 11);

    reg |= (1 << 9) | (3 << 7) | (1 << 5) | (1 << 4) |
           (1 << 3) | (0 << 1);

    SW_PAD_CTL_CSI_PIXCLK_I2C_CLK_I2C_DAT = reg;
#endif
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
            imx31_clkctl_module_clock_gating(desc->cg, CGM_ON_ALL);
            avic_enable_int(desc->ints, IRQ, 7, desc->handler);
        }
    }
    else
    {
        if (desc->enable > 0 && --desc->enable == 0)
        {
            /* Last enable */
            avic_disable_int(desc->ints);
            imx31_clkctl_module_clock_gating(desc->cg, CGM_OFF);
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
