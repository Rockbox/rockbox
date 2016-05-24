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

#include "regs/ssp.h"

#if IMX233_SUBTARGET < 3700
#define IMX233_NR_SSP  1
#else
#define IMX233_NR_SSP  2
#endif

/* ssp can value 1 or 2 */
#if IMX233_NR_SSP >= 2
#define __SSP_SELECT(ssp, ssp1, ssp2) ((ssp) == 1 ? (ssp1) : (ssp2))
#else
#define __SSP_SELECT(ssp, ssp1, ssp2) (ssp1)
#endif

#define INT_SRC_SSP_DMA(ssp)    __SSP_SELECT(ssp, INT_SRC_SSP1_DMA, INT_SRC_SSP2_DMA)
#define INT_SRC_SSP_ERROR(ssp)  __SSP_SELECT(ssp, INT_SRC_SSP1_ERROR, INT_SRC_SSP2_ERROR)

#if IMX233_SUBTARGET < 3700
#define ALL_IRQ \
    SDIO_IRQ, RESP_ERR_IRQ, RESP_TIMEOUT_IRQ, DATA_TIMEOUT_IRQ, \
        DATA_CRC_IRQ, RECV_TIMEOUT_IRQ, RECV_OVRFLW_IRQ
#define ALL_IRQ_EN \
    SDIO_IRQ_EN, RESP_ERR_IRQ_EN, RESP_TIMEOUT_IRQ_EN, DATA_TIMEOUT_IRQ_EN, \
        DATA_CRC_IRQ_EN, RECV_TIMEOUT_IRQ_EN, RECV_OVRFLW_IRQ_EN
#else
#define ALL_IRQ \
    SDIO_IRQ, RESP_ERR_IRQ, RESP_TIMEOUT_IRQ, DATA_TIMEOUT_IRQ, \
        DATA_CRC_IRQ, FIFO_UNDERRUN_IRQ, RECV_TIMEOUT_IRQ, FIFO_OVERRUN_IRQ
#define ALL_IRQ_EN \
    SDIO_IRQ_EN, RESP_ERR_IRQ_EN, RESP_TIMEOUT_IRQ_EN, DATA_TIMEOUT_IRQ_EN, \
        DATA_CRC_IRQ_EN, FIFO_UNDERRUN_EN, RECV_TIMEOUT_IRQ_EN, FIFO_OVERRUN_IRQ_EN
#endif

#define TIMEOUT_IRQ \
    RESP_TIMEOUT_IRQ, DATA_TIMEOUT_IRQ, RECV_TIMEOUT_IRQ

/* for debug purpose */
#if 0
#define ASSERT_SSP(ssp) if(ssp < 1 || ssp > 2) panicf("ssp=%d in %s", ssp, __func__);
#else
#define ASSERT_SSP(ssp) (void) ssp;
#endif

/* Hack to handle both single and multi devices at once */
#if IMX233_SUBTARGET < 3700
#define SSP_SETn(reg, n, field) BF_SET(reg, field)
#define SSP_CLRn(reg, n, field) BF_CLR(reg, field)
#define SSP_RDn(reg, n, field) BF_RD(reg, field)
#define SSP_WRn(reg, n, field, val) BF_WR(reg, field(val))
#define SSP_REGn(reg, n) HW_##reg
#else
#define SSP_SETn(reg, n, field) BF_SET(reg(n), field)
#define SSP_CLRn(reg, n, field) BF_CLR(reg(n), field)
#define SSP_RDn(reg, n, field) BF_RD(reg(n), field)
#define SSP_WRn(reg, n, field, val) BF_WR(reg(n), field(val))
#define SSP_REGn(reg, n) HW_##reg(n)
#endif

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

void INT_SSP(int ssp, bool err)
{
    /* reset dma channel on error */
    if(imx233_dma_is_channel_error_irq(APB_SSP(ssp)) || err)
        imx233_dma_reset_channel(APB_SSP(ssp));
    /* clear irq flags */
    imx233_dma_clear_channel_interrupt(APB_SSP(ssp));
    SSP_CLRn(SSP_CTRL1, ssp, ALL_IRQ_EN);
    semaphore_release(&ssp_sema[ssp - 1]);
}

void INT_SSP1_DMA(void)
{
    INT_SSP(1, false);
}

void INT_SSP2_DMA(void)
{
    INT_SSP(2, false);
}

void INT_SSP1_ERROR(void)
{
    INT_SSP(1, true);
}

