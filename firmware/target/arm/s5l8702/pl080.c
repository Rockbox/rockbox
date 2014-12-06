/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Cástor Muñoz
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
#include <stddef.h>
#include "system.h"
#include "pl080.h"
#include "panic.h"

/*
 * ARM PrimeCell PL080 Multiple Master DMA controller
 */

/*#define PANIC_DEBUG*/
#ifdef PANIC_DEBUG
void dmac_ch_panicf(const char *fn, struct dmac_ch* ch)
{
    char *err = NULL;

    if (!ch)
        err = "NULL channel";
    else if (!ch->dmac)
        err = "NULL ch->dmac";
    else if (ch->dmac->ch_l[ch->prio] != ch)
        err = "not initialized channel";

    if (err)
        panicf("%s(): <%d> %s", fn, ch ? (int)ch->prio : -1, err);
}
#define PANIC_DEBUG_CHANNEL(ch)  dmac_ch_panicf(__func__,(ch))
#else
#define PANIC_DEBUG_CHANNEL(ch)  {}
#endif

/* task helpers */
static inline struct dmac_tsk *dmac_ch_tsk_by_idx(
                        struct dmac_ch *ch, uint32_t idx)
{
    return ch->tskbuf + (idx & ch->tskbuf_mask);
}
#define CH_TSK_TOP(ch)  dmac_ch_tsk_by_idx((ch), (ch)->tasks_queued)
#define CH_TSK_TAIL(ch) dmac_ch_tsk_by_idx((ch), (ch)->tasks_done)

static inline bool dmac_ch_task_queue_empty(struct dmac_ch *ch)
{
    return (ch->tasks_done == ch->tasks_queued);
}

/* enable/disable DMA controller */
static inline void dmac_hw_enable(struct dmac *dmac)
{
    DMACCONFIG(dmac->baddr) |= DMACCONFIG_E_BIT;
}

static inline void dmac_hw_disable(struct dmac *dmac)
{
    DMACCONFIG(dmac->baddr) &= ~DMACCONFIG_E_BIT;
}

/* enable/disable DMA channel */
static inline void dmac_ch_enable(struct dmac_ch *ch)
{
    DMACCxCONFIG(ch->baddr) |= DMACCxCONFIG_E_BIT;
}

static inline void dmac_ch_disable(struct dmac_ch *ch)
{
    uint32_t baddr = ch->baddr;

    /* Disable the channel, clears the FIFO after
       completing current AHB transfer */
    DMACCxCONFIG(baddr) &= ~DMACCxCONFIG_E_BIT;
    /* Wait for it to go inactive */
    while (DMACCxCONFIG(baddr) & DMACCxCONFIG_A_BIT);
}

#if 0
static void dmac_ch_halt(struct dmac_ch *ch)
{
    uint32_t baddr = ch->baddr;

    /* Halt the channel, ignores subsequent DMA requests,
       the contents of the FIFO are drained */
    DMACCxCONFIG(baddr) |= DMACCxCONFIG_H_BIT;
    /* Wait for it to go inactive */
    while (DMACCxCONFIG(baddr) & DMACCxCONFIG_A_BIT);
    /* Disable channel and restore Halt bit */
    DMACCxCONFIG(baddr) &= ~(DMACCxCONFIG_H_BIT | DMACCxCONFIG_E_BIT);
}
#endif

/* launch next task in queue */
static void ICODE_ATTR dmac_ch_run(struct dmac_ch *ch)
{
    struct dmac *dmac = ch->dmac;

    if (!dmac->ch_run_status)
        dmac_hw_enable(dmac);
    dmac->ch_run_status |= (1 << ch->prio);

    /* Clear any pending interrupts leftover from previous operation */
    /*DMACINTTCCLR(dmac->baddr) = (1 << ch->prio);*/  /* not needed */

    /* copy whole LLI to HW registers */
    *DMACCxLLI(ch->baddr) = *(CH_TSK_TAIL(ch)->start_lli);

    dmac_ch_enable(ch);
}

static void ICODE_ATTR dmac_ch_abort(struct dmac_ch* ch)
{
    struct dmac *dmac = ch->dmac;

    dmac_ch_disable(ch);

    /* Clear any pending interrupt */
    DMACINTTCCLR(dmac->baddr) = (1 << ch->prio);

    dmac->ch_run_status &= ~(1 << ch->prio);
    if (!dmac->ch_run_status)
        dmac_hw_disable(dmac);
}

