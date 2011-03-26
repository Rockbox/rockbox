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

#include "config.h" /* for HAVE_MULTIVOLUME */
#include "fat.h"
#include "thread.h"
#include "gcc_extensions.h"
#include "led.h"
#include "sdmmc.h"
#include "system.h"
#include "kernel.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "as3525v2.h"
#include "pl081.h"  /* DMA controller */
#include "dma-target.h" /* DMA request lines */
#include "clock-target.h"
#include "panic.h"
#include "stdbool.h"
#include "ata_idle_notify.h"
#include "sd.h"
#include "usb.h"

#ifdef HAVE_HOTSWAP
#include "disk.h"
#endif

#if defined(SANSA_FUZEV2)
#include "backlight-target.h"
#endif

#include "lcd.h"
#include <stdarg.h>
#include "sysfont.h"

#define     INTERNAL_AS3525  0   /* embedded SD card */
#define     SD_SLOT_AS3525   1   /* SD slot if present */

/* Clipv2 Clip+ and Fuzev2 OF all occupy the same size */
#define AMS_OF_SIZE 0xf000

/* command flags */
#define MCI_NO_RESP     (0<<0)
#define MCI_RESP        (1<<0)
#define MCI_LONG_RESP   (1<<1)
#define MCI_ACMD        (1<<2)

/* controller registers */
#define SD_BASE 0xC6070000

#define SD_REG(x)       (*(volatile unsigned long *) (SD_BASE+x))

#define MCI_CTRL        SD_REG(0x00)

/* control bits */
#define CTRL_RESET      (1<<0)
#define FIFO_RESET      (1<<1)
#define DMA_RESET       (1<<2)
#define INT_ENABLE      (1<<4)
#define DMA_ENABLE      (1<<5)
#define READ_WAIT       (1<<6)
#define SEND_IRQ_RESP   (1<<7)
#define ABRT_READ_DATA  (1<<8)
#define SEND_CCSD       (1<<9)
#define SEND_AS_CCSD    (1<<10)
#define EN_OD_PULLUP    (1<<24)


#define MCI_PWREN       SD_REG(0x04)    /* power enable */

#define PWR_CRD_0       (1<<0)
#define PWR_CRD_1       (1<<1)
#define PWR_CRD_2       (1<<2)
#define PWR_CRD_3       (1<<3)

#define MCI_CLKDIV      SD_REG(0x08)    /* clock divider */
/* CLK_DIV_0 :    bits 7:0
 * CLK_DIV_1 :    bits 15:8
 * CLK_DIV_2 :    bits 23:16
 * CLK_DIV_3 :    bits 31:24
 */

#define MCI_CLKSRC      SD_REG(0x0C)    /* clock source */
/* CLK_SRC_CRD0:  bits 1:0
 * CLK_SRC_CRD1:  bits 3:2
 * CLK_SRC_CRD2:  bits 5:4
 * CLK_SRC_CRD3:  bits 7:6
 */

#define MCI_CLKENA      SD_REG(0x10)    /* clock enable */

#define CCLK_ENA_CRD0   (1<<0)
#define CCLK_ENA_CRD1   (1<<1)
#define CCLK_ENA_CRD2   (1<<2)
#define CCLK_ENA_CRD3   (1<<3)
#define CCLK_LP_CRD0    (1<<16)         /* LP --> Low Power Mode? */
#define CCLK_LP_CRD1    (1<<17)
#define CCLK_LP_CRD2    (1<<18)
#define CCLK_LP_CRD3    (1<<19)

#define MCI_TMOUT       SD_REG(0x14)    /* timeout */
/* response timeout bits 0:7
 * data timeout bits     8:31
 */

#define MCI_CTYPE       SD_REG(0x18)    /* card type */
                                        /* 1 bit per card, set = wide bus */
#define WIDTH4_CRD0     (1<<0)
#define WIDTH4_CRD1     (1<<1)
#define WIDTH4_CRD2     (1<<2)
#define WIDTH4_CRD3     (1<<3)

#define MCI_BLKSIZ      SD_REG(0x1C)    /* block size  bits 0:15*/
#define MCI_BYTCNT      SD_REG(0x20)    /* byte count  bits 0:31*/
#define MCI_MASK        SD_REG(0x24)    /* interrupt mask */



#define MCI_ARGUMENT    SD_REG(0x28)
#define MCI_COMMAND     SD_REG(0x2C)

