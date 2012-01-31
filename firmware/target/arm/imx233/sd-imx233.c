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
#include "config.h"
#include "system.h"
#include "sd.h"
#include "sdmmc.h"
#include "ssp-imx233.h"
#include "pinctrl-imx233.h"
#include "button-target.h"
#include "fat.h"
#include "disk.h"
#include "usb.h"
#include "debug.h"

/**
 * This code assumes a single SD card slot
 */

#ifdef SANSA_FUZEPLUS
#define SD_SSP      1
#else
#error You need to configure the ssp to use
#endif

static tCardInfo card_info;
static long sd_stack [(DEFAULT_STACK_SIZE*2 + 0x200)/sizeof(long)];
static struct mutex sd_mutex;
static const char sd_thread_name[] = "sd";
static struct event_queue sd_queue;
static int sd_first_drive;
static int last_disk_activity;

static void sd_detect_callback(int ssp)
{
    (void)ssp;
    /* This is called only if the state was stable for 300ms - check state
     * and post appropriate event. */
    if(imx233_ssp_sdmmc_detect(SD_SSP))
        queue_broadcast(SYS_HOTSWAP_INSERTED, 0);
    else
        queue_broadcast(SYS_HOTSWAP_EXTRACTED, 0);
    imx233_ssp_sdmmc_setup_detect(SD_SSP, true, sd_detect_callback, false);
}

void sd_power(bool on)
{
    #ifdef SANSA_FUZEPLUS
    /* The Fuze+ uses pin B0P8 for whatever reason, power ? */
    imx233_set_pin_function(0, 8, PINCTRL_FUNCTION_GPIO);
    imx233_enable_gpio_output(0, 8, true);
    imx233_set_gpio_output(0, 8, !on);
    /* disable pull ups when not needed to save power */
    imx233_ssp_setup_ssp1_sd_mmc_pins(on, 4, PINCTRL_DRIVE_4mA, false);
    #endif
}

void sd_enable(bool on)
{
    static int sd_enable = 2; /* 2 means not on and not off, for init purpose */
    if(sd_enable == on)
        return;

    sd_enable = on;
}

#define MCI_NO_RESP     0
#define MCI_RESP        (1<<0)
#define MCI_LONG_RESP   (1<<1)
#define MCI_ACMD        (1<<2)
#define MCI_NOCRC       (1<<3)
#define MCI_BUSY        (1<<4)

static bool send_cmd(uint8_t cmd, uint32_t arg, uint32_t flags, uint32_t *resp)
{
    if((flags & MCI_ACMD) && !send_cmd(SD_APP_CMD, card_info.rca, MCI_RESP, resp))
        return false;

    enum imx233_ssp_resp_t resp_type = (flags & MCI_LONG_RESP) ? SSP_LONG_RESP :
        (flags & MCI_RESP) ? SSP_SHORT_RESP : SSP_NO_RESP;
    enum imx233_ssp_error_t ret = imx233_ssp_sd_mmc_transfer(SD_SSP, cmd, arg,
        resp_type, NULL, 0, !!(flags & MCI_BUSY), false, resp);
    if(resp_type == SSP_LONG_RESP)
    {
        /* Our SD codes assume most significant word first, so reverse resp */
        uint32_t tmp = resp[0];
        resp[0] = resp[3];
        resp[3] = tmp;
        tmp = resp[1];
        resp[1] = resp[2];
        resp[2] = tmp;
    }
    return ret == SSP_SUCCESS;
}

static int sd_wait_for_tran_state(void)
{
    unsigned long response;
    unsigned int timeout = current_tick + 5*HZ;
    int cmd_retry = 10;
    
    while (1)
    {
        while(!send_cmd(SD_SEND_STATUS, card_info.rca, MCI_RESP, &response) && cmd_retry > 0)
            cmd_retry--;

        if(cmd_retry <= 0)
            return -1;

        if(((response >> 9) & 0xf) == SD_TRAN)
            return 0;

        if(TIME_AFTER(current_tick, timeout))
            return -10 * ((response >> 9) & 0xf);

        last_disk_activity = current_tick;
    }
}