void INT_SSP2_ERROR(void)
{
    INT_SSP(2, true);
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

static void start_ssp_clock(void)
{
    /* If first block to start, start SSP clock */
    if(ssp_nr_in_use == 0)
    {
        /** 2.3.1: the clk_ssp maximum frequency is 102.858 MHz */
        /* fracdiv = 18 => clk_io = pll = 480Mhz
         * intdiv = 5 => clk_ssp = 96Mhz */
        imx233_clkctrl_enable(CLK_SSP, false);
        imx233_clkctrl_set_div(CLK_SSP, 5);
#if IMX233_SUBTARGET >= 3700
        imx233_clkctrl_set_bypass(CLK_SSP, false); /* use IO */
#endif
        imx233_clkctrl_enable(CLK_SSP, true);
    }
}

void imx233_ssp_start(int ssp)
{
    ASSERT_SSP(ssp)
    if(ssp_in_use[ssp - 1])
        return;
    ssp_in_use[ssp - 1] = true;
    /* Enable SSP clock (need to start block) */
    start_ssp_clock();
    /* Gate block */
    imx233_ssp_softreset(ssp);
    /* Gate dma channel */
    imx233_dma_clkgate_channel(APB_SSP(ssp), true);
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
        imx233_clkctrl_enable(CLK_SSP, false);
}

void imx233_ssp_softreset(int ssp)
{
    ASSERT_SSP(ssp)
    imx233_reset_block(&SSP_REGn(SSP_CTRL0, ssp));
}

void imx233_ssp_set_timings(int ssp, int divide, int rate, int timeout)
{
    ASSERT_SSP(ssp)
    if(divide == 0 || (divide % 2) == 1)
        panicf("SSP timing divide must be event");
    SSP_REGn(SSP_TIMING, ssp) = BF_OR(SSP_TIMING, CLOCK_DIVIDE(divide),
        CLOCK_RATE(rate), TIMEOUT(timeout));
}

void imx233_ssp_setup_ssp1_sd_mmc_pins(bool enable_pullups, unsigned bus_width, bool use_alt)
{
    (void) use_alt;
    unsigned clk_drive = PINCTRL_DRIVE_8mA;
    unsigned dat_drive = PINCTRL_DRIVE_4mA;
    /* SSP_{CMD,SCK} */
    imx233_pinctrl_setup_vpin(VPIN_SSP1_CMD, "ssp1_cmd", dat_drive, enable_pullups);
    imx233_pinctrl_setup_vpin(VPIN_SSP1_SCK, "ssp1_sck", clk_drive, false);
    /* SSP_DATA{0-3} */
    imx233_pinctrl_setup_vpin(VPIN_SSP1_D0, "ssp1_d0", dat_drive, enable_pullups);
    if(bus_width >= 4)
    {
        imx233_pinctrl_setup_vpin(VPIN_SSP1_D1, "ssp1_d1", dat_drive, enable_pullups);
        imx233_pinctrl_setup_vpin(VPIN_SSP1_D2, "ssp1_d2", dat_drive, enable_pullups);
        imx233_pinctrl_setup_vpin(VPIN_SSP1_D3, "ssp1_d3", dat_drive, enable_pullups);
    }
    if(bus_width >= 8)
    {
#ifdef VPIN_SSP1_D4
        if(use_alt)
        {
#ifdef VPIN_SSP1_D4_ALT
            imx233_pinctrl_setup_vpin(VPIN_SSP1_D4_ALT, "ssp1_d4", dat_drive, enable_pullups);
            imx233_pinctrl_setup_vpin(VPIN_SSP1_D5_ALT, "ssp1_d5", dat_drive, enable_pullups);
            imx233_pinctrl_setup_vpin(VPIN_SSP1_D6_ALT, "ssp1_d6", dat_drive, enable_pullups);
            imx233_pinctrl_setup_vpin(VPIN_SSP1_D7_ALT, "ssp1_d7", dat_drive, enable_pullups);
#else
            panicf("there is ssp1 alt on this soc!");
#endif
        }
        else
        {
            imx233_pinctrl_setup_vpin(VPIN_SSP1_D4, "ssp1_d4", dat_drive, enable_pullups);
            imx233_pinctrl_setup_vpin(VPIN_SSP1_D5, "ssp1_d5", dat_drive, enable_pullups);
            imx233_pinctrl_setup_vpin(VPIN_SSP1_D6, "ssp1_d6", dat_drive, enable_pullups);
            imx233_pinctrl_setup_vpin(VPIN_SSP1_D7, "ssp1_d7", dat_drive, enable_pullups);
        }
#else
        panicf("ssp1 bus width is limited to 4 on this soc!");
#endif
    }
}

void imx233_ssp_setup_ssp2_sd_mmc_pins(bool enable_pullups, unsigned bus_width)
{
    (void) enable_pullups;
    (void) bus_width;
    unsigned clk_drive = PINCTRL_DRIVE_8mA;
    unsigned dat_drive = PINCTRL_DRIVE_4mA;
#ifdef VPIN_SSP2_CMD
    /* SSP_{CMD,SCK} */
    imx233_pinctrl_setup_vpin(VPIN_SSP2_CMD, "ssp2_cmd", dat_drive, enable_pullups);
    imx233_pinctrl_setup_vpin(VPIN_SSP2_SCK, "ssp2_sck", clk_drive, false);
    /* SSP_DATA{0-3} */
    imx233_pinctrl_setup_vpin(VPIN_SSP2_D0, "ssp2_d0", dat_drive, enable_pullups);
    if(bus_width >= 4)
    {
        imx233_pinctrl_setup_vpin(VPIN_SSP2_D1, "ssp2_d1", dat_drive, enable_pullups);
        imx233_pinctrl_setup_vpin(VPIN_SSP2_D2, "ssp2_d2", dat_drive, enable_pullups);
        imx233_pinctrl_setup_vpin(VPIN_SSP2_D3, "ssp2_d3", dat_drive, enable_pullups);
    }
    if(bus_width >= 8)
    {
        imx233_pinctrl_setup_vpin(VPIN_SSP2_D4, "ssp2_d4", dat_drive, enable_pullups);
        imx233_pinctrl_setup_vpin(VPIN_SSP2_D5, "ssp2_d5", dat_drive, enable_pullups);
        imx233_pinctrl_setup_vpin(VPIN_SSP2_D6, "ssp2_d6", dat_drive, enable_pullups);
        imx233_pinctrl_setup_vpin(VPIN_SSP2_D7, "ssp2_d7", dat_drive, enable_pullups);
    }
#else
    panicf("there is no ssp2 on this soc!");
#endif
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
            SSP_WRn(SSP_CTRL1, ssp, WORD_LENGTH_V, EIGHT_BITS);
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
        /* STMP3600 cannot do 8-bit bus */
#if IMX233_SUBTARGET >= 3700
        case 8: ssp_bus_width[ssp - 1] = BV_SSP_CTRL0_BUS_WIDTH__EIGHT_BIT; break;
#endif
        default: panicf("ssp: target doesn't handle %d-bits bus", width);
    }
}