/* command bits (bits 5:0 are the command index) */
#define CMD_RESP_EXP_BIT        (1<<6)
#define CMD_RESP_LENGTH_BIT     (1<<7)
#define CMD_CHECK_CRC_BIT       (1<<8)
#define CMD_DATA_EXP_BIT        (1<<9)
#define CMD_RW_BIT              (1<<10)
#define CMD_TRANSMODE_BIT       (1<<11)
#define CMD_SENT_AUTO_STOP_BIT  (1<<12)
#define CMD_WAIT_PRV_DAT_BIT    (1<<13)
#define CMD_ABRT_CMD_BIT        (1<<14)
#define CMD_SEND_INIT_BIT       (1<<15)
#define CMD_CARD_NO(x)          ((x)<<16)         /* 5 bits wide  */
#define CMD_SEND_CLK_ONLY       (1<<21)
#define CMD_READ_CEATA          (1<<22)
#define CMD_CCS_EXPECTED        (1<<23)
#define CMD_DONE_BIT            (1<<31)

#define TRANSFER_CMD  (cmd == SD_READ_MULTIPLE_BLOCK ||   \
                       cmd == SD_WRITE_MULTIPLE_BLOCK)

#define MCI_RESP0       SD_REG(0x30)
#define MCI_RESP1       SD_REG(0x34)
#define MCI_RESP2       SD_REG(0x38)
#define MCI_RESP3       SD_REG(0x3C)

#define MCI_MASK_STATUS SD_REG(0x40)    /* masked interrupt status */
#define MCI_RAW_STATUS  SD_REG(0x44)    /* raw interrupt status, also used as
                                         * status clear */

/* interrupt bits */                /* C D E   (Cmd) (Data) (End) */
#define MCI_INT_CRDDET  (1<<0)      /*          card detect */
#define MCI_INT_RE      (1<<1)      /* x         response error */
#define MCI_INT_CD      (1<<2)      /*     x     command done */
#define MCI_INT_DTO     (1<<3)      /*     x     data transfer over */
#define MCI_INT_TXDR    (1<<4)      /*          tx fifo data request */
#define MCI_INT_RXDR    (1<<5)      /*          rx fifo data request */
#define MCI_INT_RCRC    (1<<6)      /* x         response crc error */
#define MCI_INT_DCRC    (1<<7)      /*   x       data crc error */
#define MCI_INT_RTO     (1<<8)      /* x         response timeout */
#define MCI_INT_DRTO    (1<<9)      /*   x       data read timeout */
#define MCI_INT_HTO     (1<<10)     /*   x       data starv timeout */
#define MCI_INT_FRUN    (1<<11)     /*   x       fifo over/underrun */
#define MCI_INT_HLE     (1<<12)     /* x x        hw locked while error */
#define MCI_INT_SBE     (1<<13)     /*   x       start bit error */
#define MCI_INT_ACD     (1<<14)     /*          auto command done */
#define MCI_INT_EBE     (1<<15)     /*   x       end bit error */
#define MCI_INT_SDIO    (0xf<<16)

/*
 *  STATUS register
 *  & 0xBA80    = MCI_INT_DCRC | MCI_INT_DRTO | MCI_INT_FRUN | \
 *                  MCI_INT_HLE | MCI_INT_SBE | MCI_INT_EBE
 *  & 8         = MCI_INT_DTO
 *  & 0x428     = MCI_INT_DTO | MCI_INT_RXDR | MCI_INT_HTO
 *  & 0x418     = MCI_INT_DTO | MCI_INT_TXDR | MCI_INT_HTO
 */

#define MCI_CMD_ERROR    \
         (MCI_INT_RE   | \
          MCI_INT_RCRC | \
          MCI_INT_RTO  | \
          MCI_INT_HLE)

#define MCI_DATA_ERROR    \
        ( MCI_INT_DCRC  | \
          MCI_INT_DRTO  | \
          MCI_INT_HTO   | \
          MCI_INT_FRUN  | \
          MCI_INT_HLE   | \
          MCI_INT_SBE   | \
          MCI_INT_EBE)

#define MCI_STATUS      SD_REG(0x48)

