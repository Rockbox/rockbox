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
#include "mmc.h"
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
#include "ata_idle_notify.h"
#include "led.h"

/** NOTE For convenience, this drivers relies on the many similar commands
 * between SD and MMC. The following assumptions are made:
 * - SD_SEND_STATUS = MMC_SEND_STATUS
 * - SD_SELECT_CARD = MMC_SELECT_CARD
 * - SD_TRAN = MMC_TRAN
 * - MMC_WRITE_MULTIPLE_BLOCK = SD_WRITE_MULTIPLE_BLOCK
 * - MMC_READ_MULTIPLE_BLOCK = SD_READ_MULTIPLE_BLOCK
 * - SD_STOP_TRANSMISSION = MMC_STOP_TRANSMISSION
 * - SD_DESELECT_CARD = MMC_DESELECT_CARD
 */
#if SD_SEND_STATUS != MMC_SEND_STATUS || SD_SELECT_CARD != MMC_SELECT_CARD || \
    SD_TRAN != MMC_TRAN || MMC_WRITE_MULTIPLE_BLOCK != SD_WRITE_MULTIPLE_BLOCK || \
    MMC_READ_MULTIPLE_BLOCK != SD_READ_MULTIPLE_BLOCK || \
    SD_STOP_TRANSMISSION != MMC_STOP_TRANSMISSION || \
    SD_DESELECT_CARD != MMC_DESELECT_CARD
#error SD/MMC mismatch
#endif

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

/** WARNING
 * to be consistent with all our SD drivers, the .rca field of sdmmc_card_info
 * in reality holds (rca << 16) because all command arguments actually require
 * the RCA is the 16-bit msb. Be careful that this is not the actuall RCA ! */

/* common */
static unsigned window_start[SDMMC_NUM_DRIVES];
static unsigned window_end[SDMMC_NUM_DRIVES];
static uint8_t aligned_buffer[SDMMC_NUM_DRIVES][512] CACHEALIGN_ATTR;
static tCardInfo sdmmc_card_info[SDMMC_NUM_DRIVES];
static struct mutex mutex[SDMMC_NUM_DRIVES];
static int disk_last_activity[SDMMC_NUM_DRIVES];
#define MIN_YIELD_PERIOD 5  /* ticks */
static int next_yield = 0;

#define SDMMC_INFO(drive) sdmmc_card_info[drive]
#define SDMMC_RCA(drive) SDMMC_INFO(drive).rca

/* sd only */
static long sdmmc_stack[(DEFAULT_STACK_SIZE*2 + 0x200)/sizeof(long)];
static const char sdmmc_thread_name[] = "sdmmc";
static struct event_queue sdmmc_queue;
#if CONFIG_STORAGE & STORAGE_SD
static int sd_first_drive;
static unsigned _sd_num_drives;
static int sd_map[SDMMC_NUM_DRIVES]; /* sd->sdmmc map */
#endif
/* mmc only */
#if CONFIG_STORAGE & STORAGE_MMC
static int mmc_first_drive;
static unsigned _mmc_num_drives;
static int mmc_map[SDMMC_NUM_DRIVES]; /* mmc->sdmmc map */
#endif

static int init_drive(int drive);

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
#define MCI_ACMD        (1<<2) /* sd only */
#define MCI_NOCRC       (1<<3)
#define MCI_BUSY        (1<<4)

