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

struct sd_config_t
{
    const char *name; /* name(for debug) */
    int flags; /* flags */
    int power_pin; /* power pin */
    int power_delay; /* extra power up delay */
    int ssp; /* associated ssp block */
};

/* flags */
#define POWER_PIN       (1 << 0)
#define POWER_INVERTED  (1 << 1)
#define REMOVABLE       (1 << 2)
#define DETECT_INVERTED (1 << 3)
#define POWER_DELAY     (1 << 4)
#define WINDOW          (1 << 5)

#define PIN(bank,pin) ((bank) << 5 | (pin))
#define PIN2BANK(v) ((v) >> 5)
#define PIN2PIN(v) ((v) & 0x1f)

struct sd_config_t sd_config[] =
{
#ifdef SANSA_FUZEPLUS
    /* The Fuze+ uses pin #B0P8 for power */
    {
        .name = "microSD",
        .flags = POWER_PIN | POWER_INVERTED | REMOVABLE,
        .power_pin = PIN(0, 8),
        .ssp = 1
    },
#elif defined(CREATIVE_ZENXFI2)
    /* The Zen X-Fi2 uses pin B1P29 for power*/
    {
        .name = "microSD",
        .flags = POWER_PIN | REMOVABLE | DETECT_INVERTED,
        .power_pin = PIN(1, 29),
        .ssp = 1,
    },
#elif defined(CREATIVE_ZENXFI3)
    {
        .name = "internal/SD",
        .flags = WINDOW,
        .ssp = 2
    },
    /* The Zen X-Fi3 uses pin #B0P07 for power*/
    {
        .name = "microSD",
        .flags = POWER_PIN | POWER_INVERTED | REMOVABLE | POWER_DELAY,
        .power_pin = PIN(0, 7),
        .power_delay = HZ / 10, /* extra delay, to ramp up voltage? */
        .ssp = 1
    },
#else
#error You need to write the sd config!
#endif
};

#define SD_NUM_DRIVES   (sizeof(sd_config) / sizeof(sd_config[0]))

#define SD_CONF(drive) sd_config[drive]
#define SD_FLAGS(drive) SD_CONF(drive).flags
#define SD_SSP(drive) SD_CONF(drive).ssp
#define IF_FIRST_DRIVE(drive) if((drive) == 0)
#define IF_SECOND_DRIVE(drive) if((drive) == 1)

static tCardInfo card_info[SD_NUM_DRIVES];
static long sd_stack[(DEFAULT_STACK_SIZE*2 + 0x200)/sizeof(long)];
static struct mutex sd_mutex;
static const char sd_thread_name[] = "sd";
static struct event_queue sd_queue;
static int sd_first_drive;
static int last_disk_activity;
static unsigned sd_window_start[SD_NUM_DRIVES];
static unsigned sd_window_end[SD_NUM_DRIVES];

static void sd_detect_callback(int ssp)
{
    /* This is called only if the state was stable for 300ms - check state
     * and post appropriate event. */
    if(imx233_ssp_sdmmc_detect(ssp))
        queue_broadcast(SYS_HOTSWAP_INSERTED, 0);
    else
        queue_broadcast(SYS_HOTSWAP_EXTRACTED, 0);
    imx233_ssp_sdmmc_setup_detect(ssp, true, sd_detect_callback, false,
        imx233_ssp_sdmmc_is_detect_inverted(ssp));
}

void sd_power(int drive, bool on)
{
    /* power chip if needed */
    if(SD_FLAGS(drive) & POWER_PIN)
    {
        int bank = PIN2BANK(SD_CONF(drive).power_pin);
        int pin = PIN2PIN(SD_CONF(drive).power_pin);
        imx233_pinctrl_acquire_pin(bank, pin, "sd power");
        imx233_set_pin_function(bank, pin, PINCTRL_FUNCTION_GPIO);
        imx233_enable_gpio_output(bank, pin, true);
        if(SD_FLAGS(drive) & POWER_INVERTED)
            imx233_set_gpio_output(bank, pin, !on);
        else
            imx233_set_gpio_output(bank, pin, on);
    }
    if(SD_FLAGS(drive) & POWER_DELAY)
        sleep(SD_CONF(drive).power_delay);
    /* setup pins, never use alternatives pin on SSP1 because these are force
     * bus width >= 4 and SD cannot use more than 4 data lines. */
    if(SD_SSP(drive) == 1)
        imx233_ssp_setup_ssp1_sd_mmc_pins(on, 4, PINCTRL_DRIVE_4mA, false);
    else
        imx233_ssp_setup_ssp2_sd_mmc_pins(on, 4, PINCTRL_DRIVE_4mA);
}

void sd_enable(bool on)
{
    (void) on;
}