/* ISR */
static inline void dmac_ch_callback(struct dmac_ch *ch)
{
    PANIC_DEBUG_CHANNEL(ch);

    /* backup current task cb_data */
    void *cb_data = CH_TSK_TAIL(ch)->cb_data;

    /* mark current task as finished (resources can be reused) */
    ch->tasks_done++;

    /* launch next DMA task */
    if (ch->queue_mode == QUEUE_NORMAL)
        if (!dmac_ch_task_queue_empty(ch))
            dmac_ch_run(ch);

    /* run user callback, new tasks could be launched/queued here */
    if (ch->cb_fn)
        ch->cb_fn(cb_data);

    /* disable DMA channel if there are no running tasks */
    if (dmac_ch_task_queue_empty(ch))
        dmac_ch_abort(ch);
}

void ICODE_ATTR dmac_callback(struct dmac *dmac)
{
    #ifdef PANIC_DEBUG
    if (!dmac)
        panicf("dmac_callback(): NULL dmac");
    #endif

    unsigned int ch_n;
    uint32_t baddr = dmac->baddr;
    uint32_t intsts = DMACINTSTS(baddr);

    /* Lowest channel index is serviced first */
    for (ch_n = 0; ch_n < DMAC_CH_COUNT; ch_n++) {
        if ((intsts & (1 << ch_n))) {
            if (DMACINTERRSTS(baddr) & (1 << ch_n))
                panicf("DMA ch%d: HW error", ch_n);

            /* clear terminal count interrupt */
            DMACINTTCCLR(baddr) = (1 << ch_n);

            dmac_ch_callback(dmac->ch_l[ch_n]);
        }
    }
}

/*
 * API
 */
void dmac_open(struct dmac *dmac)
{
    uint32_t baddr = dmac->baddr;
    int ch_n;

    dmac_hw_enable(dmac);

    DMACCONFIG(baddr) = ((dmac->m1 & DMACCONFIG_M1_MSK) << DMACCONFIG_M1_POS)
                      | ((dmac->m2 & DMACCONFIG_M2_MSK) << DMACCONFIG_M2_POS);

    for (ch_n = 0; ch_n < DMAC_CH_COUNT; ch_n++) {
        DMACCxCONFIG(DMAC_CH_BASE(baddr, ch_n)) = 0;  /* disable channel */
        dmac->ch_l[ch_n] = NULL;
    }
    dmac->ch_run_status = 0;

    /* clear channel interrupts */
    DMACINTTCCLR(baddr) = 0xff;
    DMACINTERRCLR(baddr) = 0xff;

    dmac_hw_disable(dmac);
}

void dmac_ch_init(struct dmac_ch *ch, struct dmac_ch_cfg *cfg)
{
    #ifdef PANIC_DEBUG
    if (!ch)
        panicf("%s(): NULL channel", __func__);
    else if (!ch->dmac)
        panicf("%s(): NULL ch->dmac", __func__);
    else if (ch->dmac->ch_l[ch->prio])
        panicf("%s(): channel %d already initilized", __func__, ch->prio);
    uint32_t align_mask = (1 << MIN(cfg->swidth, cfg->dwidth)) - 1;
    if (ch->cfg->lli_xfer_max_count & align_mask)
        panicf("%s(): bad bus width: sw=%u dw=%u max_cnt=%u", __func__,
                    cfg->swidth, cfg->dwidth, ch->cfg->lli_xfer_max_count);
    #endif

    struct dmac *dmac = ch->dmac;
    int ch_n = ch->prio;

    dmac->ch_l[ch_n] = ch;

    ch->baddr = DMAC_CH_BASE(dmac->baddr, ch_n);
    ch->llibuf_top = ch->llibuf;
    ch->tasks_queued = 0;
    ch->tasks_done = 0;
    ch->cfg = cfg;

    ch->control =
        ((cfg->sbsize & DMACCxCONTROL_SBSIZE_MSK) << DMACCxCONTROL_SBSIZE_POS) |
        ((cfg->dbsize & DMACCxCONTROL_DBSIZE_MSK) << DMACCxCONTROL_DBSIZE_POS) |
        ((cfg->swidth & DMACCxCONTROL_SWIDTH_MSK) << DMACCxCONTROL_SWIDTH_POS) |
        ((cfg->dwidth & DMACCxCONTROL_DWIDTH_MSK) << DMACCxCONTROL_DWIDTH_POS) |
        ((cfg->sbus & DMACCxCONTROL_S_MSK) << DMACCxCONTROL_S_POS) |
        ((cfg->dbus & DMACCxCONTROL_D_MSK) << DMACCxCONTROL_D_POS) |
        ((cfg->sinc & DMACCxCONTROL_SI_MSK) << DMACCxCONTROL_SI_POS) |
        ((cfg->dinc & DMACCxCONTROL_DI_MSK) << DMACCxCONTROL_DI_POS) |
        ((cfg->prot & DMACCxCONTROL_PROT_MSK) << DMACCxCONTROL_PROT_POS);

    /* flow control notes:
     *  - currently only master modes are supported (FLOWCNTRL_x_DMA).
     *  - must use DMAC_PERI_NONE when srcperi and/or dstperi are memory.
     */
    uint32_t flowcntrl = (((cfg->srcperi != DMAC_PERI_NONE) << 1) |
                (cfg->dstperi != DMAC_PERI_NONE)) << DMACCxCONFIG_FLOWCNTRL_POS;

    DMACCxCONFIG(ch->baddr) =
        ((cfg->srcperi & DMACCxCONFIG_SRCPERI_MSK) << DMACCxCONFIG_SRCPERI_POS) |
        ((cfg->dstperi & DMACCxCONFIG_DESTPERI_MSK) << DMACCxCONFIG_DESTPERI_POS) |
        flowcntrl | DMACCxCONFIG_IE_BIT | DMACCxCONFIG_ITC_BIT;
}

