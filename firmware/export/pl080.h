/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Cástor Muñoz
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
#ifndef _PL080_H
#define _PL080_H

/*
 * ARM PrimeCell PL080 Multiple Master DMA controller
 */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* general defines */
#define DMAC_CH_COUNT               8
#define DMAC_LLI_MAX_COUNT          0xfff
#define DMAC_CH_PRIO(x)             (x)
#define DMAC_CH_BASE(dmac_ba,ch_n)  ((dmac_ba) + 0x100 + ((ch_n) << 5))

/* PL080 controller registers */
#define DMACINTSTS(base)            (*((uint32_t volatile*)((base) + 0x00)))
#define DMACINTTCSTS(base)          (*((uint32_t volatile*)((base) + 0x04)))
#define DMACINTTCCLR(base)          (*((uint32_t volatile*)((base) + 0x08)))
#define DMACINTERRSTS(base)         (*((uint32_t volatile*)((base) + 0x0c)))
#define DMACINTERRCLR(base)         (*((uint32_t volatile*)((base) + 0x10)))
#define DMACRAWINTTCSTS(base)       (*((uint32_t volatile*)((base) + 0x14)))
#define DMACRAWINTERRSTS(base)      (*((uint32_t volatile*)((base) + 0x18)))
#define DMACENABLEDCHANS(base)      (*((uint32_t volatile*)((base) + 0x1c)))
#define DMACSOFTBREQ(base)          (*((uint32_t volatile*)((base) + 0x20)))
#define DMACSOFTSREQ(base)          (*((uint32_t volatile*)((base) + 0x24)))
#define DMACSOFTLBREQ(base)         (*((uint32_t volatile*)((base) + 0x28)))
#define DMACSOFTLSREQ(base)         (*((uint32_t volatile*)((base) + 0x2c)))
#define DMACCONFIG(base)            (*((uint32_t volatile*)((base) + 0x30)))
#define DMACSYNC(base)              (*((uint32_t volatile*)((base) + 0x34)))

/* PL080 controller channel registers */
#define DMACCxSRCADDR(base)         (*((void* volatile*)((base) + 0x00)))
#define DMACCxDESTADDR(base)        (*((void* volatile*)((base) + 0x04)))
#define DMACCxLINK(base)            (*((uint32_t volatile*)((base) + 0x08)))
#define DMACCxCONTROL(base)         (*((uint32_t volatile*)((base) + 0x0c)))
#define DMACCxCONFIG(base)          (*((uint32_t volatile*)((base) + 0x10)))

/* PL080 controller channel LLI */
#define DMACCxLLI(base)             ((struct dmac_lli volatile*)(base))

/* PL080 DMA controller configuration register */
#define DMACCONFIG_E_POS            0       /* DMAC enable */
#define DMACCONFIG_E_MSK            0x1
#define DMACCONFIG_M1_POS           1       /* AHB Master 1 endianness */
#define DMACCONFIG_M1_MSK           0x1
#define DMACCONFIG_M2_POS           2       /* AHB Master 2 endianness */
#define DMACCONFIG_M2_MSK           0x1

#define DMACCONFIG_E_BIT            (1 << DMACCONFIG_E_POS)
#define DMACCONFIG_M1_BIT           (1 << DMACCCONFI_M1_POS)
#define DMACCONFIG_M2_BIT           (1 << DMACCCONFI_M2_POS)

#define DMACCONFIG_M_LITTLE_ENDIAN  0
#define DMACCONFIG_M_BIG_ENDIAN     1

/* PL080 DMA controller channel LLI register */
#define DMACCxLINK_LM_POS           0
#define DMACCxLINK_LM_MSK           0x1
#define DMACCxLINK_NEXTLLI_POS      2
#define DMACCxLINK_NEXTLLI_MSK      0x3fffffff

