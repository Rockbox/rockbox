/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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

/* for debug purpose */
#if 0
#define ASSERT_SSP(ssp) if(ssp < 1 || ssp > 2) panicf("ssp=%d in %s", ssp, __func__);
#else
#define ASSERT_SSP(ssp) (void) ssp;
#endif

/* Hack to handle both single and multi devices at once */
#define SSP_SETn(reg, n, field) BF_SETn(reg, n, field)
#define SSP_CLRn(reg, n, field) BF_CLRn(reg, n, field)
#define SSP_RDn(reg, n, field) BF_RDn(reg, n, field)
#define SSP_WRn(reg, n, field, val) BF_WRn(reg, n, field, val)
#define SSP_WRn_V(reg, n, field, val) BF_WRn_V(reg, n, field, val)
#define SSP_REGn(reg, n) HW_##reg(n)

/* Used for DMA */
struct ssp_dma_command_t
{
    struct apb_dma_command_t dma;
    /* PIO words */
    uint32_t ctrl0;
    uint32_t cmd0;
    uint32_t cmd1;
    /* padded to next multiple of cache line size (32 bytes) */
    uint32_t pad[2];
} __attribute__((packed)) CACHEALIGN_ATTR;

__ENSURE_STRUCT_CACHE_FRIENDLY(struct ssp_dma_command_t)

static bool ssp_in_use[IMX233_NR_SSP];
static int ssp_nr_in_use = 0;
static struct mutex ssp_mutex[IMX233_NR_SSP];
static struct semaphore ssp_sema[IMX233_NR_SSP];
static struct ssp_dma_command_t ssp_dma_cmd[IMX233_NR_SSP];
static uint32_t ssp_bus_width[IMX233_NR_SSP];
static unsigned ssp_log_block_size[IMX233_NR_SSP];
static ssp_detect_cb_t ssp_detect_cb[IMX233_NR_SSP];
static bool ssp_detect_invert[IMX233_NR_SSP];

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
    /* power down and init data structures */
    ssp_nr_in_use = 0;
    for(int i = 0; i < IMX233_NR_SSP; i++)
    {
        SSP_SETn(SSP_CTRL0, 1 + i, CLKGATE);
        semaphore_init(&ssp_sema[i], 1, 0);
        mutex_init(&ssp_mutex[i]);
        ssp_bus_width[i] = BV_SSP_CTRL0_BUS_WIDTH__ONE_BIT;
    }
}

void imx233_ssp_start(int ssp)
{
    ASSERT_SSP(ssp)
    if(ssp_in_use[ssp - 1])
        return;
    ssp_in_use[ssp - 1] = true;
    /* Gate block */
    imx233_reset_block(&SSP_REGn(SSP_CTRL0, ssp));
    /* Gate dma channel */
    imx233_dma_clkgate_channel(APB_SSP(ssp), true);
    /* If first block to start, start SSP clock */
    if(ssp_nr_in_use == 0)
    {
        /** 2.3.1: the clk_ssp maximum frequency is 102.858 MHz */
        /* fracdiv = 18 => clk_io = pll = 480Mhz
         * intdiv = 5 => clk_ssp = 96Mhz */
        imx233_clkctrl_set_fractional_divisor(CLK_IO, 18);
        imx233_clkctrl_enable_clock(CLK_SSP, false);
        imx233_clkctrl_set_clock_divisor(CLK_SSP, 5);
        imx233_clkctrl_set_bypass_pll(CLK_SSP, false); /* use IO */
        imx233_clkctrl_enable_clock(CLK_SSP, true);
    }
    ssp_nr_in_use++;
}

void imx233_ssp_stop(int ssp)
{
    ASSERT_SSP(ssp)
    if(!ssp_in_use[ssp - 1])
        return;
    ssp_in_use[ssp - 1] = false;
    /* Gate off */
    SSP_SETn(SSP_CTRL0, ssp, CLKGATE);
    /* Gate off dma */
    imx233_dma_clkgate_channel(APB_SSP(ssp), false);
    /* If last block to stop, stop SSP clock */
    ssp_nr_in_use--;
    if(ssp_nr_in_use == 0)
    {
        imx233_clkctrl_enable_clock(CLK_SSP, false);
        imx233_clkctrl_set_fractional_divisor(CLK_IO, 0);
    }
}

