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

#ifndef BACKTRACE_MIPSUNWINDER_H
#define BACKTRACE_MIPSUNWINDER_H

/*#define MIPSUNWINDER_DEBUG*/

#include <stdint.h>

enum {
    MIPSBT_RA,
    MIPSBT_NREG,
};

struct mips_bt_context {
    void* pc;
    void* sp;
    int depth;
    uint32_t valid;
    uint32_t reg[MIPSBT_NREG];
#ifdef MIPSUNWINDER_DEBUG
    void(*debugf)(void*, const char*, ...);
    void* debug_arg;
#endif
};

int mips_bt_step(struct mips_bt_context* ctx);
void mips_bt_start(struct mips_bt_context* ctx);

#ifdef MIPSUNWINDER_DEBUG
# define mips_bt_debug(ctx, ...) \
    do { struct mips_bt_context* __ctx = ctx; \
         if(__ctx->debugf) \
             __ctx->debugf(__ctx->debug_arg, __VA_ARGS__);\
    } while(0)
#else
# define mips_bt_debug(...) do { } while(0)
#endif

/* NOTE: ignores pcAddr and spAddr, backtrace starts from caller */
void rb_backtrace(int pcAddr, int spAddr, unsigned* line);

/* given struct mips_bt_context argument, print stack traceback */
void rb_backtrace_ctx(void* arg, unsigned* line);

#endif /* BACKTRACE_MIPSUNWINDER_H */
