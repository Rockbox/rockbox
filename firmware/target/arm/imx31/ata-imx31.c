/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Will Robertson
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
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "power.h"
#include "panic.h"
#include "ata.h"
#include "ata-target.h"
#include "ccm-imx31.h"
#ifdef HAVE_ATA_DMA
#include "sdma-imx31.h"
#include "mmu-imx31.h"
#endif

/* PIO modes timing info */
static const struct ata_pio_timings
{
    uint16_t time_2w;       /* t2 during write */
    uint16_t time_2r;       /* t2 during read */
    uint8_t  time_ax;       /* tA */
    uint8_t  time_1;        /* t1 */
    uint8_t  time_4;        /* t4 */
    uint8_t  time_9;        /* t9 */
} pio_timings[5] =
{
    [0] =  /* PIO mode 0 */
    {
        .time_1 = 70,
        .time_2w = 290,
        .time_2r = 290,
        .time_ax = 35,
        .time_4 = 30,
        .time_9 = 20
    },
    [1] = /* PIO mode 1 */
    {
        .time_1 = 50,
        .time_2w = 290,
        .time_2r = 290,
        .time_ax = 35,
        .time_4 = 20,
        .time_9 = 15
    },
    [2] = /* PIO mode 2 */
    {
        .time_1 = 30,
        .time_2w = 290,
        .time_2r = 290,
        .time_ax = 35,
        .time_4 = 15,
        .time_9 = 10
    },
    [3] = /* PIO mode 3 */
    {
        .time_1 = 30,
        .time_2w = 80,
        .time_2r = 80,
        .time_ax = 35,
        .time_4 = 10,
        .time_9 = 10
    },
    [4] = /* PIO mode 4 */
    {
        .time_1 = 25,
        .time_2w = 70,
        .time_2r = 70,
        .time_ax = 35,
        .time_4 = 10,
        .time_9 = 10
    }
};

/* Track first init */
static bool initialized = false;

#ifdef HAVE_ATA_DMA
/* One DMA channel for reads, the other for writes othewise one channel would
 * have to be reinitialized every time the direction changed. (Different
 * SDMA scripts are used for reading or writing) */
#define ATA_DMA_CH_NUM_RD   3
#define ATA_DMA_CH_NUM_WR   4
/* Use default priority for these channels (1) - ATA isn't realtime urgent. */
/* Maximum DMA size per buffer descriptor (32-byte aligned) */
#define ATA_MAX_BD_SIZE     (65534 & ~31) /* 65504 */

/* Number of buffer descriptors required for a maximum sector count trasfer.
 * NOTE: Assumes LBA28 and 512-byte sectors! */
#define ATA_BASE_BD_COUNT   ((256*512 + (ATA_MAX_BD_SIZE-1)) / ATA_MAX_BD_SIZE)
#define ATA_BD_COUNT        (ATA_BASE_BD_COUNT + 2)

static const struct ata_mdma_timings
{
    uint8_t time_m;         /* tM */
    uint8_t time_jn;        /* tH */
    uint8_t time_d;         /* tD */
    uint8_t time_k;         /* tKW */
} mdma_timings[] =
{
    [0] = /* MDMA mode 0 */
    {
        .time_m = 50,
        .time_jn = 20,
        .time_d = 215,
        .time_k = 215
    },
    [1] = /* MDMA mode 1 */
    {
        .time_m = 30,
        .time_jn = 15,
        .time_d = 80,
        .time_k = 50
    },
    [2] = /* MDMA mode 2 */
    {
        .time_m = 25,
        .time_jn = 10,
        .time_d = 70,
        .time_k = 25
    }
};

