/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Daniel Ankers
 * Copyright © 2008-2009 Rafaël Carré
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

/* Driver for the ARM PL180 SD/MMC controller inside AS3525 SoC */

#include "config.h" /* for HAVE_MULTIDRIVE & AMS_OF_SIZE */
#include "fat.h"
#include "thread.h"
#include "led.h"
#include "sdmmc.h"
#include "system.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gcc_extensions.h"
#include "as3525.h"
#include "pl180.h"  /* SD controller */
#include "pl081.h"  /* DMA controller */
#include "dma-target.h" /* DMA request lines */
#include "clock-target.h"
#include "panic.h"
#ifdef HAVE_BUTTON_LIGHT
#include "backlight-target.h"
#endif
#include "stdbool.h"
#include "ata_idle_notify.h"
#include "sd.h"
#include "usb.h"
/*#define LOGF_ENABLE*/
#include "logf.h"

#ifdef HAVE_HOTSWAP
#include "disk.h"
#endif

//#define VERIFY_WRITE 1

/* command flags */
#define MCI_NO_RESP     (0<<0)
#define MCI_RESP        (1<<0)
#define MCI_LONG_RESP   (1<<1)
#define MCI_ACMD        (1<<2)
#define MCI_NOCRC       (1<<3)

/* ARM PL180 registers */
#define MCI_POWER(i)       (*(volatile unsigned char *) (pl180_base[i]+0x00))
#define MCI_CLOCK(i)       (*(volatile unsigned long *) (pl180_base[i]+0x04))
#define MCI_ARGUMENT(i)    (*(volatile unsigned long *) (pl180_base[i]+0x08))
#define MCI_COMMAND(i)     (*(volatile unsigned long *) (pl180_base[i]+0x0C))
#define MCI_RESPCMD(i)     (*(volatile unsigned long *) (pl180_base[i]+0x10))
#define MCI_RESP0(i)       (*(volatile unsigned long *) (pl180_base[i]+0x14))
#define MCI_RESP1(i)       (*(volatile unsigned long *) (pl180_base[i]+0x18))
#define MCI_RESP2(i)       (*(volatile unsigned long *) (pl180_base[i]+0x1C))
#define MCI_RESP3(i)       (*(volatile unsigned long *) (pl180_base[i]+0x20))
#define MCI_DATA_TIMER(i)  (*(volatile unsigned long *) (pl180_base[i]+0x24))
#define MCI_DATA_LENGTH(i) (*(volatile unsigned short*) (pl180_base[i]+0x28))
#define MCI_DATA_CTRL(i)   (*(volatile unsigned char *) (pl180_base[i]+0x2C))
#define MCI_DATA_CNT(i)    (*(volatile unsigned short*) (pl180_base[i]+0x30))
#define MCI_STATUS(i)      (*(volatile unsigned long *) (pl180_base[i]+0x34))
#define MCI_CLEAR(i)       (*(volatile unsigned long *) (pl180_base[i]+0x38))
#define MCI_MASK0(i)       (*(volatile unsigned long *) (pl180_base[i]+0x3C))
#define MCI_MASK1(i)       (*(volatile unsigned long *) (pl180_base[i]+0x40))
#define MCI_SELECT(i)      (*(volatile unsigned long *) (pl180_base[i]+0x44))
#define MCI_FIFO_CNT(i)    (*(volatile unsigned long *) (pl180_base[i]+0x48))

#define MCI_DATA_ERROR    \
    ( MCI_DATA_CRC_FAIL   \
    | MCI_DATA_TIMEOUT    \
    | MCI_TX_UNDERRUN     \
    | MCI_RX_OVERRUN      \
    | MCI_START_BIT_ERR)

#define MCI_RESPONSE_ERROR    \
    ( MCI_CMD_TIMEOUT         \
    | MCI_CMD_CRC_FAIL)

#define MCI_FIFO(i)        ((unsigned long *) (pl180_base[i]+0x80))
/* volumes */
#define     INTERNAL_AS3525 0   /* embedded SD card */
#define     SD_SLOT_AS3525   1   /* SD slot if present */

static const int pl180_base[NUM_DRIVES] = {
            NAND_FLASH_BASE
#ifdef HAVE_MULTIDRIVE
            , SD_MCI_BASE
#endif
};

static int sd_wait_for_tran_state(const int drive);
static int sd_select_bank(signed char bank);
static int sd_init_card(const int drive);
static void init_pl180_controller(const int drive);

#define BLOCKS_PER_BANK 0x7a7800u

static tCardInfo card_info[NUM_DRIVES];

