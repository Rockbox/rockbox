/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bob Cousins
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
 
//#define SD_DEBUG

#include "sd.h"
#include "system.h"
#include <string.h>
#include "thread.h"
#include "panic.h"

#ifdef SD_DEBUG
#include "uart-s3c2440.h"
#endif
#ifdef HAVE_HOTSWAP
#include "hotswap.h"
#include "disk.h"
#include "fat.h"
#endif
#include "dma-target.h"     
#include "system-target.h"
#include "led-mini2440.h"

/* The configuration method is not very flexible. */
#define CARD_NUM_SLOT   0
#define NUM_CARDS       2

#if NUM_CARDS < NUM_DRIVES
#error NUM_CARDS less than NUM_DRIVES
#endif

#define EC_OK                    0
#define EC_FAILED                1
#define EC_NOCARD                2
#define EC_WAIT_STATE_FAILED     3
#define EC_POWER_UP              4
#define EC_FIFO_WR_EMPTY         5
#define EC_FIFO_WR_DONE          6
#define EC_TRAN_READ_ENTRY       7
#define EC_TRAN_READ_EXIT        8
#define EC_TRAN_WRITE_ENTRY      9
#define EC_TRAN_WRITE_EXIT       10
#define EC_COMMAND               11
#define EC_WRITE_PROTECT         12

#define MIN_YIELD_PERIOD        1000
#define UNALIGNED_NUM_SECTORS   10
#define MAX_TRANSFER_ERRORS     10

/* command flags for send_cmd */
#define MCI_NO_FLAGS    (0<<0)
#define MCI_RESP        (1<<0)
#define MCI_LONG_RESP   (1<<1)
#define MCI_ARG         (1<<2)

#define INITIAL_CLK    400000   /* Initial clock */
#define SD_CLK       24000000   /* Clock for SD cards */
#define MMC_CLK      15000000   /* Clock for MMC cards */

#define SD_ACTIVE_LED   LED4

#ifdef SD_DEBUG
#define dbgprintf uart_printf
#else
#define dbgprintf(...)
#endif

struct sd_card_status
{
    int retry;
    int retry_max;
};

/** static, private data **/ 

/* for compatibility */
static long last_disk_activity = -1;

static bool initialized = false;
static bool sd_enabled = false;
static long next_yield = 0;

static tCardInfo card_info [NUM_CARDS];

#ifdef HAVE_MULTIDRIVE
static int curr_card = 0; /* current active card */
#if 0
static struct sd_card_status sd_status[NUM_CARDS] =
{
#if NUM_CARDS > 1
    {0, 10},
#endif
    {0, 10}
};
#endif
#endif

/* Shoot for around 75% usage */
static long             sd_stack [(DEFAULT_STACK_SIZE*2 + 0x1c0)/sizeof(long)];
static const char               sd_thread_name[] = "sd";
static struct mutex             sd_mtx SHAREDBSS_ATTR;
static struct event_queue       sd_queue;
static struct wakeup            transfer_completion_signal;
static volatile unsigned int    transfer_error[NUM_VOLUMES];
/* align on cache line size */
static unsigned char    aligned_buffer[UNALIGNED_NUM_SECTORS * SD_BLOCK_SIZE] 
                        __attribute__((aligned(32)));
static unsigned char *  uncached_buffer;

static inline void mci_delay(void) 
{ 
    int i = 0xffff; 
    while (i--)
        asm volatile ("nop\n");
}

/* TODO: should be in target include file */
/*****************************************************************************
    Definitions specific to Mini2440
 *****************************************************************************/

#define SD_CD (1<<8)    /* Port G */
#define SD_WP (1<<8)    /* Port H */

/*****************************************************************************
    Functions specific to S3C2440 SoC
 *****************************************************************************/

#ifdef SD_DEBUG
static unsigned reg_copy[16], reg_copy2[16];
static void get_regs (unsigned *regs)
{
    unsigned j;
    volatile unsigned long *sdi_reg = &SDICON;
    
    for (j=0; j < 16;j++)
    {
        *regs++ = *sdi_reg++;
    }
}

