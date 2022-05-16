/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * MIPS32 backtrace implementation
 * Copyright (C) 2022 Aidan MacDonald
 *
 * References:
 * https://yosefk.com/blog/getting-the-call-stack-without-a-frame-pointer.html
 * https://elinux.org/images/6/68/ELC2008_-_Back-tracing_in_MIPS-based_Linux_Systems.pdf
 * System V ABI MIPS Supplement, 3rd edition
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

#include "backtrace.h"
#include "lcd.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdio.h>

#define MIN_ADDR    0x80000000ul
#define MAX_ADDR    (MIN_ADDR + (MEMORYSIZE * 1024 * 1024) - 1)

static bool read_check(const void* addr, size_t alignment)
{
    if(addr < (const void*)MIN_ADDR ||
       addr > (const void*)MAX_ADDR)
        return false;

    if((uintptr_t)addr & (alignment - 1))
        return false;

    return true;
}

static bool read32(const void* addr, uint32_t* val)
{
    if(!read_check(addr, alignof(uint32_t)))
        return false;

    *val = *(const uint32_t*)addr;
    return true;
}

#if 0
static bool read16(const void* addr, uint16_t* val)
{
    if(!read_check(addr, alignof(uint16_t)))
        return false;

    *val = *(const uint16_t*)addr;
    return true;
}

static bool read8(const void* addr, uint8_t* val)
{
    if(!read_check(addr, alignof(uint8_t)))
        return false;

    *val = *(const uint8_t*)addr;
    return true;
}
#endif

static long extract_s16(uint32_t val)
{
#if 1
    /* not ISO C, but gets GCC to emit 'seh' which is more compact */
    return (int32_t)(val << 16) >> 16;
#else
    val &= 0xffff;

    if(val > 0x7fff)
        return (long)val - 0x10000l;
    else
        return val;
#endif
}

/* TODO - cases not handled by the backtrace algorithm
 *
 * 1. functions that save the frame pointer will not be handled correctly
 *    (need to implement the algorithm specified by the System V ABI).
 *
 * 2. GCC can generate functions with a "false" stack pointer decrement,
 *    for some examples see read_bmp_fd and walk_path. those functions
 *    seem to be more difficult to deal with and the SysV algorithm will
 *    also get confused by them, but they are not common.
 */
int mips_bt_step(struct mips_bt_context* ctx)
{
    /* go backward and look for the stack pointer decrement */
    uint32_t* pc = ctx->pc;
    uint32_t insn;
    long sp_off;

    while(true) {
        if(!read32(pc, &insn))
            return 0;

        /* addiu sp, sp, sp_off */
        if((insn >> 16) == 0x27bd) {
            sp_off = extract_s16(insn);
            if(sp_off < 0)
                break;
        }

        /* jr ra */
        if(insn == 0x03e00008) {
            /* end if this is not a leaf or we lack register info */
            if(ctx->depth > 0 || !(ctx->valid & (1 << MIPSBT_RA))) {
                mips_bt_debug(ctx, "unexpected leaf function");
                return 0;
            }

            /* this is a leaf function - ra contains the return address
             * and sp is unchanged */
            ctx->pc = (void*)ctx->reg[MIPSBT_RA] - 8;
            ctx->depth++;
            return 1;
        }

        --pc;
    }

    mips_bt_debug(ctx, "found sp_off=%ld at %p", sp_off, pc);

    /* now go forward and find the saved return address */
    while((void*)pc < ctx->pc) {
        if(!read32(pc, &insn))
            return 0;

        /* sw ra, ra_off(sp) */
        if((insn >> 16) == 0xafbf) {
            long ra_off = extract_s16(insn);
            uint32_t save_ra;

            /* load the saved return address */
            mips_bt_debug(ctx, "load ra from %p+%ld", ctx->sp, ra_off);
            if(!read32(ctx->sp + ra_off, &save_ra))
                return 0;

            if(save_ra == 0) {
                mips_bt_debug("hit root");
                return 0;
            }

            /* update bt context */
            ctx->pc = (void*)save_ra - 8;
            ctx->sp -= sp_off;

            /* update saved register info */
            ctx->reg[MIPSBT_RA] = save_ra;
            ctx->valid |= (1 << MIPSBT_RA);

            ctx->depth++;
            return 1;
        }

        ++pc;
    }

    /* sometimes an exception occurs before ra is saved - in this case
     * the ra register should contain the caller PC, but we still need
     * to adjust the stack frame. */
    if(ctx->depth == 0 && (ctx->valid & (1 << MIPSBT_RA))) {
        ctx->pc = (void*)ctx->reg[MIPSBT_RA] - 8;
        ctx->sp -= sp_off;

        ctx->depth++;
        return 1;
    }

    mips_bt_debug(ctx, "ra not found");
    return 0;
}

#ifdef MIPSUNWINDER_DEBUG
static void rb_backtrace_debugf(void* arg, const char* fmt, ...)
{
    static char buf[64];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    unsigned int* line = arg;
    lcd_putsf(4, (*line)++, "%s", buf);
    lcd_update();
}
#endif

void rb_backtrace_ctx(void* arg, unsigned* line)
{
    struct mips_bt_context* ctx = arg;
#ifdef MIPSUNWINDER_DEBUG
    ctx->debugf = rb_backtrace_debugf;
    ctx->debug_arg = line;
#endif

    do {
        lcd_putsf(0, (*line)++, "%02d pc:%08lx sp:%08lx",
                  ctx->depth, (unsigned long)ctx->pc, (unsigned long)ctx->sp);
        lcd_update();
    } while(mips_bt_step(ctx));

    lcd_puts(0, (*line)++, "bt end");
    lcd_update();
}

void rb_backtrace(int pcAddr, int spAddr, unsigned* line)
{
    (void)pcAddr;
    (void)spAddr;

    struct mips_bt_context ctx;
    mips_bt_start(&ctx);
    mips_bt_step(&ctx); /* step over this function */

    rb_backtrace_ctx(&ctx, line);
}
