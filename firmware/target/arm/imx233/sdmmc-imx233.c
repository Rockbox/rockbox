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
#include "partitions-imx233.h"
#include "button-target.h"
#include "fat.h"
#include "disk.h"
#include "usb.h"
#include "debug.h"
#include "string.h"

struct sdmmc_config_t
{
    const char *name; /* name(for debug) */
    int flags; /* flags */
    int power_pin; /* power pin */
    int power_delay; /* extra power up delay */
    int ssp; /* associated ssp block */
    int mode; /* mode (SD vs MMC) */
};

/* flags */
#define POWER_PIN       (1 << 0)
#define POWER_INVERTED  (1 << 1)
#define REMOVABLE       (1 << 2)
#define DETECT_INVERTED (1 << 3)
#define POWER_DELAY     (1 << 4)
#define WINDOW          (1 << 5)

/* modes */
#define SD_MODE         0
#define MMC_MODE        1

#define PIN(bank,pin) ((bank) << 5 | (pin))
#define PIN2BANK(v) ((v) >> 5)
#define PIN2PIN(v) ((v) & 0x1f)

struct sdmmc_config_t sdmmc_config[] =
{
#ifdef SANSA_FUZEPLUS
    /* The Fuze+ uses pin #B0P8 for power */
    {
        .name = "microSD",
        .flags = POWER_PIN | POWER_INVERTED | REMOVABLE,
        .power_pin = PIN(0, 8),
        .ssp = 1,
        .mode = SD_MODE,
    },
    /* The Fuze+ uses pin #B1P29 for power */
    {
        .name = "eMMC",
        .flags = POWER_PIN | POWER_INVERTED | WINDOW | POWER_DELAY,
        .power_pin = PIN(1, 29),
        .power_delay = HZ / 5, /* extra delay, to ramp up voltage? */
        .ssp = 2,
        .mode = MMC_MODE,
    },
#elif defined(CREATIVE_ZENXFI2)
    /* The Zen X-Fi2 uses pin B1P29 for power*/
    {
        .name = "microSD",
        .flags = POWER_PIN | REMOVABLE | DETECT_INVERTED,
        .power_pin = PIN(1, 29),
        .ssp = 1,
        .mode = SD_MODE,
    },
#elif defined(CREATIVE_ZENXFI3)
    {
        .name = "internal/SD",
        .flags = WINDOW,
        .ssp = 2,
        .mode = SD_MODE,
    },
    /* The Zen X-Fi3 uses pin #B0P07 for power*/
    {
        .name = "microSD",
        .flags = POWER_PIN | POWER_INVERTED | REMOVABLE | POWER_DELAY,
        .power_pin = PIN(0, 7),
        .power_delay = HZ / 10, /* extra delay, to ramp up voltage? */
        .ssp = 1,
        .mode = SD_MODE,
    },
#else
#error You need to write the sd/mmc config!
#endif
};

#define SDMMC_NUM_DRIVES    (sizeof(sdmmc_config) / sizeof(sdmmc_config[0]))

#define SDMMC_CONF(drive) sdmmc_config[drive]
#define SDMMC_FLAGS(drive) SDMMC_CONF(drive).flags
#define SDMMC_SSP(drive) SDMMC_CONF(drive).ssp
#define SDMMC_MODE(drive) SDMMC_CONF(drive).mode

/* common */
static unsigned window_start[SDMMC_NUM_DRIVES];
static unsigned window_end[SDMMC_NUM_DRIVES];
static uint8_t aligned_buffer[SDMMC_NUM_DRIVES][512] CACHEALIGN_ATTR;
static tCardInfo sdmmc_card_info[SDMMC_NUM_DRIVES];

#define SDMMC_INFO(drive) sdmmc_card_info[drive]
#define SDMMC_RCA(drive) SDMMC_INFO(drive).rca

/* sd only */
#if CONFIG_STORAGE & STORAGE_SD
static long sd_stack[(DEFAULT_STACK_SIZE*2 + 0x200)/sizeof(long)];
static struct mutex sd_mutex;
static const char sd_thread_name[] = "sd";
static struct event_queue sd_queue;
static int sd_first_drive;
static unsigned _sd_num_drives;
static int _sd_last_disk_activity;
static int sd_map[SDMMC_NUM_DRIVES]; /* sd->sdmmc map */
#endif
/* mmc only */
#if CONFIG_STORAGE & STORAGE_MMC
static int mmc_first_drive;
static unsigned _mmc_num_drives;
static int _mmc_last_disk_activity;
static int mmc_map[SDMMC_NUM_DRIVES]; /* mmc->sdmmc map */
#endif