void imx233_ssp_set_block_size(int ssp, unsigned log_block_size)
{
    ASSERT_SSP(ssp)
    /* STMP3600 cannot change block size */
#if IMX233_SUBTARGET < 3600
    if(log_block_size != 9)
        panicf("ssp: target doesn't block size %d", 1 << log_block_size);
#endif
    ssp_log_block_size[ssp - 1] = log_block_size;
}

enum imx233_ssp_error_t imx233_ssp_sd_mmc_transfer(int ssp, uint8_t cmd,
    uint32_t cmd_arg, enum imx233_ssp_resp_t resp, void *buffer, unsigned block_count,
    bool wait4irq, bool read, uint32_t *resp_ptr)
{
    ASSERT_SSP(ssp)
    mutex_lock(&ssp_mutex[ssp - 1]);
    /* Enable all interrupts */
    imx233_dma_reset_channel(APB_SSP(ssp));
    imx233_icoll_enable_interrupt(INT_SRC_SSP_DMA(ssp), true);
    imx233_icoll_enable_interrupt(INT_SRC_SSP_ERROR(ssp), true);
    imx233_dma_enable_channel_interrupt(APB_SSP(ssp), true);

    unsigned xfer_size = block_count * (1 << ssp_log_block_size[ssp - 1]);

#if IMX233_SUBTARGET < 3700
    ssp_dma_cmd[ssp - 1].cmd0 = BF_OR(SSP_CMD0, CMD(cmd));
#else
    ssp_dma_cmd[ssp - 1].cmd0 = BF_OR(SSP_CMD0, CMD(cmd), APPEND_8CYC(1),
        BLOCK_SIZE(ssp_log_block_size[ssp - 1]), BLOCK_COUNT(block_count - 1));
#endif
    ssp_dma_cmd[ssp - 1].cmd1 = cmd_arg;
    /* setup all flags and run */
    ssp_dma_cmd[ssp - 1].ctrl0 = BF_OR(SSP_CTRL0, XFER_COUNT(xfer_size),
        ENABLE(1), IGNORE_CRC(buffer == NULL), WAIT_FOR_IRQ(wait4irq),
        GET_RESP(resp != SSP_NO_RESP), LONG_RESP(resp == SSP_LONG_RESP),
        BUS_WIDTH(ssp_bus_width[ssp - 1]), DATA_XFER(buffer != NULL),
        READ(read));
    /* setup the dma parameters */
    ssp_dma_cmd[ssp - 1].dma.buffer = buffer;
    ssp_dma_cmd[ssp - 1].dma.next = NULL;
    ssp_dma_cmd[ssp - 1].dma.cmd = BF_OR(APB_CHx_CMD,
        COMMAND(buffer == NULL ? BV_APB_CHx_CMD_COMMAND__NO_XFER :
        read ? BV_APB_CHx_CMD_COMMAND__WRITE : BV_APB_CHx_CMD_COMMAND__READ),
        IRQONCMPLT(1), SEMAPHORE(1), WAIT4ENDCMD(1), CMDWORDS(3), XFER_COUNT(xfer_size));

    SSP_CLRn(SSP_CTRL1, ssp, ALL_IRQ);
    SSP_SETn(SSP_CTRL1, ssp, ALL_IRQ_EN);
    imx233_dma_start_command(APB_SSP(ssp), &ssp_dma_cmd[ssp - 1].dma);

    /* the SSP hardware already has a timeout but we never know; 1 sec is a maximum
     * for all operations */
    enum imx233_ssp_error_t ret;
    if(semaphore_wait(&ssp_sema[ssp - 1], HZ) == OBJ_WAIT_TIMEDOUT)
    {
        imx233_dma_reset_channel(APB_SSP(ssp));
        ret = SSP_TIMEOUT;
    }
    else if((SSP_REGn(SSP_CTRL1, ssp) & BM_OR(SSP_CTRL1, ALL_IRQ)) == 0)
        ret =  SSP_SUCCESS;
    else if((SSP_REGn(SSP_CTRL1, ssp) & BM_OR(SSP_CTRL1, TIMEOUT_IRQ)))
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
#if IMX233_SUBTARGET >= 3780
    SSP_CLRn(SSP_CMD0, ssp, SLOW_CLKING_EN);
    SSP_SETn(SSP_CMD0, ssp, CONT_CLKING_EN);
    mdelay(1);
    SSP_CLRn(SSP_CMD0, ssp, CONT_CLKING_EN);
#endif
}