/* PL080 channel control register */
#define DMACCxCONTROL_I_POS         31      /* terminal count interrupt */
#define DMACCxCONTROL_I_MSK         0x1
#define DMACCxCONTROL_PROT_POS      28      /* protection bits */
#define DMACCxCONTROL_PROT_MSK      0x7
#define DMACCxCONTROL_DI_POS        27      /* destination addr increment */
#define DMACCxCONTROL_DI_MSK        0x1
#define DMACCxCONTROL_SI_POS        26      /* source addr increment */
#define DMACCxCONTROL_SI_MSK        0x1
#define DMACCxCONTROL_D_POS         25      /* destinantion AHB master */
#define DMACCxCONTROL_D_MSK         0x1
#define DMACCxCONTROL_S_POS         24      /* source AHB master */
#define DMACCxCONTROL_S_MSK         0x1
#define DMACCxCONTROL_DWIDTH_POS    21      /* destinantion transfer width */
#define DMACCxCONTROL_DWIDTH_MSK    0x7
#define DMACCxCONTROL_SWIDTH_POS    18      /* source transfer width */
#define DMACCxCONTROL_SWIDTH_MSK    0x7
#define DMACCxCONTROL_DBSIZE_POS    15      /* destinantion burst size */
#define DMACCxCONTROL_DBSIZE_MSK    0x7
#define DMACCxCONTROL_SBSIZE_POS    12      /* source burst size */
#define DMACCxCONTROL_SBSIZE_MSK    0x7
#define DMACCxCONTROL_COUNT_POS     0       /* n SWIDTH size transfers */
#define DMACCxCONTROL_COUNT_MSK     0xfff

#define DMACCxCONTROL_WIDTH_8       0
#define DMACCxCONTROL_WIDTH_16      1
#define DMACCxCONTROL_WIDTH_32      2

#define DMACCxCONTROL_BSIZE_1       0
#define DMACCxCONTROL_BSIZE_4       1
#define DMACCxCONTROL_BSIZE_8       2
#define DMACCxCONTROL_BSIZE_16      3
#define DMACCxCONTROL_BSIZE_32      4
#define DMACCxCONTROL_BSIZE_64      5
#define DMACCxCONTROL_BSIZE_128     6
#define DMACCxCONTROL_BSIZE_256     7

#define DMACCxCONTROL_INC_DISABLE   0
#define DMACCxCONTROL_INC_ENABLE    1

#define DMACCxCONTROL_I_BIT         (1 << DMACCxCONTROL_I_POS)

/* protection bits */
#define DMAC_PROT_PRIV              (1 << 0)
#define DMAC_PROT_BUFF              (1 << 1)
#define DMAC_PROT_CACH              (1 << 2)

/* bus */
#define DMAC_MASTER_AHB1            0
#define DMAC_MASTER_AHB2            1

/* PL080 channel configuration register */
#define DMACCxCONFIG_E_POS          0       /* enable */
#define DMACCxCONFIG_E_MSK          0x1
#define DMACCxCONFIG_SRCPERI_POS    1       /* source peripheral */
#define DMACCxCONFIG_SRCPERI_MSK    0xf
#define DMACCxCONFIG_DESTPERI_POS   6       /* destination peripheral */
#define DMACCxCONFIG_DESTPERI_MSK   0xf
#define DMACCxCONFIG_FLOWCNTRL_POS  11      /* DMA transfer type */
#define DMACCxCONFIG_FLOWCNTRL_MSK  0x7
#define DMACCxCONFIG_IE_POS         14      /* interrupt error mask */
#define DMACCxCONFIG_IE_MSK         0x1
#define DMACCxCONFIG_ITC_POS        15      /* interrupt terminal count mask */
#define DMACCxCONFIG_ITC_MSK        0x1
#define DMACCxCONFIG_L_POS          16      /* lock */
#define DMACCxCONFIG_L_MSK          0x1
#define DMACCxCONFIG_A_POS          17      /* active */
#define DMACCxCONFIG_A_MSK          0x1
#define DMACCxCONFIG_H_POS          18      /* halt */
#define DMACCxCONFIG_H_MSK          0x1

