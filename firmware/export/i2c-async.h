/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __I2C_ASYNC_H__
#define __I2C_ASYNC_H__

#include "config.h"

/* i2c-async provides an API for asynchronous communication over an I2C bus.
 * It's not linked to a specific target, so device drivers using this API can
 * be shared more easily among multiple targets.
 *
 * Transactions are built using descriptors, and callbacks can be used to
 * perform work directly from interrupt context. Callbacks can even change
 * the descriptor chain on the fly, so the transaction can be altered based
 * on data recieved over the I2C bus.
 *
 * There's an API for synchronous operations on devices using 8-bit register
 * addresses. This API demonstrates how you can build more specialized routines
 * on top of the asynchronous API, and is useful in its own right for dealing
 * with simple devices.
 */

#ifdef HAVE_I2C_ASYNC

#include "i2c-target.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Queueing codes */
#define I2C_RC_OK       0
#define I2C_RC_BUSY     1
#define I2C_RC_NOTADDED 2
#define I2C_RC_NOTFOUND 3

/* Descriptor status codes */
#define I2C_STATUS_OK       0
#define I2C_STATUS_ERROR    1
#define I2C_STATUS_TIMEOUT  2
#define I2C_STATUS_SKIPPED  3

/* I2C bus end conditions */
#define I2C_START    (1 << 0)
#define I2C_RESTART  (1 << 1)
#define I2C_CONTINUE (1 << 2)
#define I2C_STOP     (1 << 3)
#define I2C_HOLD     (1 << 4)

/* Transfer modes */
#define I2C_READ    0
#define I2C_WRITE   1

/* Queue modes */
#define I2C_Q_ADD       0
#define I2C_Q_ONCE      1
#define I2C_Q_REPLACE   2

/* Flag for using 10-bit addresses */
#define I2C_10BIT_ADDR  0x8000

/* Descriptors are used to set up an I2C transfer. The transfer mode is
 * specified by 'tran_mode', and is normally a READ or WRITE operation.
 * The transfer mode dictates how the buffer/count fields are interpreted.
 *
 * - I2C_WRITE
 *      buffer[0] must be non-NULL and count[0] must be at least 1.
 *      The transfer sends count[0] bytes from buffer[0] on the bus.
 *
 *      buffer[1] and count[1] can be used to specify a second buffer
 *      whose contents are written after the buffer[0]'s last byte.
 *      No start/stop conditions are issued between the buffers; it is
 *      as if you had appended the contents of buffer[1] to buffer[0].
 *
 *      If not used, buffer[1] must be NULL and count[1] must be 0.
 *      If used, buffer[1] must be non-NULL and count[1] must be >= 1.
 *
 * - I2C_READ
 *      buffer[1] must be non-NULL and count[1] must be at least 1.
 *      The transfer will request count[1] bytes and writes the data
 *      into buffer[1].
 *
 *      This type of transfer can send some data bytes before the
 *      read operation, eg. to send a register address for the read.
 *      If this feature is not used, set buffer[0] to NULL and set
 *      count[0] to zero.
 *
 *      If used, buffer[0] must be non-NULL and count[0] must be >= 1.
 *      Between the last byte written and the first byte read, the bus
 *      will automatically issue a RESTART condition.
 *
 * Bus conditions are divided into two classes: start and end conditions.
 * You MUST supply both a start and end condition in the 'bus_cond' field,
 * by OR'ing together one start condition and one end condition:
 *
 * - I2C_START
 *      Issue a START condition before the first data byte. This must be
 *      specified on the first descriptor of a transaction.
 *
 * - I2C_RESTART
 *      Issue a RESTART condition before the first data byte. On the bus this
 *      is physically identical to a START condition, but drivers might need
 *      this to distinguish between these two cases.
 *
 * - I2C_CONTINUE
 *      Do not issue any condition before the first data byte. This is only
 *      valid if the descriptor continues a previous descriptor which ended
 *      with the HOLD condition; otherwise the results are undefined.
 *
 * - I2C_STOP
 *      Issue a STOP condition after the last data byte. This must be set on
 *      the final descriptor in a transaction, so the bus is left in a usable
 *      state when the descriptor finishes.
 *
 * - I2C_HOLD
 *      Do not issue any condition after the last data byte. This is only
 *      valid if the next descriptor starts with an I2C_CONTINUE condition.
 */
typedef struct i2c_descriptor {
    /* Address of the target device. To use 10-bit addresses, simply
     * OR the address with the I2C_10BIT_ADDR. */
    uint16_t slave_addr;

    /* What to do at the ends of the data transfer */
    uint8_t bus_cond;

    /* Transfer mode */
    uint8_t tran_mode;

    /* Buffer/length fields. Their use depends on the transfer mode. */
    void* buffer[2];
    int count[2];

    /* Callback which is invoked when the descriptor completes.
     *
     * The first argument is a status code and the second argument is a
     * pointer to the completed descriptor. The status code is the only
     * way of checking whether the descriptor completed successfully,
     * and it is NOT saved, so ensure you save it yourself if needed.
     */
    void(*callback)(int, struct i2c_descriptor*);

    /* Argument field reserved for the user; not touched by the driver. */
    intptr_t arg;

    /* Pointer to the next descriptor. */
    struct i2c_descriptor* next;
} i2c_descriptor;

/* Public API */