static const struct ata_udma_timings
{
    uint8_t time_ack;       /* tACK */
    uint8_t time_env;       /* tENV */
    uint8_t time_rpx;       /* tRP */
    uint8_t time_zah;       /* tZAH */
    uint8_t time_mlix;      /* tMLI */
    uint8_t time_dvh;       /* tDVH */
    uint8_t time_dzfs;      /* tDVS+tDVH? */
    uint8_t time_dvs;       /* tDVS */
    uint8_t time_cvh;       /* ?? */
    uint8_t time_ss;        /* tSS */
    uint8_t time_cyc;       /* tCYC */
} udma_timings[] =
{
    [0] = /* UDMA mode 0 */
    {
        .time_ack = 20,
        .time_env = 20,
        .time_rpx = 160,
        .time_zah = 20,
        .time_mlix = 20,
        .time_dvh = 6,
        .time_dzfs = 80,
        .time_dvs = 70,
        .time_cvh = 6,
        .time_ss = 50,
        .time_cyc = 114
    },
    [1] = /* UDMA mode 1 */
    {
        .time_ack = 20,
        .time_env = 20,
        .time_rpx = 125,
        .time_zah = 20,
        .time_mlix = 20,
        .time_dvh = 6,
        .time_dzfs = 63,
        .time_dvs = 48,
        .time_cvh = 6,
        .time_ss = 50,
        .time_cyc = 75
    },
    [2] = /* UDMA mode 2 */
    {
        .time_ack = 20,
        .time_env = 20,
        .time_rpx = 100,
        .time_zah = 20,
        .time_mlix = 20,
        .time_dvh = 6,
        .time_dzfs = 47,
        .time_dvs = 34,
        .time_cvh = 6,
        .time_ss = 50,
        .time_cyc = 55
    },
    [3] = /* UDMA mode 3 */
    {
        .time_ack = 20,
        .time_env = 20,
        .time_rpx = 100,
        .time_zah = 20,
        .time_mlix = 20,
        .time_dvh = 6,
        .time_dzfs = 35,
        .time_dvs = 20,
        .time_cvh = 6,
        .time_ss = 50,
        .time_cyc = 39
    },
    [4] = /* UDMA mode 4 */
    {
        .time_ack = 20,
        .time_env = 20,
        .time_rpx = 100,
        .time_zah = 20,
        .time_mlix = 20,
        .time_dvh = 6,
        .time_dzfs = 25,
        .time_dvs = 7,
        .time_cvh = 6,
        .time_ss = 50,
        .time_cyc = 25
    },
#if 0
    [5] = /* UDMA mode 5 (bus clock 80MHz or higher only) */
    {
        .time_ack = 20,
        .time_env = 20,
        .time_rpx = 85,
        .time_zah = 20,
        .time_mlix = 20,
        .time_dvh = 6,
        .time_dzfs = 40,
        .time_dvs = 5,
        .time_cvh = 10,
        .time_ss = 50,
        .time_cyc = 17
    }
#endif
};

/** Threading **/
/* Signal to tell thread when DMA is done */
static struct wakeup ata_dma_wakeup;

/** SDMA **/
/* Array of buffer descriptors for large transfers and alignnment */
static struct buffer_descriptor ata_bda[ATA_BD_COUNT] NOCACHEBSS_ATTR;
/* ATA channel descriptors */
static struct channel_descriptor ata_cd_rd NOCACHEBSS_ATTR; /* read channel */
static struct channel_descriptor ata_cd_wr NOCACHEBSS_ATTR; /* write channel */
/* DMA channel to be started for transfer */
static unsigned int current_channel = 0;

/** Buffers **/
/* Scatter buffer for first and last 32 bytes of a non cache-aligned transfer
 * to cached RAM. */
static uint32_t scatter_buffer[32/4*2] NOCACHEBSS_ATTR;
/* Address of ends in destination buffer for unaligned reads - copied after
 * DMA completes. */
static void *sb_dst[2] = { NULL, NULL };

/** Modes **/
#define ATA_DMA_MWDMA    0x00000000             /* Using multiword DMA */
#define ATA_DMA_UDMA     ATA_DMA_ULTRA_SELECTED /* Using Ultra DMA */
#define ATA_DMA_PIO      0x80000000             /* Using PIO */
#define ATA_DMA_DISABLED 0x80000001             /* DMA init error - use PIO */
static unsigned long ata_dma_selected = ATA_DMA_PIO;
#endif /* HAVE_ATA_DMA */