/* WARNING NOTE BUG FIXME
 * There are three numbering schemes involved in the driver:
 * - the sdmmc indexes into sdmmc_config[]
 * - the sd drive indexes
 * - the mmc drive indexes
 * By convention, [drive] refers to a sdmmc index whereas sd_drive/mmc_drive
 * refer to sd/mmc drive indexes. We keep two maps sd->sdmmc and mmc->sdmmc
 * to find the sdmmc index from the sd or mmc one */

static void sdmmc_detect_callback(int ssp)
{
    /* This is called only if the state was stable for 300ms - check state
     * and post appropriate event. */
    if(imx233_ssp_sdmmc_detect(ssp))
        queue_broadcast(SYS_HOTSWAP_INSERTED, 0);
    else
        queue_broadcast(SYS_HOTSWAP_EXTRACTED, 0);
    imx233_ssp_sdmmc_setup_detect(ssp, true, sdmmc_detect_callback, false,
        imx233_ssp_sdmmc_is_detect_inverted(ssp));
}

static void sdmmc_power(int drive, bool on)
{
    /* power chip if needed */
    if(SDMMC_FLAGS(drive) & POWER_PIN)
    {
        int bank = PIN2BANK(SDMMC_CONF(drive).power_pin);
        int pin = PIN2PIN(SDMMC_CONF(drive).power_pin);
        imx233_pinctrl_acquire_pin(bank, pin, "sd/mmc power");
        imx233_set_pin_function(bank, pin, PINCTRL_FUNCTION_GPIO);
        imx233_enable_gpio_output(bank, pin, true);
        if(SDMMC_FLAGS(drive) & POWER_INVERTED)
            imx233_set_gpio_output(bank, pin, !on);
        else
            imx233_set_gpio_output(bank, pin, on);
    }
    if(SDMMC_FLAGS(drive) & POWER_DELAY)
        sleep(SDMMC_CONF(drive).power_delay);
    /* setup pins, never use alternatives pin on SSP1 because no device use it
     * but this could be made a flag */
    int bus_width = SDMMC_MODE(drive) == MMC_MODE ? 8 : 4;
    if(SDMMC_SSP(drive) == 1)
        imx233_ssp_setup_ssp1_sd_mmc_pins(on, bus_width, PINCTRL_DRIVE_4mA, false);
    else
        imx233_ssp_setup_ssp2_sd_mmc_pins(on, bus_width, PINCTRL_DRIVE_4mA);
}

#define MCI_NO_RESP     0
#define MCI_RESP        (1<<0)
#define MCI_LONG_RESP   (1<<1)
#define MCI_ACMD        (1<<2)
#define MCI_NOCRC       (1<<3)
#define MCI_BUSY        (1<<4)