static int ssp_detect_oneshot_callback(struct timeout *tmo)
{
    int ssp = tmo->data;
    ASSERT_SSP(ssp)
    if(ssp_detect_cb[ssp - 1])
        ssp_detect_cb[ssp - 1](ssp);

    return 0;
}

static struct timeout ssp_detect_oneshot[2];

static void detect_irq(int bank, int pin, intptr_t ssp)
{
    (void) bank;
    (void) pin;
    timeout_register(&ssp_detect_oneshot[ssp - 1], ssp_detect_oneshot_callback, (3*HZ/10), ssp);
}

void imx233_ssp_sdmmc_setup_detect(int ssp, bool enable, ssp_detect_cb_t fn,
    bool first_time, bool invert)
{
    ASSERT_SSP(ssp)
    vpin_t vpin = VPIN_SSP1_DET;
    if(ssp == 2)
    {
#ifdef VPIN_SSP2_DET
        vpin = VPIN_SSP2_DET;
#else
        panicf("there is no ssp2 det on this soc!");
#endif
    }
    unsigned bank = VPIN_UNPACK_BANK(vpin), pin = VPIN_UNPACK_PIN(vpin);
    ssp_detect_cb[ssp - 1] = fn;
    ssp_detect_invert[ssp - 1] = invert;
    if(enable)
    {
        imx233_pinctrl_acquire(bank, pin, ssp == 1 ? "ssp1_det" : "ssp2_det");
        imx233_pinctrl_set_function(bank, pin, PINCTRL_FUNCTION_GPIO);
        imx233_pinctrl_enable_gpio(bank, pin, false);
    }
    if(first_time && imx233_ssp_sdmmc_detect(ssp))
        detect_irq(bank, pin, ssp);
    imx233_pinctrl_setup_irq(bank, pin, enable,
        true, !imx233_ssp_sdmmc_detect_raw(ssp), detect_irq, ssp);
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