void dmac_ch_lock_int(struct dmac_ch *ch)
{
    PANIC_DEBUG_CHANNEL(ch);

    int flags = disable_irq_save();
    DMACCxCONFIG(ch->baddr) &= ~DMACCxCONFIG_ITC_BIT;
    restore_irq(flags);
}

void dmac_ch_unlock_int(struct dmac_ch *ch)
{
    PANIC_DEBUG_CHANNEL(ch);

    int flags = disable_irq_save();
    DMACCxCONFIG(ch->baddr) |= DMACCxCONFIG_ITC_BIT;
    restore_irq(flags);
}

/* 1D->2D DMA transfers:
 *
 *   srcaddr:  aaaaaaaaaaabbbbbbbbbbbccccccc
 *             <-           size          ->
 *             <- width -><- width -><- r ->
 *
 *   dstaddr:               aaaaaaaaaaa.....
 *   dstaddr + stride:      bbbbbbbbbbb.....
 *   dstaddr + 2*stride:    ccccccc.........
 *                          <-   stride   ->
 *                          <- width ->
 *
 * 1D->1D DMA transfers:
 *
 *   If 'width'=='stride', uses 'lli_xfer_max_count' for LLI count.
 *
 * Queue modes:
 *
 *   QUEUE_NORMAL: each task in the queue is launched using a new
 *   DMA transfer once previous task is finished.
 *
 *   QUEUE_LINK: when a task is queued, it is linked with the last
 *   queued task, creating a single continuous DMA transfer. New
 *   tasks must be queued while the channel is running, otherwise
 *   the continuous DMA transfer will be broken.
 *
 * Misc notes:
 *
 *   Arguments 'size', 'width' and 'stride' are in bytes.
 *
 *   Maximum supported 'width' depends on bus 'swidth' size, it is:
 *   maximum width = DMAC_LLI_MAX_COUNT << swidth
 *
 *   User must supply 'srcaddr', 'dstaddr', 'width', 'size', 'stride'
 *   and 'lli_xfer_max_count' aligned to configured source and
 *   destination bus widths, otherwise transfers will be internally
 *   aligned by DMA hardware.
 */
#define LLI_COUNT(lli)  ((lli)->control & DMACCxCONTROL_COUNT_MSK)
#define LNK2LLI(link)   ((struct dmac_lli*) ((link) & ~3))

static inline void drain_write_buffer(void)
{
    asm volatile (
        "mcr p15, 0, %0, c7, c10, 4\n"
    : : "r"(0));
}

static inline void clean_dcache_line(void volatile *addr)
{
    asm volatile (
        "mcr p15, 0, %0, c7, c10, 1\n"  /* clean d-cache line by MVA */
    : : "r"((uint32_t)addr & ~(CACHEALIGN_SIZE - 1)));
}

