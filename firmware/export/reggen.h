/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
 * Based on headergen_v2 macro.h (see utils/regtools)
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
#ifndef __REGGEN_H__
#define __REGGEN_H__

#include <stdint.h>

#define __REGGEN_VAR_OR1(p, s1) \
    ((p ## s1))
#define __REGGEN_VAR_OR2(p, s1, s2) \
    ((p ## s1) | (p ## s2))
#define __REGGEN_VAR_OR3(p, s1, s2, s3) \
    ((p ## s1) | (p ## s2) | (p ## s3))
#define __REGGEN_VAR_OR4(p, s1, s2, s3, s4) \
    ((p ## s1) | (p ## s2) | (p ## s3) | (p ## s4))
#define __REGGEN_VAR_OR5(p, s1, s2, s3, s4, s5) \
    ((p ## s1) | (p ## s2) | (p ## s3) | (p ## s4) | (p ## s5))
#define __REGGEN_VAR_OR6(p, s1, s2, s3, s4, s5, s6) \
    ((p ## s1) | (p ## s2) | (p ## s3) | (p ## s4) | (p ## s5) | (p ## s6))
#define __REGGEN_VAR_OR7(p, s1, s2, s3, s4, s5, s6, s7) \
    ((p ## s1) | (p ## s2) | (p ## s3) | (p ## s4) | (p ## s5) | (p ## s6) | \
     (p ## s7))
#define __REGGEN_VAR_OR8(p, s1, s2, s3, s4, s5, s6, s7, s8) \
    ((p ## s1) | (p ## s2) | (p ## s3) | (p ## s4) | (p ## s5) | (p ## s6) | \
     (p ## s7) | (p ## s8))
#define __REGGEN_VAR_OR9(p, s1, s2, s3, s4, s5, s6, s7, s8, s9) \
    ((p ## s1) | (p ## s2) | (p ## s3) | (p ## s4) | (p ## s5) | (p ## s6) | \
     (p ## s7) | (p ## s8) | (p ## s9))
#define __REGGEN_VAR_OR10(p, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10) \
    ((p ## s1) | (p ## s2) | (p ## s3) | (p ## s4) | (p ## s5) | (p ## s6) | \
     (p ## s7) | (p ## s8) | (p ## s9) | (p ## s10))
#define __REGGEN_VAR_OR11(p, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11) \
    ((p ## s1) | (p ## s2) | (p ## s3) | (p ## s4)  | (p ## s5) | (p ## s6) | \
     (p ## s7) | (p ## s8) | (p ## s9) | (p ## s10) | (p ## s11))
#define __REGGEN_VAR_OR12(p, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12) \
    ((p ## s1) | (p ## s2) | (p ## s3) | (p ## s4)  | (p ## s5)  | (p ## s6) | \
     (p ## s7) | (p ## s8) | (p ## s9) | (p ## s10) | (p ## s11) | (p ## s12))
#define __REGGEN_VAR_OR13(p, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13) \
    ((p ## s1) | (p ## s2) | (p ## s3) | (p ## s4)  | (p ## s5)  | (p ## s6)  | \
     (p ## s7) | (p ## s8) | (p ## s9) | (p ## s10) | (p ## s11) | (p ## s12) | \
     (p ## s13))
#define __REGGEN_VAR_OR14(p, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14) \
    ((p ## s1)  | (p ## s2) | (p ## s3) | (p ## s4)  | (p ## s5)  | (p ## s6)  | \
     (p ## s7)  | (p ## s8) | (p ## s9) | (p ## s10) | (p ## s11) | (p ## s12) | \
     (p ## s13) | (p ## s14))
#define __REGGEN_VAR_OR15(p, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15) \
    ((p ## s1)  | (p ## s2)  | (p ## s3) | (p ## s4)  | (p ## s5)  | (p ## s6)  | \
     (p ## s7)  | (p ## s8)  | (p ## s9) | (p ## s10) | (p ## s11) | (p ## s12) | \
     (p ## s13) | (p ## s14) | (p ## s15))
#define __REGGEN_VAR_OR16(p, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15, s16) \
    ((p ## s1)  | (p ## s2)  | (p ## s3)  | (p ## s4)  | (p ## s5)  | (p ## s6)  | \
     (p ## s7)  | (p ## s8)  | (p ## s9)  | (p ## s10) | (p ## s11) | (p ## s12) | \
     (p ## s13) | (p ## s14) | (p ## s15) | (p ## s16))
#define __REGGEN_VAR_OR17(p, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15, s16, s17) \
    ((p ## s1)  | (p ## s2)  | (p ## s3)  | (p ## s4)  | (p ## s5)  | (p ## s6)  | \
     (p ## s7)  | (p ## s8)  | (p ## s9)  | (p ## s10) | (p ## s11) | (p ## s12) | \
     (p ## s13) | (p ## s14) | (p ## s15) | (p ## s16) | (p ## s17))
#define __REGGEN_VAR_OR18(p, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15, s16, s17, s18) \
    ((p ## s1)  | (p ## s2)  | (p ## s3)  | (p ## s4)  | (p ## s5)  | (p ## s6)  | \
     (p ## s7)  | (p ## s8)  | (p ## s9)  | (p ## s10) | (p ## s11) | (p ## s12) | \
     (p ## s13) | (p ## s14) | (p ## s15) | (p ## s16) | (p ## s17) | (p ## s18))
#define __REGGEN_VAR_OR19(p, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15, s16, s17, s18, s19) \
    ((p ## s1)  | (p ## s2)  | (p ## s3)  | (p ## s4)  | (p ## s5)  | (p ## s6)  | \
     (p ## s7)  | (p ## s8)  | (p ## s9)  | (p ## s10) | (p ## s11) | (p ## s12) | \
     (p ## s13) | (p ## s14) | (p ## s15) | (p ## s16) | (p ## s17) | (p ## s18) | \
     (p ## s19))
#define __REGGEN_VAR_OR20(p, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15, s16, s17, s18, s19, s20) \
    ((p ## s1)  | (p ## s2)  | (p ## s3)  | (p ## s4)  | (p ## s5)  | (p ## s6)  | \
     (p ## s7)  | (p ## s8)  | (p ## s9)  | (p ## s10) | (p ## s11) | (p ## s12) | \
     (p ## s13) | (p ## s14) | (p ## s15) | (p ## s16) | (p ## s17) | (p ## s18) | \
     (p ## s19) | (p ## s20))

#define __REGGEN_VAR_NARGS(...) __REGGEN_VAR_NARGS_(__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define __REGGEN_VAR_NARGS_(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N

#define __REGGEN_VAR_EXPAND(macro, prefix, ...) \
    __REGGEN_VAR_EXPAND_(macro, __REGGEN_VAR_NARGS(__VA_ARGS__), prefix, __VA_ARGS__)
#define __REGGEN_VAR_EXPAND_(macro, cnt, prefix, ...) \
    __REGGEN_VAR_EXPAND__(macro, cnt, prefix, __VA_ARGS__)
#define __REGGEN_VAR_EXPAND__(macro, cnt, prefix, ...) \
    __REGGEN_VAR_EXPAND___(macro##cnt, prefix, __VA_ARGS__)
#define __REGGEN_VAR_EXPAND___(macro, prefix, ...) \
    macro(prefix, __VA_ARGS__)

/*
 * Usage: __reg_orf(register, f1(v1), f2(v2), ...)
 *
 * Expands to the register value where each field fi has value vi.
 * Enumerated values can be obtained using the syntax: f1_V(name).
 */
#define __reg_orf(reg, ...) \
    __REGGEN_VAR_EXPAND(__REGGEN_VAR_OR, BF_##reg##_, __VA_ARGS__)

/*
 * Usage: __reg_orm(register, f1, f2, ...)
 *
 * Expands to a bitwise OR of the bitmask of each field fi.
 */
#define __reg_orm(reg, ...) \
    __REGGEN_VAR_EXPAND(__REGGEN_VAR_OR, BM_##reg##_, __VA_ARGS__)

/*
 * Usage: __reg_orm(register, f1(v1), f2(v2), ...)
 *
 * Expands to a bitwise OR of the bitmask of each field fi; the
 * arguments vi are ignored.
 */
#define __reg_orfm(reg, ...) \
    __REGGEN_VAR_EXPAND(__REGGEN_VAR_OR, BFM_##reg##_, __VA_ARGS__)

/*
 * Usage: __reg_fmask(register, field)
 *
 * Expands to the bitmask of the specified register field.
 */
#define __reg_fmask(reg, field) \
    BM_ ## reg ## _ ## field

/*
 * Usage: __reg_fpos(register, field)
 *
 * Expands to the LSB bit position of the specified register field.
 */
#define __reg_fpos(reg, field) \
    BP_ ## reg ## _ ## field

/*
 * Usage: reg_vreadf(value, register, field)
 *
 * Extract a field of the specified register type from the
 * unsigned integer 'value'.
 */
#define reg_vreadf(val, reg, field) \
    (((val) & __reg_fmask(reg, field)) >> __reg_fpos(reg, field))

/*
 * Usage: reg_vwritef(value, register, f1(v1), f2(v2), ...)
 *
 * Perform a read-modify-write operation on the variable 'value',
 * setting the field fi to value vi while leaving other bits of
 * 'value' unchanged. 'value' will be evaluated twice, once to
 * read it and once to write it.
 */
#define reg_vwritef(val, reg, ...) \
    ((val) = ((val) & ~__reg_orfm(reg, __VA_ARGS__)) | __reg_orf(reg, __VA_ARGS__))

/*
 * Usage: reg_vassignf(value, register, f1(v1), f2(v2), ...)
 *
 * Perform an assignment to the variable 'value', setting the
 * field fi to value vi. All uninitialized bits are set to 0.
 * 'value' will be evaluated once on the left hand side of an
 * assignment.
 */
#define reg_vassignf(val, reg, ...) \
    ((val) = __reg_orf(reg, __VA_ARGS__))

/*
 * Usage: reg_var(instance)
 *        reg_varl(base, instance)
 *
 * Expands to an lvalue which can be used to read or write the
 * specified register instance. The instance must have a unique
 * address calculation from a root instance. If any arrayed
 * instances are present in the address calculation then all
 * index parameters must be provided, eg: reg_var(GPIO_MODER(0)).
 *
 * With the "*l" suffix, the register address is calculated by
 * an offset relative to the provided base address.
 */
#define reg_var(inst) \
    (*(volatile ITTA_##inst*)ITA_##inst)
#define reg_varl(base, inst) \
    (*(volatile ITTO_##inst*)((((void*)(base)) + ITO_##inst)))

/*
 * Usage: reg_ptr(instance)
 *        reg_ptrl(base, instance)
 *
 * Like reg_var() but expands to a pointer to the register
 * address instead of an lvalue. The pointer will be typed
 * according to the register's word type (eg. uint32_t).
 */
#define reg_ptr(inst) \
    (&reg_var(inst))
#define reg_ptrl(base, inst) \
    (&reg_varl((base), inst))

/*
 * Usage: reg_read(instance)
 *        reg_readl(base, instance)
 *
 * Like reg_var() but expands to an rvalue which can be used
 * to read the register instance.
 */
#define reg_read(inst) \
    (*(const volatile ITTA_##inst*)ITA_##inst)
#define reg_readl(base, inst) \
    (*(const volatile ITTO_##inst*)((((void*)(base)) + ITO_##inst)))

/*
 * Usage: reg_readf(instance, field)
 *        reg_readlf(base, instance, field)
 */
#define reg_readf(inst, field) \
    reg_vreadf(reg_read(inst), ITNA_##inst, field)
#define reg_readlf(base, inst, field) \
    reg_vreadf(reg_readl((base), inst), ITNO_##inst, field)

/*
 * Usage: reg_writef(instance, f1(v1), f2(v2), ...)
 *        reg_writelf(base, instance, f1(v1), f2(v2), ...)
 */
#define reg_writef(inst, ...) \
    reg_vwritef(reg_var(inst), ITNA_##inst, __VA_ARGS__)
#define reg_writelf(base, inst, ...) \
    reg_vwritef(reg_varl((base), inst), ITNO_##inst, __VA_ARGS__)

/*
 * Usage: reg_assignf(instance, f1(v1), f2(v2), ...)
 *        reg_assignlf(base, instance, f1(v1), f2(v2), ...)
 */
#define reg_assignf(inst, ...) \
    reg_vassignf(reg_var(inst), ITNA_##inst, __VA_ARGS__)
#define reg_assignlf(base, inst, ...) \
    reg_vassignf(reg_varl((base), inst), ITNO_##inst, __VA_ARGS__)

#endif /* __REGGEN_H__ */