/* maximum timeouts recommanded in the SD Specification v2.00 */
#define SD_MAX_READ_TIMEOUT     ((AS3525_PCLK_FREQ) / 1000 * 100) /* 100 ms */
#define SD_MAX_WRITE_TIMEOUT    ((AS3525_PCLK_FREQ) / 1000 * 250) /* 250 ms */

/* for compatibility */
static long last_disk_activity = -1;

#define MIN_YIELD_PERIOD 5  /* ticks */
static long next_yield = 0;

static long sd_stack [(DEFAULT_STACK_SIZE*2 + 0x200)/sizeof(long)];
static const char         sd_thread_name[] = "ata/sd";
static struct mutex       sd_mtx;
static struct event_queue sd_queue;
bool sd_enabled = false;

#if defined(HAVE_MULTIDRIVE)
static bool hs_card = false;
#define EXT_SD_BITS (1<<2)
#endif

static struct wakeup transfer_completion_signal;
static volatile unsigned int transfer_error[NUM_VOLUMES];
#define PL180_MAX_TRANSFER_ERRORS 10

#define UNALIGNED_NUM_SECTORS 10
static unsigned char aligned_buffer[UNALIGNED_NUM_SECTORS* SD_BLOCK_SIZE] __attribute__((aligned(32)));   /* align on cache line size */
static unsigned char *uncached_buffer = AS3525_UNCACHED_ADDR(&aligned_buffer[0]);


static inline void mci_delay(void) { udelay(1000) ; }


static inline bool card_detect_target(void)
{
#if defined(HAVE_MULTIDRIVE)
    return !(GPIOA_PIN(2));
#else
    return false;
#endif
}


#ifdef HAVE_HOTSWAP
static int sd1_oneshot_callback(struct timeout *tmo)
{
    (void)tmo;

    /* This is called only if the state was stable for 300ms - check state
     * and post appropriate event. */
    if (card_detect_target())
    {
        queue_broadcast(SYS_HOTSWAP_INSERTED, 0);
    }
    else
        queue_broadcast(SYS_HOTSWAP_EXTRACTED, 0);

    return 0;
}

void sd_gpioa_isr(void)
{
    static struct timeout sd1_oneshot;
    if (GPIOA_MIS & EXT_SD_BITS)
        timeout_register(&sd1_oneshot, sd1_oneshot_callback, (3*HZ/10), 0);
    /* acknowledge interrupt */
    GPIOA_IC = EXT_SD_BITS;
}
#endif  /* HAVE_HOTSWAP */

void INT_NAND(void)
{
    const int status = MCI_STATUS(INTERNAL_AS3525);

    transfer_error[INTERNAL_AS3525] = status & MCI_DATA_ERROR;

    wakeup_signal(&transfer_completion_signal);
    MCI_CLEAR(INTERNAL_AS3525) = status;
}

#ifdef HAVE_MULTIDRIVE
void INT_MCI0(void)
{
    const int status = MCI_STATUS(SD_SLOT_AS3525);

    transfer_error[SD_SLOT_AS3525] = status & MCI_DATA_ERROR;

    wakeup_signal(&transfer_completion_signal);
    MCI_CLEAR(SD_SLOT_AS3525) = status;
}
#endif