#define FIFO_RX_WM              (1<<0)
#define FIFO_TX_WM              (1<<1)
#define FIFO_EMPTY              (1<<2)
#define FIFO_FULL               (1<<3)
#define CMD_FSM_STATE_B0        (1<<4)
#define CMD_FSM_STATE_B1        (1<<5)
#define CMD_FSM_STATE_B2        (1<<6)
#define CMD_FSM_STATE_B3        (1<<7)
#define DATA_3_STAT             (1<<8)
#define DATA_BUSY               (1<<9)
#define DATA_STAT_MC_BUSY       (1<<10)
#define RESP_IDX_B0             (1<<11)
#define RESP_IDX_B1             (1<<12)
#define RESP_IDX_B2             (1<<13)
#define RESP_IDX_B3             (1<<14)
#define RESP_IDX_B4             (1<<15)
#define RESP_IDX_B5             (1<<16)
#define FIFO_CNT_B00            (1<<17)
#define FIFO_CNT_B01            (1<<18)
#define FIFO_CNT_B02            (1<<19)
#define FIFO_CNT_B03            (1<<20)
#define FIFO_CNT_B04            (1<<21)
#define FIFO_CNT_B05            (1<<22)
#define FIFO_CNT_B06            (1<<23)
#define FIFO_CNT_B07            (1<<24)
#define FIFO_CNT_B08            (1<<25)
#define FIFO_CNT_B09            (1<<26)
#define FIFO_CNT_B10            (1<<27)
#define FIFO_CNT_B11            (1<<28)
#define FIFO_CNT_B12            (1<<29)
#define DMA_ACK                 (1<<30)
#define START_CMD               (1<<31)

#define MCI_FIFOTH      SD_REG(0x4C)    /* FIFO threshold */
/* TX watermark :    bits 11:0
 * RX watermark :    bits 27:16
 * DMA MTRANS SIZE : bits 30:28
 * bits 31, 15:12 : unused
 */
#define MCI_FIFOTH_MASK 0x8000f000

#define MCI_CDETECT     SD_REG(0x50)    /* card detect */

#define CDETECT_CRD_0   (1<<0)
#define CDETECT_CRD_1   (1<<1)
#define CDETECT_CRD_2   (1<<2)
#define CDETECT_CRD_3   (1<<3)

#define MCI_WRTPRT      SD_REG(0x54)    /* write protect */
#define MCI_GPIO        SD_REG(0x58)
#define MCI_TCBCNT      SD_REG(0x5C)    /* transferred CIU byte count (card)*/
#define MCI_TBBCNT      SD_REG(0x60)    /* transferred host/DMA to/from bytes (FIFO)*/
#define MCI_DEBNCE      SD_REG(0x64)    /* card detect debounce  bits 23:0*/
#define MCI_USRID       SD_REG(0x68)    /* user id */
#define MCI_VERID       SD_REG(0x6C)    /* version id */

#define MCI_HCON        SD_REG(0x70)    /* hardware config */
/* bit  0       : card type
 * bits 5:1     : maximum card index
 * bit  6       : BUS TYPE
 * bits 9:7     : DATA WIDTH
 * bits 15:10   : ADDR WIDTH
 * bits 17:16   : DMA IF
 * bits 20:18   : DMA WIDTH
 * bit  21      : FIFO RAM INSIDE
 * bit  22      : IMPL HOLD REG
 * bit  23      : SET CLK FALSE
 * bits 25:24   : MAX CLK DIV IDX
 * bit  26      : AREA OPTIM
 */

#define MCI_BMOD        SD_REG(0x80)    /* bus mode */
/* bit  0       : SWR
 * bit  1       : FB
 * bits 6:2     : DSL
 * bit  7       : DE
 * bit  10:8    : PBL
 */

#define MCI_PLDMND      SD_REG(0x84)    /* poll demand */
#define MCI_DBADDR      SD_REG(0x88)    /* descriptor base address */
#define MCI_IDSTS       SD_REG(0x8C)    /* internal DMAC status */
/* bit  0       : TI
 * bit  1       : RI
 * bit  2       : FBE
 * bit  3       : unused
 * bit  4       : DU
 * bit  5       : CES
 * bits 7:6     : unused
 * bits 8       : NIS
 * bit  9       : AIS
 * bits 12:10   : EB
 * bits 16:13   : FSM
 */

#define MCI_IDINTEN     SD_REG(0x90)    /* internal DMAC interrupt enable */
/* bit  0       : TI
 * bit  1       : RI
 * bit  2       : FBE
 * bit  3       : unused
 * bit  4       : DU
 * bit  5       : CES
 * bits 7:6     : unused
 * bits 8       : NI
 * bit  9       : AI
 */
#define MCI_DSCADDR     SD_REG(0x94)    /* current host descriptor address */
#define MCI_BUFADDR     SD_REG(0x98)    /* current host buffer address */

#define MCI_FIFO        ((unsigned long *) (SD_BASE+0x100))

#define UNALIGNED_NUM_SECTORS 10
static unsigned char aligned_buffer[UNALIGNED_NUM_SECTORS* SD_BLOCK_SIZE] __attribute__((aligned(32)));   /* align on cache line size */
static unsigned char *uncached_buffer = AS3525_UNCACHED_ADDR(&aligned_buffer[0]);

