/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Aidan MacDonald
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
#include "sdmmc_host.h"
#include "sdmmc_poll.h"
#include "msc-x1000.h"
#include "gpio-x1000.h"
#include "irq-x1000.h"
#include "clk-x1000.h"

/* All current X1000 targets use MSC0 */
#define MSC_NUM                 0

/* And all targets use an active low card dtect GPIO */
#define MSC_CD_GPIO             GPIO_MSC0_CD
#define MSC_CD_ACTIVE_LEVEL     0

/* Q1 uses MPLL, other targets are using SCLK_A */
#if defined(SHANLING_Q1)
# define MSC_SOURCE_CLOCK       X1000_CLK_MPLL
#else
# define MSC_SOURCE_CLOCK       X1000_CLK_SCLK_A
#endif

/* 300ms poll interval */
#define SDCARD_POLL_TICKS (300 * HZ / 1000)

static struct sdmmc_host sdmmc;
static struct sdmmc_poll sdmmc_poll;
static struct x1000_msc_controller msc_ctl;
static struct x1000_msc_dma_desc msc_dma_desc;

static const struct sdmmc_controller_ops sdmmc_ops = {
    .set_power_enabled  = x1000_msc_set_power_enabled,
    .set_bus_width      = x1000_msc_set_bus_width,
    .set_bus_clock      = x1000_msc_set_bus_clock,
    .submit_command     = x1000_msc_submit_command,
    .abort_command      = x1000_msc_abort_command,
};

static const struct sdmmc_host_config sdmmc_config INITDATA_ATTR = {
    .type = STORAGE_SD,
    .bus_voltages = SDMMC_BUS_VOLTAGE_3V2_3V3 |
                    SDMMC_BUS_VOLTAGE_3V3_3V4,
    .bus_widths = SDMMC_BUS_WIDTH_1BIT |
                  SDMMC_BUS_WIDTH_4BIT,
    .bus_clocks = SDMMC_BUS_CLOCK_400KHZ |
                  SDMMC_BUS_CLOCK_25MHZ |
                  SDMMC_BUS_CLOCK_50MHZ,
    .max_nr_blocks = 65535,
    .is_removable = true,
};

static bool is_sdcard_inserted(void)
{
    return gpio_get_level(MSC_CD_GPIO) == MSC_CD_ACTIVE_LEVEL;
}

static void sdcard_insert_irq(void)
{
    sdmmc_poll_event(&sdmmc_poll);
    gpio_flip_edge_irq(MSC_CD_GPIO);
}

void sdmmc_host_target_init(void)
{
    /* Configure clock source */
    jz_writef(CPM_MSC0CDR, CE(1),
              CLKSRC(MSC_SOURCE_CLOCK == X1000_CLK_MPLL ? 1 : 0));
    while (jz_readf(CPM_MSC0CDR, BUSY));
    jz_writef(CPM_MSC0CDR, CE(0));

    /* Initialize controller */
    x1000_msc_init(&msc_ctl, &msc_dma_desc, MSC_NUM, clk_get(MSC_SOURCE_CLOCK));
    system_enable_irq(MSC_NUM == 0 ? IRQ_MSC0 : IRQ_MSC1);

    /* Initialize SD/MMC host driver */
    sdmmc_host_init(&sdmmc, &sdmmc_config, &sdmmc_ops, &msc_ctl);
    sdmmc_host_init_medium_present(&sdmmc, is_sdcard_inserted());

    /* Setup card detect handling */
    sdmmc_poll_init(&sdmmc_poll, &sdmmc, is_sdcard_inserted);

    system_set_irq_handler(GPIO_TO_IRQ(MSC_CD_GPIO), sdcard_insert_irq);
    gpio_set_function(MSC_CD_GPIO, GPIOF_IRQ_EDGE(1));
    gpio_flip_edge_irq(MSC_CD_GPIO);
    gpio_enable_irq(MSC_CD_GPIO);
}

#if MSC_NUM == 0
void MSC0(void)
{
    x1000_msc_irq_handler(&msc_ctl);
}
#else
void MSC1(void)
{
    x1000_msc_irq_handler(&msc_ctl);
}
#endif