static void dump_regs (unsigned *regs1, unsigned *regs2)
{
    unsigned j;
    volatile unsigned long*sdi_reg = &SDICON;
    unsigned long diff;
    
    for (j=0; j < 16;j++)
    {
        diff = *regs1 ^ *regs2;
        if (diff)
            dbgprintf ("%8x %8x %8x %8x\n", sdi_reg, *regs1, *regs2, diff );
        regs1++;
        regs2++;
        sdi_reg++;
    }
}
#endif

static void debug_r1(int cmd)
{
#if defined(SD_DEBUG)
    dbgprintf("CMD%2.2d:SDICSTA=%04x [%c%c%c%c%c-%c%c%c%c%c%c%c]  SDIRSP0=%08x [%d %s] \n", 
        cmd, 
        SDICSTA, 
        (SDICSTA & S3C2410_SDICMDSTAT_CRCFAIL)    ? 'C' : ' ', 
        (SDICSTA & S3C2410_SDICMDSTAT_CMDSENT)    ? 'S' : ' ', 
        (SDICSTA & S3C2410_SDICMDSTAT_CMDTIMEOUT) ? 'T' : ' ', 
        (SDICSTA & S3C2410_SDICMDSTAT_RSPFIN)     ? 'R' : ' ', 
        (SDICSTA & S3C2410_SDICMDSTAT_XFERING)    ? 'X' : ' ', 
        
        (SDICSTA & 0x40)    ? 'P' : ' ', 
        (SDICSTA & 0x20)    ? 'A' : ' ', 
        (SDICSTA & 0x10)    ? 'E' : ' ', 
        (SDICSTA & 0x08)    ? 'C' : ' ', 
        (SDICSTA & 0x04)    ? 'I' : ' ', 
        (SDICSTA & 0x02)    ? 'R' : ' ', 
        (SDICSTA & 0x01)    ? 'Z' : ' ', 
        
        SDIRSP0,
        SD_R1_CURRENT_STATE(SDIRSP0),
        (SDIRSP0 & SD_R1_READY_FOR_DATA) ? "RDY " : "    "
        );
#else
    (void)cmd;
#endif
}

void SDI (void)
{
    int status = SDIDSTA;
#ifndef HAVE_MULTIDRIVE
    const int curr_card = 0;
#endif     
    
    transfer_error[curr_card] = status
#if 0
        & ( S3C2410_SDIDSTA_CRCFAIL | S3C2410_SDIDSTA_RXCRCFAIL |
            S3C2410_SDIDSTA_DATATIMEOUT )
#endif
        ;

    SDIDSTA |= S3C2410_SDIDSTA_CLEAR_BITS;  /* needed to clear int  */

    dbgprintf ("SDI %x\n", transfer_error[curr_card]);
    
    wakeup_signal(&transfer_completion_signal);

    /* Ack the interrupt */
    SRCPND = SDI_MASK;
    INTPND = SDI_MASK;
}

#if 0
void dma_callback (void)
{
    const int status = SDIDSTA;
    
    transfer_error[0] = status & (S3C2410_SDIDSTA_CRCFAIL |
        S3C2410_SDIDSTA_RXCRCFAIL   |
        S3C2410_SDIDSTA_DATATIMEOUT );

    SDIDSTA |= S3C2410_SDIDSTA_CLEAR_BITS;  /* needed to clear int  */
    
    dbgprintf ("dma_cb\n");
    wakeup_signal(&transfer_completion_signal);
}
#endif