static tCardInfo card_info[NUM_DRIVES];

/* for compatibility */
static long last_disk_activity = -1;

static long sd_stack [(DEFAULT_STACK_SIZE*2 + 0x200)/sizeof(long)];
static const char         sd_thread_name[] = "ata/sd";
static struct mutex       sd_mtx SHAREDBSS_ATTR;
static struct event_queue sd_queue;
#ifndef BOOTLOADER
bool sd_enabled = false;
#endif

static struct semaphore transfer_completion_signal;
static struct semaphore command_completion_signal;
static volatile bool retry;
static volatile int cmd_error;

#if defined(HAVE_MULTIDRIVE)
#define EXT_SD_BITS (1<<2)
#endif

static inline void mci_delay(void) { udelay(1000); }

void INT_NAND(void)
{
    MCI_CTRL &= ~INT_ENABLE;
    /* use raw status here as we need to check some Ints that are masked */
    const int status = MCI_RAW_STATUS;

    MCI_RAW_STATUS = status;    /* clear status */

    if(status & MCI_DATA_ERROR)
        retry = true;

    if( status & (MCI_INT_DTO|MCI_DATA_ERROR))
        semaphore_release(&transfer_completion_signal);

    cmd_error = status & MCI_CMD_ERROR;

    if(status & MCI_INT_CD)
        semaphore_release(&command_completion_signal);

    MCI_CTRL |= INT_ENABLE;
}

static inline bool card_detect_target(void)
{
#if defined(HAVE_MULTIDRIVE)
#if defined(SANSA_FUZEV2)
    return GPIOA_PIN(2);
#elif defined(SANSA_CLIPPLUS)
    return !(GPIOA_PIN(2));
#else
#error "microSD pin not defined for your target"
#endif
#else
    return false;
#endif
}

static bool send_cmd(const int drive, const int cmd, const int arg, const int flags,
        unsigned long *response)
{
    int card_no;

    if ((flags & MCI_ACMD) && /* send SD_APP_CMD first */
        !send_cmd(drive, SD_APP_CMD, card_info[drive].rca, MCI_RESP, response))
        return false;

#if defined(HAVE_MULTIDRIVE)
    if(sd_present(SD_SLOT_AS3525))
        GPIOB_PIN(5) = (1-drive) << 5;
#endif

    MCI_ARGUMENT = arg;

#if defined(SANSA_FUZEV2) || defined(SANSA_CLIPPLUS)
    if (amsv2_variant == 1)
        card_no = 1 << 16;
    else
#endif
        card_no = CMD_CARD_NO(drive);

    /* Construct MCI_COMMAND  */
    MCI_COMMAND =
      /*b5:0*/    cmd
      /*b6  */  | ((flags & MCI_RESP)                ? CMD_RESP_EXP_BIT:      0)
      /*b7  */  | ((flags & MCI_LONG_RESP)           ? CMD_RESP_LENGTH_BIT:   0)
      /*b8      | CMD_CHECK_CRC_BIT       unused  */
      /*b9  */  | (TRANSFER_CMD                      ? CMD_DATA_EXP_BIT:      0)
      /*b10 */  | ((cmd == SD_WRITE_MULTIPLE_BLOCK)  ? CMD_RW_BIT:            0)
      /*b11     | CMD_TRANSMODE_BIT       unused  */
      /*b12     | CMD_SENT_AUTO_STOP_BIT  unused  */
      /*b13 */  | ((cmd != SD_STOP_TRANSMISSION)     ? CMD_WAIT_PRV_DAT_BIT:  0)
      /*b14     | CMD_ABRT_CMD_BIT        unused  */
      /*b15 */  | ((cmd == SD_GO_IDLE_STATE)         ? CMD_SEND_INIT_BIT:     0)
   /*b20:16 */  |                                      card_no
      /*b21     | CMD_SEND_CLK_ONLY       unused  */
      /*b22     | CMD_READ_CEATA          unused  */
      /*b23     | CMD_CCS_EXPECTED        unused  */
      /*b31 */  |                                      CMD_DONE_BIT;

#if defined(SANSA_FUZEV2)
    if (amsv2_variant == 0)
    {
        extern int buttonlight_is_on;
        if(buttonlight_is_on)
            _buttonlight_on();
        else
            _buttonlight_off();
    }
#endif
    semaphore_wait(&command_completion_signal, TIMEOUT_BLOCK);

    /*  Handle command responses & errors */
    if(flags & MCI_RESP)
    {
        if(cmd_error & (MCI_INT_RCRC | MCI_INT_RTO))
            return false;

        if(flags & MCI_LONG_RESP)
        {
            response[0] = MCI_RESP3;
            response[1] = MCI_RESP2;
            response[2] = MCI_RESP1;
            response[3] = MCI_RESP0;
        }
        else
            response[0] = MCI_RESP0;
    }
    return true;
}

