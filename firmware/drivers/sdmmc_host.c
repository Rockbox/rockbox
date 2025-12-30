/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#include "system.h"
#include "storage.h"
#include "disk_cache.h"
#include "debug.h"
#include "panic.h"
#include "logf.h"
#include <string.h>

/*
 * SD/MMC host interface
 */

#ifdef CONFIG_STORAGE_MULTI
# define DECLARE_FIRST_DRIVE(decl) static decl
#else
# define DECLARE_FIRST_DRIVE(decl) static const decl = 0
#endif

#if CONFIG_STORAGE & STORAGE_SD
static struct sdmmc_host *sdmmc_sd_hosts[SDMMC_HOST_NUM_SD_CONTROLLERS];
DECLARE_FIRST_DRIVE(int sdmmc_sd_first_drive);
static volatile long sdmmc_sd_last_activity;
#endif

#if CONFIG_STORAGE & STORAGE_MMC
static struct sdmmc_host *sdmmc_mmc_hosts[SDMMC_HOST_NUM_MMC_CONTROLLERS];
DECLARE_FIRST_DRIVE(int sdmmc_mmc_first_drive);
static volatile long sdmmc_mmc_last_activity;
#endif

static int sdmmc_host_get_logical_drive(struct sdmmc_host *host)
{
    switch (host->config.type)
    {
#if CONFIG_STORAGE & STORAGE_SD
    case STORAGE_SD:
        return sdmmc_sd_first_drive + host->drive;
#endif

#if CONFIG_STORAGE & STORAGE_MMC
    case STORAGE_MMC:
        return sdmmc_mmc_first_drive + host->drive;
#endif

    default:
        return 0;
    }
}

void sdmmc_host_init(struct sdmmc_host *host,
                     const struct sdmmc_host_config *config,
                     const struct sdmmc_controller_ops *ops,
                     void *controller)
{
    struct sdmmc_host **array = NULL;
    size_t array_size = 0;

    switch (config->type)
    {
#if CONFIG_STORAGE & STORAGE_SD
    case STORAGE_SD:
        array = sdmmc_sd_hosts;
        array_size = ARRAYLEN(sdmmc_sd_hosts);
        break;
#endif

#if CONFIG_STORAGE & STORAGE_MMC
    case STORAGE_MMC:
        array = sdmmc_mmc_hosts;
        array_size = ARRAYLEN(sdmmc_mmc_hosts);
        break;
#endif

    default:
        panicf("%s: bad type", __func__);
        return;
    }

    if (!ops->submit_command || !ops->set_bus_clock)
    {
        panicf("%s: missing ops", __func__);
        return;
    }

    for (size_t i = 0; i < array_size; ++i)
    {
        if (array[i] == NULL)
        {
            memset(host, 0, sizeof(*host));

            host->config = *config;
            host->drive = i;
            host->ops = ops;
            host->controller = controller;
            host->present = !config->is_removable;
            mutex_init(&host->lock);

            array[i] = host;
            return;
        }
    }

    panicf("%s: too many controllers", __func__);
}

#ifdef HAVE_HOTSWAP
void sdmmc_host_init_medium_present(struct sdmmc_host *host, bool present)
{
    if (!host->config.is_removable)
        panicf("%s called for non-removable drive", __func__);

    host->present = present;
}

void sdmmc_host_set_medium_present(struct sdmmc_host *host, bool present)
{
    long event = present ? Q_STORAGE_MEDIUM_INSERTED : Q_STORAGE_MEDIUM_REMOVED;

    if (!host->config.is_removable)
        panicf("%s called for non-removable drive", __func__);

    storage_post_event(event, sdmmc_host_get_logical_drive(host));
}
#endif

#ifdef CONFIG_STORAGE_MULTI
static int sdmmc_host_count_drives(struct sdmmc_host **array,
                                   size_t array_size)
{
    size_t num_drives = 0;
    for (; num_drives < array_size; ++num_drives)
    {
        if (array[num_drives] == NULL)
            break;
    }

    return num_drives;
}
#endif

/*
 * Changes the bus power state if the target supports it.
 */
static void sdmmc_host_set_power_enabled(struct sdmmc_host *host, bool enabled)
{
    if (host->powered != enabled)
    {
        if (host->ops->set_power_enabled)
            host->ops->set_power_enabled(host->controller, enabled);

        host->powered = enabled;
    }
}

/*
 * Resets the bus by powering it down and clearing all device-related
 * state. The bus must be powered back on manually before trying to
 * reinitialize the SD/MMC device.
 */