static bool send_sd_cmd(int drive, uint8_t cmd, uint32_t arg, uint32_t flags, uint32_t *resp)
{
    if((flags & MCI_ACMD) && !send_sd_cmd(drive, SD_APP_CMD, SDMMC_RCA(drive), MCI_RESP, resp))
        return false;

    enum imx233_ssp_resp_t resp_type = (flags & MCI_LONG_RESP) ? SSP_LONG_RESP :
        (flags & MCI_RESP) ? SSP_SHORT_RESP : SSP_NO_RESP;
    enum imx233_ssp_error_t ret = imx233_ssp_sd_mmc_transfer(SDMMC_SSP(drive), cmd,
        arg, resp_type, NULL, 0, !!(flags & MCI_BUSY), false, resp);
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

static int wait_for_sd_tran_state(int drive)
{
    unsigned long response;
    unsigned int timeout = current_tick + 5*HZ;
    int cmd_retry = 10;
    
    while (1)
    {
        while(!send_sd_cmd(drive, SD_SEND_STATUS, SDMMC_RCA(drive), MCI_RESP, &response) && cmd_retry > 0)
            cmd_retry--;

        if(cmd_retry <= 0)
            return -1;

        if(((response >> 9) & 0xf) == SD_TRAN)
            return 0;

        if(TIME_AFTER(current_tick, timeout))
            return -10 * ((response >> 9) & 0xf);

        _sd_last_disk_activity = current_tick;
    }

    return 0;
}

#if CONFIG_STORAGE & STORAGE_SD
static int sdmmc_init_sd_card(int drive)
{
    int ssp = SDMMC_SSP(drive);
    sdmmc_power(drive, false);
    sdmmc_power(drive, true);
    imx233_ssp_start(ssp);
    imx233_ssp_softreset(ssp);
    imx233_ssp_set_mode(ssp, HW_SSP_CTRL1__SSP_MODE__SD_MMC);
    /* SSPCLK @ 96MHz
     * gives bitrate of 96000 / 240 / 1 = 400kHz */
    imx233_ssp_set_timings(ssp, 240, 0, 0xffff);

    imx233_ssp_sd_mmc_power_up_sequence(ssp);
    imx233_ssp_set_bus_width(ssp, 1);
    imx233_ssp_set_block_size(ssp, 9);

    SDMMC_RCA(drive) = 0;
    bool sd_v2 = false;
    uint32_t resp;
    long init_timeout;
    /* go to idle state */
    if(!send_sd_cmd(drive, SD_GO_IDLE_STATE, 0, MCI_NO_RESP, NULL))
        return -1;
    /* CMD8 Check for v2 sd card.  Must be sent before using ACMD41
       Non v2 cards will not respond to this command */
    if(send_sd_cmd(drive, SD_SEND_IF_COND, 0x1AA, MCI_RESP, &resp))
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
        if(!send_sd_cmd(drive, SD_APP_OP_COND, (0x00FF8000 | (sd_v2 ? 1<<30 : 0)),
                MCI_ACMD|MCI_NOCRC|MCI_RESP, &SDMMC_INFO(drive).ocr))
            return -100;
    } while(!(SDMMC_INFO(drive).ocr & (1<<31)));

    /* CMD2 send CID */
    if(!send_sd_cmd(drive, SD_ALL_SEND_CID, 0, MCI_RESP|MCI_LONG_RESP, SDMMC_INFO(drive).cid))
        return -3;

    /* CMD3 send RCA */
    if(!send_sd_cmd(drive, SD_SEND_RELATIVE_ADDR, 0, MCI_RESP, &SDMMC_INFO(drive).rca))
        return -4;

    /* Try to switch V2 cards to HS timings, non HS seem to ignore this */
    if(sd_v2)
    {
        /*  CMD7 w/rca: Select card to put it in TRAN state */
        if(!send_sd_cmd(drive, SD_SELECT_CARD, SDMMC_RCA(drive), MCI_RESP, NULL))
            return -5;

        if(wait_for_sd_tran_state(drive))
            return -6;

        /* CMD6 */
        if(!send_sd_cmd(drive, SD_SWITCH_FUNC, 0x80fffff1, MCI_NO_RESP, NULL))
            return -7;
        sleep(HZ/10);

        /*  go back to STBY state so we can read csd */
        /*  CMD7 w/rca=0:  Deselect card to put it in STBY state */
        if(!send_sd_cmd(drive, SD_DESELECT_CARD, 0, MCI_NO_RESP, NULL))
            return -8;
    }

    /* CMD9 send CSD */
    if(!send_sd_cmd(drive, SD_SEND_CSD, SDMMC_RCA(drive), MCI_RESP|MCI_LONG_RESP,
            SDMMC_INFO(drive).csd))
        return -9;

    sd_parse_csd(&SDMMC_INFO(drive));
    window_start[drive] = 0;
    window_end[drive] = SDMMC_INFO(drive).numblocks;

    /* SSPCLK @ 96MHz
     * gives bitrate of 96 / 4 / 1 = 24MHz */
    imx233_ssp_set_timings(ssp, 4, 0, 0xffff);

    /* CMD7 w/rca: Select card to put it in TRAN state */
    if(!send_sd_cmd(drive, SD_SELECT_CARD, SDMMC_RCA(drive), MCI_RESP, &resp))
        return -12;
    if(wait_for_sd_tran_state(drive) < 0)
        return -13;

    /* ACMD6: set bus width to 4-bit */
    if(!send_sd_cmd(drive, SD_SET_BUS_WIDTH, 2, MCI_RESP|MCI_ACMD, &resp))
        return -15;
    /* ACMD42: disconnect the pull-up resistor on CD/DAT3 */
    if(!send_sd_cmd(drive, SD_SET_CLR_CARD_DETECT, 0, MCI_RESP|MCI_ACMD, &resp))
        return -17;

    /* Switch to 4-bit */
    imx233_ssp_set_bus_width(ssp, 4);

    SDMMC_INFO(drive).initialized = 1;

    return 0;
}