static int sd_wait_for_tran_state(const int drive)
{
    unsigned long response;
    unsigned int timeout = current_tick + 5*HZ;
    int cmd_retry = 10;

    while (1)
    {
        while (!send_cmd(drive, SD_SEND_STATUS, card_info[drive].rca, MCI_RESP,
                         &response) && cmd_retry > 0)
        {
            cmd_retry--;
        }

        if (cmd_retry <= 0)
            return -1;

        if (((response >> 9) & 0xf) == SD_TRAN)
            return 0;

        if(TIME_AFTER(current_tick, timeout))
            return -10 * ((response >> 9) & 0xf);

        last_disk_activity = current_tick;
    }
}


static int sd_init_card(const int drive)
{
    unsigned long response;
    long init_timeout;
    bool sd_v2 = false;

    card_info[drive].rca = 0;

    /*  assume 24 MHz clock / 60 = 400 kHz  */
    MCI_CLKDIV = (MCI_CLKDIV & ~(0xFF)) | 0x3C;    /* CLK_DIV_0 : bits 7:0  */

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
        if(!send_cmd(drive, SD_APP_OP_COND, (0x00FF8000 | (sd_v2 ? 1<<30 : 0)),
                        MCI_ACMD|MCI_RESP, &card_info[drive].ocr))
            return -3;
    } while(!(card_info[drive].ocr & (1<<31)) );

    /* CMD2 send CID */
    if(!send_cmd(drive, SD_ALL_SEND_CID, 0, MCI_RESP|MCI_LONG_RESP, card_info[drive].cid))
        return -4;

    /* CMD3 send RCA */
    if(!send_cmd(drive, SD_SEND_RELATIVE_ADDR, 0, MCI_RESP, &card_info[drive].rca))
        return -5;

#ifdef HAVE_MULTIDRIVE
    /* Make sure we have 2 unique rca numbers */
    if(card_info[INTERNAL_AS3525].rca == card_info[SD_SLOT_AS3525].rca)
        if(!send_cmd(drive, SD_SEND_RELATIVE_ADDR, 0, MCI_RESP, &card_info[drive].rca))
            return -6;
#endif
    /*  End of Card Identification Mode   ************************************/

    /*  Card back to full speed  */
    MCI_CLKDIV &= ~(0xFF);    /* CLK_DIV_0 : bits 7:0 = 0x00 */

    if (sd_v2)
    {
        /* Attempt to switch cards to HS timings, non HS cards just ignore this */
        /*  CMD7 w/rca: Select card to put it in TRAN state */
        if(!send_cmd(drive, SD_SELECT_CARD, card_info[drive].rca, MCI_RESP, &response))
            return -7;

        if(sd_wait_for_tran_state(drive))
            return -8;

        /* CMD6 */
        if(!send_cmd(drive, SD_SWITCH_FUNC, 0x80fffff1, MCI_RESP, &response))
            return -9;

        /* This delay is a bit of a hack, but seems to fix card detection
           problems with some SD cards (particularly 16 GB and bigger cards).
           Preferably we should handle this properly instead of using a delay,
           see also FS#11870. */
        sleep(HZ/10);

        /*  We need to go back to STBY state now so we can read csd */
        /*  CMD7 w/rca=0:  Deselect card to put it in STBY state */
        if(!send_cmd(drive, SD_DESELECT_CARD, 0, MCI_NO_RESP, NULL))
            return -10;
    }

    /* CMD9 send CSD */
    if(!send_cmd(drive, SD_SEND_CSD, card_info[drive].rca,
                 MCI_RESP|MCI_LONG_RESP, card_info[drive].csd))
        return -11;

    /* Another delay hack, see FS#11798 */
    mci_delay();

    sd_parse_csd(&card_info[drive]);

    if(drive == INTERNAL_AS3525) /* The OF is stored in the first blocks */
        card_info[INTERNAL_AS3525].numblocks -= AMS_OF_SIZE;