static void init_sdi_controller(const int card_no)
{
    (void)card_no;

/*****************************************************************************/
#ifdef MINI2440
    /* Specific to Mini2440 */
    
    /* Enable pullups on SDCMD and SDDAT pins */
    S3C2440_GPIO_PULLUP (GPEUP, 6, GPIO_PULLUP_ENABLE);
    S3C2440_GPIO_PULLUP (GPEUP, 7, GPIO_PULLUP_ENABLE);
    S3C2440_GPIO_PULLUP (GPEUP, 8, GPIO_PULLUP_ENABLE);
    S3C2440_GPIO_PULLUP (GPEUP, 9, GPIO_PULLUP_ENABLE);
    S3C2440_GPIO_PULLUP (GPEUP, 10, GPIO_PULLUP_ENABLE);
    
    /* Enable special function for SDCMD, SDCLK and SDDAT pins */
    S3C2440_GPIO_CONFIG (GPECON, 5, GPIO_FUNCTION);
    S3C2440_GPIO_CONFIG (GPECON, 6, GPIO_FUNCTION);
    S3C2440_GPIO_CONFIG (GPECON, 7, GPIO_FUNCTION);
    S3C2440_GPIO_CONFIG (GPECON, 8, GPIO_FUNCTION);
    S3C2440_GPIO_CONFIG (GPECON, 9, GPIO_FUNCTION);
    S3C2440_GPIO_CONFIG (GPECON, 10, GPIO_FUNCTION);
    
    /* Card Detect input */
    S3C2440_GPIO_CONFIG (GPGCON, 8, GPIO_INPUT);
    
    /* Write Protect input */
    S3C2440_GPIO_CONFIG (GPHCON, 8, GPIO_INPUT);
/*****************************************************************************/
#else
#error Unsupported target
#endif
/*****************************************************************************/
    
    /* About 400KHz for initial comms with card */
    SDIPRE   = PCLK / INITIAL_CLK - 1;
    /* Byte order=Type A (Little Endian), clock enable */
    SDICON   = S3C2410_SDICON_CLOCKTYPE;       
    SDIFSTA  |= S3C2440_SDIFSTA_FIFORESET;
    SDIBSIZE = SD_BLOCK_SIZE;
    SDIDTIMER= 0x7fffff;      /* Set timeout count - max value */

    /* Enable interupt on Data Finish or data transfer error */
    /* Clear pending source */
    SRCPND = SDI_MASK;
    INTPND = SDI_MASK;

#if 1
    /* Enable interrupt in controller */
    s3c_regclr32(&INTMOD, SDI_MASK);
    s3c_regclr32(&INTMSK, SDI_MASK);
    
    SDIIMSK |= S3C2410_SDIIMSK_DATAFINISH 
               | S3C2410_SDIIMSK_DATATIMEOUT
               | S3C2410_SDIIMSK_DATACRC     
               | S3C2410_SDIIMSK_CRCSTATUS   
               | S3C2410_SDIIMSK_FIFOFAIL
               ;
#endif
}

static bool send_cmd(const int card_no, const int cmd, const int arg,
                     const int flags, long *response)
{
    bool ret;
    unsigned val, status;
    (void)card_no;

#ifdef SD_DEBUG
    get_regs (reg_copy);
#endif
    /* A major bodge. For some reason a delay is required here */
    mci_delay();
    dbgprintf ("send_cmd: c=%3.3d a=%08x f=%02x  \n", cmd, arg, flags);

#ifdef SD_DEBUG
    get_regs (reg_copy2);
    dump_regs (reg_copy, reg_copy2);
#endif
    
#if 0
    while (SDICSTA & S3C2410_SDICMDSTAT_XFERING)
        ; /* wait ?? */
#endif    
    /* set up new command */
    
    if (flags & MCI_ARG)
        SDICARG = arg;
    else
        SDICARG = 0;
    
    val = cmd | S3C2410_SDICMDCON_CMDSTART | S3C2410_SDICMDCON_SENDERHOST;
    if(flags & MCI_RESP)
    {
        val |= S3C2410_SDICMDCON_WAITRSP;
        if(flags & MCI_LONG_RESP)
            val |= S3C2410_SDICMDCON_LONGRSP;
    }
    
    /* Clear command/data status flags */
    SDICSTA |= 0x0f << 9;
    SDIDSTA |= S3C2410_SDIDSTA_CLEAR_BITS;
        
    /* Initiate the command */
    SDICCON = val;
             
    if (flags & MCI_RESP)
    {
        /* wait for response or timeout */
        do 
        {
            status = SDICSTA;
        } while ( (status & (S3C2410_SDICMDSTAT_RSPFIN | 
                             S3C2410_SDICMDSTAT_CMDTIMEOUT) ) == 0);
        debug_r1(cmd);
        if (status & S3C2410_SDICMDSTAT_CMDTIMEOUT)
            ret = false;
        else if (status & (S3C2410_SDICMDSTAT_RSPFIN))
        {   
            /* resp received */
            if(flags & MCI_LONG_RESP)
            {
                /* store the response in reverse word order */
                response[0] = SDIRSP3;
                response[1] = SDIRSP2;
                response[2] = SDIRSP1;
                response[3] = SDIRSP0;
            }
            else
                response[0] = SDIRSP0;
            ret = true;
        }
        else
            ret = true;
    }
    else 
    {
        /* wait for command completion or timeout */
        do 
        {
            status = SDICSTA;
        } while ( (status & (S3C2410_SDICMDSTAT_CMDSENT |
                          S3C2410_SDICMDSTAT_CMDTIMEOUT)) == 0);
        debug_r1(cmd);
        if (status & S3C2410_SDICMDSTAT_CMDTIMEOUT)
            ret = false;
        else
            ret = true;
    }
    
    /* Clear Command status flags */
    SDICSTA |= 0x0f << 9;
    
    mci_delay();
    
    return ret;
}