static int transfer_sd_sectors(int drive, unsigned long start, int count, void *buf, bool read)
{
    int ret = 0;
    uint32_t resp;

    _sd_last_disk_activity = current_tick;

    mutex_lock(&sd_mutex);

    if(SDMMC_INFO(drive).initialized <= 0)
    {
        ret = sdmmc_init_sd_card(drive);
        if(SDMMC_INFO(drive).initialized <= 0)
            goto Lend;
    }

    /* check window */
    start += window_start[drive];
    if((start + count) > window_end[drive])
    {
        ret = -201;
        goto Lend;
    }

    if(!send_sd_cmd(drive, SD_SELECT_CARD, SDMMC_RCA(drive), MCI_NO_RESP, NULL))
    {
        ret = -20;
        goto Lend;
    }
    ret = wait_for_sd_tran_state(drive);
    if(ret < 0)
        goto Ldeselect;
    while(count != 0)
    {
        /* FIXME implement this_count > 1 by using a sub-buffer of [sub] that is
         * cache-aligned and then moving the data when possible. This way we could
         * transfer much greater amount of data at once */
        int this_count = 1;
        /* Set bank_start to the correct unit (blocks or bytes) */
        int bank_start = start;
        if(!(SDMMC_INFO(drive).ocr & (1<<30)))   /* not SDHC */
            bank_start *= SD_BLOCK_SIZE;
        if(!read)
            memcpy(aligned_buffer[drive], buf, 512);
        ret = imx233_ssp_sd_mmc_transfer(SDMMC_SSP(drive),
            read ? SD_READ_MULTIPLE_BLOCK : SD_WRITE_MULTIPLE_BLOCK,
            bank_start, SSP_SHORT_RESP, aligned_buffer[drive], this_count, false, read, &resp);
        if(ret != SSP_SUCCESS)
            break;
        if(!send_sd_cmd(drive, SD_STOP_TRANSMISSION, 0, MCI_RESP|MCI_BUSY, &resp))
        {
            ret = -15;
            break;
        }
        if(read)
            memcpy(buf, aligned_buffer[drive], 512);
        count -= this_count;
        start += this_count;
        buf += this_count * 512;
    }

    Ldeselect:
    /*  CMD7 w/rca =0 : deselects card & puts it in STBY state */
    if(!send_sd_cmd(drive, SD_DESELECT_CARD, 0, MCI_NO_RESP, NULL))
        ret = -23;
    Lend:
    mutex_unlock(&sd_mutex);
    return ret;
}
#endif

#if CONFIG_STORAGE & STORAGE_MMC
static int transfer_mmc_sectors(int drive, unsigned long start, int count, void *buf, bool read)
{
    /* check window */
    start += window_start[drive];
    if((start + count) > window_end[drive])
        return -201;
    int ret = 0;
    uint32_t resp;

    _mmc_last_disk_activity = current_tick;

    do
    {
        /* FIXME implement this_count > 1 by using a sub-buffer of [sub] that is
         * cache-aligned and then moving the data when possible. This way we could
         * transfer much greater amount of data at once */
        int this_count = 1;
        if(!read)
            memcpy(aligned_buffer[drive], buf, 512);
        ret = imx233_ssp_sd_mmc_transfer(SDMMC_SSP(drive), read ? 17 : 24, start,
                SSP_SHORT_RESP, aligned_buffer[drive], this_count, false, read, &resp);
        if(read)
            memcpy(buf, aligned_buffer[drive], 512);
        count -= this_count;
        start += this_count;
        buf += this_count * 512;
    }while(count != 0 && ret == SSP_SUCCESS);

    return ret;
}