static bool send_cmd(const int drive, const int cmd, const int arg,
                     const int flags, long *response)
{
    int status;

    unsigned cmd_retries = 6;
    while(cmd_retries--)
    {
        if ((flags & MCI_ACMD) && /* send SD_APP_CMD before each try */
            !send_cmd(drive, SD_APP_CMD, card_info[drive].rca, MCI_RESP, response))
            return false;

        /* Clear old status flags */
        MCI_CLEAR(drive) = 0x7ff;

        /* Load command argument or clear if none */
        MCI_ARGUMENT(drive) = arg;

        /* Construct MCI_COMMAND & enable CPSM */
        MCI_COMMAND(drive) =
           /*b0:5*/  cmd
           /* b6 */| ((flags & (MCI_RESP|MCI_LONG_RESP)) ? MCI_COMMAND_RESPONSE : 0)
           /* b7 */| ((flags & MCI_LONG_RESP) ? MCI_COMMAND_LONG_RESPONSE : 0)
           /* b8   | MCI_COMMAND_INTERRUPT */
           /* b9   | MCI_COMMAND_PENDING */  /*Only used with stream data transfer*/
           /* b10*/| MCI_COMMAND_ENABLE;     /* Enables CPSM */

        /* Wait while cmd completes then disable CPSM */
        while(MCI_STATUS(drive) & MCI_CMD_ACTIVE);
        MCI_COMMAND(drive) = 0;

        status = MCI_STATUS(drive);

        /*  Handle command responses */
        if(flags & MCI_RESP)                /* CMD expects response */
        {
            response[0] = MCI_RESP0(drive); /* Always prepare short response */

            if(status & MCI_RESPONSE_ERROR) {/* timeout or crc failure */
                if ((status & MCI_CMD_CRC_FAIL) &&
                    (flags & MCI_NOCRC))
                    break;
                logf("sd cmd error: drive %d cmd %d arg %08x sd_status %08x resp0 %08lx",
                     drive, cmd, arg, status, response[0]);
                continue;
            }

            if((flags & MCI_RESP) &&
               !(flags & MCI_LONG_RESP) &&
               (response[0] & SD_R1_CARD_ERROR)) {
                logf("sd card error: drive %d cmd %d arg %08x r1 %08lx",
                     drive, cmd, arg, response[0]);
            }

            if(status & MCI_CMD_RESP_END)   /* Response passed CRC check */
            {
                if(flags & MCI_LONG_RESP)
                {   /* response[0] has already been read */
                    response[1] = MCI_RESP1(drive);
                    response[2] = MCI_RESP2(drive);
                    response[3] = MCI_RESP3(drive);
                }
                return true;
            }
        }
        else if(status & MCI_CMD_SENT)  /* CMD sent, no response required */
            return true;
    }

    return false;
}

#define MCI_FULLSPEED     (MCI_CLOCK_ENABLE | MCI_CLOCK_BYPASS)     /* MCLK   */
#define MCI_HALFSPEED     (MCI_CLOCK_ENABLE)                        /* MCLK/2 */
#define MCI_QUARTERSPEED  (MCI_CLOCK_ENABLE | 1)                    /* MCLK/4 */
#define MCI_IDENTSPEED    (MCI_CLOCK_ENABLE | AS3525_SD_IDENT_DIV)  /* IDENT  */

