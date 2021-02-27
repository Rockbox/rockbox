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

#include "i2c-async.h"
#include "config.h"
#include "system.h"
#include "kernel.h"

/* To decide on the queue size, typically you should use the formula:
 *
 *   queue size = max { D_1, ..., D_n } * max { T_1, ..., T_m }
 *
 * where
 *
 *     n = number of busses
 *     m = number of devices across all busses
 *   D_i = number of devices on bus i
 *   T_j = number of queued transactions needed for device j
 *
 * The idea is to ensure nobody waits for a queue slot, but keep the queue
 * size to a minimum so that queue management features don't take too long.
 * (They have to iterate over the entire queue with IRQs disabled, so...)
 *
 * If you don't use the asychronous features then waiting for a queue slot
 * is a moot point since you'll be blocking until completion anyway; in that
 * case feel free to use single-entry queues.
 *
 * Note that each bus gets the same sized queue. If this isn't good for
 * your target for whatever reason, it might be worth implementing per-bus
 * queue sizes here.
 */
#if !defined(I2C_ASYNC_BUS_COUNT) || !defined(I2C_ASYNC_QUEUE_SIZE)
# error "Need to add #defines for i2c-async frontend"
#endif

#if I2C_ASYNC_QUEUE_SIZE < 1
# error "i2c-async queue size must be >= 1"
#endif

/* Singly-linked list for queuing up transactions */
typedef struct i2c_async_txn {
    struct i2c_async_txn* next;
    i2c_descriptor* desc;
    int cookie;
} i2c_async_txn;

typedef struct i2c_async_bus {
    /* Head/tail of transaction queue. The head always points to the
     * currently running transaction, or NULL if the bus is idle.
     */
    i2c_async_txn* head;
    i2c_async_txn* tail;

    /* Head of a free list used to allocate nodes. */
    i2c_async_txn* free;

    /* Next unallocated cookie value */
    int nextcookie;

    /* Semaphore for reserving a node in the free list. Nodes can only be
     * allocated after a successful semaphore_wait(), and after being freed
     * semaphore_release() must be called.
     */
    struct semaphore sema;

#ifdef HAVE_CORELOCK_OBJECT
    /* Corelock is required for multi-core CPUs */
    struct corelock cl;
#endif
} i2c_async_bus;

static i2c_async_txn i2c_async_txns[I2C_ASYNC_BUS_COUNT][I2C_ASYNC_QUEUE_SIZE];
static i2c_async_bus i2c_async_busses[I2C_ASYNC_BUS_COUNT];

static i2c_async_txn* i2c_async_popfree(i2c_async_txn** head)
{
    i2c_async_txn* node = *head;
    *head = node->next;
    return node;
}

static void i2c_async_pushfree(i2c_async_txn** head, i2c_async_txn* node)
{
    node->next = *head;
    *head = node;
}

static i2c_async_txn* i2c_async_pool_init(i2c_async_txn* array, int count)
{
    /* Populate forward pointers */
    for(int i = 0; i < count - 1; ++i)
        array[i].next = &array[i+1];

    /* Last pointer is NULL */
    array[count - 1].next = NULL;

    /* Return head of pool */
    return &array[0];
}

static void i2c_async_bus_init(int busnr)
{
    const int q_size = I2C_ASYNC_QUEUE_SIZE;

    i2c_async_bus* bus = &i2c_async_busses[busnr];
    bus->head = bus->tail = NULL;
    bus->free = i2c_async_pool_init(i2c_async_txns[busnr], q_size);
    bus->nextcookie = 1;
    semaphore_init(&bus->sema, q_size, q_size);
    corelock_init(&bus->cl);
}

/* Add a node to the end of the transaction queue */
static void i2c_async_bus_enqueue(i2c_async_bus* bus, i2c_async_txn* node)
{
    node->next = NULL;
    if(bus->head == NULL)
        bus->head = bus->tail = node;
    else {
        bus->tail->next = node;
        bus->tail = node;
    }
}

/* Helper function called to run descriptor completion tasks */
static void i2c_async_bus_complete_desc(i2c_async_bus* bus, int status)
{
    i2c_descriptor* d = bus->head->desc;
    if(d->callback)
        d->callback(status, d);

    bus->head->desc = d->next;
}