static int sdmmc_init_mmc_drive(int drive)
{
    int ssp = SDMMC_SSP(drive);
    // we can choose the RCA of mmc cards: pick drive
    SDMMC_RCA(drive) = drive;

    sdmmc_power(drive, false);
    sdmmc_power(drive, true);
    imx233_ssp_start(ssp);
    imx233_ssp_softreset(ssp);
    imx233_ssp_set_mode(ssp, HW_SSP_CTRL1__SSP_MODE__SD_MMC);
    /* SSPCLK @ 96MHz
     * gives bitrate of 96000 / 240 / 1 = 400kHz */
    imx233_ssp_set_timings(ssp, 240, 0, 0xffff);
    imx233_ssp_sd_mmc_power_up_sequence(ssp);
    imx233_ssp_set_bus_width(ssp, 1);
    imx233_ssp_set_block_size(ssp, 9);
    /* go to idle state */
    int ret = imx233_ssp_sd_mmc_transfer(ssp, 0, 0, SSP_NO_RESP, NULL, 0, false, false, NULL);
    if(ret != 0)
        return -1;
    /* send op cond until the card respond with busy bit set; it must complete within 1sec */
    unsigned timeout = current_tick + HZ;
    do
    {
        uint32_t ocr;
        ret = imx233_ssp_sd_mmc_transfer(ssp, 1, 0x40ff8000, SSP_SHORT_RESP, NULL, 0, false, false, &ocr);
        if(ret == 0 && ocr & (1 << 31))
            break;
    }while(!TIME_AFTER(current_tick, timeout));

    if(ret != 0)
        return -2;
    /* get CID */
    uint32_t cid[4];
    ret = imx233_ssp_sd_mmc_transfer(ssp, 2, 0, SSP_LONG_RESP, NULL, 0, false, false, cid);
    if(ret != 0)
        return -3;
    /* Set RCA */
    uint32_t status;
    ret = imx233_ssp_sd_mmc_transfer(ssp, 3, SDMMC_RCA(drive) << 16, SSP_SHORT_RESP, NULL, 0, false, false, &status);
    if(ret != 0)
        return -4;
    /* Select card */
    ret = imx233_ssp_sd_mmc_transfer(ssp, 7, SDMMC_RCA(drive) << 16, SSP_SHORT_RESP, NULL, 0, false, false, &status);
    if(ret != 0)
        return -5;
    /* Check TRAN state */
    ret = imx233_ssp_sd_mmc_transfer(ssp, 13, SDMMC_RCA(drive) << 16, SSP_SHORT_RESP, NULL, 0, false, false, &status);
    if(ret != 0)
        return -6;
    if(((status >> 9) & 0xf) != 4)
        return -7;
    /* Switch to 8-bit bus */
    ret = imx233_ssp_sd_mmc_transfer(ssp, 6, 0x3b70200, SSP_SHORT_RESP, NULL, 0, true, false, &status);
    if(ret != 0)
        return -8;
    /* switch error ? */
    if(status & 0x80)
        return -9;
    imx233_ssp_set_bus_width(ssp, 8);
    /* Switch to high speed mode */
    ret = imx233_ssp_sd_mmc_transfer(ssp, 6, 0x3b90100, SSP_SHORT_RESP, NULL, 0, true, false, &status);
    if(ret != 0)
        return -10;
    /* switch error ?*/
    if(status & 0x80)
        return -11;
    /* SSPCLK @ 96MHz
     * gives bitrate of 96 / 2 / 1 = 48MHz */
    imx233_ssp_set_timings(ssp, 2, 0, 0xffff);

    /* read extended CSD */
    {
        uint8_t ext_csd[512];
        ret = imx233_ssp_sd_mmc_transfer(ssp, 8, 0, SSP_SHORT_RESP, aligned_buffer[drive], 1, true, true, &status);
        if(ret != 0)
            return -12;
        memcpy(ext_csd, aligned_buffer[drive], 512);
        uint32_t *sec_count = (void *)&ext_csd[212];
        window_start[drive] = 0;
        window_end[drive] = *sec_count;
    }

    return 0;
}
#endif

static int sdmmc_read_sectors(int drive, unsigned long start, int count, void* buf)
{
    switch(SDMMC_MODE(drive))
    {
#if CONFIG_STORAGE & STORAGE_SD
        case SD_MODE: return transfer_sd_sectors(drive, start, count, buf, true);
#endif
#if CONFIG_STORAGE & STORAGE_MMC
        case MMC_MODE: return transfer_mmc_sectors(drive, start, count, buf, true);
#endif
        default: return -1;
    }
}