#ifndef BOOTLOADER
    /*  Switch to to 4 bit widebus mode  */

    /*  CMD7 w/rca: Select card to put it in TRAN state */
    if(!send_cmd(drive, SD_SELECT_CARD, card_info[drive].rca, MCI_RESP, &response))
        return -12;
    if(sd_wait_for_tran_state(drive) < 0)
        return -13;

    /* ACMD6: set bus width to 4-bit */
    if(!send_cmd(drive, SD_SET_BUS_WIDTH, 2, MCI_ACMD|MCI_RESP, &response))
        return -15;
    /* ACMD42: disconnect the pull-up resistor on CD/DAT3 */
    if(!send_cmd(drive, SD_SET_CLR_CARD_DETECT, 0, MCI_ACMD|MCI_RESP, &response))
        return -17;

    /* Now that card is widebus make controller aware */
#if defined(SANSA_FUZEV2) || defined(SANSA_CLIPPLUS)
    if (amsv2_variant == 1)
        MCI_CTYPE |= 1<<1;
    else
#endif
        MCI_CTYPE |= (1<<drive);

#endif /* ! BOOTLOADER */

    /*  Set low power mode  */
#if defined(SANSA_FUZEV2) || defined(SANSA_CLIPPLUS)
    if (amsv2_variant == 1)
        MCI_CLKENA |= 1<<16;
    else
#endif
        MCI_CLKENA |= 1<<(drive + 16);

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

            sd_enable(false);

            /* Access is now safe */
            mutex_unlock(&sd_mtx);
            fat_unlock();
            }
            break;
#endif
        case SYS_TIMEOUT:
            if (TIME_BEFORE(current_tick, last_disk_activity+(3*HZ)))
            {
                idle_notified = false;
            }
            else if (!idle_notified)
            {
                call_storage_idle_notifys(false);
                idle_notified = true;
            }
            break;

        case SYS_USB_CONNECTED:
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            /* Wait until the USB cable is extracted again */
            usb_wait_for_disconnect(&sd_queue);

            break;
        }
    }
}

static void init_controller(void)
{
    int hcon_numcards = ((MCI_HCON>>1) & 0x1F) + 1;
    int card_mask = (1 << hcon_numcards) - 1;
    int pwr_mask;

#if defined(SANSA_FUZEV2) || defined(SANSA_CLIPPLUS)
    if (amsv2_variant == 1)
        pwr_mask = 1 << 1;
    else
#endif
        pwr_mask = card_mask;

    MCI_PWREN &= ~pwr_mask;             /*  power off all cards  */
    MCI_PWREN = pwr_mask;               /*  power up cards  */

    MCI_CTRL |= CTRL_RESET;
    while(MCI_CTRL & CTRL_RESET)
        ;

    MCI_RAW_STATUS = 0xffffffff;        /* Clear all MCI Interrupts  */

    MCI_TMOUT = 0xffffffff;             /*  data b31:8, response b7:0 */

    MCI_CTYPE = 0x0;                    /*  all cards 1 bit bus for now */

    MCI_CLKENA = card_mask;             /*  Enables card clocks  */

    MCI_ARGUMENT = 0;
    MCI_COMMAND = CMD_DONE_BIT|CMD_SEND_CLK_ONLY|CMD_WAIT_PRV_DAT_BIT;
    while(MCI_COMMAND & CMD_DONE_BIT)
        ;

    MCI_DEBNCE = 0xfffff;               /* default value */

    /* Rx watermark = 63(sd reads)  Tx watermark = 128 (sd writes) */
    MCI_FIFOTH = (MCI_FIFOTH & MCI_FIFOTH_MASK) | 0x503f0080;

/*  RCRC & RTO interrupts should be set together with the CD interrupt but
 *  in practice sometimes incorrectly precede the CD interrupt.  If we leave
 *  them masked for now we can check them in the isr by reading raw status when
 *  the CD int is triggered.
 */
    MCI_MASK |= (MCI_DATA_ERROR | MCI_INT_DTO | MCI_INT_CD);

    MCI_CTRL |= INT_ENABLE | DMA_ENABLE;

    MCI_BLKSIZ = SD_BLOCK_SIZE;
}