static void sdmmc_host_bus_reset(struct sdmmc_host *host)
{
    sdmmc_host_set_power_enabled(host, false);

    host->need_reset = false;
    host->initialized = false;
    host->is_hcs_card = false;
    memset(&host->cardinfo, 0, sizeof(host->cardinfo));
}

#ifdef HAVE_HOTSWAP
static void sdmmc_host_hotswap_event(struct sdmmc_host *host, bool is_present)
{
    /* Ignore spurious events */
    if (host->present == is_present)
        return;

    /* Update medium presence flag so I/O threads see removals */
    host->present = is_present;

    /* Handle sudden medium removal */
    if (!is_present)
    {
        if (host->ops->abort_command)
            host->ops->abort_command(host->controller);

        mutex_lock(&host->lock);
        sdmmc_host_bus_reset(host);
        mutex_unlock(&host->lock);
    }

    /* Broadcast hotswap event to the system */
    queue_broadcast(is_present ? SYS_HOTSWAP_INSERTED : SYS_HOTSWAP_EXTRACTED,
                    sdmmc_host_get_logical_drive(host));
}
#endif

static bool sdmmc_host_medium_present(struct sdmmc_host *host)
{
#ifdef HAVE_HOTSWAP
    return host->present;
#else
    return true;
#endif
}

/*
 * Submit one command to the host controller.
 */
static int sdmmc_host_submit_cmd(struct sdmmc_host *host,
                                 const struct sdmmc_host_command *cmd,
                                 struct sdmmc_host_response *resp)
{
    /*
     * TODO: generic SD/MMC error detection & recovery, examples:
     *
     * - timeout to abort the command if controller is hung
     * - automatic retrying of commands if CRC error occurs
     */

    return host->ops->submit_command(host->controller, cmd, resp);
}

/*
 * Execute an SD APP_CMD
 */
static int sdmmc_host_submit_app_cmd(struct sdmmc_host *host,
                                     const struct sdmmc_host_command *cmd,
                                     struct sdmmc_host_response *resp)
{
    struct sdmmc_host_response aresp;
    struct sdmmc_host_command acmd = {
        .command  = SD_APP_CMD,
        .argument = host->cardinfo.rca,
        .flags    = SDMMC_RESP_SHORT,
    };

    int rc = sdmmc_host_submit_cmd(host, &acmd, &aresp);
    if (rc)
    {
        logf("%s: cmd55 err %d", __func__, rc);
        return rc;
    }

    /* Check that card accepted the APP_CMD */
    if ((aresp.data[0] & SD_R1_APP_CMD) == 0)
    {
        logf("%s: cmd55 not acked", __func__);
        return SDMMC_STATUS_ERROR;
    }

    return sdmmc_host_submit_cmd(host, cmd, resp);
}

static int sdmmc_host_cmd_go_idle_state(struct sdmmc_host *host)
{
    struct sdmmc_host_command cmd = {
        .command = SD_GO_IDLE_STATE,
        .flags   = SDMMC_RESP_NONE,
    };

    return sdmmc_host_submit_cmd(host, &cmd, NULL);
}

static int sdmmc_host_cmd_send_if_cond(struct sdmmc_host *host)
{
    struct sdmmc_host_response resp;
    struct sdmmc_host_command cmd = {
        .command  = SD_SEND_IF_COND,
        .argument = 0x1aa,
        .flags    = SDMMC_RESP_SHORT,
    };

    int rc = sdmmc_host_submit_cmd(host, &cmd, &resp);
    switch (rc)
    {
    case SDMMC_STATUS_OK:
        /* Valid response means it's an HCS card */
        if ((resp.data[0] & 0xff) == 0xaa)
        {
            host->is_hcs_card = true;
            break;
        }
        // fallthrough

    case SDMMC_STATUS_TIMEOUT:
        host->is_hcs_card = false;
        return SDMMC_STATUS_OK;

    default:
        return rc;
    }
    return 0;
}

static int sdmmc_host_cmd_send_app_op_cond(struct sdmmc_host *host)
{
    struct sdmmc_host_response resp;
    struct sdmmc_host_command cmd = {
        .command = SD_APP_OP_COND,
        .flags   = SDMMC_RESP_SHORT | SDMMC_RESP_NOCRC,
    };

    /* Add operating voltages */
    cmd.argument |= host->config.bus_voltages;
    cmd.argument &= (0x1FFu << 15);

    /* Set HCS bit for SDHC/SDXC */
    if (host->is_hcs_card)
        cmd.argument |= SD_OCR_CARD_CAPACITY_STATUS;

    /*
     * Maximum wait time to initialize is 1 second according
     * to the SD specs.
     */
    int timeout = HZ;
    for (; timeout > 0; timeout--)
    {
        int rc = sdmmc_host_submit_app_cmd(host, &cmd, &resp);
        if (rc)
            return rc;

        /* Exit when card says it's initialized */
        if (resp.data[0] & (1u << 31))
            break;

        /* Card not initialized yet, wait & try again */
        sleep(1);
    }

    if (timeout == 0)
        return SDMMC_STATUS_TIMEOUT;

    return SDMMC_STATUS_OK;
}