static bool send_cmd(int drive, uint8_t cmd, uint32_t arg, uint32_t flags, uint32_t *resp)
{
    if((flags & MCI_ACMD) && !send_cmd(drive, SD_APP_CMD, SDMMC_RCA(drive), MCI_RESP, resp))
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

static int wait_for_state(int drive, unsigned state)
{
    unsigned long response;
    unsigned int timeout = current_tick + 5*HZ;
    int cmd_retry = 10;
    
    while (1)
    {
        /* NOTE: rely on SD_SEND_STATUS=MMC_SEND_STATUS */
        while(!send_cmd(drive, SD_SEND_STATUS, SDMMC_RCA(drive), MCI_RESP, &response) && cmd_retry > 0)
            cmd_retry--;

        if(cmd_retry <= 0)
            return -1;

        if(((response >> 9) & 0xf) == state)
            return 0;

        if(TIME_AFTER(current_tick, timeout))
            return -10 * ((response >> 9) & 0xf);

        if(TIME_AFTER(current_tick, next_yield))
        {
            yield();
            next_yield = current_tick + MIN_YIELD_PERIOD;
        }
    }

    return 0;
}

#if CONFIG_STORAGE & STORAGE_SD
static int init_sd_card(int drive)
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
    if(!send_cmd(drive, SD_GO_IDLE_STATE, 0, MCI_NO_RESP, NULL))
        return -1;
    /* CMD8 Check for v2 sd card.  Must be sent before using ACMD41
       Non v2 cards will not respond to this command */
    if(send_cmd(drive, SD_SEND_IF_COND, 0x1AA, MCI_RESP, &resp))
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
        if(!send_cmd(drive, SD_APP_OP_COND, (0x00FF8000 | (sd_v2 ? 1<<30 : 0)),
                MCI_ACMD|MCI_NOCRC|MCI_RESP, &SDMMC_INFO(drive).ocr))
            return -100;
    } while(!(SDMMC_INFO(drive).ocr & (1<<31)));

    /* CMD2 send CID */
    if(!send_cmd(drive, SD_ALL_SEND_CID, 0, MCI_RESP|MCI_LONG_RESP, SDMMC_INFO(drive).cid))
        return -3;

    /* CMD3 send RCA */
    if(!send_cmd(drive, SD_SEND_RELATIVE_ADDR, 0, MCI_RESP, &SDMMC_INFO(drive).rca))
        return -4;

    /* Try to switch V2 cards to HS timings, non HS seem to ignore this */
    if(sd_v2)
    {
        /*  CMD7 w/rca: Select card to put it in TRAN state */
        if(!send_cmd(drive, SD_SELECT_CARD, SDMMC_RCA(drive), MCI_RESP, NULL))
            return -5;

        if(wait_for_state(drive, SD_TRAN))
            return -6;

        /* CMD6 */
        if(!send_cmd(drive, SD_SWITCH_FUNC, 0x80fffff1, MCI_NO_RESP, NULL))
            return -7;
        sleep(HZ/10);

        /*  go back to STBY state so we can read csd */
        /*  CMD7 w/rca=0:  Deselect card to put it in STBY state */
        if(!send_cmd(drive, SD_DESELECT_CARD, 0, MCI_NO_RESP, NULL))
            return -8;
    }

    /* CMD9 send CSD */
    if(!send_cmd(drive, SD_SEND_CSD, SDMMC_RCA(drive), MCI_RESP|MCI_LONG_RESP,
            SDMMC_INFO(drive).csd))
        return -9;

    sd_parse_csd(&SDMMC_INFO(drive));
    window_start[drive] = 0;
    window_end[drive] = SDMMC_INFO(drive).numblocks;

    /* SSPCLK @ 96MHz
     * gives bitrate of 96 / 4 / 1 = 24MHz */
    imx233_ssp_set_timings(ssp, 4, 0, 0xffff);

    /* CMD7 w/rca: Select card to put it in TRAN state */
    if(!send_cmd(drive, SD_SELECT_CARD, SDMMC_RCA(drive), MCI_RESP, &resp))
        return -12;
    if(wait_for_state(drive, SD_TRAN))
        return -13;

    /* ACMD6: set bus width to 4-bit */
    if(!send_cmd(drive, SD_SET_BUS_WIDTH, 2, MCI_RESP|MCI_ACMD, &resp))
        return -15;
    /* ACMD42: disconnect the pull-up resistor on CD/DAT3 */
    if(!send_cmd(drive, SD_SET_CLR_CARD_DETECT, 0, MCI_RESP|MCI_ACMD, &resp))
        return -17;

    /* Switch to 4-bit */
    imx233_ssp_set_bus_width(ssp, 4);

    SDMMC_INFO(drive).initialized = 1;

    return 0;
}
#endif

