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
#include "system.h"
#include "kernel.h"
#include "ssp-imx233.h"
#include "clkctrl-imx233.h"
#include "pinctrl-imx233.h"
#include "dma-imx233.h"

/* Used for DMA */
struct ssp_dma_command_t
{
    struct apb_dma_command_t dma;
    /* PIO words */
    uint32_t ctrl0;
    uint32_t cmd0;
    uint32_t cmd1;
};

static int ssp_in_use = 0;
static struct mutex ssp_mutex[2];
static struct semaphore ssp_sema[2];
static struct ssp_dma_command_t ssp_dma_cmd[2];

void INT_SSP(int ssp)
{
    /* reset dma channel on error */
    if(imx233_dma_is_channel_error_irq(APB_SSP(ssp)))
        imx233_dma_reset_channel(APB_SSP(ssp));
    /* clear irq flags */
    imx233_dma_clear_channel_interrupt(APB_SSP(ssp));
    semaphore_release(&ssp_sema[ssp - 1]);
}

void INT_SSP1_DMA(void)
{
    INT_SSP(1);
}

void INT_SSP2_DMA(void)
{
    INT_SSP(2);
}

void INT_SSP1_ERROR(void)
{
    panicf("ssp1 error");
}

void INT_SSP2_ERROR(void)
{
    panicf("ssp2 error");
}

void imx233_ssp_init(void)
{
    /* power down */
    __REG_SET(HW_SSP_CTRL0(1)) = __BLOCK_CLKGATE;
    __REG_SET(HW_SSP_CTRL0(2)) = __BLOCK_CLKGATE;

    ssp_in_use = 0;
    semaphore_init(&ssp_sema[0], 1, 0);
    semaphore_init(&ssp_sema[1], 1, 0);
    mutex_init(&ssp_mutex[0]);
    mutex_init(&ssp_mutex[1]);
}

void imx233_ssp_start(int ssp)
{
    /* Gate block */
    __REG_CLR(HW_SSP_CTRL0(ssp)) = __BLOCK_CLKGATE;
    while(HW_SSP_CTRL0(ssp) & __BLOCK_CLKGATE);
    /* Gate dma channel */
    imx233_dma_clkgate_channel(APB_SSP(ssp), true);
    /* If first block to start, start SSP clock */
    if(ssp_in_use == 0)
    {
        /* fracdiv = 18 => clk_io = pll = 480Mhz
         * intdiv = 4 => clk_ssp = 120Mhz */
        imx233_set_fractional_divisor(CLK_IO, 18);
        imx233_enable_clock(CLK_SSP, false);
        imx233_set_clock_divisor(CLK_SSP, 4);
        imx233_set_bypass_pll(CLK_SSP, false); /* use IO */
        imx233_enable_clock(CLK_SSP, true);
    }
    ssp_in_use++;
}

void imx233_ssp_stop(int ssp)
{
    /* Gate off */
    __REG_SET(HW_SSP_CTRL0(ssp)) = __BLOCK_CLKGATE;
    /* Gate off dma */
    imx233_dma_clkgate_channel(APB_SSP(ssp), false);
    /* If last block to stop, stop SSP clock */
    ssp_in_use--;
    if(ssp_in_use == 0)
    {
        imx233_enable_clock(CLK_SSP, false);
        imx233_set_fractional_divisor(CLK_IO, 0);
    }
}

void imx233_ssp_softreset(int ssp)
{
    imx233_reset_block(&HW_SSP_CTRL0(ssp));
}

void imx233_ssp_set_timings(int ssp, int divide, int rate)
{
    __REG_CLR(HW_SSP_TIMING(ssp)) =
        HW_SSP_TIMING__CLOCK_DIVIDE_BM | HW_SSP_TIMING__CLOCK_RATE_BM;
    __REG_SET(HW_SSP_TIMING(ssp)) =
        divide << HW_SSP_TIMING__CLOCK_DIVIDE_BP | rate;
}

void imx233_ssp_set_timeout(int ssp, int timeout)
{
    __REG_CLR(HW_SSP_TIMING(ssp)) = HW_SSP_TIMING__CLOCK_TIMEOUT_BM;
    __REG_SET(HW_SSP_TIMING(ssp)) =
        timeout << HW_SSP_TIMING__CLOCK_TIMEOUT_BP;
}

