/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025-2026 Aidan MacDonald
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
#ifndef __SDMMC_POLL_H__
#define __SDMMC_POLL_H__

#include "timeout.h"
#include <stdbool.h>

struct sdmmc_host;
struct sdmmc_poll
{
    struct timeout timeout;
    struct sdmmc_host *host;

    bool (*check_inserted)(void);

    bool is_inserted;
    bool is_polling;
    bool last_state;
    bool curr_state;
};

/*
 * Simple helper for polling for SD card insertion state.
 * This calls sdmmc_host_set_medium_present() when a change
 * in insertion state is detected via the check_inserted()
 * callback.
 *
 * Call sdmmc_poll_start() to start the poll timer if you
 * use level triggered polling.
 *
 * To use edge triggered events call sdmmc_poll_event() if
 * the return value of check_inserted() may have changed.
 *
 * Changes in the insertion state are debounced before they
 * are reported to the sdmmc_host.
 */

void sdmmc_poll_init(struct sdmmc_poll *poll,
                     struct sdmmc_host *host,
                     bool (*check_inserted) (void));
void sdmmc_poll_start(struct sdmmc_poll *poll);
void sdmmc_poll_event(struct sdmmc_poll *poll);

#endif /* __SDMMC_POLL_H__ */