#if CONFIG_STORAGE & STORAGE_MMC
static int init_mmc_drive(int drive)
{
    int ssp = SDMMC_SSP(drive);
    /* we can choose the RCA of mmc cards: pick drive. Following our convention,
     * .rca is actually RCA << 16 */
    SDMMC_RCA(drive) = drive << 16;

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
    if(!send_cmd(drive, MMC_GO_IDLE_STATE, 0, MCI_NO_RESP, NULL))
        return -1;
    /* send op cond until the card respond with busy bit set; it must complete within 1sec */
    unsigned timeout = current_tick + HZ;
    bool ret = false;
    do
    {
        uint32_t ocr;
        ret = send_cmd(drive, MMC_SEND_OP_COND, 0x40ff8000, MCI_RESP, &ocr);
        if(ret && ocr & (1 << 31))
            break;
    }while(!TIME_AFTER(current_tick, timeout));

    if(!ret)
        return -2;
    /* get CID */
    uint32_t cid[4];
    if(!send_cmd(drive, MMC_ALL_SEND_CID, 0, MCI_LONG_RESP, cid))
        return -3;
    /* Set RCA */
    uint32_t status;
    if(!send_cmd(drive, MMC_SET_RELATIVE_ADDR, SDMMC_RCA(drive), MCI_RESP, &status))
        return -4;
    /* Select card */
    if(!send_cmd(drive, MMC_SELECT_CARD, SDMMC_RCA(drive), MCI_RESP, &status))
        return -5;
    /* Check TRAN state */
    if(wait_for_state(drive, MMC_TRAN))
        return -6;
    /* Switch to 8-bit bus */
    if(!send_cmd(drive, MMC_SWITCH, 0x3b70200, MCI_RESP|MCI_BUSY, &status))
        return -8;
    /* switch error ? */
    if(status & 0x80)
        return -9;
    imx233_ssp_set_bus_width(ssp, 8);
    /* Switch to high speed mode */
    if(!send_cmd(drive, MMC_SWITCH, 0x3b90100,  MCI_RESP|MCI_BUSY, &status))
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
        if(imx233_ssp_sd_mmc_transfer(ssp, 8, 0, SSP_SHORT_RESP, aligned_buffer[drive], 1, true, true, &status))
            return -12;
        memcpy(ext_csd, aligned_buffer[drive], 512);
        uint32_t *sec_count = (void *)&ext_csd[212];
        window_start[drive] = 0;
        window_end[drive] = *sec_count;
    }
    /* deselect card */
    if(!send_cmd(drive, MMC_DESELECT_CARD, 0, MCI_NO_RESP, NULL))
        return -13;

    return 0;
}
#endif

// low-level function, don't call directly!
static int __xfer_sectors(int drive, unsigned long start, int count, void *buf, bool read)
{
    uint32_t resp;
    int ret = 0;
    while(count != 0)
    {
        int this_count = MIN(count, IMX233_MAX_SINGLE_DMA_XFER_SIZE / 512);
        /* Set bank_start to the correct unit (blocks or bytes).
         * MMC drives use block addressing, SD cards bytes or blocks */
        int bank_start = start;
        if(SDMMC_MODE(drive) == SD_MODE && !(SDMMC_INFO(drive).ocr & (1<<30)))   /* not SDHC */
            bank_start *= SD_BLOCK_SIZE;
        /* issue read/write
         * NOTE: rely on SD_{READ,WRITE}_MULTIPLE_BLOCK=MMC_{READ,WRITE}_MULTIPLE_BLOCK */
        ret = imx233_ssp_sd_mmc_transfer(SDMMC_SSP(drive),
            read ? SD_READ_MULTIPLE_BLOCK : SD_WRITE_MULTIPLE_BLOCK,
            bank_start, SSP_SHORT_RESP, buf, this_count, false, read, &resp);
        if(ret != SSP_SUCCESS)
            break;
        /* stop transmission
         * NOTE: rely on SD_STOP_TRANSMISSION=MMC_STOP_TRANSMISSION */
        if(!send_cmd(drive, SD_STOP_TRANSMISSION, 0, MCI_RESP|MCI_BUSY, &resp))
        {
            ret = -15;
            break;
        }
        count -= this_count;
        start += this_count;
        buf += this_count * 512;
    }
    return ret;
}

