/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright (C) 2015 by Cástor Muñoz
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
#ifndef __CWHEEL_S5L8702_H__
#define __CWHEEL_S5L8702_H__

#include "inttypes.h"

/* GPIOE2: RTS (output)  - HIGH when idle, LOW pulse to Request To Send
 * GPIOE3: CLK (input)   - HIGH when idle, ~52890 Hz for Tx/Rx
 * GPIOE4: DOUT (output) - HIGH when idle
 * GPIOE5: DIN (input)   - HIGH when idle
 *
 * TODO: document host and external controllers
 */

/* control register */
#define WHEELCON     (*((uint32_t volatile*)(0x3C200000)))

#define WHEELCON_DLY_POS    0
#define WHEELCON_DLY_MSK    0xffff

#define WHEELCON_TXED_POS   16
#define WHEELCON_TXED_MSK   0x7

#define WHEELCON_TXMOD_BIT  (1 << 19)
#define WHEELCON_TMO_BIT    (1 << 20)
#define WHEELCON_EN_BIT     (1 << 21)
#define WHEELCON_FDX_BIT    (1 << 22)

/* command register */
#define WHEELCMD    (*((uint32_t volatile*)(0x3C200004)))
#define WHEELCMD_RTS_BIT    (1 << 0)

/* Tx/Rx timeout, PClk pulses*/
#define WHEELTMO    (*((uint32_t volatile*)(0x3C200008)))

/* Tx/Rx status */
#define WHEELSTA    (*((uint32_t volatile*)(0x3C20000C)))

#define WHEELSTA_RXOK_BIT   (1 << 0)
#define WHEELSTA_TXOK_BIT   (1 << 1)
#define WHEELSTA_RXING_BIT  (1 << 2)
#define WHEELSTA_TXING_BIT  (1 << 3)
#define WHEELSTA_RXERR_BIT  (1 << 4)
#define WHEELSTA_TXERR_BIT  (1 << 5)
#define WHEELSTA_RXOVR_BIT  (1 << 6)
#define WHEELSTA_RXCNT_POS  8
#define WHEELSTA_RXCNT_MSK  0x3

/* interrupt mask */
#define WHEELINTM   (*((uint32_t volatile*)(0x3C200010)))

/* interrupt status */
#define WHEELINT    (*((uint32_t volatile*)(0x3C200014)))
#define WHEELINT_RX_BIT     (1 << 0)
#define WHEELINT_TX_BIT     (1 << 1)
#define WHEELINT_ERR_BIT    (1 << 2)

/* tx/rx data registers */
#define WHEELRX     (*((uint32_t volatile*)(0x3C200018)))
#define WHEELTX     (*((uint32_t volatile*)(0x3C20001C)))
#define WHEELRX2    (*((uint32_t volatile*)(0x3C200020)))


/* wheel message types */
#define WMSG_EVENT      0x1     /* Remote -> Host */
#define WMSG_SET        0x2     /* Host -> Remote */
#define WMSG_GET        0x3     /* Host -> Remote -> Host */
#define WMSG_HELLO      0xA     /* Remote -> Host */

/* build WMSG_SET/GET */
#define WHEEL_MSG(val, reg, type) \
    (0x8000000a | (val<<16) | ((reg & 0xff)<<8) | ((type & 0xf)<<4))

/* clickwheel GPIO modes */
enum {
    CWMODE_CTRL,    /* controller Tx/Rx */
    CWMODE_MAN,     /* manual Tx/Rx */
    /*CWMODE_MANTX,*/   /* manual Tx/Rx, ctrl Rx */
};
#define CWFLAG_LOCKED   (1 << 8)
#define CWFLAG_HSPOLL   (1 << 9)

int read_all_buttons(int n_retry);
void cwheel_set_gpio(int cwmode);
int cwheel_get_reg(int reg);
#if 0 /* not used */
bool cwheel_set_reg(int reg, int value);
#endif

#endif /* __CWHEEL_S5L8702_H__ */