static unsigned int get_T(void)
{
    /* T = ATA clock period in nanoseconds */
    return 1000 * 1000 * 1000 / ccm_get_ata_clk();
}

static void ata_wait_for_idle(void)
{
    while (!(ATA_INTERRUPT_PENDING & ATA_CONTROLLER_IDLE));
}

/* Route the INTRQ to either the MCU or SDMA depending upon whether there is
 * a DMA transfer in progress. */
static inline void ata_set_intrq(bool to_dma)
{
     ATA_INTERRUPT_ENABLE =
        (ATA_INTERRUPT_ENABLE & ~(ATA_INTRQ1 | ATA_INTRQ2)) |
            (to_dma ? ATA_INTRQ1 : ATA_INTRQ2);
}

/* Setup the timing for PIO mode */
void ata_set_pio_timings(int mode)
{
    const struct ata_pio_timings * const timings = &pio_timings[mode];
    unsigned int T = get_T();

    ata_wait_for_idle();

    ATA_TIME_1 = (timings->time_1 + T) / T;
    ATA_TIME_2W = (timings->time_2w + T) / T;
    ATA_TIME_2R = (timings->time_2r + T) / T;
    ATA_TIME_AX = (timings->time_ax + T) / T + 2; /* 1.5 + tAX */
    ATA_TIME_PIO_RDX = 1;
    ATA_TIME_4 = (timings->time_4 + T) / T;
    ATA_TIME_9 = (timings->time_9 + T) / T;
}

void ata_reset(void)
{
    /* Be sure we're not busy */
    ata_wait_for_idle();

    ATA_INTF_CONTROL &= ~(ATA_ATA_RST | ATA_FIFO_RST);
    sleep(HZ/100);
    ATA_INTF_CONTROL = ATA_ATA_RST | ATA_FIFO_RST;
    sleep(HZ/100);

    ata_wait_for_idle();
}

void ata_enable(bool on)
{
    /* Unconditionally clock module before writing regs */
    ccm_module_clock_gating(CG_ATA, CGM_ON_RUN_WAIT);
    ata_wait_for_idle();

    if (on)
    {
        ATA_INTF_CONTROL = ATA_ATA_RST | ATA_FIFO_RST;
        sleep(HZ/100);
    }
    else
    {
        ATA_INTF_CONTROL &= ~(ATA_ATA_RST | ATA_FIFO_RST);
        sleep(HZ/100);

        /* Disable off - unclock ATA module */
        ccm_module_clock_gating(CG_ATA, CGM_OFF);
    }
}

bool ata_is_coldstart(void)
{
    return true;
}

#ifdef HAVE_ATA_DMA
static void ata_set_mdma_timings(unsigned int mode)
{
    const struct ata_mdma_timings * const timings = &mdma_timings[mode];
    unsigned int T = get_T();

    ATA_TIME_M = (timings->time_m + T) / T;
    ATA_TIME_JN = (timings->time_jn + T) / T;
    ATA_TIME_D = (timings->time_d + T) / T;
    ATA_TIME_K = (timings->time_k + T) / T;
}

static void ata_set_udma_timings(unsigned int mode)
{
    const struct ata_udma_timings * const timings = &udma_timings[mode];
    unsigned int T = get_T();

    ATA_TIME_ACK = (timings->time_ack + T) / T;
    ATA_TIME_ENV = (timings->time_env + T) / T;
    ATA_TIME_RPX = (timings->time_rpx + T) / T;
    ATA_TIME_ZAH = (timings->time_zah + T) / T;
    ATA_TIME_MLIX = (timings->time_mlix + T) / T;
    ATA_TIME_DVH = (timings->time_dvh + T) / T + 1; 
    ATA_TIME_DZFS = (timings->time_dzfs + T) / T; 
    ATA_TIME_DVS = (timings->time_dvs + T) / T; 
    ATA_TIME_CVH = (timings->time_cvh + T) / T; 
    ATA_TIME_SS = (timings->time_ss + T) / T;      
    ATA_TIME_CYC = (timings->time_cyc + T) / T;     
}