static int sdmmc_write_sectors(int drive, unsigned long start, int count, const void* buf)
{
    switch(SDMMC_MODE(drive))
    {
#if CONFIG_STORAGE & STORAGE_SD
        case SD_MODE: return transfer_sd_sectors(drive, start, count, (void *)buf, false);
#endif
#if CONFIG_STORAGE & STORAGE_MMC
        case MMC_MODE: return transfer_mmc_sectors(drive, start, count, (void *)buf, false);
#endif
        default: return -1;
    }
}

static int sdmmc_init_drive(int drive)
{
    int ret;
    switch(SDMMC_MODE(drive))
    {
#if CONFIG_STORAGE & STORAGE_SD
        case SD_MODE: ret = sdmmc_init_sd_card(drive); break;
#endif
#if CONFIG_STORAGE & STORAGE_MMC
        case MMC_MODE: ret = sdmmc_init_mmc_drive(drive); break;
#endif
        default: ret = 0;
    }
    if(ret < 0)
        return ret;

    /* compute window */
    if((SDMMC_FLAGS(drive) & WINDOW) && imx233_partitions_is_window_enabled())
    {
        uint8_t mbr[512];
        int ret = sdmmc_read_sectors(IF_MD2(drive,) 0, 1, mbr);
        if(ret)
            panicf("Cannot read MBR: %d", ret);
        ret = imx233_partitions_compute_window(mbr, &window_start[drive],
            &window_end[drive]);
        if(ret)
            panicf("cannot compute partitions window: %d", ret);
        if(SDMMC_MODE(drive) == SD_MODE)
            SDMMC_INFO(drive).numblocks = window_end[drive] - window_start[drive];
    }

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
             /* lock-out FAT activity first -
              * prevent deadlocking via disk_mount that
              * would cause a reverse-order attempt with
              * another thread */
            fat_lock();
            /* lock-out card activity - direct calls
             * into driver that bypass the fat cache */
            mutex_lock(&sd_mutex);

            /* We now have exclusive control of fat cache and sd.
             * Release "by force", ensure file
             * descriptors aren't leaked and any busy
             * ones are invalid if mounting. */
            for(unsigned sd_drive = 0; sd_drive < _sd_num_drives; sd_drive++)
            {
                /* Skip non-removable drivers */
                if(!sd_removable(sd_drive))
                    continue;
                disk_unmount(sd_first_drive + sd_drive);
                /* Force card init for new card, re-init for re-inserted one or
                 * clear if the last attempt to init failed with an error. */
                SDMMC_INFO(sd_map[sd_drive]).initialized = 0;

                if(ev.id == SYS_HOTSWAP_INSERTED)
                {
                    microsd_init = sdmmc_init_drive(sd_map[sd_drive]);
                    if(microsd_init < 0) /* initialisation failed */
                        panicf("%s init failed : %d", SDMMC_CONF(sd_map[sd_drive]).name, microsd_init);

                    microsd_init = disk_mount(sd_first_drive + sd_drive); /* 0 if fail */
                }
                /*
                * Mount succeeded, or this was an EXTRACTED event,
                * in both cases notify the system about the changed filesystems
                */
                if(microsd_init)
                    queue_broadcast(SYS_FS_CHANGED, 0);
            }
            /* Access is now safe */
            mutex_unlock(&sd_mutex);
            fat_unlock();
            break;
        }
        case SYS_TIMEOUT:
            if(!TIME_BEFORE(current_tick, _sd_last_disk_activity + 3 * HZ))
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

int sdmmc_init(void)
{
    static int is_initialized = false;
    if(is_initialized)
        return 0;
    is_initialized = true;

#if CONFIG_STORAGE & STORAGE_SD
    mutex_init(&sd_mutex);
    queue_init(&sd_queue, true);
    create_thread(sd_thread, sd_stack, sizeof(sd_stack), 0,
            sd_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));
#endif

    for(unsigned drive = 0; drive < SDMMC_NUM_DRIVES; drive++)
    {
        if(SDMMC_FLAGS(drive) & REMOVABLE)
            imx233_ssp_sdmmc_setup_detect(SDMMC_SSP(drive), true, sdmmc_detect_callback,
                false, SDMMC_FLAGS(drive) & DETECT_INVERTED);
    }
    
    return 0;
}