int sd_init(void)
{
    int ret;

    bitset32(&CGU_PERI, CGU_MCI_CLOCK_ENABLE);

    CGU_IDE =   (1<<7)          /* AHB interface enable */
            |   (AS3525_IDE_DIV << 2)
            |   1;              /* clock source = PLLA */

    CGU_MEMSTICK =  (1<<7)      /* interface enable */
                 |  (AS3525_MS_DIV << 2)
                 |  1;          /* clock source = PLLA */

    CGU_SDSLOT = (1<<7)         /* interface enable */
               | (AS3525_SDSLOT_DIV << 2)
               | 1;             /* clock source = PLLA */

    semaphore_init(&transfer_completion_signal, 1, 0);
    semaphore_init(&command_completion_signal, 1, 0);

#if defined(SANSA_FUZEV2) || defined(SANSA_CLIPPLUS)
    if (amsv2_variant == 1)
        GPIOB_DIR |= 1 << 5;
#endif

#ifdef HAVE_MULTIDRIVE
    /* clear previous irq */
    GPIOA_IC = EXT_SD_BITS;
    /* enable edge detecting */
    GPIOA_IS &= ~EXT_SD_BITS;
    /* detect both raising and falling edges */
    GPIOA_IBE |= EXT_SD_BITS;
    /* enable the card detect interrupt */
    GPIOA_IE |= EXT_SD_BITS;
#endif /* HAVE_MULTIDRIVE */

#ifndef SANSA_CLIPV2
    /* Configure XPD for SD-MCI interface */
    bitset32(&CCU_IO, 1<<2);
#endif

    VIC_INT_ENABLE = INTERRUPT_NAND;

    init_controller();
    ret = sd_init_card(INTERNAL_AS3525);
    if(ret < 0)
        return ret;

    /* init mutex */
    mutex_init(&sd_mtx);

    queue_init(&sd_queue, true);
    create_thread(sd_thread, sd_stack, sizeof(sd_stack), 0,
            sd_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));

#ifndef BOOTLOADER
    sd_enabled = true;
    sd_enable(false);
#endif
    return 0;
}

static int sd_transfer_sectors(IF_MD2(int drive,) unsigned long start,
                                int count, void* buf, bool write)
{
    int ret = 0;
#ifndef HAVE_MULTIDRIVE
    const int drive = 0;
#endif
    bool aligned = !((uintptr_t)buf & (CACHEALIGN_SIZE - 1));
    int const retry_all_max = 1;
    int retry_all = 0;
    int const retry_data_max = 100; /* Generous, methinks */
    int retry_data;
    unsigned int real_numblocks;

    mutex_lock(&sd_mtx);
#ifndef BOOTLOADER
    sd_enable(true);
    led(true);
#endif

    if(count < 0) /* XXX: why is it signed ? */
    {
        ret = -18;
        goto sd_transfer_error_no_dma;
    }

    /* skip SanDisk OF */
    if (drive == INTERNAL_AS3525)
        start += AMS_OF_SIZE;

    /* no need for complete retry on main, just SD */
    if (drive == SD_SLOT_AS3525)
        retry_all = retry_all_max;

sd_transfer_retry_with_reinit:
    if (card_info[drive].initialized <= 0)
    {
        ret = sd_init_card(drive);
        if (!(card_info[drive].initialized))
            goto sd_transfer_error_no_dma;
    }

    /* Check the real block size after the card has been initialized */
    real_numblocks = card_info[drive].numblocks;
    /* 'start' represents the real (physical) starting sector
     *  so we must compare it to the real (physical) number of sectors */
    if (drive == INTERNAL_AS3525)
        real_numblocks += AMS_OF_SIZE;
    if ((start+count) > real_numblocks)
    {
        ret = -19;
        goto sd_transfer_error_no_dma;
    }

    /*  CMD7 w/rca: Select card to put it in TRAN state */
    if(!send_cmd(drive, SD_SELECT_CARD, card_info[drive].rca, MCI_NO_RESP, NULL))
    {
        ret = -20;
        goto sd_transfer_error_no_dma;
    }

    dma_retain();

    if(aligned)
    {   /* direct transfer, indirect is always uncached */
        if(write)
            commit_dcache_range(buf, count * SECTOR_SIZE);
        else
            discard_dcache_range(buf, count * SECTOR_SIZE);
    }

    const int cmd = write ? SD_WRITE_MULTIPLE_BLOCK : SD_READ_MULTIPLE_BLOCK;
    retry_data = retry_data_max;

    while (1)
    {
        void *dma_buf;
        unsigned int transfer = count;

        last_disk_activity = current_tick;

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

        /* Interrupt handler might set this to true during transfer */
        retry = false;

        MCI_BYTCNT = transfer * SD_BLOCK_SIZE;

        ret = sd_wait_for_tran_state(drive);
        if (ret < 0)
        {
            ret -= 25;
            goto sd_transfer_error;
        }

        int arg = start;
        if(!(card_info[drive].ocr & (1<<30))) /* not SDHC */
            arg *= SD_BLOCK_SIZE;

        if(write)
            dma_enable_channel(0, dma_buf, MCI_FIFO, DMA_PERI_SD,
                DMAC_FLOWCTRL_PERI_MEM_TO_PERI, true, false, 0, DMA_S8, NULL);
        else
            dma_enable_channel(0, MCI_FIFO, dma_buf, DMA_PERI_SD,
                DMAC_FLOWCTRL_PERI_PERI_TO_MEM, false, true, 0, DMA_S8, NULL);

        unsigned long dummy; /* if we don't ask for a response, writing fails */
        if(!send_cmd(drive, cmd, arg, MCI_RESP, &dummy))
        {
            ret = -21;
            goto sd_transfer_error;
        }

        semaphore_wait(&transfer_completion_signal, TIMEOUT_BLOCK);

        last_disk_activity = current_tick;

        if(write)
        {
            /* wait for the card to exit programming state */
            while(MCI_STATUS & DATA_BUSY) ;
        }

        if(!send_cmd(drive, SD_STOP_TRANSMISSION, 0, MCI_NO_RESP, NULL))
        {
            ret = -22;
            goto sd_transfer_error;
        }

        if(!retry)
        {
            if(!write && !aligned)
                memcpy(buf, uncached_buffer, transfer * SD_BLOCK_SIZE);
            buf += transfer * SD_BLOCK_SIZE;
            start += transfer;
            count -= transfer;

            if (count > 0)
                continue;
        }
        else   /*  reset controller if we had an error  */
        {
            MCI_CTRL |= (FIFO_RESET|DMA_RESET);
            while(MCI_CTRL & (FIFO_RESET|DMA_RESET))
                ;
            if (--retry_data >= 0)
                continue;
        }

        break;
    }

    dma_release();

    /* CMD lines are separate, not common, so we need to actively deselect */
    /*  CMD7 w/rca =0 : deselects card & puts it in STBY state */
    if(!send_cmd(drive, SD_DESELECT_CARD, 0, MCI_NO_RESP, NULL))
    {
        ret = -23;
        goto sd_transfer_error;
    }

    while (1)
    {
#ifndef BOOTLOADER
        sd_enable(false);
        led(false);
#endif
        mutex_unlock(&sd_mtx);
        return ret;

sd_transfer_error:
        dma_release();

sd_transfer_error_no_dma:
        card_info[drive].initialized = 0;

        /* .initialized might have been >= 0 but now stale if the ata sd thread
         * isn't handling an insert because of USB */
        if (--retry_all >= 0)
            goto sd_transfer_retry_with_reinit;
    }
}

