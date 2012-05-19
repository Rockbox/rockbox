/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by amaury Pouly
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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
#include "kernel.h"
#include "dma-imx233.h"
#include "i2c-imx233.h"
#include "pinctrl-imx233.h"

/* Used for DMA */
struct i2c_dma_command_t
{
    struct apb_dma_command_t dma;
    /* PIO words */
    uint32_t ctrl0;
    /* padded to next multiple of cache line size (32 bytes) */
    uint32_t pad[4];
} __attribute__((packed)) CACHEALIGN_ATTR;

__ENSURE_STRUCT_CACHE_FRIENDLY(struct i2c_dma_command_t)

#define I2C_NR_STAGES   4
/* Current transfer */
static int i2c_nr_stages;
static struct i2c_dma_command_t i2c_stage[I2C_NR_STAGES];
static struct mutex i2c_mutex;
static struct semaphore i2c_sema;

void INT_I2C_DMA(void)
{
    /* reset dma channel on error */
    if(imx233_dma_is_channel_error_irq(APB_I2C))
        imx233_dma_reset_channel(APB_I2C);
    /* clear irq flags */
    imx233_dma_clear_channel_interrupt(APB_I2C);
    semaphore_release(&i2c_sema);
}

void imx233_i2c_init(void)
{
    //imx233_reset_block(&HW_I2C_CTRL0);
    __REG_SET(HW_I2C_CTRL0) = __BLOCK_SFTRST;
    /* setup pins (must be done when shutdown) */
    imx233_pinctrl_acquire_pin(0, 30, "i2c");
    imx233_pinctrl_acquire_pin(0, 31, "i2c");
    imx233_set_pin_function(0, 30, PINCTRL_FUNCTION_MAIN);
    imx233_set_pin_function(0, 31, PINCTRL_FUNCTION_MAIN);
    /* clear softreset */
    __REG_CLR(HW_I2C_CTRL0) = __BLOCK_SFTRST | __BLOCK_CLKGATE;
    /* Errata:
     * When RETAIN_CLOCK is set, the ninth clock pulse (ACK) is not generated. However, the SDA
     * line is read at the proper timing interval. If RETAIN_CLOCK is cleared, the ninth clock pulse is
     * generated.
     * HW_I2C_CTRL1[ACK_MODE] has default value of 0. It should be set to 1 to enable the fix for
     * this issue.
     */
    __REG_SET(HW_I2C_CTRL1) = HW_I2C_CTRL1__ACK_MODE;
    __REG_SET(HW_I2C_CTRL0) = __BLOCK_CLKGATE;
    /* Fast-mode @ 400K */
    HW_I2C_TIMING0 = 0x000F0007; /* tHIGH=0.6us, read at 0.3us */
    HW_I2C_TIMING1 = 0x001F000F; /* tLOW=1.3us, write at 0.6us */
    HW_I2C_TIMING2 = 0x0015000D;
    
    mutex_init(&i2c_mutex);
    semaphore_init(&i2c_sema, 1, 0);
}

void imx233_i2c_begin(void)
{
    mutex_lock(&i2c_mutex);
    /* wakeup */
    __REG_CLR(HW_I2C_CTRL0) = __BLOCK_CLKGATE;
    i2c_nr_stages = 0;
}

enum imx233_i2c_error_t imx233_i2c_add(bool start, bool transmit, void *buffer, unsigned size, bool stop)
{
    if(i2c_nr_stages == I2C_NR_STAGES)
        return I2C_ERROR;
    if(i2c_nr_stages > 0)
    {
        i2c_stage[i2c_nr_stages - 1].dma.next = &i2c_stage[i2c_nr_stages].dma;
        i2c_stage[i2c_nr_stages - 1].dma.cmd |= HW_APB_CHx_CMD__CHAIN;
        if(!start)
            i2c_stage[i2c_nr_stages - 1].ctrl0 |= HW_I2C_CTRL0__RETAIN_CLOCK;
    }
    i2c_stage[i2c_nr_stages].dma.buffer = buffer;
    i2c_stage[i2c_nr_stages].dma.next = NULL;
    i2c_stage[i2c_nr_stages].dma.cmd =
        (transmit ? HW_APB_CHx_CMD__COMMAND__READ : HW_APB_CHx_CMD__COMMAND__WRITE) |
        HW_APB_CHx_CMD__WAIT4ENDCMD |
        1 << HW_APB_CHx_CMD__CMDWORDS_BP |
        size << HW_APB_CHx_CMD__XFER_COUNT_BP;
    /* assume that any read is final (send nak on last) */
    i2c_stage[i2c_nr_stages].ctrl0 = size |
        (transmit ? HW_I2C_CTRL0__TRANSMIT : HW_I2C_CTRL0__SEND_NAK_ON_LAST) |
        (start ? HW_I2C_CTRL0__PRE_SEND_START : 0) |
        (stop ? HW_I2C_CTRL0__POST_SEND_STOP : 0) |
        HW_I2C_CTRL0__MASTER_MODE;
    i2c_nr_stages++;
    return I2C_SUCCESS;
}