static int transfer_sectors(int drive, unsigned long start, int count, void *buf, bool read)
{
    int ret = 0;

    /* update disk activity */
    disk_last_activity[drive] = current_tick;

    /* lock per-drive mutex */
    mutex_lock(&mutex[drive]);

    /* update led status */
    led(true);

    /* for SD cards, init if necessary */
#if CONFIG_STORAGE & STORAGE_SD
    if(SDMMC_MODE(drive) == SD_MODE && SDMMC_INFO(drive).initialized <= 0)
    {
        ret = init_drive(drive);
        if(SDMMC_INFO(drive).initialized <= 0)
            goto Lend;
    }
#endif

    /* check window */
    start += window_start[drive];
    if((start + count) > window_end[drive])
    {
        ret = -201;
        goto Lend;
    }
    /* select card.
     * NOTE: rely on SD_SELECT_CARD=MMC_SELECT_CARD */
    if(!send_cmd(drive, SD_SELECT_CARD, SDMMC_RCA(drive), MCI_NO_RESP, NULL))
    {
        ret = -20;
        goto Lend;
    }
    /* wait for TRAN state */
    /* NOTE: rely on SD_TRAN=MMC_TRAN */
    ret = wait_for_state(drive, SD_TRAN);
    if(ret < 0)
        goto Ldeselect;

    /**
     * NOTE: we need to make sure dma transfers are aligned. This is handled
     * differently for read and write transfers. We do not repeat it each
     * time but it should be noted that all transfers are limited by
     * IMX233_MAX_SINGLE_DMA_XFER_SIZE and thus need to be split if needed.
     *
     * Read transfers:
     *   If the buffer is already aligned, transfer everything at once.
     *   Otherwise, transfer all sectors but one to the sub-buffer starting
     *   on the next cache ligned and then move the data. Then transfer the
     *   last sector to the aligned_buffer and then copy to the buffer.
     *
     * Write transfers:
     *   If the buffer is already aligned, transfer everything at once.
     *   Otherwise, copy the first sector to the aligned_buffer and transfer.
     *   Then move all other sectors within the buffer to make it cache
     *   aligned and transfer it.
     */
    if(read)
    {
        void *ptr = CACHEALIGN_UP(buf);
        if(buf != ptr)
        {
            // copy count-1 sector and then move within the buffer
            ret = __xfer_sectors(drive, start, count - 1, ptr, read);
            memmove(buf, ptr, 512 * (count - 1));
            if(ret >= 0)
            {
                // transfer the last sector the aligned_buffer and copy
                ret = __xfer_sectors(drive, start + count - 1, 1,
                    aligned_buffer[drive], read);
                memcpy(buf + 512 * (count - 1), aligned_buffer[drive], 512);
            }
        }
        else
            ret = __xfer_sectors(drive, start, count, buf, read);
    }
    else
    {
        void *ptr = CACHEALIGN_UP(buf);
        if(buf != ptr)
        {
            // transfer the first sector to aligned_buffer and copy
            memcpy(aligned_buffer[drive], buf, 512);
            ret = __xfer_sectors(drive, start, 1, aligned_buffer[drive], read);
            if(ret >= 0)
            {
                // move within the buffer and transfer
                memmove(ptr, buf + 512, 512 * (count - 1));
                ret = __xfer_sectors(drive, start + 1, count - 1, ptr, read);
            }
        }
        else
            ret = __xfer_sectors(drive, start, count, buf, read);
    }
    
    /* deselect card */
    Ldeselect:
    /*  CMD7 w/rca =0 : deselects card & puts it in STBY state
     * NOTE: rely on SD_DESELECT_CARD=MMC_DESELECT_CARD */
    if(!send_cmd(drive, SD_DESELECT_CARD, 0, MCI_NO_RESP, NULL))
        ret = -23;
    Lend:
    /* update led status */
    led(false);
    /* release per-drive mutex */
    mutex_unlock(&mutex[drive]);
    return ret;
}

static int init_drive(int drive)
{
    int ret;
    switch(SDMMC_MODE(drive))
    {
#if CONFIG_STORAGE & STORAGE_SD
        case SD_MODE: ret = init_sd_card(drive); break;
#endif
#if CONFIG_STORAGE & STORAGE_MMC
        case MMC_MODE: ret = init_mmc_drive(drive); break;
#endif
        default: ret = 0;
    }
    if(ret < 0)
        return ret;

    /* compute window */
    if((SDMMC_FLAGS(drive) & WINDOW) && imx233_partitions_is_window_enabled())
    {
        uint8_t mbr[512];
        int ret = transfer_sectors(drive, 0, 1, mbr, true);
        if(ret)
            panicf("Cannot read MBR: %d", ret);
        ret = imx233_partitions_compute_window(mbr, &window_start[drive],
            &window_end[drive]);
        if(ret)
            panicf("cannot compute partitions window: %d", ret);
        SDMMC_INFO(drive).numblocks = window_end[drive] - window_start[drive];
    }

    return 0;
}

