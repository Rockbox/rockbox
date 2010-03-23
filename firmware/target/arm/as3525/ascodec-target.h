/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for AS3514 audio codec
 *
 * Copyright (c) 2007 Daniel Ankers
 * Copyright (c) 2007 Christian Gmeiner
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

#ifndef _ASCODEC_TARGET_H
#define _ASCODEC_TARGET_H

#ifndef SIMULATOR

#include "as3514.h"
#include "kernel.h" /* for struct wakeup */

/*  Charge Pump and Power management Settings  */
#define AS314_CP_DCDC3_SETTING    \
               ((0<<7) |  /* CP_SW  Auto-Switch Margin 0=200/300 1=150/255 */  \
                (0<<6) |  /* CP_on  0=Normal op 1=Chg Pump Always On  */       \
                (0<<5) |  /* LREG_CPnot  Always write 0 */                     \
                (0<<3) |  /* DCDC3p  BVDD setting 3.6/3.2/3.1/3.0  */          \
                (1<<2) |  /* LREG_off 1=Auto mode switching 0=Length Reg only*/\
                (0<<0) )  /* CVDDp  Core Voltage Setting 1.2/1.15/1.10/1.05*/

#define CVDD_1_20          0
#define CVDD_1_15          1
#define CVDD_1_10          2
#define CVDD_1_05          3

#define ASCODEC_REQ_READ   0
#define ASCODEC_REQ_WRITE  1

/*
 * How many bytes we using in struct ascodec_request for the data buffer.
 * 4 fits the alignment best right now.
 * We don't actually use more than 2 at the moment (in adc_read).
 * Upper limit would be 255 since DACNT is 8 bits!
 */
#define ASCODEC_REQ_MAXLEN 4

typedef void (ascodec_cb_fn)(unsigned const char *data, unsigned cnt);

struct ascodec_request {
    unsigned char type;
    unsigned char index;
    unsigned char status;
    unsigned char cnt;
    unsigned char data[ASCODEC_REQ_MAXLEN];
    struct wakeup wkup;
    ascodec_cb_fn *callback;
    struct ascodec_request *next;
};

void ascodec_init(void);

void ascodec_init_late(void);

int ascodec_write(unsigned int index, unsigned int value);

int ascodec_read(unsigned int index);

int ascodec_readbytes(unsigned int index, unsigned int len, unsigned char *data);

/*
 * The request struct passed in must be allocated statically.
 * If you call ascodec_async_write from different places, each
 * call needs it's own request struct.
 * This comment is duplicated in .c and .h for your convenience.
 */
void ascodec_async_write(unsigned index, unsigned int value, struct ascodec_request *req);

/*
 * The request struct passed in must be allocated statically.
 * If you call ascodec_async_read from different places, each
 * call needs it's own request struct.
 * If len is bigger than ASCODEC_REQ_MAXLEN it will be
 * set to ASCODEC_REQ_MAXLEN.
 * This comment is duplicated in .c and .h for your convenience.
 */
void ascodec_async_read(unsigned index, unsigned int len,
                        struct ascodec_request *req, ascodec_cb_fn *cb);

void ascodec_req_init(struct ascodec_request *req, int type,
                      unsigned int index, unsigned int cnt);

void ascodec_submit(struct ascodec_request *req);

void ascodec_lock(void);

void ascodec_unlock(void);

void ascodec_wait_adc_finished(void);

void ascodec_enable_endofch_irq(void);

void ascodec_disable_endofch_irq(void);

#endif /* !SIMULATOR */

#endif /* !_ASCODEC_TARGET_H */