static int sdmmc_host_cmd_all_send_cid(struct sdmmc_host *host)
{
    struct sdmmc_host_response resp;
    struct sdmmc_host_command cmd = {
        .command = SD_ALL_SEND_CID,
        .flags   = SDMMC_RESP_LONG,
    };

    int rc = sdmmc_host_submit_cmd(host, &cmd, &resp);
    if (rc)
        return rc;

    for (int i = 0; i < 4; ++i)
        host->cardinfo.cid[i] = resp.data[i];

    return rc;
}

static int sdmmc_host_cmd_send_rca(struct sdmmc_host *host)
{
    struct sdmmc_host_response resp;
    struct sdmmc_host_command cmd = {
        .command = SD_SEND_RELATIVE_ADDR,
        .flags   = SDMMC_RESP_SHORT,
    };

    int rc = sdmmc_host_submit_cmd(host, &cmd, &resp);
    if (rc)
        return rc;

    host->cardinfo.rca = resp.data[0] & (0xFFFFu << 16);
    return rc;
}

static int sdmmc_host_cmd_send_csd(struct sdmmc_host *host)
{
    struct sdmmc_host_response resp;
    struct sdmmc_host_command cmd = {
        .command  = SD_SEND_CSD,
        .argument = host->cardinfo.rca,
        .flags    = SDMMC_RESP_LONG,
    };

    int rc = sdmmc_host_submit_cmd(host, &cmd, &resp);
    if (rc)
        return rc;

    for (int i = 0; i < 4; ++i)
        host->cardinfo.csd[i] = resp.data[i];

    sd_parse_csd(&host->cardinfo);
    return rc;
}

static int sdmmc_host_cmd_select_card(struct sdmmc_host *host)
{
    struct sdmmc_host_command cmd = {
        .command  = SD_SELECT_CARD,
        .argument = host->cardinfo.rca,
        .flags    = SDMMC_RESP_SHORT | SDMMC_RESP_BUSY,
    };

    return sdmmc_host_submit_cmd(host, &cmd, NULL);
}

static int sdmmc_host_cmd_clr_card_detect(struct sdmmc_host *host)
{
    struct sdmmc_host_command cmd = {
        .command  = SD_SET_CLR_CARD_DETECT,
        .argument = 0,
        .flags    = SDMMC_RESP_SHORT,
    };

    return sdmmc_host_submit_app_cmd(host, &cmd, NULL);
}

static int sdmmc_host_cmd_set_bus_width(struct sdmmc_host *host,
                                        uint32_t bus_width)
{
    struct sdmmc_host_command cmd = {
        .command = SD_SET_BUS_WIDTH,
        .flags   = SDMMC_RESP_SHORT,
    };

    if (bus_width == SDMMC_BUS_WIDTH_1BIT)
        cmd.argument = 0;
    else if (bus_width == SDMMC_BUS_WIDTH_4BIT)
        cmd.argument = 2;
    else
        return SDMMC_STATUS_ERROR;

    return sdmmc_host_submit_app_cmd(host, &cmd, NULL);
}

static int sdmmc_host_cmd_switch_freq(struct sdmmc_host *host,
                                      uint32_t clock)
{
    struct sdmmc_host_command cmd = {
        .command   = SD_SWITCH_FUNC,
        .flags     = SDMMC_RESP_SHORT | SDMMC_DATA_READ,
        .nr_blocks = 1,
        .block_len = 64,
    };

    if (clock == SDMMC_BUS_CLOCK_25MHZ)
        cmd.argument = 0x80fffff0;
    else if (clock == SDMMC_BUS_CLOCK_50MHZ)
        cmd.argument = 0x80fffff1;
    else
        return SDMMC_STATUS_ERROR;

    /*
     * We'll just assume the disk cache will have a free buffer.
     * Since the disk isn't mounted, we should have at least one
     * free that would otherwise be used by the FAT filesystem.
     */
    cmd.buffer = dc_get_buffer();
    if (!cmd.buffer)
        panicf("%s: OOM", __func__);

    int rc = sdmmc_host_submit_cmd(host, &cmd, NULL);

    dc_release_buffer(cmd.buffer);
    return rc;
}

