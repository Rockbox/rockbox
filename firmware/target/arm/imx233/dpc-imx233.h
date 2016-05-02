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
#ifndef DPC_IMX233_H
#define DPC_IMX233_H

#include "system.h"

/** Deferred Procedure Call
 *
 * This driver provides a very flexible and powerful DPC interface.
 * In provides a number of handy features but should be used with caution.
 * - callee is called in IRQ context
 * - caller can be automatically blocked
 * - callee can relaunch DPC to defer even more work
 * - high precision (micro-second) and long delays (as long as it fits on 32-bits)
 * NOTE caller is blocked using system semaphore, thus subject to usual
 * scheduling, as a result there is an unpredictable delay between the end
 * of the callee and the release of the caller.
 *
 * The typical use of this interface is to schedule work at a deterministic time
 * in the near future, typically between 50us to 10ms. For longer delays or when
 * accuracy is not needed, the system tick tasks or timeout are probably simpler
 * to use.
 */

/** DPC context */
struct imx233_dpc_ctx_t;
/** DPC id (for cancellation) */
typedef int imx233_dpc_id_t;

#define DPC_INVALID_ID  -1 /* ID that is guaranteed to be invalid */

/** DPC function
 * The DPC function can continue a DPC using the given context,
 * If the caller was blocked, it will be released at the function return, unless
 * the DPC is continued.
 */
typedef void (*imx233_dpc_fn_t)(struct imx233_dpc_ctx_t *ctx, intptr_t user);

void imx233_dpc_init(void);
/* Launch a DPC, optionally block the caller, return ID for cancellation (if not blocking) */
imx233_dpc_id_t imx233_dpc_start(bool block, uint32_t delay_us, imx233_dpc_fn_t fn, intptr_t user);
/* Abort a DPC
 * Note that there is an inherent race condition between cancelling and timer
 * expiring. This function returns true if the DPC was aborted, and false if the
 * DPC expired. If the DPC is successfully aborted, the DPC function is NOT called.
 * NOTE: only nonblocking DPC can be cancelled at the moment. */
bool imx233_dpc_abort(imx233_dpc_id_t id);
/* Relaunch a DPC from the the callee */
imx233_dpc_id_t imx233_dpc_continue(struct imx233_dpc_ctx_t *ctx, uint32_t delay_us,
    imx233_dpc_fn_t fn, intptr_t user);

#endif /* DPC_IMX233_H */
 