static int sd_init_card(const int card_no)
{
    unsigned long temp_reg[4];
    unsigned long response;
    long init_timeout;
    bool sdhc;
    int i;

    if(!send_cmd(card_no, SD_GO_IDLE_STATE, 0, MCI_NO_FLAGS, NULL))
        return -1;

    mci_delay();

    sdhc = false;
    if(send_cmd(card_no, SD_SEND_IF_COND, 0x1AA, MCI_RESP|MCI_ARG, &response))
        if((response & 0xFFF) == 0x1AA)
            sdhc = true;

    /* timeout for initialization is 1sec, from SD Specification 2.00 */
    init_timeout = current_tick + HZ;

    do {
        /* timeout */
        if(current_tick > init_timeout)
            return -2;

        /* app_cmd */
        if( !send_cmd(card_no, SD_APP_CMD, 0, MCI_RESP|MCI_ARG, &response) ||
            !(response & (1<<5)) )
        {
            return -3;
        }

        /* acmd41 */
        if(!send_cmd(card_no, SD_APP_OP_COND, (sdhc ? 0x40FF8000 : (1<<23)),
                        MCI_RESP|MCI_ARG, &card_info[card_no].ocr))
        {
            return -4;
        }

    } while(!(card_info[card_no].ocr & (1<<31)));

    /* send CID */
    if(!send_cmd(card_no, SD_ALL_SEND_CID, 0, MCI_RESP|MCI_LONG_RESP|MCI_ARG,
                            temp_reg))
        return -5;

    for(i=0; i<4; i++)
        card_info[card_no].cid[3-i] = temp_reg[i];

    /* send RCA */
    if(!send_cmd(card_no, SD_SEND_RELATIVE_ADDR, 0, MCI_RESP|MCI_ARG,
                &card_info[card_no].rca))
        return -6;

    /* send CSD */
    if(!send_cmd(card_no, SD_SEND_CSD, card_info[card_no].rca,
                 MCI_RESP|MCI_LONG_RESP|MCI_ARG, temp_reg))
        return -7;

    for(i=0; i<4; i++)
        card_info[card_no].csd[3-i] = temp_reg[i];

    sd_parse_csd(&card_info[card_no]);

    if(!send_cmd(card_no, SD_SELECT_CARD, card_info[card_no].rca, MCI_ARG, NULL))
        return -9;

    if(!send_cmd(card_no, SD_APP_CMD, card_info[card_no].rca, MCI_ARG, NULL))
        return -10;

    if(!send_cmd(card_no, SD_SET_BUS_WIDTH, card_info[card_no].rca | 2, MCI_ARG, NULL))
        return -11;

    if(!send_cmd(card_no, SD_SET_BLOCKLEN, card_info[card_no].blocksize, MCI_ARG,
                 NULL))
        return -12;

    card_info[card_no].initialized = 1;

    /* full speed for controller clock */
    SDIPRE   = PCLK / SD_CLK - 1;
    mci_delay();

    return EC_OK;
}