void ICODE_ATTR dmac_ch_queue_2d(
                struct dmac_ch *ch, void *srcaddr, void *dstaddr,
                size_t size, size_t width, size_t stride, void *cb_data)
{
    #ifdef PANIC_DEBUG
    PANIC_DEBUG_CHANNEL(ch);
    uint32_t align = (1 << MIN(ch->cfg->swidth, ch->cfg->dwidth)) - 1;
    if (((uint32_t)srcaddr | (uint32_t)dstaddr | size | width | stride) & align)
        panicf("dmac_ch_queue_2d(): %d,%p,%p,%u,%u,%u: bad alignment?",
                            ch->prio, srcaddr, dstaddr, size, width, stride);
    #endif

    struct dmac_tsk *tsk;
    unsigned int srcinc, dstinc;
    uint32_t control, llibuf_idx;
    struct dmac_lli volatile *lli, *next_lli;

    /* get and fill new task */
    tsk = CH_TSK_TOP(ch);
    tsk->start_lli = ch->llibuf_top;
    tsk->size = size;
    tsk->cb_data = cb_data;

    /* use maximum LLI transfer count for 1D->1D transfers */
    if (width == stride)
        width = stride = ch->cfg->lli_xfer_max_count << ch->cfg->swidth;

    srcinc = (ch->cfg->sinc) ? stride : 0;
    dstinc = (ch->cfg->dinc) ? width : 0;

    size >>= ch->cfg->swidth;
    width >>= ch->cfg->swidth;

    /* fill LLI circular buffer */
    control = ch->control | width;
    lli = ch->llibuf_top;
    llibuf_idx = lli - ch->llibuf;

    while (1)
    {
        llibuf_idx = (llibuf_idx + 1) & ch->llibuf_mask;
        next_lli = ch->llibuf + llibuf_idx;

        lli->srcaddr = srcaddr;
        lli->dstaddr = dstaddr;

        if (size <= width)
            break;

        lli->link = (uint32_t)next_lli | ch->llibuf_bus;
        lli->control = control;

        srcaddr += srcinc;
        dstaddr += dstinc;
        size -= width;

        /* clean dcache after completing a line */
        if (((uint32_t)next_lli & (CACHEALIGN_SIZE - 1)) == 0)
            clean_dcache_line(lli);

        lli = next_lli;
    }
    /* last lli, enable terminal count interrupt */
    lli->link = 0;
    lli->control = ch->control | size | DMACCxCONTROL_I_BIT;
    clean_dcache_line(lli);
    drain_write_buffer();

    tsk->end_lli = lli;

    /* previous code is not protected against IRQs, it is fine to
       enter the DMA interrupt handler while an application is
       queuing a task, but the aplication must be protected when
       doing concurrent queueing. */

    int flags = disable_irq_save();

    ch->llibuf_top = next_lli;

    /* queue new task, launch it if it is the only queued task */
    if (ch->tasks_done == ch->tasks_queued++)
    {
        dmac_ch_run(ch);
    }
    else if (ch->queue_mode == QUEUE_LINK)
    {
        uint32_t baddr = ch->baddr;
        uint32_t link, hw_link;

        link = (uint32_t)tsk->start_lli | ch->llibuf_bus;
        hw_link = DMACCxLINK(baddr);

        /* if it is a direct HW link, do it ASAP */
        if (!hw_link) {
            DMACCxLINK(baddr) = link;
            /* check if the link was successful */
            link = DMACCxLINK(baddr);  /* dummy read for delay */
            if (!(DMACCxCONFIG(baddr) & DMACCxCONFIG_E_BIT))
                panicf("DMA ch%d: link error", ch->prio);
        }

        /* Locate the LLI where the new task must be linked. Link it even
           if it was a direct HW link, dmac_ch_get_info() needs it. */
        lli = dmac_ch_tsk_by_idx(ch, ch->tasks_queued-2)->end_lli;
        lli->link = link;
        clean_dcache_line(lli);
        drain_write_buffer();

        /* If the updated LLI was loaded by the HW while it was being
           modified, verify that the HW link is correct. */
        if (LNK2LLI(hw_link) == lli) {
            uint32_t cur_hw_link = DMACCxLINK(baddr);
            if ((cur_hw_link != hw_link) && (cur_hw_link != link))
                DMACCxLINK(baddr) = link;
        }
    }

    restore_irq(flags);
}

