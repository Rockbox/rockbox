/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 by Aidan MacDonald
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
#include "clock-echoplayer.h"
#include "sdmmc-stm32h7.h"
#include "gpio-stm32h7.h"
#include "nvic-arm.h"
#include "regs/stm32h743/sdmmc.h"

static struct sdmmc_host sdmmc1;
static struct sdmmc_poll sdmmc1_poll;
static struct stm32h7_sdmmc_controller sdmmc1_ctl;

static const struct sdmmc_controller_ops sdmmc_ops = {
    .set_power_enabled        = stm32h7_sdmmc_set_power_enabled,
    .set_bus_width            = stm32h7_sdmmc_set_bus_width,
    .set_bus_clock            = stm32h7_sdmmc_set_bus_clock,
    .submit_command           = stm32h7_sdmmc_submit_command,
    .abort_command            = stm32h7_sdmmc_abort_command,
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
    return gpio_get_level(GPIO_SDMMC_DETECT) == 0;
}

void sdmmc_host_target_init(void)
{
    /* Initialize controller */
    stm32h7_sdmmc_init(&sdmmc1_ctl, ITA_SDMMC1, &sdmmc1_ker_clock,
                       stm32h7_reset_sdmmc1, NULL);
    nvic_enable_irq(NVIC_IRQN_SDMMC1);

    /* Initialize SD/MMC host driver */
    sdmmc_host_init(&sdmmc1, &sdmmc_config, &sdmmc_ops, &sdmmc1_ctl);
    sdmmc_host_init_medium_present(&sdmmc1, is_sdcard_inserted());

    /* Start insertion poller */
    sdmmc_poll_init(&sdmmc1_poll, &sdmmc1, is_sdcard_inserted);
    sdmmc_poll_start(&sdmmc1_poll);
}

void sdmmc1_irq_handler(void)
{
    stm32h7_sdmmc_irq_handler(&sdmmc1_ctl);
}