/*****************************************************************************
    Generic functions
 *****************************************************************************/

static inline bool card_detect_target(void)
{
    /* TODO - use interrupt on change? */
#ifdef MINI2440
    return (GPGDAT & SD_CD) == 0;
#else
#error Unsupported target
#endif
}


/*****************************************************************************/
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

void card_enable_monitoring_target(bool on)
{
    if (on)
    {   /* enable external irq 8-23 on the internal interrupt controller */
        INTMSK &= ~1<<5;
        /* enable GPG8 IRQ on the external interrupt controller */
        EINTMASK &= ~(1<<16);
    }
    else
    {
        /* mask internal and external IRQs */
        INTMSK |=  1<<5;
        EINTMASK |= (1<<16);
    }
}

void EINT8_23(void)
{
    static struct timeout sd1_oneshot;
    EINTPEND = (1<<16); /* ack irq on external, then internal irq controller */
    SRCPND = (1<<5);
    INTPND = (1<<5);
    /* add task to inform the system about the SD insertion
     * sanity check if it's still inserted after 300ms  */
    timeout_register(&sd1_oneshot, sd1_oneshot_callback, (3*HZ/10), 0);
}

bool sd_removable(IF_MD_NONVOID(int card_no))
{
#ifndef HAVE_MULTIDRIVE
    const int card_no = 0;
#endif
    dbgprintf ("sd_remov (hs) [%d] %d\n", card_no, card_no == CARD_NUM_SLOT );
    return (card_no == CARD_NUM_SLOT);
}

bool sd_present(IF_MD_NONVOID(int card_no))
{
#ifdef HAVE_MULTIDRIVE
    (void)card_no;
#endif
    dbgprintf ("sd_pres (hs) [%d] %d\n", card_no, card_detect_target());
    return card_detect_target();
}

/*****************************************************************************/
#else

bool sd_removable(IF_MD_NONVOID(int card_no))
{
#ifndef HAVE_MULTIDRIVE
    const int card_no = 0;
#endif
    (void)card_no;
    
    /* not applicable */
    dbgprintf ("sd_remov");
    return false;
}

#endif /* HAVE_HOTSWAP */
/*****************************************************************************/

static void sd_thread(void) __attribute__((noreturn));
static void sd_thread(void)
{
    struct queue_event ev;

    /* TODO */
    while (1)
    {
        queue_wait_w_tmo(&sd_queue, &ev, HZ);
        switch ( ev.id )
        {
#ifdef HAVE_HOTSWAP
        case SYS_HOTSWAP_INSERTED:
        case SYS_HOTSWAP_EXTRACTED:
        {
            int success = 1;
            fat_lock();          /* lock-out FAT activity first -
                                    prevent deadlocking via disk_mount that
                                    would cause a reverse-order attempt with
                                    another thread */
            mutex_lock(&sd_mtx); /* lock-out card activity - direct calls
                                    into driver that bypass the fat cache */

            /* We now have exclusive control of fat cache and ata */

            disk_unmount(0);     /* release "by force", ensure file
                                    descriptors aren't leaked and any busy
                                    ones are invalid if mounting */

            /* Force card init for new card, re-init for re-inserted one or
             * clear if the last attempt to init failed with an error. */
            card_info[0].initialized = 0;

            if (ev.id == SYS_HOTSWAP_INSERTED)
            {
                /* FIXME: once sd_enabled is implement properly,
                 * reinitializing the controllers might be needed */
                sd_enable(true);
                if (success < 0) /* initialisation failed */
                    panicf("SD init failed : %d", success);
                success = disk_mount(0); /* 0 if fail */
            }

            /* notify the system about the changed filesystems
             */
            if (success)
                queue_broadcast(SYS_FS_CHANGED, 0);

            /* Access is now safe */
            mutex_unlock(&sd_mtx);
            fat_unlock();
            sd_enable(false);
        }
            break;
#endif
        }        
    }
}