static int sd_init_card(const int drive)
{
    unsigned long response;
    long init_timeout;
    bool sd_v2 = false;

    card_info[drive].rca = 0;

    /* MCLCK on and set to 400kHz ident frequency  */
    MCI_CLOCK(drive) = MCI_IDENTSPEED;

    /* 100 - 400kHz clock required for Identification Mode  */
    /*  Start of Card Identification Mode ************************************/

    /* CMD0 Go Idle  */
    if(!send_cmd(drive, SD_GO_IDLE_STATE, 0, MCI_NO_RESP, NULL))
        return -1;
    mci_delay();

    /* CMD8 Check for v2 sd card.  Must be sent before using ACMD41
      Non v2 cards will not respond to this command*/
    if(send_cmd(drive, SD_SEND_IF_COND, 0x1AA, MCI_RESP, &response))
        if((response & 0xFFF) == 0x1AA)
            sd_v2 = true;

    /* timeout for initialization is 1sec, from SD Specification 2.00 */
    init_timeout = current_tick + HZ;

    do {
        /* this timeout is the only valid error for this loop*/
        if(TIME_AFTER(current_tick, init_timeout))
            return -2;

        /* ACMD41 For v2 cards set HCS bit[30] & send host voltage range to all */
        send_cmd(drive, SD_APP_OP_COND, (0x00FF8000 | (sd_v2 ? 1<<30 : 0)),
                        MCI_ACMD|MCI_NOCRC|MCI_RESP, &card_info[drive].ocr);

    } while(!(card_info[drive].ocr & (1<<31)));

    /* CMD2 send CID */
    if(!send_cmd(drive, SD_ALL_SEND_CID, 0, MCI_RESP|MCI_LONG_RESP,
                            card_info[drive].cid))
        return -3;

    /* CMD3 send RCA */
    if(!send_cmd(drive, SD_SEND_RELATIVE_ADDR, 0, MCI_RESP,
                &card_info[drive].rca))
        return -4;

    /*  End of Card Identification Mode   ************************************/

#ifdef HAVE_MULTIDRIVE        /*  The internal SDs are v1 */

    /* Try to switch V2 cards to HS timings, non HS seem to ignore this */
    if(sd_v2)
    {
        /*  CMD7 w/rca: Select card to put it in TRAN state */
        if(!send_cmd(drive, SD_SELECT_CARD, card_info[drive].rca, MCI_RESP, &response))
            return -5;

        if(sd_wait_for_tran_state(drive))
            return -6;
        /* CMD6 */
        if(!send_cmd(drive, SD_SWITCH_FUNC, 0x80fffff1, MCI_NO_RESP, NULL))
            return -7;
        mci_delay();

        /*  go back to STBY state so we can read csd */
        /*  CMD7 w/rca=0:  Deselect card to put it in STBY state */
        if(!send_cmd(drive, SD_DESELECT_CARD, 0, MCI_NO_RESP, NULL))
            return -8;
        mci_delay();
    }
#endif /*  HAVE_MULTIDRIVE  */

    /* CMD9 send CSD */
    if(!send_cmd(drive, SD_SEND_CSD, card_info[drive].rca,
                 MCI_RESP|MCI_LONG_RESP, card_info[drive].csd))
        return -9;

    sd_parse_csd(&card_info[drive]);

#if defined(HAVE_MULTIDRIVE)
    hs_card = (card_info[drive].speed == 50000000);
#endif

    /* Boost MCICLK to operating speed */
    if(drive == INTERNAL_AS3525)
        MCI_CLOCK(drive) = MCI_HALFSPEED;  /* MCICLK = IDE_CLK/2 = 25 MHz  */
#if defined(HAVE_MULTIDRIVE)
    else
        /* MCICLK = PCLK/2 = 31MHz(HS) or PCLK/4 = 15.5 Mhz (STD)*/
        MCI_CLOCK(drive) = (hs_card ? MCI_HALFSPEED : MCI_QUARTERSPEED);
#endif

    /*  CMD7 w/rca: Select card to put it in TRAN state */
    if(!send_cmd(drive, SD_SELECT_CARD, card_info[drive].rca, MCI_RESP, &response))
        return -10;

#if 0 /* FIXME : it seems that reading fails on some models */
    /*  Switch to to 4 bit widebus mode  */
    if(sd_wait_for_tran_state(drive) < 0)
        return -11;
    /* ACMD42  */
    if(!send_cmd(drive, SD_SET_CLR_CARD_DETECT, 0, MCI_ACMD|MCI_RESP, &response))
        return -15;
    /* ACMD6  */
    if(!send_cmd(drive, SD_SET_BUS_WIDTH, 2, MCI_ACMD|MCI_RESP, &response))
        return -13;
    /* Now that card is widebus make controller aware */
    MCI_CLOCK(drive) |= MCI_CLOCK_WIDEBUS;
#endif

    /*
     * enable bank switching
     * without issuing this command, we only have access to 1/4 of the blocks
     * of the first bank (0x1E9E00 blocks, which is the size reported in the
     * CSD register)
     */
    if(drive == INTERNAL_AS3525)
    {
        const int ret = sd_select_bank(-1);
        if(ret < 0)
            return ret -16;

        /*  CMD7 w/rca = 0: Unselect card to put it in STBY state */
        if(!send_cmd(drive, SD_SELECT_CARD, 0, MCI_NO_RESP, NULL))
            return -17;
        mci_delay();

        /* CMD9 send CSD again, so we got the correct number of blocks */
        if(!send_cmd(drive, SD_SEND_CSD, card_info[drive].rca,
                     MCI_RESP|MCI_LONG_RESP, card_info[drive].csd))
            return -18;

        sd_parse_csd(&card_info[drive]);
        /* The OF is stored in the first blocks */
        card_info[INTERNAL_AS3525].numblocks -= AMS_OF_SIZE;

        /*  CMD7 w/rca: Select card to put it in TRAN state */
        if(!send_cmd(drive, SD_SELECT_CARD, card_info[drive].rca, MCI_RESP, &response))
            return -19;
    }

    card_info[drive].initialized = 1;

    return 0;
}