void ata_dma_set_mode(unsigned char mode)
{
    unsigned int modeidx = mode & 0x07;
    unsigned int dmamode = mode & 0xf8;

    ata_wait_for_idle();

    if (ata_dma_selected == ATA_DMA_DISABLED)
    {
        /* Configuration error - no DMA */
    }
    else if (dmamode == 0x40 && modeidx <= ATA_MAX_UDMA)
    {
        /* Using Ultra DMA */
        ata_set_udma_timings(dmamode);
        ata_dma_selected = ATA_DMA_UDMA;
    }
    else if (dmamode == 0x20 && modeidx <= ATA_MAX_MWDMA)
    {
        /* Using Multiword DMA */
        ata_set_mdma_timings(dmamode);
        ata_dma_selected = ATA_DMA_MWDMA;
    }
    else
    {
        /* Don't understand this - force PIO. */
        ata_dma_selected = ATA_DMA_PIO;
    }
}

/* Called by SDMA when transfer is complete */
static void ata_dma_callback(void)
{
    /* Clear FIFO if not empty - shouldn't happen */
    while (ATA_FIFO_FILL != 0)
        ATA_FIFO_DATA_32;

    /* Clear FIFO interrupts (the only ones that can be) */
    ATA_INTERRUPT_CLEAR = ATA_INTERRUPT_PENDING;

    ata_set_intrq(false);           /* Return INTRQ to MCU */
    wakeup_signal(&ata_dma_wakeup); /* Signal waiting thread */
}

