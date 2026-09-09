/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Aidan MacDonald
 * Copyright (C) 2026 Michael McAllister
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
#include "msc-ingenic.h"
#include "gpio-ingenic.h"
#include "irq-x1600.h"
#include "clk-x1600.h"
#include "spin-x1600.h"
#include "x1600/cpm.h"
#include "panic.h"

/* MSC0 is connected to the unsupported Wi-Fi module. */
#define MSC_NUM 1

static struct sdmmc_host sdmmc;
static struct sdmmc_poll sdmmc_poll;
static struct ingenic_msc_controller msc_ctl;
static struct ingenic_msc_dma_desc msc_dma_desc CACHEALIGN_ATTR;

static bool is_sdcard_inserted(void)
{
    return gpio_get_level(GPIO_MSC1_CD) == 0;
}

static void sdcard_insert_irq(void)
{
    sdmmc_poll_event(&sdmmc_poll);
    gpio_flip_edge_irq(GPIO_MSC1_CD);
}

static void sdcard_set_power_enabled(void *controller, bool enabled)
{
    if (enabled)
    {
        gpio_set_level(GPIO_MSC1_POWER, 1);
        udelay(1000);
    }

    ingenic_msc_set_power_enabled(controller, enabled);

    if (!enabled)
    {
        gpio_set_level(GPIO_MSC1_POWER, 0);
        udelay(1000);
    }
}

static const struct sdmmc_controller_ops sdmmc_ops = {
    .set_power_enabled = sdcard_set_power_enabled,
    .set_bus_width = ingenic_msc_set_bus_width,
    .set_bus_clock = ingenic_msc_set_bus_clock,
    .submit_command = ingenic_msc_submit_command,
    .abort_command = ingenic_msc_abort_command,
};

static const struct sdmmc_host_config sdmmc_config INITDATA_ATTR = {
    .type = STORAGE_SD,
    .bus_voltages = SDMMC_BUS_VOLTAGE_3V2_3V3 |
                    SDMMC_BUS_VOLTAGE_3V3_3V4,
    .bus_widths = SDMMC_BUS_WIDTH_1BIT | SDMMC_BUS_WIDTH_4BIT,
    .bus_clocks = SDMMC_BUS_CLOCK_400KHZ |
                  SDMMC_BUS_CLOCK_25MHZ |
                  SDMMC_BUS_CLOCK_50MHZ,
    .max_nr_blocks = 65535,
    .is_removable = true,
};

void sdmmc_host_target_init(void)
{
    /* PC25 has a pull-down; drive the slot power switch explicitly. */
    gpio_set_function(GPIO_MSC1_POWER, GPIOF_OUTPUT(0));
    gpioz_configure(GPIO_D, 0x3f, GPIOF_DEVICE(0));

    /* MPLL / 2 supports exact 25/50 MHz card clocks. Start at the
     * slowest divider until the host selects the initial bus rate. */
    jz_writef(CPM_MSC1CDR, CE(1), MPCS_V(MPLL), CLKDIV(255));
    if (x1600_spin_while(jz_readf(CPM_MSC1CDR, BUSY)))
        panicf("msc1: clock busy");
    jz_writef(CPM_MSC1CDR, CE(0));

    uint32_t src_freq = clk_get(X1600_CLK_MPLL);
    if (!src_freq)
        panicf("msc1: MPLL is stopped");

    ingenic_msc_init(&msc_ctl, &msc_dma_desc, MSC_NUM, src_freq);
    system_enable_irq(IRQ_MSC1);

    sdmmc_host_init(&sdmmc, &sdmmc_config, &sdmmc_ops, &msc_ctl);
    sdmmc_poll_init(&sdmmc_poll, &sdmmc, is_sdcard_inserted);
    sdmmc_host_init_medium_present(&sdmmc, sdmmc_poll.is_inserted);

    /* The host and poller must be ready before an IRQ can arrive. */
    system_set_irq_handler(GPIO_TO_IRQ(GPIO_MSC1_CD), sdcard_insert_irq);
    gpio_set_function(GPIO_MSC1_CD, GPIOF_IRQ_EDGE(1));
    gpio_flip_edge_irq(GPIO_MSC1_CD);
    gpio_enable_irq(GPIO_MSC1_CD);

    /* Reconcile changes between the initial sample and IRQ setup. */
    sdmmc_poll_event(&sdmmc_poll);
}

void MSC1(void)
{
    ingenic_msc_irq_handler(&msc_ctl);
}