/* Aysnchronously enqueue a descriptor, optionally waiting on a timeout
 * if the queue is full. The exact behavior depends on 'q_mode':
 *
 * - I2C_Q_ADD
 *      Always try to enqueue the descriptor.
 *
 * - I2C_Q_ONCE
 *      Only attempt to enqueue the descriptor if no descriptor with the same
 *      cookie is already running or queued. If this is not the case, then
 *      returns I2C_RC_NOTADDED.
 *
 * - I2C_Q_REPLACE
 *      If a descriptor with the same cookie is queued, replace it with this
 *      descriptor and do not run the old descriptor's callbacks. If the
 *      matching descriptor is running, returns I2C_RC_NOTADDED and does not
 *      queue the new descriptor. If no match was found, then simply add the
 *      new descriptor to the queue.
 *
 * The 'cookie' is only useful if you want to use the ONCE or REPLACE queue
 * modes, or if you want to use i2c_async_cancel(). Cookies used for queue
 * management must be reserved with i2c_async_reserve_cookies(), to prevent
 * different drivers from stepping on each other's toes.
 *
 * When you do not need queue management, you can use a 'cookie' of 0, which
 * is reserved for unmanaged transactions. Only use I2C_Q_ADD if you do this.
 *
 * Queuing is only successful if I2C_RC_OK is returned. All other codes
 * indicate that the descriptor was not queued, and therefore will not be
 * executed.
 *
 * Be careful about how/when you modify and queue descriptors. It's unsafe to
 * modify a queued descriptor: it could start running at any time, and the bus
 * might see a half-rewritten version of the descriptor. You _can_ queue the
 * same descriptor twice, since the i2c-async driver is not allowed to modify
 * any fields, but your callbacks need to be written with this case in mind.
 *
 * You can use queue management to help efficiently re-use descriptors.
 * Typically you can alternate between multiple descriptors, always keeping one
 * free to modify, and using your completion callbacks to cycle the free slot.
 * You can also probe with i2c_async_cancel() to ensure a specific descriptor
 * is not running before modifying it.
 */
extern int i2c_async_queue(int bus, int timeout, int q_mode,
                           int cookie, i2c_descriptor* desc);

/* Cancel a queued descriptor. Searches the queue, starting with the running
 * descriptor, for a descriptor with a matching cookie, and attempts to remove
 * it from the queue.
 *
 * - Returns I2C_RC_NOTFOUND if no match was found.
 * - Returns I2C_RC_BUSY if the match is the currently running transaction.
 * - Returns I2C_RC_OK if the match was found in the pending queue and was
 *   successfully removed from the queue.
 */
extern int i2c_async_cancel(int bus, int cookie);

/* Reserve a range of cookie values for use by a driver. This should only be
 * done once at startup. The driver doesn't care what cookies are used, so you
 * can manage them any way you like.
 *
 * A range [r, r+count) will be allocated, disjoint from all other allocated
 * ranges and with r >= 1. Returns 'r'.
 */
extern int i2c_async_reserve_cookies(int bus, int count);

/* Synchronous API to read, write, and modify registers. The register address
 * width is limited to 8 bits, although you can read and write multiple bytes.
 *
 * The modify operation can do a clear-and-set on a register with 8-bit values.
 * It also returns the original value of the register before modification, if
 * val != NULL.
 */
extern int i2c_reg_write(int bus, uint8_t addr, uint8_t reg,
                         int count, const uint8_t* buf);
extern int i2c_reg_read(int bus, uint8_t addr, uint8_t reg,
                        int count, uint8_t* buf);
extern int i2c_reg_modify1(int bus, uint8_t addr, uint8_t reg,
                           uint8_t clr, uint8_t set, uint8_t* val);

/* Variant to write a single 8-bit value to a register */
static inline int i2c_reg_write1(int bus, uint8_t addr,
                                 uint8_t reg, uint8_t val)
{
    return i2c_reg_write(bus, addr, reg, 1, &val);
}

/* Variant to read an 8-bit value from a register; returns the value
 * directly, or returns -1 on any error. */
static inline int i2c_reg_read1(int bus, uint8_t addr, uint8_t reg)
{
    uint8_t v;
    int i = i2c_reg_read(bus, addr, reg, 1, &v);
    if(i == I2C_STATUS_OK)
        return v;
    else
        return -1;
}

/* Variant to set or clear one bit in an 8-bit register */
static inline int i2c_reg_setbit1(int bus, uint8_t addr, uint8_t reg,
                                  int bit, int value, uint8_t* val)
{
    uint8_t clr = 0, set = 0;
    if(value)
        set = 1 << bit;
    else
        clr = 1 << bit;

    return i2c_reg_modify1(bus, addr, reg, clr, set, val);
}

/* Internal API */

/* Must be called by the target's i2c_init() before anyone uses I2C. */
extern void __i2c_async_init(void);

/* Called by the target's interrupt handlers to signal completion of the
 * currently running descriptor. You must ensure IRQs are disabled before
 * calling this function.
 *
 * If another descriptor is queued for submission, either as part of the
 * same transaction or another one, then this may call __i2c_async_submit()
 * to start the next descriptor.
 */
extern void __i2c_async_complete_callback(int bus, int status);

/* Called by the i2c-async core to submit a descriptor to the hardware bus.
 * This function is implemented by the target. Just start the transfer and
 * unmask needed interrupts here, and try to return as quickly as possible.
 */
extern void __i2c_async_submit(int bus, i2c_descriptor* desc);

#endif /* HAVE_I2C_ASYNC */
#endif /* __I2C_ASYNC_H__ */