void __i2c_async_complete_callback(int busnr, int status)
{
    i2c_async_bus* bus = &i2c_async_busses[busnr];
    corelock_lock(&bus->cl);

    i2c_async_bus_complete_desc(bus, status);
    if(status != I2C_STATUS_OK) {
        /* Skip remainder of transaction after an error */
        while(bus->head->desc)
            i2c_async_bus_complete_desc(bus, I2C_STATUS_SKIPPED);
    }

    /* Dequeue next transaction if we finished the current one */
    if(!bus->head->desc) {
        i2c_async_txn* node = bus->head;
        bus->head = node->next;
        i2c_async_pushfree(&bus->free, node);
        semaphore_release(&bus->sema);
    }

    if(bus->head) {
        /* Submit the next descriptor */
        __i2c_async_submit(busnr, bus->head->desc);
    } else {
        /* Fixup tail after last transaction */
        bus->tail = NULL;
    }

    corelock_unlock(&bus->cl);
}

void __i2c_async_init(void)
{
    for(int i = 0; i < I2C_ASYNC_BUS_COUNT; ++i)
        i2c_async_bus_init(i);
}

int i2c_async_queue(int busnr, int timeout, int q_mode,
                    int cookie, i2c_descriptor* desc)
{
    i2c_async_txn* node;
    i2c_async_bus* bus = &i2c_async_busses[busnr];
    int rc = I2C_RC_OK;

    int irq = disable_irq_save();
    corelock_lock(&bus->cl);

    /* Scan the queue unless q_mode is a simple ADD */
    if(q_mode != I2C_Q_ADD) {
        for(node = bus->head; node != NULL; node = node->next) {
            if(node->cookie == cookie) {
                if(q_mode == I2C_Q_REPLACE && node != bus->head)
                    node->desc = desc;
                else
                    rc = I2C_RC_NOTADDED;
                goto _exit;
            }
        }
    }

    /* Try to claim a queue node without blocking if we can */
    if(semaphore_wait(&bus->sema, TIMEOUT_NOBLOCK) != OBJ_WAIT_SUCCEEDED) {
        /* Bail out now if caller doesn't want to wait */
        if(timeout == TIMEOUT_NOBLOCK) {
            rc = I2C_RC_BUSY;
            goto _exit;
        }

        /* Wait on the semaphore */
        corelock_unlock(&bus->cl);
        restore_irq(irq);
        if(semaphore_wait(&bus->sema, timeout) == OBJ_WAIT_TIMEDOUT)
            return I2C_RC_BUSY;

        /* Got a node; re-lock */
        irq = disable_irq_save();
        corelock_lock(&bus->cl);
    }

    /* Alloc the node and push it to queue */
    node = i2c_async_popfree(&bus->free);
    node->desc = desc;
    node->cookie = cookie;
    i2c_async_bus_enqueue(bus, node);

    /* Start the first descriptor if the bus is idle */
    if(node == bus->head)
        __i2c_async_submit(busnr, desc);

  _exit:
    corelock_unlock(&bus->cl);
    restore_irq(irq);
    return rc;
}

int i2c_async_cancel(int busnr, int cookie)
{
    i2c_async_bus* bus = &i2c_async_busses[busnr];
    int rc = I2C_RC_NOTFOUND;

    int irq = disable_irq_save();
    corelock_lock(&bus->cl);

    /* Bail if queue is empty */
    if(!bus->head)
        goto _exit;

    /* Check the running transaction for a match */
    if(bus->head->cookie == cookie) {
        rc = I2C_RC_BUSY;
        goto _exit;
    }

    /* Walk the queue, starting after the head */
    i2c_async_txn* prev = bus->head;
    i2c_async_txn* node = prev->next;
    while(node) {
        if(node->cookie == cookie) {
            prev->next = node->next;
            rc = I2C_RC_OK;
            goto _exit;
        }

        prev = node;
        node = node->next;
    }

  _exit:
    corelock_unlock(&bus->cl);
    restore_irq(irq);
    return rc;
}