static int sd_wait_for_state(const int card_no, unsigned int state)
{
    unsigned long response = 0;
    unsigned int timeout = HZ; /* ticks */
    long t = current_tick;

    while (1)
    {
        long tick;

        if(!send_cmd(card_no, SD_SEND_STATUS, card_info[card_no].rca,
                    MCI_RESP|MCI_ARG, &response))
            return -1;

        if( (SD_R1_CURRENT_STATE(response) == state) )
            return 0;

        if(TIME_AFTER(current_tick, t + timeout))
            return -2;

        if (TIME_AFTER((tick = current_tick), next_yield))
        {
            yield();
            timeout += current_tick - tick;
            next_yield = tick + MIN_YIELD_PERIOD;
        }
    }
}

static int sd_transfer_sectors(int card_no, unsigned long start,
                               int count, void* buf, const bool write)
{
    int ret = EC_OK;
    unsigned loops = 0;
    struct dma_request request;

    mutex_lock(&sd_mtx);
    sd_enable(true);
    set_leds(SD_ACTIVE_LED);

#ifdef HAVE_MULTIDRIVE
    curr_card = card_no;
#endif
    if (card_info[card_no].initialized <= 0)
    {
        ret = sd_init_card(card_no);
        if (!(card_info[card_no].initialized))
            goto sd_transfer_error;
    }

    last_disk_activity = current_tick;

    ret = sd_wait_for_state(card_no, SD_TRAN);
    if (ret < 0)
    {
        ret -= 20;
        goto sd_transfer_error;
    }

    dma_retain();

    while(count)
    {
        /* 128 * 512 = 2^16, and doesn't fit in the 16 bits of DATA_LENGTH
         * register, so we have to transfer maximum 127 sectors at a time. */
        unsigned int transfer = (count >= 128) ? 127 : count; /* sectors */
        void *dma_buf;
        const int cmd =
            write ? SD_WRITE_MULTIPLE_BLOCK : SD_READ_MULTIPLE_BLOCK;
        unsigned long start_addr = start;

        dma_buf = aligned_buffer;
        if(transfer > UNALIGNED_NUM_SECTORS)
            transfer = UNALIGNED_NUM_SECTORS;
        if(write)
            memcpy(uncached_buffer, buf, transfer * SD_BLOCK_SIZE);

        /* Set start_addr to the correct unit (blocks or bytes) */
        if(!(card_info[card_no].ocr & SD_OCR_CARD_CAPACITY_STATUS))/* not SDHC */
            start_addr *= SD_BLOCK_SIZE;

        /* TODO? */
        SDIFSTA = SDIFSTA | S3C2440_SDIFSTA_FIFORESET;
        SDIDCON = S3C2440_SDIDCON_DS_WORD   | 
                  S3C2410_SDIDCON_BLOCKMODE | S3C2410_SDIDCON_WIDEBUS |
                  S3C2410_SDIDCON_DMAEN     |
                  S3C2440_SDIDCON_DATSTART  |  
                  ( transfer << 0);
        if (write)
            SDIDCON |= S3C2410_SDIDCON_TXAFTERRESP | S3C2410_SDIDCON_XFER_TXSTART;
        else
            SDIDCON |= S3C2410_SDIDCON_RXAFTERCMD | S3C2410_SDIDCON_XFER_RXSTART;

        SDIDSTA |= S3C2410_SDIDSTA_CLEAR_BITS;  /* needed to clear int  */
        SRCPND = SDI_MASK;
        INTPND = SDI_MASK;

        /* Initiate read/write command */
        if(!send_cmd(card_no, cmd, start_addr, MCI_ARG | MCI_RESP, NULL))
        {
            ret -= 3*20;
            goto sd_transfer_error;
        }

        if(write)
        {
            request.source_addr    = dma_buf;
            request.source_control = DISRCC_LOC_AHB | DISRCC_INC_AUTO;            
            request.dest_addr      = &SDIDAT_LLE;
            request.dest_control   = DISRCC_LOC_APB | DISRCC_INC_FIXED;            
            request.count          = transfer * SD_BLOCK_SIZE / sizeof(long);
            request.source_map     = DMA_SRC_MAP_SDI;
            request.control        = DCON_DMD_HS | DCON_SYNC_APB |  
                                     DCON_HW_SEL |
                                     DCON_NO_RELOAD | DCON_DSZ_WORD;
            request.callback       = NULL;        
            
            dma_enable_channel(0, &request);
        }
        else
        {
            request.source_addr    = &SDIDAT_LLE;
            request.source_control = DISRCC_LOC_APB | DISRCC_INC_FIXED;            
            request.dest_addr      = dma_buf;
            request.dest_control   = DISRCC_LOC_AHB | DISRCC_INC_AUTO;            
            request.count          = transfer * SD_BLOCK_SIZE / sizeof(long);
            request.source_map     = DMA_SRC_MAP_SDI;
            request.control        = DCON_DMD_HS | DCON_SYNC_APB |  
                                     DCON_HW_SEL |
                                     DCON_NO_RELOAD | DCON_DSZ_WORD;
            request.callback       = NULL;        
            
            dma_enable_channel(0, &request); 
        }

#if 0
        /* FIXME : we should check if the timeouts calculated from the card's
         * CSD are lower, and use them if it is the case
         * Note : the OF doesn't seem to use them anyway */
        MCI_DATA_TIMER(drive) = write ?
                SD_MAX_WRITE_TIMEOUT : SD_MAX_READ_TIMEOUT;
        MCI_DATA_LENGTH(drive) = transfer * card_info[drive].blocksize;
        MCI_DATA_CTRL(drive) =  (1<<0) /* enable */                     |
                                (!write<<1) /* transfer direction */    |
                                (1<<3) /* DMA */                        |
                                (9<<4) /* 2^9 = 512 */ ;
#endif

        wakeup_wait(&transfer_completion_signal, 100 /*TIMEOUT_BLOCK*/);
        
        /* wait for DMA to finish */
        while (DSTAT0 & DSTAT_STAT_BUSY)
            ;
            
#if 0        
        status = SDIDSTA;
        while ((status & (S3C2410_SDIDSTA_DATATIMEOUT|S3C2410_SDIDSTA_XFERFINISH)) == 0)
        {
            status = SDIDSTA;
        }
        dbgprintf("%x \n", status);
#endif
        if( transfer_error[card_no] & S3C2410_SDIDSTA_XFERFINISH )
        {
            if(!write)
                memcpy(buf, uncached_buffer, transfer * SD_BLOCK_SIZE);
            buf += transfer * SD_BLOCK_SIZE;
            start += transfer;
            count -= transfer;
            loops = 0;  /* reset errors counter */
        }
        else 
        {
            dbgprintf ("SD transfer error : 0x%x\n", transfer_error[card_no]);
                
            if(loops++ > MAX_TRANSFER_ERRORS)
            {
                led_flash(LED1|LED2, LED3|LED4);
                /* panicf("SD transfer error : 0x%x", transfer_error[card_no]); */
            }
        }

        last_disk_activity = current_tick;

        if(!send_cmd(card_no, SD_STOP_TRANSMISSION, 0, MCI_RESP, NULL))
        {
            ret = -4*20;
            goto sd_transfer_error;
        }

#if 0
        ret = sd_wait_for_state(card_no, SD_TRAN);
        if (ret < 0)
        {
            ret -= 5*20;
            goto sd_transfer_error;
        }
#endif
    }

    ret = EC_OK;

sd_transfer_error:

    dma_release();

    clear_leds(SD_ACTIVE_LED);
    sd_enable(false);

    if (ret)    /* error */
        card_info[card_no].initialized = 0;

    mutex_unlock(&sd_mtx);
    return ret;
}

