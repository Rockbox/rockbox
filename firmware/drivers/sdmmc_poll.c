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
#include "sdmmc_poll.h"
#include "sdmmc_host.h"

/* 300ms poll interval */
#define SDCARD_POLL_TICKS (300 * HZ / 1000)

static int sdmmc_poll_timeout(struct timeout *tmo)
{
    struct sdmmc_poll *poll = (void *)tmo->data;

    poll->last_state = poll->curr_state;
    poll->curr_state = poll->check_inserted();

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

    if (poll->is_polling || (poll->curr_state != poll->last_state))
        return SDCARD_POLL_TICKS;

    return 0;
}

void sdmmc_poll_init(struct sdmmc_poll *poll,
                     struct sdmmc_host *host,
                     bool (*check_inserted) (void))
{
    poll->host = host;
    poll->check_inserted = check_inserted;

    poll->is_inserted = check_inserted();
    poll->curr_state = poll->is_inserted;
    poll->last_state = poll->is_inserted;
}

void sdmmc_poll_start(struct sdmmc_poll *poll)
{
    poll->is_polling = true;
    timeout_register(&poll->timeout, sdmmc_poll_timeout,
                     SDCARD_POLL_TICKS, (intptr_t)poll);
}

void sdmmc_poll_event(struct sdmmc_poll *poll)
{
    timeout_register(&poll->timeout, sdmmc_poll_timeout,
                     SDCARD_POLL_TICKS, (intptr_t)poll);
    sdmmc_poll_timeout(&poll->timeout);
}