bool ata_dma_setup(void *addr, unsigned long bytes, bool write)
{
    struct buffer_descriptor *bd_p;
    unsigned char *buf;

    if (UNLIKELY(bytes > ATA_BASE_BD_COUNT*ATA_MAX_BD_SIZE ||
                 (ata_dma_selected & ATA_DMA_PIO)))
    {
        /* Too much? Implies BD count should be reevaluated since this
         * shouldn't be reached based upon size. Otherwise we simply didn't
         * understand the DMA mode setup. Force PIO in both cases. */
        ATA_INTF_CONTROL = ATA_FIFO_RST | ATA_ATA_RST;
        return false;
    }

    bd_p = &ata_bda[0];
    buf = (unsigned char *)addr_virt_to_phys((unsigned long)addr);
    sb_dst[0] = NULL; /* Assume not needed */

    if (write)
    {
        /* No cache alignment concerns */
        current_channel = ATA_DMA_CH_NUM_WR;

        if (LIKELY(buf != addr))
        {
            /* addr is virtual */
            clean_dcache_range(addr, bytes);
        }

        /* Setup ATA controller for DMA transmit */
        ATA_INTF_CONTROL = ATA_FIFO_RST | ATA_ATA_RST | ATA_FIFO_TX_EN |
            ATA_DMA_PENDING | ata_dma_selected | ATA_DMA_WRITE;
        ATA_FIFO_ALARM = SDMA_ATA_WML / 2;
    }
    else
    {
        current_channel = ATA_DMA_CH_NUM_RD;

        /* Setup ATA controller for DMA receive */
        ATA_INTF_CONTROL = ATA_FIFO_RST | ATA_ATA_RST | ATA_FIFO_RCV_EN |
            ATA_DMA_PENDING | ata_dma_selected;
        ATA_FIFO_ALARM = SDMA_ATA_WML / 2;

        if (LIKELY(buf != addr))
        {
            /* addr is virtual */
            dump_dcache_range(addr, bytes);

            if ((unsigned long)addr & 31)
            {
                /* Not cache aligned, must use scatter buffers for first and
                 * last 32 bytes. */
                unsigned char *bufstart = buf;

                sb_dst[0] = addr;
                bd_p->buf_addr = scatter_buffer;
                bd_p->mode.count = 32;
                bd_p->mode.status = BD_DONE | BD_CONT;

                buf += 32;
                bytes -= 32;
                bd_p++;

                while (bytes > ATA_MAX_BD_SIZE)
                {
                    bd_p->buf_addr = buf;
                    bd_p->mode.count = ATA_MAX_BD_SIZE;
                    bd_p->mode.status = BD_DONE | BD_CONT;
                    buf += ATA_MAX_BD_SIZE;
                    bytes -= ATA_MAX_BD_SIZE;
                    bd_p++;
                }

                if (bytes > 32)
                {
                    unsigned long size = bytes - 32;
                    bd_p->buf_addr = buf;
                    bd_p->mode.count = size;
                    bd_p->mode.status = BD_DONE | BD_CONT;
                    buf += size;
                    bd_p++;
                }

                /* There will be exactly 32 bytes left */

                /* Final buffer - wrap to base bd, interrupt */
                sb_dst[1] = addr + (buf - bufstart);
                bd_p->buf_addr = &scatter_buffer[32/4];
                bd_p->mode.count = 32;
                bd_p->mode.status = BD_DONE | BD_WRAP | BD_INTR;

                return true;
            }
        }
    }

    /* Setup buffer descriptors for both cache-aligned reads and all write
     * operations. */
    while (bytes > ATA_MAX_BD_SIZE)
    {
        bd_p->buf_addr = buf;
        bd_p->mode.count = ATA_MAX_BD_SIZE;
        bd_p->mode.status = BD_DONE | BD_CONT;
        buf += ATA_MAX_BD_SIZE;
        bytes -= ATA_MAX_BD_SIZE;
        bd_p++;
    }

    /* Final buffer - wrap to base bd, interrupt */
    bd_p->buf_addr = buf;
    bd_p->mode.count = bytes;
    bd_p->mode.status = BD_DONE | BD_WRAP | BD_INTR;

    return true;
}

bool ata_dma_finish(void)
{
    unsigned int channel = current_channel;
    long timeout = current_tick + HZ*10;

    current_channel = 0;

    ata_set_intrq(true);       /* Give INTRQ to DMA */
    sdma_channel_run(channel); /* Kick the channel to wait for events */

    while (1)
    {
        int oldirq;

        if (LIKELY(wakeup_wait(&ata_dma_wakeup, HZ/2) == OBJ_WAIT_SUCCEEDED))
            break;

        ata_keep_active();

        if (TIME_BEFORE(current_tick, timeout))
            continue;

        /* Epic fail - timed out - maybe. */
        oldirq = disable_irq_save();
        ata_set_intrq(false);       /* Strip INTRQ from DMA */
        sdma_channel_stop(channel); /* Stop DMA */
        restore_irq(oldirq);

        if (wakeup_wait(&ata_dma_wakeup, TIMEOUT_NOBLOCK) == OBJ_WAIT_SUCCEEDED)
            break; /* DMA really did finish after timeout */

        sdma_channel_reset(channel); /* Reset everything + clear error */
        return false;
    }

    if (sdma_channel_is_error(channel))
    {
        /* Channel error in one or more descriptors */
        sdma_channel_reset(channel); /* Reset everything + clear error */
        return false;
    }

    if (sb_dst[0] != NULL)
    {
        /* NOTE: This requires that unaligned access support be enabled! */
        register void *sbs = scatter_buffer;
        register void *sbd0 = sb_dst[0];
        register void *sbd1 = sb_dst[1];
        asm volatile(
            "add    r0, %1, #32         \n" /* Prefetch at DMA-direct boundaries */
            "mcrr   p15, 2, r0, r0, c12 \n"
            "mcrr   p15, 2, %2, %2, c12 \n"
            "ldmia  %0!, { r0-r3 }      \n" /* Copy the 32-bytes to destination */
            "str    r0, [%1], #4        \n" /* stmia doesn't work unaligned */
            "str    r1, [%1], #4        \n"
            "str    r2, [%1], #4        \n"
            "str    r3, [%1], #4        \n"
            "ldmia  %0!, { r0-r3 }      \n"
            "str    r0, [%1], #4        \n"
            "str    r1, [%1], #4        \n"
            "str    r2, [%1], #4        \n"
            "str    r3, [%1]            \n"
            "ldmia  %0!, { r0-r3 }      \n" /* Copy the 32-bytes to destination */
            "str    r0, [%2], #4        \n" /* stmia doesn't work unaligned */
            "str    r1, [%2], #4        \n"
            "str    r2, [%2], #4        \n"
            "str    r3, [%2], #4        \n"
            "ldmia  %0!, { r0-r3 }      \n"
            "str    r0, [%2], #4        \n"
            "str    r1, [%2], #4        \n"
            "str    r2, [%2], #4        \n"
            "str    r3, [%2]            \n"
            : "+r"(sbs), "+r"(sbd0), "+r"(sbd1)
            :
            : "r0", "r1", "r2", "r3");
    }

    return true;
}
#endif /* HAVE_ATA_DMA */

