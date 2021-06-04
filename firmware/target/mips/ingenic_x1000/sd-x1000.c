/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

#include "storage.h"
#include "sdmmc.h"
#include "sd.h"
#include "msc-x1000.h"
#include "gpio-x1000.h"
#include <string.h>

/* #define LOGF_ENABLE */
#include "logf.h"

static msc_drv* sd_to_msc[MSC_COUNT];
static long _sd_last_disk_activity = 0;

static int sd_init_card(msc_drv* d)
{
    int s;
    if(s = msc_cmd_go_idle_state(d))            return -100 - s;
    if(s = msc_cmd_send_if_cond(d))             return -110 - s;
    if(s = msc_cmd_app_op_cond(d))              return -120 - s;
    if(s = msc_cmd_all_send_cid(d))             return -130 - s;
    if(s = msc_cmd_send_rca(d))                 return -140 - s;
    if(s = msc_cmd_send_csd(d))                 return -150 - s;
    if(s = msc_cmd_select_card(d))              return -160 - s;
    if(s = msc_cmd_set_clr_card_detect(d, 0))   return -170 - s;
    if(s = msc_cmd_set_bus_width(d, 4))         return -180 - s;
    if(s = msc_cmd_switch_freq(d))              return -190 - s;
    d->driver_flags |= MSC_DF_READY;
    d->cardinfo.initialized = 1;
    return 0;
}

static int sd_transfer(msc_drv* d, bool write,
                       unsigned long start, int count, void* buf)
{
    int status = -1;

    msc_lock(d);
    if(!d->card_present)
        goto _exit;

    /* Hopefully puts the driver into a working state */
    if(d->driver_flags & MSC_DF_ERRSTATE) {
        logf("MSC%d: attempting to reset after ERRSTATE", d->msc_nr);
        msc_full_reset(d);
    }

    /* Init card if needed */
    if((d->driver_flags & MSC_DF_READY) == 0) {
        if(status = sd_init_card(d)) {
            logf("MSC%d: card init failed (code %d)", d->msc_nr, status);
            d->driver_flags |= MSC_DF_ERRSTATE;
            d->cardinfo.initialized = status;
            goto _exit;
        }
    }

    /* Ensure parameters are within range */
    if(count < 1)
        goto _exit;
    if(start + count > d->cardinfo.numblocks)
        goto _exit;

    do {
        /* We can only do 65536 blocks at a time */
        int xfer_count = count > 0xffff ? 0xffff : count;

        /* Set block length. I think this is only necessary for non-HCS cards.
         * HCS cards always use 512 bytes so we shouldn't need it.
         */
        if((d->driver_flags & MSC_DF_HCS_CARD) == 0)
            if(status = msc_cmd_set_block_len(d, SD_BLOCK_SIZE))
                goto _exit;

        /* TODO - look into using CMD23 to improve transfer performance.
         * This specifies the number of blocks ahead of time, instead of
         * relying on CMD12 to stop transmission. CMD12 is still needed
         * in the event of errors though.
         */
        msc_req req = {0};
        req.data = buf;
        req.nr_blocks = xfer_count;
        req.block_len = SD_BLOCK_SIZE;
        req.resptype = MSC_RESP_R1;
        req.flags = MSC_RF_DATA;
        if(xfer_count > 1)
            req.flags |= MSC_RF_AUTO_CMD12;
        if(write) {
            req.command = xfer_count == 1 ? SD_WRITE_BLOCK
                                          : SD_WRITE_MULTIPLE_BLOCK;
            req.flags |= MSC_RF_PROG | MSC_RF_WRITE;
        } else {
            req.command = xfer_count == 1 ? SD_READ_SINGLE_BLOCK
                                          : SD_READ_MULTIPLE_BLOCK;
        }

        if(d->driver_flags & MSC_DF_V2_CARD)
            req.argument = start;
        else
            req.argument = start * SD_BLOCK_SIZE;

        if(status = msc_cmd_exec(d, &req))
            goto _exit;

        /* TODO - properly handle reading the last block of the SD card
         * This is likely to fail if we're reading near the end because
         * the SD card will try to read past the last sector and then
         * signal an error. So we need to ignore that error, but only if
         * it was expected to occur. (See SD spec sec. 4.3.3, "Block Read")
         */
        if(status = msc_cmd_send_status(d))
            goto _exit;

        /* Advance the buffer and adjust start/count */
        buf += xfer_count * SD_BLOCK_SIZE;
        start += xfer_count;
        count -= xfer_count;
    } while(count > 0);

  _exit:
    msc_unlock(d);
    return status;
}

int sd_read_sectors(IF_MD(int drive,) unsigned long start,
                    int count, void* buf)
{
    return sd_transfer(sd_to_msc[IF_MD_DRV(drive)], false,
                       start, count, buf);
}

int sd_write_sectors(IF_MD(int drive,) unsigned long start,
                     int count, const void* buf)
{
    return sd_transfer(sd_to_msc[IF_MD_DRV(drive)], true,
                       start, count, (void*)buf);
}

tCardInfo* card_get_info_target(int card_nr)
{
    /* Defensive measures */
    if(card_nr < 0 || card_nr >= MSC_COUNT || sd_to_msc[card_nr] == NULL) {
        static tCardInfo null_info = { 0 };
        return &null_info;
    }

    return &sd_to_msc[card_nr]->cardinfo;
}

int sd_event(long id, intptr_t data)
{
    if(id == SYS_HOTSWAP_EXTRACTED) {
        msc_drv* d = msc_get_by_drive(data);
        if(d)
            msc_full_reset(d);
        return 0;
    } else {
        return storage_event_default_handler(id, data, _sd_last_disk_activity,
                                             STORAGE_SD);
    }
}

long sd_last_disk_activity(void)
{
    return _sd_last_disk_activity;
}

bool sd_present(IF_MD_NONVOID(int drive))
{
    /* Seems that volume_properties() in firmware/common/disk.c may pass
     * drive = -1 when the SD card is not inserted, so just return false.
     */
    if(drive < 0)
        return false;

    return sd_to_msc[IF_MD_DRV(drive)]->card_present;
}

bool sd_removable(IF_MD_NONVOID(int drive))
{
    /* Same reason as sd_present() */
    if(drive < 0)
        return false;

    return sd_to_msc[IF_MD_DRV(drive)]->config->cd_gpio != GPIO_NONE;
}

#ifndef CONFIG_STORAGE_MULTI
static
#endif
int sd_num_drives(int first_drive)
{
    int n = 0;
    for(; n < MSC_COUNT; ++n) {
        msc_drv* d = msc_get(MSC_TYPE_SD, n);
        if(d == NULL)
            break;

        d->drive_nr = first_drive + n;
        sd_to_msc[n] = d;
    }

    for(int i = n; i < MSC_COUNT; ++i)
        sd_to_msc[i] = NULL;

    return n;
}

int sd_init(void)
{
    msc_init();
#ifndef CONFIG_STORAGE_MULTI
    sd_num_drives(0);
#endif

    return 0;
}