static void sd_thread(void) NORETURN_ATTR;
static void sd_thread(void)
{
    struct queue_event ev;
    bool idle_notified = false;

    while (1)
    {
        queue_wait_w_tmo(&sd_queue, &ev, HZ);

        switch ( ev.id )
        {
#ifdef HAVE_HOTSWAP
        case SYS_HOTSWAP_INSERTED:
        case SYS_HOTSWAP_EXTRACTED:
        {
            int microsd_init = 1;
            fat_lock();          /* lock-out FAT activity first -
                                    prevent deadlocking via disk_mount that
                                    would cause a reverse-order attempt with
                                    another thread */
            mutex_lock(&sd_mtx); /* lock-out card activity - direct calls
                                    into driver that bypass the fat cache */

            /* We now have exclusive control of fat cache and ata */

            disk_unmount(SD_SLOT_AS3525);     /* release "by force", ensure file
                                    descriptors aren't leaked and any busy
                                    ones are invalid if mounting */

            /* Force card init for new card, re-init for re-inserted one or
             * clear if the last attempt to init failed with an error. */
            card_info[SD_SLOT_AS3525].initialized = 0;

            if (ev.id == SYS_HOTSWAP_INSERTED)
            {
                sd_enable(true);
                init_pl180_controller(SD_SLOT_AS3525);
                microsd_init = sd_init_card(SD_SLOT_AS3525);
                if (microsd_init < 0) /* initialisation failed */
                    panicf("microSD init failed : %d", microsd_init);

                microsd_init = disk_mount(SD_SLOT_AS3525); /* 0 if fail */
            }

            /*
             * Mount succeeded, or this was an EXTRACTED event,
             * in both cases notify the system about the changed filesystems
             */
            if (microsd_init)  
                queue_broadcast(SYS_FS_CHANGED, 0);

            /* Access is now safe */
            mutex_unlock(&sd_mtx);
            fat_unlock();
            sd_enable(false);
        }
            break;
#endif
        case SYS_TIMEOUT:
            if (TIME_BEFORE(current_tick, last_disk_activity+(3*HZ)))
            {
                idle_notified = false;
            }
            else
            {
                /* never let a timer wrap confuse us */
                next_yield = current_tick;

                if (!idle_notified)
                {
                    call_storage_idle_notifys(false);
                    idle_notified = true;
                }
            }
            break;

        case SYS_USB_CONNECTED:
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            /* Wait until the USB cable is extracted again */
            usb_wait_for_disconnect(&sd_queue);

            break;
        case SYS_USB_DISCONNECTED:
            usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
            break;
        }
    }
}

static void init_pl180_controller(const int drive)
{
    MCI_COMMAND(drive) = MCI_DATA_CTRL(drive) = 0;
    MCI_CLEAR(drive) = 0x7ff;

    MCI_MASK0(drive) = MCI_DATA_ERROR | MCI_DATA_END;
    MCI_MASK1(drive) = 0;
#ifdef HAVE_MULTIDRIVE
    VIC_INT_ENABLE =
        (drive == INTERNAL_AS3525) ? INTERRUPT_NAND : INTERRUPT_MCI0;
    /* clear previous irq */
    GPIOA_IC = EXT_SD_BITS;
    /* enable edge detecting */
    GPIOA_IS &= ~EXT_SD_BITS;
    /* detect both raising and falling edges */
    GPIOA_IBE |= EXT_SD_BITS;
    /* enable the card detect interrupt */
    GPIOA_IE |= EXT_SD_BITS;

#else
    VIC_INT_ENABLE = INTERRUPT_NAND;
#endif

    MCI_POWER(drive) = MCI_POWER_UP | (MCI_VDD_3_0);  /*  OF Setting  */
    mci_delay();

    MCI_POWER(drive) |= MCI_POWER_ON;
    mci_delay();

    MCI_SELECT(drive) = 0;

    /* Pl180 clocks get turned on at start of card init */
}

int sd_init(void)
{
    int ret;
    CGU_IDE =   (1<<6)                  /*  enable non AHB interface*/
            |   (AS3525_IDE_DIV << 2)
            |    AS3525_CLK_PLLA;       /* clock source = PLLA */

    bitset32(&CGU_PERI, CGU_NAF_CLOCK_ENABLE);
#ifdef HAVE_MULTIDRIVE
    bitset32(&CGU_PERI, CGU_MCI_CLOCK_ENABLE);
    bitclr32(&CCU_IO, 1<<3);    /* bits 3:2 = 01, xpd is SD interface */
    bitset32(&CCU_IO, 1<<2);
#endif

    wakeup_init(&transfer_completion_signal);

    init_pl180_controller(INTERNAL_AS3525);
    ret = sd_init_card(INTERNAL_AS3525);
    if(ret < 0)
        return ret;
#ifdef HAVE_MULTIDRIVE
    init_pl180_controller(SD_SLOT_AS3525);
#endif

    /* init mutex */
    mutex_init(&sd_mtx);

    queue_init(&sd_queue, true);
    create_thread(sd_thread, sd_stack, sizeof(sd_stack), 0,
            sd_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));

    sd_enabled = true;
    sd_enable(false);

    return 0;
}

#ifdef HAVE_HOTSWAP
bool sd_removable(IF_MD_NONVOID(int drive))
{
    return (drive==1);
}

bool sd_present(IF_MD_NONVOID(int drive))
{
    return (drive == 0) ? true : card_detect_target();
}
#endif /* HAVE_HOTSWAP */