static int sdmmc_present(int drive)
{
    if(SDMMC_FLAGS(drive) & REMOVABLE)
        return imx233_ssp_sdmmc_detect(SDMMC_SSP(drive));
    else
        return true;
}

static inline int sdmmc_removable(int drive)
{
    return SDMMC_FLAGS(drive) & REMOVABLE;
}

#if CONFIG_STORAGE & STORAGE_SD
int sd_init(void)
{
    int ret = sdmmc_init();
    if(ret < 0) return ret;

    _sd_num_drives = 0;
    for(unsigned drive = 0; drive < SDMMC_NUM_DRIVES; drive++)
        if(SDMMC_MODE(drive) == SD_MODE)
            sd_map[_sd_num_drives++] = drive;
    return 0;
}

tCardInfo *card_get_info_target(int sd_card_no)
{
    return &SDMMC_INFO(sd_map[sd_card_no]);
}

int sd_num_drives(int first_drive)
{
    sd_first_drive = first_drive;
    return _sd_num_drives;
}

bool sd_present(IF_MV_NONVOID(int sd_drive))
{
    return sdmmc_present(sd_map[sd_drive]);
}

bool sd_removable(IF_MV_NONVOID(int sd_drive))
{
    return sdmmc_removable(sd_map[sd_drive]);
}

long sd_last_disk_activity(void)
{
    return _sd_last_disk_activity;
}

void sd_enable(bool on)
{
    (void) on;
}

int sd_read_sectors(IF_MD2(int sd_drive,) unsigned long start, int count, void *buf)
{
    return sdmmc_read_sectors(sd_map[sd_drive], start, count, buf);
}

int sd_write_sectors(IF_MD2(int sd_drive,) unsigned long start, int count, const void* buf)
{
    return sdmmc_write_sectors(sd_map[sd_drive], start, count, buf);
}
#endif

#if CONFIG_STORAGE & STORAGE_MMC
int mmc_init(void)
{
    int ret = sdmmc_init();
    if(ret < 0) return ret;

    _sd_num_drives = 0;
    for(unsigned drive = 0; drive < SDMMC_NUM_DRIVES; drive++)
        if(SDMMC_MODE(drive) == MMC_MODE)
        {
            mmc_map[_mmc_num_drives++] = drive;
            sdmmc_init_drive(drive);
        }
    return 0;
}

void mmc_get_info(IF_MD2(int mmc_drive,) struct storage_info *info)
{
    int drive = mmc_map[mmc_drive];
    info->sector_size = 512;
    info->num_sectors = window_end[drive] - window_start[drive];
    info->vendor = "Rockbox";
    info->product = "Internal Storage";
    info->revision = "0.00";
}

int mmc_num_drives(int first_drive)
{
    mmc_first_drive = first_drive;
    return _mmc_num_drives;
}

bool mmc_present(IF_MV_NONVOID(int mmc_drive))
{
    return sdmmc_present(mmc_map[mmc_drive]);
}

bool mmc_removable(IF_MV_NONVOID(int mmc_drive))
{
    return sdmmc_removable(mmc_map[mmc_drive]);
}

long mmc_last_disk_activity(void)
{
    return _mmc_last_disk_activity;
}

void mmc_enable(bool on)
{
    (void) on;
}

void mmc_sleep(void)
{
}

void mmc_sleepnow(void)
{
}

bool mmc_disk_is_active(void)
{
    return false;
}

bool mmc_usb_active(void)
{
    return mmc_disk_is_active();
}

int mmc_soft_reset(void)
{
    return 0;
}

int mmc_flush(void)
{
    return 0;
}

void mmc_spin(void)
{
}

void mmc_spindown(int seconds)
{
    (void) seconds;
}

int mmc_spinup_time(void)
{
    return 0;
}

int mmc_read_sectors(IF_MD2(int mmc_drive,) unsigned long start, int count, void *buf)
{
    return sdmmc_read_sectors(mmc_map[mmc_drive], start, count, buf);
}

int mmc_write_sectors(IF_MD2(int mmc_drive,) unsigned long start, int count, const void* buf)
{
    return sdmmc_write_sectors(mmc_map[mmc_drive], start, count, buf);
}

#endif