int i2c_async_reserve_cookies(int busnr, int count)
{
    i2c_async_bus* bus = &i2c_async_busses[busnr];
    int ret = bus->nextcookie;
    bus->nextcookie += count;
    return ret;
}

static void i2c_sync_callback(int status, i2c_descriptor* d)
{
    struct semaphore* sem = (struct semaphore*)d->arg;
    d->arg = status;
    semaphore_release(sem);
}

static void i2c_reg_modify1_callback(int status, i2c_descriptor* d)
{
    if(status == I2C_STATUS_OK) {
        uint8_t* buf = (uint8_t*)d->buffer[1];
        uint8_t val = *buf;
        *buf = (val & ~(d->arg >> 8)) | (d->arg & 0xff);
        d->arg = val;
    }
}

int i2c_reg_write(int bus, uint8_t addr, uint8_t reg,
                  int count, const uint8_t* buf)
{
    struct semaphore sem;
    semaphore_init(&sem, 1, 0);

    i2c_descriptor desc;
    desc.slave_addr = addr;
    desc.bus_cond = I2C_START | I2C_STOP;
    desc.tran_mode = I2C_WRITE;
    desc.buffer[0] = &reg;
    desc.count[0] = 1;
    desc.buffer[1] = (uint8_t*)buf;
    desc.count[1] = count;
    desc.callback = &i2c_sync_callback;
    desc.arg = (intptr_t)&sem;
    desc.next = NULL;

    i2c_async_queue(bus, TIMEOUT_BLOCK, I2C_Q_ADD, 0, &desc);
    semaphore_wait(&sem, TIMEOUT_BLOCK);

    return desc.arg;
}

int i2c_reg_read(int bus, uint8_t addr, uint8_t reg,
                 int count, uint8_t* buf)
{
    struct semaphore sem;
    semaphore_init(&sem, 1, 0);

    i2c_descriptor desc;
    desc.slave_addr = addr;
    desc.bus_cond = I2C_START | I2C_STOP;
    desc.tran_mode = I2C_READ;
    desc.buffer[0] = &reg;
    desc.count[0] = 1;
    desc.buffer[1] = buf;
    desc.count[1] = count;
    desc.callback = &i2c_sync_callback;
    desc.arg = (intptr_t)&sem;
    desc.next = NULL;

    i2c_async_queue(bus, TIMEOUT_BLOCK, I2C_Q_ADD, 0, &desc);
    semaphore_wait(&sem, TIMEOUT_BLOCK);

    return desc.arg;
}

int i2c_reg_modify1(int bus, uint8_t addr, uint8_t reg,
                    uint8_t clr, uint8_t set, uint8_t* val)
{
    struct semaphore sem;
    semaphore_init(&sem, 1, 0);

    uint8_t buf[2];
    buf[0] = reg;

    i2c_descriptor desc[2];
    desc[0].slave_addr = addr;
    desc[0].bus_cond = I2C_START | I2C_STOP;
    desc[0].tran_mode = I2C_READ;
    desc[0].buffer[0] = &buf[0];
    desc[0].count[0] = 1;
    desc[0].buffer[1] = &buf[1];
    desc[0].count[1] = 1;
    desc[0].callback = &i2c_reg_modify1_callback;
    desc[0].arg = (clr << 8) | set;
    desc[0].next = &desc[1];

    desc[1].slave_addr = addr;
    desc[1].bus_cond = I2C_START | I2C_STOP;
    desc[1].tran_mode = I2C_WRITE;
    desc[1].buffer[0] = &buf[0];
    desc[1].count[0] = 2;
    desc[1].buffer[1] = NULL;
    desc[1].count[1] = 0;
    desc[1].callback = &i2c_sync_callback;
    desc[1].arg = (intptr_t)&sem;
    desc[1].next = NULL;

    i2c_async_queue(bus, TIMEOUT_BLOCK, I2C_Q_ADD, 0, &desc[0]);
    semaphore_wait(&sem, TIMEOUT_BLOCK);

    if(val && desc[1].arg == I2C_STATUS_OK)
        *val = desc[0].arg;

    return desc[1].arg;
}