#define MCI_NO_RESP     0
#define MCI_RESP        (1<<0)
#define MCI_LONG_RESP   (1<<1)
#define MCI_ACMD        (1<<2)
#define MCI_NOCRC       (1<<3)
#define MCI_BUSY        (1<<4)

static bool send_cmd(int drive, uint8_t cmd, uint32_t arg, uint32_t flags, uint32_t *resp)
{
    if((flags & MCI_ACMD) && !send_cmd(drive, SD_APP_CMD, card_info[drive].rca, MCI_RESP, resp))
        return false;

    enum imx233_ssp_resp_t resp_type = (flags & MCI_LONG_RESP) ? SSP_LONG_RESP :
        (flags & MCI_RESP) ? SSP_SHORT_RESP : SSP_NO_RESP;
    enum imx233_ssp_error_t ret = imx233_ssp_sd_mmc_transfer(SD_SSP(drive), cmd,
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

static int sd_wait_for_tran_state(int drive)
{
    unsigned long response;
    unsigned int timeout = current_tick + 5*HZ;
    int cmd_retry = 10;
    
    while (1)
    {
        while(!send_cmd(drive, SD_SEND_STATUS, card_info[drive].rca, MCI_RESP, &response) && cmd_retry > 0)
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

static int sd_init_card(int drive)
{
    /* sanity check against bad configuration of SD_NUM_DRIVES/NUM_DRIVES */
    if((unsigned)drive >= SD_NUM_DRIVES)
        panicf("drive >= SD_NUM_DRIVES in sd_init_card!");
    int ssp = SD_SSP(drive);
    sd_power(drive, false);
    sd_power(drive, true);
    imx233_ssp_start(ssp);
    imx233_ssp_softreset(ssp);
    imx233_ssp_set_mode(ssp, HW_SSP_CTRL1__SSP_MODE__SD_MMC);
    /* SSPCLK @ 96MHz
     * gives bitrate of 96000 / 240 / 1 = 400kHz */
    imx233_ssp_set_timings(ssp, 240, 0, 0xffff);
    
    imx233_ssp_sd_mmc_power_up_sequence(ssp);
    imx233_ssp_set_bus_width(ssp, 1);
    imx233_ssp_set_block_size(ssp, 9);

    card_info[drive].rca = 0;
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
                MCI_ACMD|MCI_NOCRC|MCI_RESP, &card_info[drive].ocr))
            return -100;
    } while(!(card_info[drive].ocr & (1<<31)));

    /* CMD2 send CID */
    if(!send_cmd(drive, SD_ALL_SEND_CID, 0, MCI_RESP|MCI_LONG_RESP, card_info[drive].cid))
        return -3;

    /* CMD3 send RCA */
    if(!send_cmd(drive, SD_SEND_RELATIVE_ADDR, 0, MCI_RESP, &card_info[drive].rca))
        return -4;

    /* Try to switch V2 cards to HS timings, non HS seem to ignore this */
    if(sd_v2)
    {
        /*  CMD7 w/rca: Select card to put it in TRAN state */
        if(!send_cmd(drive, SD_SELECT_CARD, card_info[drive].rca, MCI_RESP, NULL))
            return -5;

        if(sd_wait_for_tran_state(drive))
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
    if(!send_cmd(drive, SD_SEND_CSD, card_info[drive].rca, MCI_RESP|MCI_LONG_RESP, card_info[drive].csd))
        return -9;

    sd_parse_csd(&card_info[drive]);

    /* SSPCLK @ 96MHz
     * gives bitrate of 96 / 4 / 1 = 24MHz */
    imx233_ssp_set_timings(ssp, 4, 0, 0xffff);

    /* CMD7 w/rca: Select card to put it in TRAN state */
    if(!send_cmd(drive, SD_SELECT_CARD, card_info[drive].rca, MCI_RESP, &resp))
        return -12;
    if(sd_wait_for_tran_state(drive) < 0)
        return -13;

    /* ACMD6: set bus width to 4-bit */
    if(!send_cmd(drive, SD_SET_BUS_WIDTH, 2, MCI_RESP|MCI_ACMD, &resp))
        return -15;
    /* ACMD42: disconnect the pull-up resistor on CD/DAT3 */
    if(!send_cmd(drive, SD_SET_CLR_CARD_DETECT, 0, MCI_RESP|MCI_ACMD, &resp))
        return -17;

    /* Switch to 4-bit */
    imx233_ssp_set_bus_width(ssp, 4);

    card_info[drive].initialized = 1;

    /* compute window */
    sd_window_start[drive] = 0;
    sd_window_end[drive] = card_info[drive].numblocks;
    if((SD_FLAGS(drive) & WINDOW) && imx233_partitions_is_window_enabled())
    {
        /* WARNING: sd_first_drive is not set at this point */
        uint8_t mbr[512];
        int ret = sd_read_sectors(IF_MD2(drive,) 0, 1, mbr);
        if(ret)
            panicf("Cannot read MBR: %d", ret);
        ret = imx233_partitions_compute_window(mbr, &sd_window_start[drive],
            &sd_window_end[drive]);
        if(ret)
            panicf("cannot compute partitions window: %d", ret);
        card_info[drive].numblocks = sd_window_end[drive] - sd_window_start[drive];
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
            for(unsigned drive = 0; drive < SD_NUM_DRIVES; drive++)
            {
                /* Skip non-removable drivers */
                if(!sd_removable(drive))
                    continue;
                disk_unmount(sd_first_drive + drive);
                /* Force card init for new card, re-init for re-inserted one or
                * clear if the last attempt to init failed with an error. */
                card_info[drive].initialized = 0;

                if(ev.id == SYS_HOTSWAP_INSERTED)
                {
                    microsd_init = sd_init_card(drive);
                    if(microsd_init < 0) /* initialisation failed */
                        panicf("%s init failed : %d", SD_CONF(drive).name, microsd_init);

                    microsd_init = disk_mount(sd_first_drive + drive); /* 0 if fail */
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
            if(!TIME_BEFORE(current_tick, last_disk_activity +3 * HZ))
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

    for(unsigned drive = 0; drive < SD_NUM_DRIVES; drive++)
    {
        if(SD_FLAGS(drive) & REMOVABLE)
            imx233_ssp_sdmmc_setup_detect(SD_SSP(drive), true, sd_detect_callback,
                false, SD_FLAGS(drive) & DETECT_INVERTED);
    }
    
    return 0;
}

static int transfer_sectors(IF_MD2(int drive,) unsigned long start, int count, void *buf, bool read)
{
    int ret = 0;
    uint32_t resp;

    last_disk_activity = current_tick;

    mutex_lock(&sd_mutex);

    if(card_info[drive].initialized <= 0)
    {
        ret = sd_init_card(drive);
        if(card_info[drive].initialized <= 0)
            goto Lend;
    }

    /* check window */
    start += sd_window_start[drive];
    if((start + count) >= sd_window_end[drive])
    {
        ret = -201;
        goto Lend;
    }
    
    if(!send_cmd(drive, SD_SELECT_CARD, card_info[drive].rca, MCI_NO_RESP, NULL))
    {
        ret = -20;
        goto Lend;
    }
    ret = sd_wait_for_tran_state(drive);
    if(ret < 0)
        goto Ldeselect;
    while(count != 0)
    {
        int this_count = MIN(count, IMX233_MAX_SSP_XFER_SIZE / 512);
        /* Set bank_start to the correct unit (blocks or bytes) */
        int bank_start = start;
        if(!(card_info[drive].ocr & (1<<30)))   /* not SDHC */
            bank_start *= SD_BLOCK_SIZE;
        ret = imx233_ssp_sd_mmc_transfer(SD_SSP(drive),
            read ? SD_READ_MULTIPLE_BLOCK : SD_WRITE_MULTIPLE_BLOCK,
            bank_start, SSP_SHORT_RESP, buf, this_count, false, read, &resp);
        if(ret != SSP_SUCCESS)
            break;
        if(!send_cmd(drive, SD_STOP_TRANSMISSION, 0, MCI_RESP|MCI_BUSY, &resp))
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
    if(!send_cmd(drive, SD_DESELECT_CARD, 0, MCI_NO_RESP, NULL))
        ret = -23;
    Lend:
    mutex_unlock(&sd_mutex);
    return ret;
}

int sd_read_sectors(IF_MD2(int drive,) unsigned long start, int count, void* buf)
{
    return transfer_sectors(IF_MD2(drive,) start, count, buf, true);
}

int sd_write_sectors(IF_MD2(int drive,) unsigned long start, int count, const void* buf)
{
    return transfer_sectors(IF_MD2(drive,) start, count, (void *)buf, false);
}

tCardInfo *card_get_info_target(int card_no)
{
    return &card_info[card_no];
}

int sd_num_drives(int first_drive)
{
    sd_first_drive = first_drive;
    return SD_NUM_DRIVES;
}

bool sd_present(IF_MV_NONVOID(int drive))
{
    if(SD_FLAGS(drive) & REMOVABLE)
        return imx233_ssp_sdmmc_detect(SD_SSP(drive));
    else
        return true;
}

bool sd_removable(IF_MV_NONVOID(int drive))
{
    return SD_FLAGS(drive) & REMOVABLE;
}

long sd_last_disk_activity(void)
{
    return last_disk_activity;
}