static int sd_init_card(void)
{
    sd_enable(false);
    sd_power(false);
    sd_power(true);
    sd_enable(true);
    imx233_ssp_start(SD_SSP);
    imx233_ssp_softreset(SD_SSP);
    imx233_ssp_set_mode(SD_SSP, HW_SSP_CTRL1__SSP_MODE__SD_MMC);
    /* SSPCLK @ 96MHz
     * gives bitrate of 96000 / 240 / 1 = 400kHz */
    imx233_ssp_set_timings(SD_SSP, 240, 0, 0xffff);
    
    imx233_ssp_sd_mmc_power_up_sequence(SD_SSP);
    imx233_ssp_set_bus_width(SD_SSP, 1);
    imx233_ssp_set_block_size(SD_SSP, 9);

    card_info.rca = 0;
    bool sd_v2 = false;
    uint32_t resp;
    long init_timeout;
    /* go to idle state */
    if(!send_cmd(SD_GO_IDLE_STATE, 0, MCI_NO_RESP, NULL))
        return -1;
    /* CMD8 Check for v2 sd card.  Must be sent before using ACMD41
       Non v2 cards will not respond to this command */
    if(send_cmd(SD_SEND_IF_COND, 0x1AA, MCI_RESP, &resp))
        if((resp & 0xFFF) == 0x1AA)
            sd_v2 = true;
    /* timeout for initialization is 1sec, from SD Specification 2.00 */
    init_timeout = current_tick + HZ;
    do
    {
        /* this timeout is the only valid error for this loop*/
        if(TIME_AFTER(current_tick, init_timeout))
            return -2;

        /* ACMD41 For v2 cards set HCS bit[30] & send host voltage range to all */
        if(!send_cmd(SD_APP_OP_COND, (0x00FF8000 | (sd_v2 ? 1<<30 : 0)),
                MCI_ACMD|MCI_NOCRC|MCI_RESP, &card_info.ocr))
            return -100;
    } while(!(card_info.ocr & (1<<31)));

    /* CMD2 send CID */
    if(!send_cmd(SD_ALL_SEND_CID, 0, MCI_RESP|MCI_LONG_RESP, card_info.cid))
        return -3;

    /* CMD3 send RCA */
    if(!send_cmd(SD_SEND_RELATIVE_ADDR, 0, MCI_RESP, &card_info.rca))
        return -4;

    /* Try to switch V2 cards to HS timings, non HS seem to ignore this */
    if(sd_v2)
    {
        /*  CMD7 w/rca: Select card to put it in TRAN state */
        if(!send_cmd(SD_SELECT_CARD, card_info.rca, MCI_RESP, NULL))
            return -5;

        if(sd_wait_for_tran_state())
            return -6;

        /* CMD6 */
        if(!send_cmd(SD_SWITCH_FUNC, 0x80fffff1, MCI_NO_RESP, NULL))
            return -7;
        sleep(HZ/10);

        /*  go back to STBY state so we can read csd */
        /*  CMD7 w/rca=0:  Deselect card to put it in STBY state */
        if(!send_cmd(SD_DESELECT_CARD, 0, MCI_NO_RESP, NULL))
            return -8;
    }

    /* CMD9 send CSD */
    if(!send_cmd(SD_SEND_CSD, card_info.rca, MCI_RESP|MCI_LONG_RESP, card_info.csd))
        return -9;

    sd_parse_csd(&card_info);

    /* SSPCLK @ 96MHz
     * gives bitrate of 96 / 4 / 1 = 24MHz */
    imx233_ssp_set_timings(SD_SSP, 4, 0, 0xffff);

    /* CMD7 w/rca: Select card to put it in TRAN state */
    if(!send_cmd(SD_SELECT_CARD, card_info.rca, MCI_RESP, &resp))
        return -12;
    if(sd_wait_for_tran_state() < 0)
        return -13;

    /* ACMD6: set bus width to 4-bit */
    if(!send_cmd(SD_SET_BUS_WIDTH, 2, MCI_RESP|MCI_ACMD, &resp))
        return -15;
    /* ACMD42: disconnect the pull-up resistor on CD/DAT3 */
    if(!send_cmd(SD_SET_CLR_CARD_DETECT, 0, MCI_RESP|MCI_ACMD, &resp))
        return -17;

    /* Switch to 4-bit */
    imx233_ssp_set_bus_width(SD_SSP, 4);

    card_info.initialized = 1;
    
    return 0;
}

