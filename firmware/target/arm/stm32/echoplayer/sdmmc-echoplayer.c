/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "clock-echoplayer.h"
#include "sdmmc-stm32h7.h"
#include "gpio-stm32h7.h"
#include "nvic-arm.h"
#include "regs/stm32h743/sdmmc.h"

/* 300ms poll interval */
#define SDCARD_POLL_TICKS (300 * HZ / 1000)

static struct sdmmc_host sdmmc1;
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
    .is_removable = true,
};

/*
 * simple SD insertion poller
 */
struct sdmmc_poll
{
    struct sdmmc_host *host;
    bool is_inserted;

    bool last_state;
    bool curr_state;
};

static bool is_sdcard_inserted(void)
{
    return gpio_get_level(GPIO_SDMMC_DETECT) == 0;
}

static int poll_sdcard_inserted(struct timeout *tmo)
{
    struct sdmmc_poll *poll = (void *)tmo->data;

    poll->last_state = poll->curr_state;
    poll->curr_state = is_sdcard_inserted();

    if (!poll->curr_state && poll->is_inserted)
    {
        poll->is_inserted = false;
        sdmmc_host_set_medium_present(poll->host, false);
    }
    else if (poll->curr_state && !poll->is_inserted &&
             poll->curr_state == poll->last_state)
    {
        poll->is_inserted = true;
        sdmmc_host_set_medium_present(poll->host, true);
    }

    return SDCARD_POLL_TICKS;
}

static struct timeout sdcard_poll_timeout;
static struct sdmmc_poll sdcard_poll;

void sdmmc_host_target_init(void)
{
    /* Initialize controller */
    stm32h7_sdmmc_init(&sdmmc1_ctl, ITA_SDMMC1, &sdmmc1_ker_clock,
                       stm32h7_reset_sdmmc1, NULL);
    nvic_enable_irq(NVIC_IRQN_SDMMC1);

    /* Initialize card detect polling */
    sdcard_poll.host = &sdmmc1;
    sdcard_poll.is_inserted = is_sdcard_inserted();
    timeout_register(&sdcard_poll_timeout, poll_sdcard_inserted,
                     SDCARD_POLL_TICKS, (intptr_t)&sdcard_poll);

    /* Initialize SD/MMC host driver */
    sdmmc_host_init(&sdmmc1, &sdmmc_config, &sdmmc_ops, &sdmmc1_ctl);
    sdmmc_host_init_medium_present(&sdmmc1, sdcard_poll.is_inserted);
}

void sdmmc1_irq_handler(void)
{
    stm32h7_sdmmc_irq_handler(&sdmmc1_ctl);
}