void ata_device_init(void)
{
    /* Make sure we're not in reset mode */
    ata_enable(true);

    if (!initialized)
    {
        ATA_INTERRUPT_ENABLE = 0;
        ATA_INTERRUPT_CLEAR = ATA_INTERRUPT_PENDING;
    }

    ata_set_intrq(false);

    if (initialized)
        return;

    /* All modes use same tOFF/tON */
    ATA_TIME_OFF = 3;
    ATA_TIME_ON = 3;

    /* Setup mode 0 for all by default
     * Mode may be switched later once identify info is ready in which
     * case the main driver calls back */
    ata_set_pio_timings(0);

#ifdef HAVE_ATA_DMA
    ata_set_mdma_timings(0);
    ata_set_udma_timings(0);

    ata_dma_selected = ATA_DMA_PIO;

    /* Called for first time at startup */
    wakeup_init(&ata_dma_wakeup);

    /* Read/write channels share buffer descriptors */
    ata_cd_rd.bd_count = ATA_BD_COUNT;
    ata_cd_rd.callback = ata_dma_callback;
    ata_cd_rd.shp_addr = SDMA_PER_ADDR_ATA_RX;
    ata_cd_rd.wml      = SDMA_ATA_WML;
    ata_cd_rd.per_type = SDMA_PER_ATA;
    ata_cd_rd.tran_type = SDMA_TRAN_PER_2_EMI;
    ata_cd_rd.event_id1 = SDMA_REQ_ATA_TXFER_END;
    ata_cd_rd.event_id2 = SDMA_REQ_ATA_RX;

    ata_cd_wr.bd_count = ATA_BD_COUNT;
    ata_cd_wr.callback = ata_dma_callback;
    ata_cd_wr.shp_addr = SDMA_PER_ADDR_ATA_TX;
    ata_cd_wr.wml      = SDMA_ATA_WML;
    ata_cd_wr.per_type = SDMA_PER_ATA;
    ata_cd_wr.tran_type = SDMA_TRAN_EMI_2_PER;
    ata_cd_wr.event_id1 = SDMA_REQ_ATA_TXFER_END;
    ata_cd_wr.event_id2 = SDMA_REQ_ATA_TX;

    if (!sdma_channel_init(ATA_DMA_CH_NUM_RD, &ata_cd_rd, ata_bda) ||
        !sdma_channel_init(ATA_DMA_CH_NUM_WR, &ata_cd_wr, ata_bda))
    {
        /* Channel init error - disable DMA forever */
        ata_dma_selected = ATA_DMA_DISABLED;
    }
#endif /* HAVE_ATA_DMA */

    initialized = true;
}
