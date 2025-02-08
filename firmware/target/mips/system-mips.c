/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2022 Aidan MacDonald
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

#include "system.h"
#include "backtrace.h"
#include "lcd.h"
#include "backlight-target.h"
#include "font.h"
#include "logf.h"
#include "mips.h"
#undef sp /* breaks backtrace lib */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* copies exception frame info and error pc to the backtrace context */
static void setup_exception_bt(struct mips_exception_frame* frame,
                               unsigned long epc, struct mips_bt_context* ctx)
{
    ctx->pc = (void*)epc;
    ctx->sp = (void*)frame->gpr[29 - 3];
    ctx->depth = 0;
    ctx->valid = (1 << MIPSBT_RA);
    ctx->reg[MIPSBT_RA] = frame->gpr[31 - 3];
}

/* dump backtrace for an exception */
static void exception_bt(void* frame, unsigned long epc, unsigned* line)
{
    struct mips_bt_context ctx;
    setup_exception_bt(frame, epc, &ctx);
    rb_backtrace_ctx(&ctx, line);
}

/*
 * TODO: This should be converted into a generic panic routine that accepts
 * a backtrace context argument but the ARM backtrace setup will need to be
 * refactored in order to do that
 */
static void exception_dump(void* frame, unsigned long epc,
                           const char* fmt, ...)
{
    extern char panic_buf[128];
    va_list ap;

    set_irq_level(DISABLE_INTERRUPTS);

    va_start(ap, fmt);
    vsnprintf(panic_buf, sizeof(panic_buf), fmt, ap);
    va_end(ap);

    lcd_set_viewport(NULL);
#if LCD_DEPTH > 1
    lcd_set_backdrop(NULL);
    lcd_set_drawinfo(DRMODE_SOLID, LCD_BLACK, LCD_WHITE);
#endif

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    unsigned y = 1;
    lcd_puts(1, y++, "*EXCEPTION*");

    /* wrap panic message */
    {
        const int linechars = (LCD_WIDTH / SYSFONT_WIDTH) - 2;

        int pos = 0, len = strlen(panic_buf);
        while(len > 0) {
            int idx = pos + MIN(len, linechars);
            char c = panic_buf[idx];
            panic_buf[idx] = 0;
            lcd_puts(1, y++, &panic_buf[pos]);
            panic_buf[idx] = c;

            len -= linechars;
            pos += linechars;
        }
    }

    exception_bt(frame, epc, &y);
#ifdef ROCKBOX_HAS_LOGF
    logf_panic_dump(&y);
#endif

    lcd_update();
    backlight_hw_on();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    if (cpu_boost_lock())
    {
        set_cpu_frequency(0);
        cpu_boost_unlock();
    }
#endif

#ifdef HAVE_ATA_POWER_OFF
    ide_power_enable(false);
#endif

    system_exception_wait();
    system_reboot();
    while(1);
}

#define EXC(x,y) case (x): return (y);
static char* parse_exception(unsigned long cause)
{
    switch(cause & M_CauseExcCode)
    {
        EXC(EXC_INT, "Interrupt");
        EXC(EXC_MOD, "TLB Modified");
        EXC(EXC_TLBL, "TLB Exception (Load or Ifetch)");
        EXC(EXC_ADEL, "Address Error (Load or Ifetch)");
        EXC(EXC_ADES, "Address Error (Store)");
        EXC(EXC_TLBS, "TLB Exception (Store)");
        EXC(EXC_IBE, "Instruction Bus Error");
        EXC(EXC_DBE, "Data Bus Error");
        EXC(EXC_SYS, "Syscall");
        EXC(EXC_BP, "Breakpoint");
        EXC(EXC_RI, "Reserved Instruction");
        EXC(EXC_CPU, "Coprocessor Unusable");
        EXC(EXC_OV, "Overflow");
        EXC(EXC_TR, "Trap Instruction");
        EXC(EXC_FPE, "Floating Point Exception");
        EXC(EXC_C2E, "COP2 Exception");
        EXC(EXC_MDMX, "MDMX Exception");
        EXC(EXC_WATCH, "Watch Exception");
        EXC(EXC_MCHECK, "Machine Check Exception");
        EXC(EXC_CacheErr, "Cache error caused re-entry to Debug Mode");
        default:
            return 0;
    }
}
#undef EXC

void exception_handler(void* frame, unsigned long epc)
{
    unsigned long cause = read_c0_cause();

    exception_dump(frame, epc, "%s [0x%08x] at %08lx",
                   parse_exception(cause), read_c0_badvaddr(), epc);
}

void cache_error_handler(void* frame, unsigned long epc)
{
    exception_dump(frame, epc, "Cache Error [0x%08x] at %08lx",
                   read_c0_cacheerr(), epc);
}

void tlb_refill_handler(void* frame, unsigned long epc)
{
    exception_dump(frame, epc, "TLB refill at %08lx [0x%x]",
                   epc, read_c0_badvaddr());
}
