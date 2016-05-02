/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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

#include "dpc-imx233.h"
#include "kernel.h"
#include "timrot-imx233.h"

#include "regs/digctl.h"

/**
 * The code uses the DIGCTL microseconds counter as the absolute reference. It
 * also uses a timer with 24MHz source to schedule work.
 * In the following code, every absolute time means using DIGCTL microseconds timer,
 * taking into account possible wrapping. */

/** DPC context state */
enum dpc_ctx_state_t
{
    CTX_UNUSED, /* context is unused */
    CTX_RUNNING, /* context is up and running */
    CTX_STOPPED /* context is stopped but blocked caller need to acknowledge */
};

/** DPC context */
struct imx233_dpc_ctx_t
{
    struct semaphore sema; /* semaphore to block the caller */
    uint32_t setup_time; /* absolute time at which work was scheduled (for wrapping) */
    uint32_t fire_time; /* absolute time at which work should take place */
    intptr_t user; /* user data */
    imx233_dpc_fn_t fn; /* user function */
    bool blocked; /* is caller blocked ? */
    enum dpc_ctx_state_t state; /* channel state */
    imx233_dpc_id_t id; /* ID for cancellation */
};

#define NR_CONTEXT  8

static struct imx233_dpc_ctx_t dpc_ctx[NR_CONTEXT];
static imx233_dpc_id_t dpc_next_id = 0; /* ID of next DPC */

/* Return absolute */
static inline uint32_t now(void)
{
    return HW_DIGCTL_MICROSECONDS;
}

static void irq(void);

/* called with interrupts disabled */
static void reschedule(void)
{
    uint32_t cur_time = now();
    /* go through all tasks: call those that expired, and schedule timer for next
     * one if needed */
    bool has_work = false;
    uint32_t next_work_time = 0; /* absolute time of the next task: furthest time in the future (with wrapping) */
    for(int i = 0; i < NR_CONTEXT; i++)
    {
        /* ignore non-running contexts */
        if(dpc_ctx[i].state != CTX_RUNNING)
            continue;
        /* check if fire time has been reached, beware of wrapping!
         * we use the setup time to handle wrapping */
        bool fire = false;
        if(dpc_ctx[i].setup_time <= dpc_ctx[i].fire_time)
            fire = cur_time >= dpc_ctx[i].fire_time || cur_time < dpc_ctx[i].setup_time;
        else
            fire = cur_time >= dpc_ctx[i].fire_time && cur_time < dpc_ctx[i].setup_time;

        if(fire)
        {
            /* mark context as stopped, if the function relaunch DPC, it will be
             * back to running state after call but still to be scheduled */
            dpc_ctx[i].state = CTX_STOPPED;
            dpc_ctx[i].fn(&dpc_ctx[i], dpc_ctx[i].user);
            /* context still marked as stopped after call: perform cleanup */
            if(dpc_ctx[i].state == CTX_STOPPED)
            {
                /* if caller is blocked, release him: he will do the cleanup,
                 * otherwise cleanup */
                if(dpc_ctx[i].blocked)
                    semaphore_release(&dpc_ctx[i].sema);
                else
                    dpc_ctx[i].state = CTX_UNUSED;
                /* do not continue, this context will not be scheduled anymore */
                continue;
            }
        }
        /* schedule context */
        has_work = true;
        /* update next work time if needed, beware of wrapping */
        if(cur_time < next_work_time)
        {
            /* next work is not wrapped: only consider non-wrapping times */
            if(cur_time < dpc_ctx[i].fire_time)
                next_work_time = MIN(next_work_time, dpc_ctx[i].fire_time);
        }
        else
        {
            /* next work is wrapped: non-wrapped is always better, otherwise min */
            if(cur_time < dpc_ctx[i].fire_time)
                next_work_time = dpc_ctx[i].fire_time;
            else
                next_work_time = MIN(next_work_time, dpc_ctx[i].fire_time);
        }
    }
    /* program timer if needed */
    if(has_work)
    {
        /* compute delay */
        uint32_t delay_in_us = 0;
        if(cur_time < next_work_time)
            delay_in_us = next_work_time - cur_time;
        else
            delay_in_us = 0xffffffff - cur_time + next_work_time;
        /* if it possible that the delay is too longer for the 3MHz timer, in this
         * case just take the maximum possible delay */
        delay_in_us = MIN(delay_in_us, IMX233_TIMROT_MAX_COUNT / 3);
        imx233_timrot_setup_simple(TIMER_DPC, false, delay_in_us * 3, TIMER_SRC_3MHZ, &irq);
    }
}