#if 0
static void setup_ssp_sd_pins(int ssp)
{
    imx233_set_pin_function(1, 29, PINCTRL_FUNCTION_GPIO);
    imx233_enable_gpio_output(1, 29, true);
    imx233_set_gpio_output(1, 29, false);

    if(ssp == 1)
    {
        /* SSP_SCK: drive 8mA */
        imx233_set_pin_drive_strength(2, 6, PINCTRL_DRIVE_8mA);
        /* SSP_{SCK,DATA{3,2,1,0},DETECT,CMD} */
        imx233_set_pin_function(2, 6, PINCTRL_FUNCTION_MAIN);
        imx233_set_pin_function(2, 5, PINCTRL_FUNCTION_MAIN);
        imx233_set_pin_function(2, 4, PINCTRL_FUNCTION_MAIN);
        imx233_set_pin_function(2, 3, PINCTRL_FUNCTION_MAIN);
        imx233_set_pin_function(2, 2, PINCTRL_FUNCTION_MAIN);
        imx233_set_pin_function(2, 1, PINCTRL_FUNCTION_MAIN);
        imx233_set_pin_function(2, 0, PINCTRL_FUNCTION_MAIN);
        /* SSP_CMD: pullup */
        imx233_enable_pin_pullup(2, 0, true);
        imx233_enable_pin_pullup(2, 2, true);
        imx233_enable_pin_pullup(2, 3, true);
        imx233_enable_pin_pullup(2, 4, true);
        imx233_enable_pin_pullup(2, 5, true);
    }
    else
    {
        
    }
}
#endif

void imx233_ssp_setup_ssp2_sd_mmc_pins(bool enable_pullups, unsigned bus_width,
    unsigned drive_strength)
{
    /* SSP_{CMD,SCK} */
    imx233_set_pin_drive_strength(0, 20, drive_strength);
    imx233_set_pin_drive_strength(0, 24, drive_strength);
    imx233_set_pin_function(0, 20, PINCTRL_FUNCTION_ALT2);
    imx233_set_pin_function(0, 24, PINCTRL_FUNCTION_ALT2);
    imx233_enable_pin_pullup(0, 20, enable_pullups);
    /* SSP_DATA{0-7}*/
    imx233_set_pin_drive_strength(0, 0, drive_strength);
    imx233_set_pin_function(0, 0, PINCTRL_FUNCTION_ALT2);
    imx233_enable_pin_pullup(0, 0, enable_pullups);
    
    if(bus_width >= 4)
    {
        imx233_set_pin_drive_strength(0, 1, drive_strength);
        imx233_set_pin_drive_strength(0, 2, drive_strength);
        imx233_set_pin_drive_strength(0, 3, drive_strength);
        imx233_set_pin_function(0, 1, PINCTRL_FUNCTION_ALT2);
        imx233_set_pin_function(0, 2, PINCTRL_FUNCTION_ALT2);
        imx233_set_pin_function(0, 3, PINCTRL_FUNCTION_ALT2);
        imx233_enable_pin_pullup(0, 1, enable_pullups);
        imx233_enable_pin_pullup(0, 2, enable_pullups);
        imx233_enable_pin_pullup(0, 3, enable_pullups);
    }
    if(bus_width >= 8)
    {
        imx233_set_pin_drive_strength(0, 4, drive_strength);
        imx233_set_pin_drive_strength(0, 5, drive_strength);
        imx233_set_pin_drive_strength(0, 6, drive_strength);
        imx233_set_pin_drive_strength(0, 7, drive_strength);
        imx233_set_pin_function(0, 4, PINCTRL_FUNCTION_ALT2);
        imx233_set_pin_function(0, 5, PINCTRL_FUNCTION_ALT2);
        imx233_set_pin_function(0, 6, PINCTRL_FUNCTION_ALT2);
        imx233_set_pin_function(0, 7, PINCTRL_FUNCTION_ALT2);
        imx233_enable_pin_pullup(0, 4, enable_pullups);
        imx233_enable_pin_pullup(0, 5, enable_pullups);
        imx233_enable_pin_pullup(0, 6, enable_pullups);
        imx233_enable_pin_pullup(0, 7, enable_pullups);
    }

    imx233_enable_gpio_output_mask(0, 0x11000ff, false);
    imx233_set_gpio_output_mask(0, 0x11000ff, false);
}

void imx233_ssp_set_mode(int ssp, unsigned mode)
{
    switch(mode)
    {
        case HW_SSP_CTRL1__SSP_MODE__SD_MMC:
            /* clear mode and word length */
            __REG_CLR(HW_SSP_CTRL1(ssp)) =
                HW_SSP_CTRL1__SSP_MODE_BM | HW_SSP_CTRL1__WORD_LENGTH_BM;
            /* set mode, set word length to 8-bit, polarity and enable dma */
            __REG_SET(HW_SSP_CTRL1(ssp)) = mode |
                HW_SSP_CTRL1__WORD_LENGTH__EIGHT_BITS << HW_SSP_CTRL1__WORD_LENGTH_BP |
                HW_SSP_CTRL1__POLARITY | HW_SSP_CTRL1__DMA_ENABLE;
            break;
        default: return;
    }
}