static int sd_wait_for_tran_state(const int drive)
{
    unsigned long response = 0;
    unsigned int timeout = current_tick + 5 * HZ;

    while (1)
    {
        if(!send_cmd(drive, SD_SEND_STATUS, card_info[drive].rca, MCI_RESP,
                    &response))
            return -1;

        if (((response >> 9) & 0xf) == SD_TRAN)
            return 0;

        if(TIME_AFTER(current_tick, timeout))
            return -2;

        if (TIME_AFTER(current_tick, next_yield))
        {
            yield();
            next_yield = current_tick + MIN_YIELD_PERIOD;
        }
    }
}

static int sd_select_bank(signed char bank)
{
    int ret;
    unsigned loops = 0;

    memset(uncached_buffer, 0, 512);
    if(bank == -1)
    {   /* enable bank switching */
        uncached_buffer[0] = 16;
        uncached_buffer[1] = 1;
        uncached_buffer[2] = 10;
    }
    else
        uncached_buffer[0] = bank;

    do {
        if(loops++ > PL180_MAX_TRANSFER_ERRORS)
            panicf("SD bank %d error : 0x%x", bank,
                    transfer_error[INTERNAL_AS3525]);

        ret = sd_wait_for_tran_state(INTERNAL_AS3525);
        if (ret < 0)
            return ret - 2;

        if(!send_cmd(INTERNAL_AS3525, SD_SWITCH_FUNC, 0x80ffffef, MCI_NO_RESP,
                     NULL))
            return -1;

        mci_delay();

        if(!send_cmd(INTERNAL_AS3525, 35, 0, MCI_NO_RESP, NULL))
            return -2;

        mci_delay();

        dma_retain();
        /* we don't use the uncached buffer here, because we need the
         * physical memory address for DMA transfers */
        dma_enable_channel(0, AS3525_PHYSICAL_ADDR(&aligned_buffer[0]),
            MCI_FIFO(INTERNAL_AS3525), DMA_PERI_SD,
            DMAC_FLOWCTRL_PERI_MEM_TO_PERI, true, false, 0, DMA_S8, NULL);

        MCI_DATA_TIMER(INTERNAL_AS3525) = SD_MAX_WRITE_TIMEOUT;
        MCI_DATA_LENGTH(INTERNAL_AS3525) = 512;
        MCI_DATA_CTRL(INTERNAL_AS3525) =  (1<<0) /* enable */   |
                                (0<<1) /* transfer direction */ |
                                (1<<3) /* DMA */                |
                                (9<<4) /* 2^9 = 512 */ ;

        /* Wakeup signal from NAND/MCIO isr on MCI_DATA_ERROR | MCI_DATA_END */
        wakeup_wait(&transfer_completion_signal, TIMEOUT_BLOCK);

        /*  Wait for FIFO to empty, card may still be in PRG state  */
        while(MCI_STATUS(INTERNAL_AS3525) & MCI_TX_ACTIVE );

        dma_release();

    } while(transfer_error[INTERNAL_AS3525]);

    card_info[INTERNAL_AS3525].current_bank = (bank == -1) ? 0 : bank;

    return 0;
}

