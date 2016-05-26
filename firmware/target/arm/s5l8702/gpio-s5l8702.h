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
 * Code based on openiBoot project
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
#ifndef __GPIO_S5L8702_H__
#define __GPIO_S5L8702_H__
#include <stdint.h>


#define REG32_PTR_T volatile uint32_t *

/*
 * s5l8702 External (GPIO) Interrupt Controller
 *
 * 7 groups of 32 interrupts, GPIO pins are seen as 'wired'
 * to groups 6..3 in reverse order.
 * On group 3, last four bits are dissbled (GPIO 124..127).
 * All bits in groups 1 and 2 are disabled (not used).
 * On group 0, all bits are masked except bits 0 and 2:
 *  bit 0: if unmasked, EINT6 is generated when ALVTCNT
 *         reachs ALVTEND.
 *  bit 2: if unmasked, EINT6 is generated when USB cable
 *         is plugged and/or(TBC) unplugged.
 *
 * EIC_GROUP0..6 are connected to EINT6..0 of the VIC.
 */
#define EIC_N_GROUPS    7

/* get EIC group and bit for a given GPIO port */
#define EIC_GROUP(n)    (6 - ((n) >> 5))
#define EIC_INDEX(n)    ((0x18 - ((n) & 0x18)) | ((n) & 0x7))

/* SoC EINTs uses these 'gpio' numbers */
#define GPIO_EINT_USB       0xd8
#define GPIO_EINT_ALIVE     0xda

/* probably a part of the system controller */
#define EIC_BASE 0x39a00000

#define EIC_INTLEVEL(g) (*((REG32_PTR_T)(EIC_BASE + 0x80 + 4*(g))))
#define EIC_INTSTAT(g)  (*((REG32_PTR_T)(EIC_BASE + 0xA0 + 4*(g))))
#define EIC_INTEN(g)    (*((REG32_PTR_T)(EIC_BASE + 0xC0 + 4*(g))))
#define EIC_INTTYPE(g)  (*((REG32_PTR_T)(EIC_BASE + 0xE0 + 4*(g))))

#define EIC_INTLEVEL_LOW    0
#define EIC_INTLEVEL_HIGH   1

#define EIC_INTTYPE_EDGE    0
#define EIC_INTTYPE_LEVEL   1


struct eint_handler {
    uint8_t gpio_n;
    uint8_t type;       /* EIC_INTTYPE_ */
    uint8_t level;      /* EIC_INTLEVEL_ */
    uint8_t autoflip;
    void (*isr)(struct eint_handler*);
};

void eint_init(void);
void eint_register(struct eint_handler *h);
void eint_unregister(struct eint_handler *h);

void gpio_preinit(void);
void gpio_init(void);
/* get/set configuration for GPIO groups (0..15) */
uint32_t gpio_group_get(int group);
void gpio_group_set(int group, uint32_t mask, uint32_t cfg);


/*
 * This is very preliminary work in progress, ATM this region is called
 * system 'alive' because it seems there are similiarities when mixing
 * concepts from:
 *  - s3c2440 datasheet (figure 7-12, Sleep mode) and
 *  - ARM-DDI-0287B (2.1.8 System Mode Control, Sleep an Doze modes)
 *
 * Known components:
 * - independent clocking
 * - 32-bit timer
 * - level/edge configurable interrupt controller
 *
 *
 *        OSCSEL
 *          |\    CKSEL
 *  OSC0 -->| |    |\
 *          | |--->| |     _________             ___________
 *  OSC1 -->| |    | |    |         | SClk      |           |
 *          |/     | |--->| 1/CKDIV |---------->| 1/ALVTDIV |--> Timer
 *                 | |    |_________|       |   |___________|    counter
 *  PClk --------->| |                      |    ___________
 *                 |/                       |   |           |
 *                                          +-->| 1/UNKDIV  |--> Unknown
 *                                              |___________|
 */
#define SYSALV_BASE 0x39a00000

#define ALVCON          (*((REG32_PTR_T)(SYSALV_BASE + 0x0)))
#define ALVUNK4         (*((REG32_PTR_T)(SYSALV_BASE + 0x4)))
#define ALVUNK100       (*((REG32_PTR_T)(SYSALV_BASE + 0x100)))
#define ALVUNK104       (*((REG32_PTR_T)(SYSALV_BASE + 0x104)))


/*
 * System Alive control register
 */
#define ALVCON_CKSEL_BIT    (1 << 25)  /* 0 -> S5L8702_OSCx, 1 -> PClk */
#define ALVCON_CKDIVEN_BIT  (1 << 24)  /* 0 -> CK divider Off, 1 -> On */
#define ALVCON_CKDIV_POS    20         /* real_val = reg_val+1 */
#define ALVCON_CKDIV_MSK    0xf

/* UNKDIV: real_val = reg_val+1 (TBC), valid reg_val=0,1,2 */
/* experimental: for registers in this region, read/write speed is
 * scaled by this divider, so probably it is related with internal
 * 'working' frequency.
 */
#define ALVCON_UNKDIV_POS   16
#define ALVCON_UNKDIV_MSK   0x3

/* bits[14:1] are UNKNOWN */

#define ALVCON_OSCSEL_BIT   (1 << 0)   /* 0 -> OSC0, 1 -> OSC1 */


/*
 * System Alive timer
 *
 * ALVCOM_RUN_BIT starts/stops count on ALVTCNT, counter frequency
 * is SClk / ALVTDIV. When count reachs ALVTEND then ALVTSTAT[0]
 * and ALVUNK4[0] are set, optionally an interrupt is generated (see
 * GPIO_EINT_ALIVE). Writing 1 to ALVTCOM_RST_BIT clears ALVSTAT[0]
 * and ALVUNK4[0], and initializes ALVTCNT to zero.
 */
#define ALVTCOM         (*((REG32_PTR_T)(SYSALV_BASE + 0x6c)))
#define ALVTCOM_RUN_BIT     (1 << 0)  /* 0 -> Stop, 1 -> Start */
#define ALVTCOM_RST_BIT     (1 << 1)  /* 1 -> Reset */

#define ALVTEND         (*((REG32_PTR_T)(SYSALV_BASE + 0x70)))
#define ALVTDIV         (*((REG32_PTR_T)(SYSALV_BASE + 0x74)))

#define ALVTCNT         (*((REG32_PTR_T)(SYSALV_BASE + 0x78)))
#define ALVTSTAT        (*((REG32_PTR_T)(SYSALV_BASE + 0x7c)))

#endif /* __GPIO_S5L8702_H__ */