void dmac_ch_stop(struct dmac_ch* ch)
{
    PANIC_DEBUG_CHANNEL(ch);

    int flags = disable_irq_save();
    dmac_ch_abort(ch);
    ch->tasks_done = ch->tasks_queued;  /* clear queue */
    restore_irq(flags);
}

bool dmac_ch_running(struct dmac_ch *ch)
{
    PANIC_DEBUG_CHANNEL(ch);

    int flags = disable_irq_save();
    bool running = !dmac_ch_task_queue_empty(ch);
    restore_irq(flags);
    return running;
}

/* returns source or destination address of the actual LLI transfer,
   remaining bytes for current task, and total remaining bytes */
void *dmac_ch_get_info(struct dmac_ch *ch, size_t *bytes, size_t *t_bytes)
{
    PANIC_DEBUG_CHANNEL(ch);

    void *cur_addr = NULL;
    size_t count = 0, t_count = 0;

    int flags = disable_irq_save();

    if (!dmac_ch_task_queue_empty(ch))
    {
        struct dmac_lli volatile *cur_lli;
        struct dmac_tsk *tsk;
        uint32_t cur_task;  /* index */
        uint32_t baddr = ch->baddr;

        /* Read DMA transfer progress:
         *
         * The recommended procedure (stop channel -> read progress ->
         * relaunch channel) is problematic for real time transfers,
         * specially when fast sample rates are combined with small
         * pheripheral FIFOs.
         *
         * An experimental method is used, it is based on the results
         * observed when reading the LLI registers at the instant they
         * are being updated by the HW (using s5l8702, 2:1 CPU/AHB
         * clock ratio):
         * - SRCADDR may return erroneous/corrupted data
         * - LINK and COUNT always returns valid data
         * - it seems that HW internally updates LINK and COUNT
         *   'atomically', this means that reading twice using the
         *   sequence LINK1->COUNT1->LINK2->COUNT2:
         *     if LINK1 == LINK2 then COUNT1 is consistent with LINK
         *     if LINK1 <> LINK2 then COUNT2 is consistent with LINK2
         */
        uint32_t link, link2, control, control2;

        /* HW read sequence */
        link = DMACCxLINK(baddr);
        control = DMACCxCONTROL(baddr);
        link2 = DMACCxLINK(baddr);
        control2 = DMACCxCONTROL(baddr);

        if (link != link2) {
            link = link2;
            control = control2;
        }

        count = control & DMACCxCONTROL_COUNT_MSK;  /* HW count */

        cur_task = ch->tasks_done;

        /* In QUEUE_LINK mode, when the task has just finished and is
         * waiting to enter the interrupt handler, the readed HW data
         * may correspont to the next linked task. Check it and update
         * real cur_task accordly.
         */
        struct dmac_lli *next_start_lli = LNK2LLI(
                        dmac_ch_tsk_by_idx(ch, cur_task)->end_lli->link);
        if (next_start_lli && (next_start_lli->link == link))
            cur_task++;

        tsk = dmac_ch_tsk_by_idx(ch, cur_task);

        /* get previous to next LLI in the circular buffer */
        cur_lli = (link) ? ch->llibuf + (ch->llibuf_mask &
                        (LNK2LLI(link) - ch->llibuf - 1)) : tsk->end_lli;

        /* Calculate current address, choose destination address when
         * dest increment is set (usually MEMMEM or PERIMEM transfers),
         * otherwise use source address (usually MEMPERI transfers).
         */
        void *start_addr;
        if (ch->control & (1 << DMACCxCONTROL_DI_POS)) {
            cur_addr = cur_lli->dstaddr;
            start_addr = tsk->start_lli->dstaddr;
        }
        else {
            cur_addr = cur_lli->srcaddr;
            start_addr = tsk->start_lli->srcaddr;
        }
        cur_addr += (LLI_COUNT(cur_lli) - count) << ch->cfg->swidth;

        /* calculate bytes for current task */
        count = tsk->size - (cur_addr - start_addr);

        /* count bytes for the remaining tasks */
        if (t_bytes)
            while (++cur_task != ch->tasks_queued)
                t_count += dmac_ch_tsk_by_idx(ch, cur_task)->size;
    }

    restore_irq(flags);

    if (bytes) *bytes = count;
    if (t_bytes) *t_bytes = count + t_count;

    return cur_addr;
}