static int sd_transfer_sectors(IF_MD2(int drive,) unsigned long start,
                               int count, void* buf, const bool write)
{
#ifndef HAVE_MULTIDRIVE
    const int drive = 0;
#endif
    int ret = 0;
    unsigned loops = 0;
    unsigned long response;
    bool aligned = !((uintptr_t)buf & (CACHEALIGN_SIZE - 1));

    sd_enable(true);
    led(true);

    if (card_info[drive].initialized <= 0)
    {
        ret = sd_init_card(drive);
        if (!(card_info[drive].initialized))
            goto sd_transfer_error_nodma;
    }

    if(count < 0) /* XXX: why is it signed ? */
    {
        ret = -20;
        goto sd_transfer_error_nodma;
    }
    if((start+count) > card_info[drive].numblocks)
    {
        ret = -21;
        goto sd_transfer_error_nodma;
    }

    /* skip SanDisk OF */
    if (drive == INTERNAL_AS3525)
        start += AMS_OF_SIZE;

    last_disk_activity = current_tick;

    dma_retain();

    if(aligned)
    {   /* direct transfer, indirect is always uncached */
        if(write)
            commit_dcache_range(buf, count * SECTOR_SIZE);
        else
            discard_dcache_range(buf, count * SECTOR_SIZE);
    }

    while(count)
    {
        /* 128 * 512 = 2^16, and doesn't fit in the 16 bits of DATA_LENGTH
         * register, so we have to transfer maximum 127 sectors at a time. */
        unsigned int transfer = (count >= 128) ? 127 : count; /* sectors */
        void *dma_buf;
        const int cmd =
            write ? SD_WRITE_MULTIPLE_BLOCK : SD_READ_MULTIPLE_BLOCK;
        unsigned long bank_start = start;
        unsigned long status;

        /* Only switch banks for internal storage */
        if(drive == INTERNAL_AS3525)
        {
            unsigned int bank = 0;
            while(bank_start >= BLOCKS_PER_BANK)
            {
                bank_start -= BLOCKS_PER_BANK;
                bank++;
            }

            /* Switch bank if needed */
            if(card_info[INTERNAL_AS3525].current_bank != bank)
            {
                ret = sd_select_bank(bank);
                if (ret < 0)
                {
                    ret -= 20;
                    goto sd_transfer_error;
                }
            }

            /* Do not cross a bank boundary in a single transfer loop */
            if((transfer + bank_start) > BLOCKS_PER_BANK)
                transfer = BLOCKS_PER_BANK - bank_start;
        }

        /* Set bank_start to the correct unit (blocks or bytes) */
        if(!(card_info[drive].ocr & (1<<30)))   /* not SDHC */
            bank_start *= SD_BLOCK_SIZE;

        if(aligned)
        {
            dma_buf = AS3525_PHYSICAL_ADDR(buf);
        }
        else
        {
            dma_buf = AS3525_PHYSICAL_ADDR(&aligned_buffer[0]);
            if(transfer > UNALIGNED_NUM_SECTORS)
                transfer = UNALIGNED_NUM_SECTORS;

            if(write)
                memcpy(uncached_buffer, buf, transfer * SD_BLOCK_SIZE);
        }

        ret = sd_wait_for_tran_state(drive);
        if (ret < 0)
        {
            ret -= 2*20;
            goto sd_transfer_error;
        }

        if(!send_cmd(drive, cmd, bank_start, MCI_RESP, &response))
        {
            ret -= 3*20;
            goto sd_transfer_error;
        }

        if(write)
        {
            dma_enable_channel(0, dma_buf, MCI_FIFO(drive),
                (drive == INTERNAL_AS3525) ? DMA_PERI_SD : DMA_PERI_SD_SLOT,
                DMAC_FLOWCTRL_PERI_MEM_TO_PERI, true, false, 0, DMA_S8, NULL);

            /*Small delay for writes prevents data crc failures at lower freqs*/
#ifdef HAVE_MULTIDRIVE
            if((drive == SD_SLOT_AS3525) && !hs_card)
            {
                int write_delay = 125;
                while(write_delay--);
            }
#endif
        }
        else
            dma_enable_channel(0, MCI_FIFO(drive), dma_buf,
                (drive == INTERNAL_AS3525) ? DMA_PERI_SD : DMA_PERI_SD_SLOT,
                DMAC_FLOWCTRL_PERI_PERI_TO_MEM, false, true, 0, DMA_S8, NULL);

        MCI_DATA_TIMER(drive) = write ?
                SD_MAX_WRITE_TIMEOUT : SD_MAX_READ_TIMEOUT;
        MCI_DATA_LENGTH(drive) = transfer * SD_BLOCK_SIZE;
        MCI_DATA_CTRL(drive) =  (1<<0) /* enable */                     |
                                (!write<<1) /* transfer direction */    |
                                (1<<3) /* DMA */                        |
                                (9<<4) /* 2^9 = 512 */ ;

        /* Wakeup signal from NAND/MCIO isr on MCI_DATA_ERROR | MCI_DATA_END */
        wakeup_wait(&transfer_completion_signal, TIMEOUT_BLOCK);

        /*  Wait for FIFO to empty, card may still be in PRG state for writes */
        while(MCI_STATUS(drive) & MCI_TX_ACTIVE);

        /*
         * If the write aborted early due to a tx underrun, disable the
         * dma channel here, otherwise there are still 4 words in the fifo
         * and the retried write will get corrupted.
         */
        dma_disable_channel(0);

        last_disk_activity = current_tick;

        if(!send_cmd(drive, SD_STOP_TRANSMISSION, 0, MCI_RESP, &status))
        {
            ret = -4*20;
            goto sd_transfer_error;
        }

        if(!transfer_error[drive])
        {
            if(!write && !aligned)
                memcpy(buf, uncached_buffer, transfer * SD_BLOCK_SIZE);
            buf += transfer * SD_BLOCK_SIZE;
            start += transfer;
            count -= transfer;
            loops = 0;  /* reset errors counter */
        }
        else if(loops++ > PL180_MAX_TRANSFER_ERRORS)
                panicf("SD Xfer %s err:0x%x Disk%d", (write? "write": "read"),
                                                  transfer_error[drive], drive);
    }

sd_transfer_error:

    dma_release();

sd_transfer_error_nodma:

    led(false);
    sd_enable(false);

    if (ret)    /* error */
        card_info[drive].initialized = 0;

    return ret;
}