static void irq(void)
{
    reschedule();
}

void imx233_dpc_init(void)
{
    for(int i = 0; i < NR_CONTEXT; i++)
    {
        semaphore_init(&dpc_ctx[i].sema, 1, 0);
        dpc_ctx[i].state = CTX_UNUSED;
    }
}

static void imx233_dpc_setup(struct imx233_dpc_ctx_t *ctx, imx233_dpc_id_t id,
    uint32_t setup_time, uint32_t delay_us, imx233_dpc_fn_t fn, intptr_t user,
    bool schedule)
{
    ctx->id = id;
    ctx->setup_time = setup_time;
    ctx->fire_time = setup_time + delay_us; /* NOTE: may wrap */
    ctx->fn = fn;
    ctx->user = user;
    ctx->state = CTX_RUNNING;
    /* possibly reschedule */
    if(schedule)
        reschedule();
}

/* caller with interrupts disabled */
static int get_context(void)
{
    for(int i = 0; i < NR_CONTEXT; i++)
        if(dpc_ctx[i].state == CTX_UNUSED)
            return i;
    return -1;
}

imx233_dpc_id_t imx233_dpc_start(bool block, uint32_t delay_us, imx233_dpc_fn_t fn, intptr_t user)
{
    uint32_t setuptime = now();
    /* disable IRQ to prevent timer interrupt */
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);
    /* get a context from the pool */
    int chan = get_context();
    if(chan == -1)
        panicf("imx233_dpc_run: ran out of contexts");
    struct imx233_dpc_ctx_t *ctx = &dpc_ctx[chan];
    /* remember channel to release if not blocked */
    ctx->blocked = block;
    /* run the job */
    imx233_dpc_id_t id = dpc_next_id++;
    imx233_dpc_setup(ctx, id, setuptime, delay_us, fn, user, true);
    /* restore IRQ */
    restore_interrupt(oldstatus);
    /* wait if requested and release the context; the context will otherwise be
     * released by the irq */
    if(block)
    {
        /* release caller */
        semaphore_wait(&ctx->sema, TIMEOUT_BLOCK);
        /* release context */
        ctx->state = CTX_UNUSED;
        /* don't leak id for blocking DPC */
        id = 0;
    }
    return id;
}

imx233_dpc_id_t imx233_dpc_continue(struct imx233_dpc_ctx_t *ctx, uint32_t delay_us,
    imx233_dpc_fn_t fn, intptr_t user)
{
    /* we are called inside a DPC: do not schedule because the scheduler will be
     * called on DPC return. NOTE irq are disabled because we are in a DPC */
    imx233_dpc_id_t id = dpc_next_id++;
    imx233_dpc_setup(ctx, id, now(), delay_us, fn, user, false);
    return id;
}

bool imx233_dpc_abort(imx233_dpc_id_t id)
{
    /* disable IRQ to prevent timer interrupt */
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);
    /* try to find ID among still running DPC */
    bool found = false;
    for(int i = 0; i < NR_CONTEXT; i++)
        if(dpc_ctx[i].state == CTX_RUNNING && dpc_ctx[i].id == id)
        {
            dpc_ctx[i].state = CTX_UNUSED;
            found = true;
            break;
        }
    /* restore IRQ */
    restore_interrupt(oldstatus);
    return found;
}