int sd_read_sectors(IF_MD2(int card_no,) unsigned long start, int incount,
                     void* inbuf)
{
    int ret;
  
#ifdef HAVE_MULTIDRIVE
    dbgprintf ("sd_read %d %x %d\n", card_no, start, incount);
#else
    dbgprintf ("sd_read %x %d\n", start, incount);
#endif
#ifdef HAVE_HOTSWAP_STORAGE_AS_MAIN
    if (!card_detect_target())
        ret = 0; /* assume success */
    else
#endif
        ret = sd_transfer_sectors(card_no, start, incount, inbuf, false);
    dbgprintf ("sd_read, ret=%d\n", ret);
    return ret;
}

/*****************************************************************************/
int sd_write_sectors(IF_MD2(int card_no,) unsigned long start, int count,
                      const void* outbuf)
{
#ifdef BOOTLOADER /* we don't need write support in bootloader */
#ifdef HAVE_MULTIDRIVE
    (void) drive;
#endif
    (void) start;
    (void) count;
    (void) outbuf;
    return -1;
#else
#ifdef HAVE_MULTIDRIVE
    dbgprintf ("sd_write %d %x %d\n", card_no, start, count);
#else
    dbgprintf ("sd_write %x %d\n", start, count);
#endif
#ifdef HAVE_HOTSWAP_STORAGE_AS_MAIN
    if (!card_detect_target())
        return 0; /* assume success */
    else
#endif
        return sd_transfer_sectors(card_no, start, count, (void*)outbuf, true);
#endif
}
/*****************************************************************************/

