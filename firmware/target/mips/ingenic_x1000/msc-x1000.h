/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2021 Aidan MacDonald
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
#ifndef __MSC_X1000_H__
#define __MSC_X1000_H__

#include "sdmmc_host.h"

/* Must be allocated on a cacheline boundary */
struct x1000_msc_dma_desc
{
    uint32_t nda;
    uint32_t mem;
    uint32_t len;
    uint32_t cmd;
};

struct x1000_msc_controller
{
    int msc_nr;
    uint32_t src_clk_freq;
    struct x1000_msc_dma_desc *dma_desc;

    uint32_t bus_clock;
    uint32_t cmdat_def;
    uint32_t iflag_done;
    int resp_len;
    int err_code;
    struct sdmmc_host_response *resp;

    struct semaphore sem;
};

void x1000_msc_init(struct x1000_msc_controller* ctl,
                    struct x1000_msc_dma_desc *dma_desc,
                    int msc_nr, uint32_t src_clk_freq);

void x1000_msc_set_power_enabled(void *controller, bool enabled);
void x1000_msc_set_bus_width(void *controller, uint32_t width);
void x1000_msc_set_bus_clock(void *controller, uint32_t clock);
int x1000_msc_submit_command(void *controller,
                             const struct sdmmc_host_command *cmd,
                             struct sdmmc_host_response *resp);
void x1000_msc_abort_command(void *controller);
void x1000_msc_irq_handler(struct x1000_msc_controller *ctl);

#endif /* __MSC_X1000_H__ */