enum imx233_i2c_error_t imx233_i2c_end(unsigned timeout)
{
    if(i2c_nr_stages == 0)
        return I2C_ERROR;
    i2c_stage[i2c_nr_stages - 1].dma.cmd |= HW_APB_CHx_CMD__SEMAPHORE | HW_APB_CHx_CMD__IRQONCMPLT;

    __REG_CLR(HW_I2C_CTRL1) = HW_I2C_CTRL1__ALL_IRQ;
    imx233_dma_reset_channel(APB_I2C);
    imx233_icoll_enable_interrupt(INT_SRC_I2C_DMA, true);
    imx233_dma_enable_channel_interrupt(APB_I2C, true);
    imx233_dma_start_command(APB_I2C, &i2c_stage[0].dma);

    enum imx233_i2c_error_t ret;
    if(semaphore_wait(&i2c_sema, timeout) == OBJ_WAIT_TIMEDOUT)
    {
        imx233_dma_reset_channel(APB_I2C);
        ret = I2C_TIMEOUT;
    }
    else if(HW_I2C_CTRL1 & HW_I2C_CTRL1__MASTER_LOSS_IRQ)
        ret = I2C_MASTER_LOSS;
    else if(HW_I2C_CTRL1 & HW_I2C_CTRL1__NO_SLAVE_ACK_IRQ)
        ret= I2C_NO_SLAVE_ACK;
    else if(HW_I2C_CTRL1 & HW_I2C_CTRL1__EARLY_TERM_IRQ)
        ret = I2C_SLAVE_NAK;
    else
        ret = I2C_SUCCESS;
    /* sleep */
    __REG_SET(HW_I2C_CTRL0) = __BLOCK_CLKGATE;
    mutex_unlock(&i2c_mutex);
    return ret;
}

void i2c_init(void)
{
}

int i2c_write(int device, const unsigned char* buf, int count)
{
    uint8_t addr = device;
    imx233_i2c_begin();
    imx233_i2c_add(true, true, &addr, 1, false); /* start + addr */
    imx233_i2c_add(false, true, (void *)buf, count, true); /* data + stop */
    return imx233_i2c_end(TIMEOUT_BLOCK);
}

int i2c_read(int device, unsigned char* buf, int count)
{
    uint8_t addr = device | 1;
    imx233_i2c_begin();
    imx233_i2c_add(true, true, &addr, 1, false); /* start + addr */
    imx233_i2c_add(false, false, buf, count, true); /* data + stop */
    return imx233_i2c_end(TIMEOUT_BLOCK);
}

int i2c_readmem(int device, int address, unsigned char* buf, int count)
{
    uint8_t start[2] = {device, address};
    uint8_t addr_rd = device | 1;
    imx233_i2c_begin();
    imx233_i2c_add(true, true, start, 2, false); /* start + addr + addr */
    imx233_i2c_add(true, true, &addr_rd, 1, false); /* start + addr */
    imx233_i2c_add(false, false, buf, count, true); /* data + stop */
    return imx233_i2c_end(TIMEOUT_BLOCK);
}

int i2c_writemem(int device, int address, const unsigned char* buf, int count)
{
    uint8_t start[2] = {device, address};
    imx233_i2c_begin();
    imx233_i2c_add(true, true, start, 2, false); /* start + addr + addr */
    imx233_i2c_add(false, true, (void *)buf, count, true); /* data + stop */
    return imx233_i2c_end(TIMEOUT_BLOCK);
}