static void sdmmc_thread(void) NORETURN_ATTR;
static void sdmmc_thread(void)
{
    struct queue_event ev;
    bool idle_notified = false;
    int timeout = 0;

    while (1)
    {
        queue_wait_w_tmo(&sdmmc_queue, &ev, HZ);

        switch(ev.id)
        {
#if CONFIG_STORAGE & STORAGE_SD
        case SYS_HOTSWAP_INSERTED:
        case SYS_HOTSWAP_EXTRACTED:
        {
            int microsd_init = 1;
             /* lock-out FAT activity first -
              * prevent deadlocking via disk_mount that
              * would cause a reverse-order attempt with
              * another thread */
            fat_lock();

            /* We now have exclusive control of fat cache and sd.
             * Release "by force", ensure file
             * descriptors aren't leaked and any busy
             * ones are invalid if mounting. */
            for(unsigned sd_drive = 0; sd_drive < _sd_num_drives; sd_drive++)
            {
                int drive = sd_map[sd_drive];
                /* Skip non-removable drivers */
                if(!sd_removable(sd_drive))
                    continue;
                /* lock-out card activity - direct calls
                 * into driver that bypass the fat cache */
                mutex_lock(&mutex[drive]);
                disk_unmount(sd_first_drive + sd_drive);
                /* Force card init for new card, re-init for re-inserted one or
                 * clear if the last attempt to init failed with an error. */
                SDMMC_INFO(sd_map[sd_drive]).initialized = 0;

                if(ev.id == SYS_HOTSWAP_INSERTED)
                {
                    microsd_init = init_drive(drive);
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
                /* unlock card */
                mutex_unlock(&mutex[drive]);
            }
            /* Access is now safe */
            fat_unlock();
            break;
        }
#endif
        case SYS_TIMEOUT:
#if CONFIG_STORAGE & STORAGE_SD
            timeout = MAX(timeout, sd_last_disk_activity()+(3*HZ));
#endif
#if CONFIG_STORAGE & STORAGE_MMC
            timeout = MAX(timeout, mmc_last_disk_activity()+(3*HZ));
#endif
            if(TIME_BEFORE(current_tick, timeout))
            {
                idle_notified = false;
            }
            else
            {
                next_yield = current_tick;

                if(!idle_notified)
                {
                    call_storage_idle_notifys(false);
                    idle_notified = true;
                }
            }
            break;
            break;
        case SYS_USB_CONNECTED:
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            /* Wait until the USB cable is extracted again */
            usb_wait_for_disconnect(&sdmmc_queue);
            break;
        }
    }
}

static int sdmmc_init(void)
{
    static int is_initialized = false;
    if(is_initialized)
        return 0;
    is_initialized = true;
    for(unsigned drive = 0; drive < SDMMC_NUM_DRIVES; drive++)
        mutex_init(&mutex[drive]);

    queue_init(&sdmmc_queue, true);
    create_thread(sdmmc_thread, sdmmc_stack, sizeof(sdmmc_stack), 0,
            sdmmc_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));

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
    long last = 0;
    for(unsigned i = 0; i < _sd_num_drives; i++)
        last = MAX(last, disk_last_activity[sd_map[i]]);
    return last;
}

void sd_enable(bool on)
{
    (void) on;
}

int sd_read_sectors(IF_MD2(int sd_drive,) unsigned long start, int count, void *buf)
{
    return transfer_sectors(sd_map[sd_drive], start, count, buf, true);
}

int sd_write_sectors(IF_MD2(int sd_drive,) unsigned long start, int count, const void* buf)
{
    return transfer_sectors(sd_map[sd_drive], start, count, (void *)buf, false);
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
            init_drive(drive);
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
    long last = 0;
    for(unsigned i = 0; i < _mmc_num_drives; i++)
        last = MAX(last, disk_last_activity[mmc_map[i]]);
    return last;
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
    return transfer_sectors(mmc_map[mmc_drive], start, count, buf, true);
}

int mmc_write_sectors(IF_MD2(int mmc_drive,) unsigned long start, int count, const void* buf)
{
    return transfer_sectors(mmc_map[mmc_drive], start, count, (void *)buf, false);
}

#endif