int sd_read_sectors(IF_MD2(int drive,) unsigned long start, int count,
                    void* buf)
{
    return sd_transfer_sectors(IF_MD2(drive,) start, count, buf, false);
}

int sd_write_sectors(IF_MD2(int drive,) unsigned long start, int count,
                     const void* buf)
{
#if defined(BOOTLOADER) /* we don't need write support in bootloader */
#ifdef HAVE_MULTIDRIVE
    (void) drive;
#endif
    (void) start;
    (void) count;
    (void) buf;
    return -1;
#else
    return sd_transfer_sectors(IF_MD2(drive,) start, count, (void*)buf, true);
#endif /* defined(BOOTLOADER) */
}

#ifndef BOOTLOADER
long sd_last_disk_activity(void)
{
    return last_disk_activity;
}

void sd_enable(bool on)
{
    if (on)
    {
        bitset32(&CGU_PERI, CGU_MCI_CLOCK_ENABLE);
        CGU_IDE |= (1<<7);                  /* AHB interface enable */
        CGU_MEMSTICK |= (1<<7);             /* interface enable */
        CGU_SDSLOT |= (1<<7);               /* interface enable */
    }
    else
    {
        CGU_SDSLOT &= ~(1<<7);              /* interface enable */
        CGU_MEMSTICK &= ~(1<<7);            /* interface enable */
        CGU_IDE &= ~(1<<7);                 /* AHB interface enable */
        bitclr32(&CGU_PERI, CGU_MCI_CLOCK_ENABLE);
    }
}

tCardInfo *card_get_info_target(int card_no)
{
    return &card_info[card_no];
}
#endif /* BOOTLOADER */

#ifdef HAVE_HOTSWAP
bool sd_removable(IF_MD_NONVOID(int drive))
{
    return (drive==1);
}

bool sd_present(IF_MD_NONVOID(int drive))
{
    return (drive == 0) ? true : card_detect_target();
}

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
#endif /* HAVE_HOTSWAP */

#ifdef CONFIG_STORAGE_MULTI
int sd_num_drives(int first_drive)
{
    /* We don't care which logical drive number(s) we have been assigned */
    (void)first_drive;

    return NUM_DRIVES;
}
#endif /* CONFIG_STORAGE_MULTI */