static int sdmmc_host_cmd_set_block_len(struct sdmmc_host *host, int len)
{
    struct sdmmc_host_command cmd = {
        .command   = SD_SET_BLOCKLEN,
        .argument  = len,
        .flags     = SDMMC_RESP_SHORT,
    };

    return sdmmc_host_submit_cmd(host, &cmd, NULL);
}

static void sdmmc_host_set_controller_bus_width(struct sdmmc_host *host, uint32_t width)
{
    if (host->ops->set_bus_width)
        host->ops->set_bus_width(host->controller, width);
}

static void sdmmc_host_set_controller_bus_clock(struct sdmmc_host *host, uint32_t clock)
{
    host->ops->set_bus_clock(host->controller, clock);
}

/*
 * Initialize the SD/MMC device and make it ready to access.
 * On success, sets host->initialized to true and returns 0.
 * Returns nonzero on error. If an error occurs the bus must
 * be reset before retrying.
 *
 * Preconditions:
 * - host->initialized is false
 * - device related state is reset to default
 */
static int sdmmc_host_device_init(struct sdmmc_host *host)
{
    /* TODO: MMC initialization */
    if (host->config.type != STORAGE_SD)
        panicf("%s: TODO mmc init", __func__);

    /* Initialize bus */
    sdmmc_host_set_controller_bus_width(host, SDMMC_BUS_WIDTH_1BIT);
    sdmmc_host_set_controller_bus_clock(host, SDMMC_BUS_CLOCK_400KHZ);
    sdmmc_host_set_power_enabled(host, true);

    /* Handle SD initialization */
    int rc = sdmmc_host_cmd_go_idle_state(host);
    if (rc)
    {
        logf("sdmmc_host_cmd_go_idle_state: %d", rc);
        return rc;
    }

    rc = sdmmc_host_cmd_send_if_cond(host);
    if (rc)
    {
        logf("sdmmc_host_cmd_send_if_cond: %d", rc);
        return rc;
    }

    rc = sdmmc_host_cmd_send_app_op_cond(host);
    if (rc)
    {
        logf("sdmmc_host_cmd_send_app_op_cond: %d", rc);
        return rc;
    }

    rc = sdmmc_host_cmd_all_send_cid(host);
    if (rc)
    {
        logf("sdmmc_host_cmd_all_send_cid: %d", rc);
        return rc;
    }

    rc = sdmmc_host_cmd_send_rca(host);
    if (rc)
    {
        logf("sdmmc_host_cmd_send_rca: %d", rc);
        return rc;
    }

    rc = sdmmc_host_cmd_send_csd(host);
    if (rc)
    {
        logf("sdmmc_host_cmd_send_csd: %d", rc);
        return rc;
    }

    rc = sdmmc_host_cmd_select_card(host);
    if (rc)
    {
        logf("sdmmc_host_cmd_select_card: %d", rc);
        return rc;
    }

    rc = sdmmc_host_cmd_clr_card_detect(host);
    if (rc)
    {
        logf("sdmmc_host_cmd_select_card: %d", rc);
        return rc;
    }

    /*
     * All SD cards must support 1-bit and 4-bit bus widths,
     * so we only need to check the controller capabilities.
     */
    if (host->config.bus_widths & SDMMC_BUS_WIDTH_4BIT)
    {
        rc = sdmmc_host_cmd_set_bus_width(host, SDMMC_BUS_WIDTH_4BIT);
        if (rc)
        {
            logf("sdmmc_host_cmd_set_bus_width: %d", rc);
            return rc;
        }

        sdmmc_host_set_controller_bus_width(host, SDMMC_BUS_WIDTH_4BIT);
    }

    /*
     * Switch to highest clock frequency supported by both
     * card and controller. 25MHz must always be supported.
     */
    if (host->cardinfo.sd2plus &&
        (host->config.bus_clocks & SDMMC_BUS_CLOCK_50MHZ))
    {
        rc = sdmmc_host_cmd_switch_freq(host, SDMMC_BUS_CLOCK_50MHZ);
        if (rc)
        {
            logf("sdmmc_host_cmd_switch_freq: %d", rc);
            return rc;
        }

        sdmmc_host_set_controller_bus_clock(host, SDMMC_BUS_CLOCK_50MHZ);
    }
    else
    {
        sdmmc_host_set_controller_bus_clock(host, SDMMC_BUS_CLOCK_25MHZ);
    }

    host->initialized = true;
    return 0;
}

