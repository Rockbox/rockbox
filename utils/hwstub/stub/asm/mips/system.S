/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2015 by Marcin Bukat
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
#include "mips.h"

/* Handling of data abort:
 * the code can register a "longjmp" buffer to restore the context in case of
 * fault */
.data
.global data_abort_jmp_ctx_ptr
data_abort_jmp_ctx_ptr:
/* buffer contains in order: s0-s7, sp, s8, ra */
.skip   44 /* = 4 * (9 callee saved registers + sp + ra) */

.set noreorder
.section .icode, "ax", %progbits
/* Prototype: int set_data_abort_jmp()
 * Return: 1 in case of data abort, 0 otherwise */
.global set_data_abort_jmp
set_data_abort_jmp:
    la v0,  data_abort_jmp_ctx_ptr
    sw s0,  0(v0)
    sw s1,  4(v0)
    sw s2,  8(v0)
    sw s3, 12(v0)
    sw s4, 16(v0)
    sw s5, 20(v0)
    sw s6, 24(v0)
    sw s7, 28(v0)
    sw sp, 32(v0)
    sw s8, 36(v0)
    sw ra, 40(v0)
    jr ra
    move v0, zero
.set reorder