enum imx233_ssp_error_t imx233_ssp_sd_mmc_transfer(int ssp, uint8_t cmd, uint32_t cmd_arg,
    enum imx233_ssp_resp_t resp, void *buffer, int xfer_size, bool read, uint32_t *resp_ptr)
{
    mutex_lock(&ssp_mutex[ssp - 1]);
    /* Enable all interrupts */
    imx233_enable_interrupt(INT_SRC_SSP_DMA(ssp), true);
    imx233_dma_enable_channel_interrupt(APB_SSP(ssp), true);
    /* Assume only one block so ignore block_count and block_size */
    ssp_dma_cmd[ssp - 1].cmd0 = cmd | HW_SSP_CMD0__APPEND_8CYC;
    ssp_dma_cmd[ssp - 1].cmd1 = cmd_arg;
    /* setup all flags and run */
    ssp_dma_cmd[ssp - 1].ctrl0 = xfer_size | HW_SSP_CTRL0__ENABLE |
        HW_SSP_CTRL0__IGNORE_CRC | 
        (resp != SSP_NO_RESP ? HW_SSP_CTRL0__GET_RESP | HW_SSP_CTRL0__WAIT_FOR_IRQ : 0) |
        (resp == SSP_LONG_RESP ? HW_SSP_CTRL0__LONG_RESP : 0) |
        HW_SSP_CTRL0__BUS_WIDTH__ONE_BIT << HW_SSP_CTRL0__BUS_WIDTH_BP |
        (buffer ? HW_SSP_CTRL0__DATA_XFER : 0) |
        (read ? HW_SSP_CTRL0__READ : 0);
    /* setup the dma parameters */
    ssp_dma_cmd[ssp - 1].dma.buffer = buffer;
    ssp_dma_cmd[ssp - 1].dma.next = NULL;
    ssp_dma_cmd[ssp - 1].dma.cmd =
        (buffer == NULL ? HW_APB_CHx_CMD__COMMAND__NO_XFER :
         read ? HW_APB_CHx_CMD__COMMAND__WRITE : HW_APB_CHx_CMD__COMMAND__READ) |
        HW_APB_CHx_CMD__IRQONCMPLT | HW_APB_CHx_CMD__SEMAPHORE |
        HW_APB_CHx_CMD__WAIT4ENDCMD | HW_APB_CHx_CMD__HALTONTERMINATE |
        (3 << HW_APB_CHx_CMD__CMDWORDS_BP) |
        (xfer_size << HW_APB_CHx_CMD__XFER_COUNT_BP);

    imx233_dma_start_command(APB_SSP(ssp), &ssp_dma_cmd[ssp - 1].dma);

    /* the SSP hardware already has a timeout but we never know; 1 sec is a maximum
     * for all operations */
    enum imx233_ssp_error_t ret;
    
    if(semaphore_wait(&ssp_sema[ssp - 1], HZ) == OBJ_WAIT_TIMEDOUT)
        ret = SSP_TIMEOUT;
    else if((HW_SSP_CTRL1(ssp) & HW_SSP_CTRL1__ALL_IRQ) == 0)
        ret =  SSP_SUCCESS;
    else if(HW_SSP_CTRL1(ssp) & (HW_SSP_CTRL1__RESP_TIMEOUT_IRQ |
            HW_SSP_CTRL1__DATA_TIMEOUT_IRQ | HW_SSP_CTRL1__RECV_TIMEOUT_IRQ))
        ret = SSP_TIMEOUT;
    else
        ret = SSP_ERROR;

    if(ret == SSP_SUCCESS && resp_ptr != NULL)
    {
        if(resp != SSP_NO_RESP)
            *resp_ptr++ = HW_SSP_SDRESP0(ssp);
        if(resp == SSP_LONG_RESP)
        {
            *resp_ptr++ = HW_SSP_SDRESP1(ssp);
            *resp_ptr++ = HW_SSP_SDRESP2(ssp);
            *resp_ptr++ = HW_SSP_SDRESP3(ssp);
        }
    }
    mutex_unlock(&ssp_mutex[ssp - 1]);
    return ret;
}

void imx233_ssp_sd_mmc_power_up_sequence(int ssp)
{
    __REG_CLR(HW_SSP_CMD0(ssp)) = HW_SSP_CMD0__SLOW_CLKING_EN;
    __REG_SET(HW_SSP_CMD0(ssp)) = HW_SSP_CMD0__CONT_CLKING_EN;
    mdelay(1);
    __REG_CLR(HW_SSP_CMD0(ssp)) = HW_SSP_CMD0__CONT_CLKING_EN;
}