static int sdmmc_host_transfer(struct sdmmc_host *host,
                               sector_t start, int count, void *buf,
                               uint32_t data_dir)
{
    int rc = -1;

    mutex_lock(&host->lock);

    if (!sdmmc_host_medium_present(host))
        goto out;

    if (host->need_reset)
    {
        /* Automatically clears need_reset flag */
        sdmmc_host_bus_reset(host);
    }

    if (!host->initialized)
    {
        rc = sdmmc_host_device_init(host);
        if (rc)
        {
            host->need_reset = true;
            goto out;
        }
    }

    if (count < 1)
        goto out;
    if (start + count > host->cardinfo.numblocks)
        goto out;

    while (count > 0)
    {
        /* TODO: multiple block transfers */
        int xfer_count = 1;

        /* Set block length for non-HCS cards */
        if (!host->is_hcs_card)
        {
            rc = sdmmc_host_cmd_set_block_len(host, SD_BLOCK_SIZE);
            if (rc)
                goto out;
        }

        struct sdmmc_host_command cmd = {
            .buffer = buf,
            .nr_blocks = 1,
            .block_len = SD_BLOCK_SIZE,
            .flags = SDMMC_RESP_SHORT | data_dir,
        };

        if (data_dir == SDMMC_DATA_WRITE)
            cmd.command = SD_WRITE_BLOCK;
        else
            cmd.command = SD_READ_SINGLE_BLOCK;

        if (host->cardinfo.sd2plus)
            cmd.argument = start;
        else
            cmd.argument = start * SD_BLOCK_SIZE;

        rc = sdmmc_host_submit_cmd(host, &cmd, NULL);
        if (rc)
            goto out;

        buf += xfer_count * SD_BLOCK_SIZE;
        start += xfer_count;
        count -= xfer_count;
    }

out:
    mutex_unlock(&host->lock);
    return rc;
}

/*
 * SD storage API
 */

#if CONFIG_STORAGE & STORAGE_SD
int sd_init(void)
{
    return 0;
}

#ifdef CONFIG_STORAGE_MULTI
int sd_num_drives(int first_drive)
{
    sdmmc_sd_first_drive = first_drive;
    return sdmmc_host_count_drives(sdmmc_sd_hosts, ARRAYLEN(sdmmc_sd_hosts));
}
#endif

tCardInfo *card_get_info_target(int drive)
{
    /* FIXME: this API is racy, no way to fix without refactoring */
    return &sdmmc_sd_hosts[drive]->cardinfo;
}

long sd_last_disk_activity(void)
{
    return sdmmc_sd_last_activity;
}

int sd_event(long id, intptr_t data)
{
#ifdef HAVE_HOTSWAP
    if (id == Q_STORAGE_MEDIUM_INSERTED ||
        id == Q_STORAGE_MEDIUM_REMOVED)
    {
        bool present = (id == Q_STORAGE_MEDIUM_INSERTED);

        sdmmc_host_hotswap_event(sdmmc_sd_hosts[IF_MD_DRV(drive)], present);
        return 0;
    }
#endif

    return storage_event_default_handler(id, data,
                                         sdmmc_sd_last_activity, STORAGE_SD);
}

int sd_read_sectors(IF_MD(int drive,) sector_t start, int count, void* buf)
{
    struct sdmmc_host *host = sdmmc_sd_hosts[IF_MD_DRV(drive)];

    sdmmc_sd_last_activity = current_tick;

    return sdmmc_host_transfer(host, start, count, buf, SDMMC_DATA_READ);
}

int sd_write_sectors(IF_MD(int drive,) sector_t start, int count, const void* buf)
{
    struct sdmmc_host *host = sdmmc_sd_hosts[IF_MD_DRV(drive)];

    sdmmc_sd_last_activity = current_tick;

    return sdmmc_host_transfer(host, start, count, (void *)buf, SDMMC_DATA_WRITE);
}

#ifdef HAVE_HOTSWAP
bool sd_removable(IF_MD_NONVOID(int drive))
{
    struct sdmmc_host *host = sdmmc_sd_hosts[IF_MD_DRV(drive)];

    return host->config.is_removable;
}

bool sd_present(IF_MD_NONVOID(int drive))
{
    struct sdmmc_host *host = sdmmc_sd_hosts[IF_MD_DRV(drive)];

    return sdmmc_host_medium_present(host);
}
#endif /* HAVE_HOTSWAP */
#endif /* CONFIG_STORAGE & STORAGE_SD */

/*
 * TODO: add MMC storage API; right now nothing needs it
 */