void imx233_ssp_softreset(int ssp)
{
    ASSERT_SSP(ssp)
    imx233_reset_block(&HW_SSP_CTRL0(ssp));
}

void imx233_ssp_set_timings(int ssp, int divide, int rate, int timeout)
{
    ASSERT_SSP(ssp)
    SSP_REGn(SSP_TIMING, ssp) = BF_OR3(SSP_TIMING, CLOCK_DIVIDE(divide),
        CLOCK_RATE(rate), TIMEOUT(timeout));
}

void imx233_ssp_setup_ssp1_sd_mmc_pins(bool enable_pullups, unsigned bus_width,
    unsigned drive_strength, bool use_alt)
{
    /* SSP_{CMD,SCK} */
    imx233_pinctrl_set_drive(2, 0, drive_strength);
    imx233_pinctrl_set_drive(2, 6, drive_strength);
    imx233_pinctrl_acquire(2, 0, "ssp1 cmd");
    imx233_pinctrl_acquire(2, 6, "ssp1 sck");
    imx233_pinctrl_set_function(2, 0, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_set_function(2, 6, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_enable_pullup(2, 0, enable_pullups);
    /* SSP_DATA{0-3} */
    for(unsigned i = 0; i < MIN(bus_width, 4); i++)
    {
        imx233_pinctrl_acquire(2, 2 + i, "ssp1 data");
        imx233_pinctrl_set_drive(2, 2 + i, drive_strength);
        imx233_pinctrl_set_function(2, 2 + i, PINCTRL_FUNCTION_MAIN);
        imx233_pinctrl_enable_pullup(2, 2 + i, enable_pullups);
    }

    /* SSP_DATA{4-7} */
    for(unsigned i = 4; i < bus_width; i++)
    {
        if(use_alt)
        {
            imx233_pinctrl_acquire(0, 22 + i, "ssp1 data");
            imx233_pinctrl_set_drive(0, 22 + i, drive_strength);
            imx233_pinctrl_set_function(0, 22 + i, PINCTRL_FUNCTION_ALT2);
            imx233_pinctrl_enable_pullup(0, 22 + i, enable_pullups);
        }
        else
        {
            imx233_pinctrl_acquire(0, 4 + i, "ssp1 data");
            imx233_pinctrl_set_drive(0, 4 + i, drive_strength);
            imx233_pinctrl_set_function(0, 4 + i, PINCTRL_FUNCTION_ALT2);
            imx233_pinctrl_enable_pullup(0, 4 + i, enable_pullups);
        }
    }
}

void imx233_ssp_setup_ssp2_sd_mmc_pins(bool enable_pullups, unsigned bus_width,
    unsigned drive_strength)
{
    /* SSP_{CMD,SCK} */
    imx233_pinctrl_acquire(0, 20, "ssp2 cmd");
    imx233_pinctrl_acquire(0, 24, "ssp2 sck");
    imx233_pinctrl_set_drive(0, 20, drive_strength);
    imx233_pinctrl_set_drive(0, 24, drive_strength);
    imx233_pinctrl_set_function(0, 20, PINCTRL_FUNCTION_ALT2);
    imx233_pinctrl_set_function(0, 24, PINCTRL_FUNCTION_ALT2);
    imx233_pinctrl_enable_pullup(0, 20, enable_pullups);
    /* SSP_DATA{0-7}*/
    for(unsigned i = 0; i < bus_width; i++)
    {
        imx233_pinctrl_acquire(0, i, "ssp2 data");
        imx233_pinctrl_set_drive(0, i, drive_strength);
        imx233_pinctrl_set_function(0, i, PINCTRL_FUNCTION_ALT2);
        imx233_pinctrl_enable_pullup(0, i, enable_pullups);
        imx233_pinctrl_enable_gpio(0, i, false);
        imx233_pinctrl_set_gpio(0, i, false);
    }
}

void imx233_ssp_set_mode(int ssp, unsigned mode)
{
    ASSERT_SSP(ssp)
    /* set mode */
    SSP_WRn(SSP_CTRL1, ssp, SSP_MODE, mode);
    /* set mode specific settings */
    switch(mode)
    {
        case BV_SSP_CTRL1_SSP_MODE__SD_MMC:
            SSP_WRn_V(SSP_CTRL1, ssp, WORD_LENGTH, EIGHT_BITS);
            SSP_SETn(SSP_CTRL1, ssp, POLARITY);
            SSP_SETn(SSP_CTRL1, ssp, DMA_ENABLE);
            break;
        default: return;
    }
}

void imx233_ssp_set_bus_width(int ssp, unsigned width)
{
    ASSERT_SSP(ssp)
    switch(width)
    {
        case 1: ssp_bus_width[ssp - 1] = BV_SSP_CTRL0_BUS_WIDTH__ONE_BIT; break;
        case 4: ssp_bus_width[ssp - 1] = BV_SSP_CTRL0_BUS_WIDTH__FOUR_BIT; break;
        case 8: ssp_bus_width[ssp - 1] = BV_SSP_CTRL0_BUS_WIDTH__EIGHT_BIT; break;
    }
}

void imx233_ssp_set_block_size(int ssp, unsigned log_block_size)
{
    ASSERT_SSP(ssp)
    ssp_log_block_size[ssp - 1] = log_block_size;
}

enum imx233_ssp_error_t imx233_ssp_sd_mmc_transfer(int ssp, uint8_t cmd,
    uint32_t cmd_arg, enum imx233_ssp_resp_t resp, void *buffer, unsigned block_count,
    bool wait4irq, bool read, uint32_t *resp_ptr)
{
    ASSERT_SSP(ssp)
    mutex_lock(&ssp_mutex[ssp - 1]);
    /* Enable all interrupts */
    imx233_icoll_enable_interrupt(INT_SRC_SSP_DMA(ssp), true);
    imx233_dma_enable_channel_interrupt(APB_SSP(ssp), true);

    unsigned xfer_size = block_count * (1 << ssp_log_block_size[ssp - 1]);
    
    ssp_dma_cmd[ssp - 1].cmd0 = BF_OR4(SSP_CMD0, CMD(cmd), APPEND_8CYC(1),
        BLOCK_SIZE(ssp_log_block_size[ssp - 1]), BLOCK_COUNT(block_count - 1));
    ssp_dma_cmd[ssp - 1].cmd1 = cmd_arg;
    /* setup all flags and run */
    ssp_dma_cmd[ssp - 1].ctrl0 = BF_OR9(SSP_CTRL0, XFER_COUNT(xfer_size),
        ENABLE(1), IGNORE_CRC(buffer == NULL), WAIT_FOR_IRQ(wait4irq),
        GET_RESP(resp != SSP_NO_RESP), LONG_RESP(resp == SSP_LONG_RESP),
        BUS_WIDTH(ssp_bus_width[ssp - 1]), DATA_XFER(buffer != NULL),
        READ(read));
    /* setup the dma parameters */
    ssp_dma_cmd[ssp - 1].dma.buffer = buffer;
    ssp_dma_cmd[ssp - 1].dma.next = NULL;
    ssp_dma_cmd[ssp - 1].dma.cmd = BF_OR6(APB_CHx_CMD,
        COMMAND(buffer == NULL ? BV_APB_CHx_CMD_COMMAND__NO_XFER :
        read ? BV_APB_CHx_CMD_COMMAND__WRITE : BV_APB_CHx_CMD_COMMAND__READ),
        IRQONCMPLT(1), SEMAPHORE(1), WAIT4ENDCMD(1), CMDWORDS(3), XFER_COUNT(xfer_size));

    SSP_CLRn(SSP_CTRL1, ssp, ALL_IRQ);

    imx233_dma_reset_channel(APB_SSP(ssp));
    imx233_dma_start_command(APB_SSP(ssp), &ssp_dma_cmd[ssp - 1].dma);

    /* the SSP hardware already has a timeout but we never know; 1 sec is a maximum
     * for all operations */
    enum imx233_ssp_error_t ret;
    
    if(semaphore_wait(&ssp_sema[ssp - 1], HZ) == OBJ_WAIT_TIMEDOUT)
    {
        imx233_dma_reset_channel(APB_SSP(ssp));
        ret = SSP_TIMEOUT;
    }
    else if((SSP_REGn(SSP_CTRL1, ssp) & BM_SSP_CTRL1_ALL_IRQ) == 0)
        ret =  SSP_SUCCESS;
    else if((SSP_REGn(SSP_CTRL1, ssp) & BM_SSP_CTRL1_TIMEOUT_IRQ))
        ret = SSP_TIMEOUT;
    else
        ret = SSP_ERROR;

    if(resp_ptr != NULL)
    {
        if(resp != SSP_NO_RESP)
            *resp_ptr++ = SSP_REGn(SSP_SDRESP0, ssp);
        if(resp == SSP_LONG_RESP)
        {
            *resp_ptr++ = SSP_REGn(SSP_SDRESP1, ssp);
            *resp_ptr++ = SSP_REGn(SSP_SDRESP2, ssp);
            *resp_ptr++ = SSP_REGn(SSP_SDRESP3, ssp);
        }
    }
    mutex_unlock(&ssp_mutex[ssp - 1]);
    return ret;
}

void imx233_ssp_sd_mmc_power_up_sequence(int ssp)
{
    ASSERT_SSP(ssp)
    SSP_CLRn(SSP_CMD0, ssp, SLOW_CLKING_EN);
    SSP_SETn(SSP_CMD0, ssp, CONT_CLKING_EN);
    mdelay(1);
    SSP_CLRn(SSP_CMD0, ssp, CONT_CLKING_EN);
}

static int ssp_detect_oneshot_callback(int ssp)
{
    ASSERT_SSP(ssp)
    if(ssp_detect_cb[ssp - 1])
        ssp_detect_cb[ssp - 1](ssp);

    return 0;
}

static int ssp1_detect_oneshot_callback(struct timeout *tmo)
{
    (void) tmo;
    return ssp_detect_oneshot_callback(1);
}

static int ssp2_detect_oneshot_callback(struct timeout *tmo)
{
    (void) tmo;
    return ssp_detect_oneshot_callback(2);
}

static void detect_irq(int bank, int pin)
{
    static struct timeout ssp1_detect_oneshot;
    static struct timeout ssp2_detect_oneshot;
    if(bank == 2 && pin == 1)
        timeout_register(&ssp1_detect_oneshot, ssp1_detect_oneshot_callback, (3*HZ/10), 0);
    else if(bank == 0 && pin == 19)
        timeout_register(&ssp2_detect_oneshot, ssp2_detect_oneshot_callback, (3*HZ/10), 0);
}

void imx233_ssp_sdmmc_setup_detect(int ssp, bool enable, ssp_detect_cb_t fn,
    bool first_time, bool invert)
{
    ASSERT_SSP(ssp)
    int bank = ssp == 1 ? 2 : 0;
    int pin = ssp == 1 ? 1 : 19;
    ssp_detect_cb[ssp - 1] = fn;
    ssp_detect_invert[ssp - 1] = invert;
    if(enable)
    {
        imx233_pinctrl_acquire(bank, pin, ssp == 1 ? "ssp1 detect" : "ssp2 detect");
        imx233_pinctrl_set_function(bank, pin, PINCTRL_FUNCTION_GPIO);
        imx233_pinctrl_enable_gpio(bank, pin, false);
    }
    if(first_time && imx233_ssp_sdmmc_detect(ssp))
        detect_irq(bank, pin);
    imx233_pinctrl_setup_irq(bank, pin, enable, true, !imx233_ssp_sdmmc_detect_raw(ssp), detect_irq);
}

bool imx233_ssp_sdmmc_is_detect_inverted(int ssp)
{
    ASSERT_SSP(ssp)
    return ssp_detect_invert[ssp - 1];
}

bool imx233_ssp_sdmmc_detect_raw(int ssp)
{
    ASSERT_SSP(ssp)
    return SSP_RDn(SSP_STATUS, ssp, CARD_DETECT);
}

bool imx233_ssp_sdmmc_detect(int ssp)
{
    ASSERT_SSP(ssp)
    return imx233_ssp_sdmmc_detect_raw(ssp) != ssp_detect_invert[ssp - 1];
}