#define DMACCxCONFIG_E_BIT          (1 << DMACCxCONFIG_E_POS)
#define DMACCxCONFIG_IE_BIT         (1 << DMACCxCONFIG_IE_POS)
#define DMACCxCONFIG_ITC_BIT        (1 << DMACCxCONFIG_ITC_POS)
#define DMACCxCONFIG_L_BIT          (1 << DMACCxCONFIG_L_POS)
#define DMACCxCONFIG_A_BIT          (1 << DMACCxCONFIG_A_POS)
#define DMACCxCONFIG_H_BIT          (1 << DMACCxCONFIG_H_POS)

#define DMACCxCONFIG_FLOWCNTRL_MEMMEM_DMA         0
#define DMACCxCONFIG_FLOWCNTRL_MEMPERI_DMA        1
#define DMACCxCONFIG_FLOWCNTRL_PERIMEM_DMA        2
#define DMACCxCONFIG_FLOWCNTRL_PERIPERI_DMA       3
#define DMACCxCONFIG_FLOWCNTRL_PERIPERI_DSTPERI   4
#define DMACCxCONFIG_FLOWCNTRL_MEMPERI_PERI       5
#define DMACCxCONFIG_FLOWCNTRL_PERIMEM_PERI       6
#define DMACCxCONFIG_FLOWCNTRL_PERIPERI_SRCPERI   7

/*
 * types
 */
struct dmac_lli {
    void* srcaddr;
    void* dstaddr;
    uint32_t link;
    uint32_t control;
} __attribute__((aligned(16)));

struct dmac_tsk {
    struct dmac_lli volatile *start_lli;
    struct dmac_lli volatile *end_lli;
    uint32_t size;
    void *cb_data;
};

/* used when src/dst peri is memory */
#define DMAC_PERI_NONE    0x80

struct dmac_ch_cfg {
    uint8_t srcperi;
    uint8_t dstperi;
    uint8_t sbsize;
    uint8_t dbsize;
    uint8_t swidth;
    uint8_t dwidth;
    uint8_t sbus;
    uint8_t dbus;
    uint8_t sinc;
    uint8_t dinc;
    uint8_t prot;
    uint16_t lli_xfer_max_count;
};

struct dmac_ch {
    /** user configurable data **/
    struct dmac *dmac;
    unsigned int prio;
    void (*cb_fn)(void *cb_data);
    /* tsk circular buffer */
    struct dmac_tsk *tskbuf;
    uint32_t tskbuf_mask;
    uint32_t queue_mode;
    /* lli circular buffer */
    struct dmac_lli volatile *llibuf;
    uint32_t llibuf_mask;
    uint32_t llibuf_bus;

    /** private driver data **/
    uint32_t baddr;
    struct dmac_lli volatile *llibuf_top;
    uint32_t tasks_queued;  /* roll-over counter */
    uint32_t tasks_done;    /* roll-over counter */
    uint32_t control;
    struct dmac_ch_cfg *cfg;
};

struct dmac {
    /* user configurable data */
    const uint32_t baddr;
    const uint8_t m1;
    const uint8_t m2;

    /* driver private data */
    struct dmac_ch *ch_l[DMAC_CH_COUNT];
    uint32_t ch_run_status; /* channel running status mask */
};

/* dmac_ch->queue_mode */
enum {
    QUEUE_NORMAL,
    QUEUE_LINK,
};

/*
 * prototypes
 */
void dmac_callback(struct dmac *dmac);

void dmac_open(struct dmac *dmac);

void dmac_ch_init(struct dmac_ch *ch, struct dmac_ch_cfg *cfg);

void dmac_ch_lock_int(struct dmac_ch *ch);
void dmac_ch_unlock_int(struct dmac_ch *ch);

void dmac_ch_queue_2d(struct dmac_ch *ch, void *srcaddr, void *dstaddr,
                size_t size, size_t width, size_t stride, void *cb_data);
#define dmac_ch_queue(ch, srcaddr, dstaddr, size, cb_data) \
                dmac_ch_queue_2d(ch, srcaddr, dstaddr, size, 0, 0, cb_data)

void dmac_ch_stop(struct dmac_ch* ch);

bool dmac_ch_running(struct dmac_ch *ch);

void *dmac_ch_get_info(struct dmac_ch *ch,
                size_t *bytes, size_t *t_bytes);

#endif /* _PL080_H */