void sd_enable(bool on)
{
    dbgprintf ("sd_enable %d\n", on);
    /* TODO: enable/disable SDI clock */
    
    if (sd_enabled == on)
        return; /* nothing to do */
    if (on)
    {
        sd_enabled = true;
    }
    else
    {
        sd_enabled = false;
    }
}
    
int sd_init(void)
{
    int ret = EC_OK;
    dbgprintf ("\n==============================\n");
    dbgprintf ("             sd_init\n");
    dbgprintf ("==============================\n");
    
    init_sdi_controller (0);
#ifndef BOOTLOADER
    sd_enabled = true;
    sd_enable(false);
#endif
    wakeup_init(&transfer_completion_signal);
    /* init mutex */
    mutex_init(&sd_mtx);
    queue_init(&sd_queue, true);
    create_thread(sd_thread, sd_stack, sizeof(sd_stack), 0,
            sd_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));

    uncached_buffer = UNCACHED_ADDR(&aligned_buffer[0]);

#ifdef HAVE_HOTSWAP
    /*
     * prepare detecting of SD insertion (not extraction) */
    unsigned long for_extint = EXTINT2;
    unsigned long for_gpgcon = GPGCON;
    for_extint &= ~0x7;
#ifdef HAVE_HOTSWAP_STORAGE_AS_MAIN
    for_extint |=  0x2; /* detect falling edge only (0 means SD inserted) */
#else
    for_extint |=  0x3; /* detect both, raising and falling, edges */
#endif
    for_gpgcon &= ~(0x3<<16);
    for_gpgcon |=  (0x2<<16); /* enable interrupt on pin 8 */
    EXTINT2 = for_extint;
    GPGCON = for_gpgcon;
#endif

    initialized = true;
    return ret;
}

long sd_last_disk_activity(void)
{
    return last_disk_activity;
}

tCardInfo *card_get_info_target(int card_no)
{    
    return &card_info[card_no];
}

/*****************************************************************************/
#ifdef CONFIG_STORAGE_MULTI

int sd_num_drives(int first_drive)
{
    dbgprintf ("sd_num_drv");
#if 0
    /* Store which logical drive number(s) we have been assigned */
    sd_first_drive = first_drive;
#endif
    
    return NUM_CARDS;
}

void sd_sleepnow(void)
{
}

bool sd_disk_is_active(void)
{
    return false;
}

int sd_soft_reset(void)
{
    return 0;
}

int sd_spinup_time(void)
{
    return 0;
}

#endif /* CONFIG_STORAGE_MULTI */
/*****************************************************************************/