static void sd_thread(void) NORETURN_ATTR;
static void sd_thread(void)
{
    struct queue_event ev;

    while (1)
    {
        queue_wait_w_tmo(&sd_queue, &ev, HZ);

        switch(ev.id)
        {
        case SYS_HOTSWAP_INSERTED:
        case SYS_HOTSWAP_EXTRACTED:
        {
            int microsd_init = 1;
            fat_lock();          /* lock-out FAT activity first -
                                    prevent deadlocking via disk_mount that
                                    would cause a reverse-order attempt with
                                    another thread */
            mutex_lock(&sd_mutex); /* lock-out card activity - direct calls
                                    into driver that bypass the fat cache */

            /* We now have exclusive control of fat cache and sd */

            disk_unmount(sd_first_drive);     /* release "by force", ensure file
                                    descriptors aren't leaked and any busy
                                    ones are invalid if mounting */
            /* Force card init for new card, re-init for re-inserted one or
             * clear if the last attempt to init failed with an error. */
            card_info.initialized = 0;

            if(ev.id == SYS_HOTSWAP_INSERTED)
            {
                microsd_init = sd_init_card();
                if(microsd_init < 0) /* initialisation failed */
                    panicf("microSD init failed : %d", microsd_init);

                microsd_init = disk_mount(sd_first_drive); /* 0 if fail */
            }
            /*
             * Mount succeeded, or this was an EXTRACTED event,
             * in both cases notify the system about the changed filesystems
             */
            if(microsd_init)
                queue_broadcast(SYS_FS_CHANGED, 0);

            sd_enable(false);
            /* Access is now safe */
            mutex_unlock(&sd_mutex);
            fat_unlock();
            break;
        }
        case SYS_TIMEOUT:
            if(!TIME_BEFORE(current_tick, last_disk_activity+(3*HZ)))
                sd_enable(false);
            break;
        case SYS_USB_CONNECTED:
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            /* Wait until the USB cable is extracted again */
            usb_wait_for_disconnect(&sd_queue);
            break;
        }
    }
}

int sd_init(void)
{
    mutex_init(&sd_mutex);
    queue_init(&sd_queue, true);
    create_thread(sd_thread, sd_stack, sizeof(sd_stack), 0,
            sd_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));
    sd_enable(false);
    imx233_ssp_sdmmc_setup_detect(SD_SSP, true, sd_detect_callback, false);
    
    return 0;
}

static int transfer_sectors(IF_MD2(int drive,) unsigned long start, int count, void *buf, bool read)
{
    IF_MD((void) drive);
    int ret = 0;
    uint32_t resp;

    last_disk_activity = current_tick;

    mutex_lock(&sd_mutex);
    sd_enable(true);

    if(card_info.initialized <= 0)
    {
        ret = sd_init_card();
        if(card_info.initialized <= 0)
            goto Lend;
    }
    
    if(!send_cmd(SD_SELECT_CARD, card_info.rca, MCI_NO_RESP, NULL))
    {
        ret = -20;
        goto Lend;
    }
    ret = sd_wait_for_tran_state();
    if(ret < 0)
        goto Ldeselect;
    while(count != 0)
    {
        int this_count = MIN(count, IMX233_MAX_SSP_XFER_SIZE / 512);
        /* Set bank_start to the correct unit (blocks or bytes) */
        int bank_start = start;
        if(!(card_info.ocr & (1<<30)))   /* not SDHC */
            bank_start *= SD_BLOCK_SIZE;
        ret = imx233_ssp_sd_mmc_transfer(SD_SSP, read ? SD_READ_MULTIPLE_BLOCK : SD_WRITE_MULTIPLE_BLOCK,
            bank_start, SSP_SHORT_RESP, buf, this_count, false, read, &resp);
        if(ret != SSP_SUCCESS)
            break;
        if(!send_cmd(SD_STOP_TRANSMISSION, 0, MCI_RESP|MCI_BUSY, &resp))
        {
            ret = -15;
            break;
        }
        count -= this_count;
        start += this_count;
        buf += this_count * 512;
    }

    Ldeselect:
    /*  CMD7 w/rca =0 : deselects card & puts it in STBY state */
    if(!send_cmd(SD_DESELECT_CARD, 0, MCI_NO_RESP, NULL))
        ret = -23;
    Lend:
    mutex_unlock(&sd_mutex);
    return ret;
}

int sd_read_sectors(IF_MD2(int drive,) unsigned long start, int count,
                     void* buf)
{
    return transfer_sectors(IF_MD2(drive,) start, count, buf, true);
}

int sd_write_sectors(IF_MD2(int drive,) unsigned long start, int count,
                     const void* buf)
{
    return transfer_sectors(IF_MD2(drive,) start, count, (void *)buf, false);
}

tCardInfo *card_get_info_target(int card_no)
{
    (void)card_no;
    return &card_info;
}

int sd_num_drives(int first_drive)
{
    sd_first_drive = first_drive;
    return 1;
}

bool sd_present(IF_MD(int drive))
{
    IF_MD((void) drive);
    return imx233_ssp_sdmmc_detect(SD_SSP);
}

bool sd_removable(IF_MD(int drive))
{
    IF_MD((void) drive);
    return true;
}

long sd_last_disk_activity(void)
{
    return last_disk_activity;
}