int sd_read_sectors(IF_MD2(int drive,) unsigned long start, int count,
                     void* buf)
{
    int ret;

    mutex_lock(&sd_mtx);
    ret = sd_transfer_sectors(IF_MD2(drive,) start, count, buf, false);
    mutex_unlock(&sd_mtx);

    return ret;
}

int sd_write_sectors(IF_MD2(int drive,) unsigned long start, int count,
                     const void* buf)
{
#ifdef VERIFY_WRITE
    unsigned long saved_start = start;
    int saved_count = count;
    void *saved_buf = (void*)buf;
#endif
    int ret;

    mutex_lock(&sd_mtx);

    ret = sd_transfer_sectors(IF_MD2(drive,) start, count, (void*)buf, true);

#ifdef VERIFY_WRITE
    if (ret) {
        /* write failed, no point in verifying */
        mutex_unlock(&sd_mtx);
        return ret;
    }

    count = saved_count;
    buf = saved_buf;
    start = saved_start;
    while (count) {
        int transfer = count;
        if(transfer > UNALIGNED_NUM_SECTORS)
            transfer = UNALIGNED_NUM_SECTORS;

        sd_transfer_sectors(IF_MD2(drive,) start, transfer, aligned_buffer, false);
        if (memcmp(buf, aligned_buffer, transfer * 512) != 0) {
            /* try the write again in the hope to repair the damage */
            sd_transfer_sectors(IF_MD2(drive,) saved_start, saved_count, saved_buf, true);
            panicf("sd: verify failed: sec=%ld n=%d!", start, transfer);
        }

        buf   += transfer * 512;
        count -= transfer;
        start += transfer;
    }
#endif

    mutex_unlock(&sd_mtx);

    return ret;
}

long sd_last_disk_activity(void)
{
    return last_disk_activity;
}

void sd_enable(bool on)
{
#if defined(HAVE_BUTTON_LIGHT) && defined(HAVE_MULTIDRIVE)
    extern int buttonlight_is_on;
#endif

#if defined(HAVE_HOTSWAP) && defined (HAVE_ADJUSTABLE_CPU_VOLTAGE)
    static bool cpu_boosted = false;
#endif

    if (sd_enabled == on)
        return; /* nothing to do */

    sd_enabled = on;

    if(on)
    {
#if defined(HAVE_BUTTON_LIGHT) && defined(HAVE_MULTIDRIVE)
        /* buttonlight AMSes need a bit of special handling for the buttonlight
         * here due to the dual mapping of GPIOD and XPD */
        bitset32(&CCU_IO, 1<<2);    /* XPD is SD-MCI interface (b3:2 = 01) */
        if (buttonlight_is_on)
            GPIOD_DIR &= ~(1<<7);
        else
            _buttonlight_off();
#endif

#if defined(HAVE_HOTSWAP) && defined (HAVE_ADJUSTABLE_CPU_VOLTAGE)
        if(card_detect_target())  /* If SD card present Boost cpu for voltage */
        {
            cpu_boosted = true;
            cpu_boost(true);
        }
#endif  /* defined(HAVE_HOTSWAP) && defined (HAVE_ADJUSTABLE_CPU_VOLTAGE) */
    }
    else
    {
#if defined(HAVE_HOTSWAP) && defined (HAVE_ADJUSTABLE_CPU_VOLTAGE)
        if(cpu_boosted)
        {
            cpu_boost(false);
            cpu_boosted = false;
        }
#endif  /* defined(HAVE_HOTSWAP) && defined (HAVE_ADJUSTABLE_CPU_VOLTAGE) */

#if defined(HAVE_BUTTON_LIGHT) && defined(HAVE_MULTIDRIVE)
        bitclr32(&CCU_IO, 1<<2);    /* XPD is general purpose IO (b3:2 = 00) */
        if (buttonlight_is_on)
            _buttonlight_on();
#endif
    }
}

tCardInfo *card_get_info_target(int card_no)
{
    return &card_info[card_no];
}

#ifdef CONFIG_STORAGE_MULTI
int sd_num_drives(int first_drive)
{
    /* We don't care which logical drive number(s) we have been assigned */
    (void)first_drive;

    return NUM_DRIVES;
}
#endif /* CONFIG_STORAGE_MULTI */
