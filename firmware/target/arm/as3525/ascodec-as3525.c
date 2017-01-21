/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Bertrik Sikken
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

/*
    Provides access to the codec/charger/rtc/adc part of the as3525.
    This part is on address 0x46 of the internal i2c bus in the as3525.
    Registers in the codec part seem to be nearly identical to the registers
    in the AS3514 (used in the "v1" versions of the sansa c200 and e200).

    I2C register description:
    * I2C2_CNTRL needs to be set to 0x51 for transfers to work at all.
      bit 0: ? possibly related to using ACKs during transfers
      bit 1: direction of transfer (0 = write, 1 = read)
      bit 2: use 2-byte slave address
    * I2C2_IMR, I2C2_RIS, I2C2_MIS, I2C2_INT_CLR interrupt bits:
      bit 2: byte read interrupt
      bit 3: byte write interrupt
      bit 4: ? possibly some kind of error status
      bit 7: ACK error
    * I2C2_SR (status register) indicates in bit 0 if a transfer is busy.
    * I2C2_SLAD0 contains the i2c slave address to read from / write to.
    * I2C2_CPSR0/1 is the divider from the peripheral clock to the i2c clock.
    * I2C2_DACNT sets the number of bytes to transfer and actually starts it.

    When a transfer is attempted to a non-existing i2c slave address,
    interrupt bit 7 is raised and DACNT is not decremented after the transfer.
 */

#include "ascodec.h"
#include "clock-target.h"
#include "kernel.h"
#include "system.h"
#include "as3525.h"
#include "i2c.h"
#include <linked_list.h>

#define I2C2_DATA       *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x00))
#define I2C2_SLAD0      *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x04))
#define I2C2_CNTRL      *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x0C))
#define I2C2_DACNT      *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x10))
#define I2C2_CPSR0      *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x1C))
#define I2C2_CPSR1      *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x20))
#define I2C2_IMR        *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x24))
#define I2C2_RIS        *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x28))
#define I2C2_MIS        *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x2C))
#define I2C2_SR         *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x30))
#define I2C2_INT_CLR    *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x40))
#define I2C2_SADDR      *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x44))

#define I2C2_CNTRL_MASTER     0x01
#define I2C2_CNTRL_READ       0x02
#define I2C2_CNTRL_WRITE      0x00
#define I2C2_CNTRL_RESET      0x10
#define I2C2_CNTRL_REPSTARTEN 0x40

#define I2C2_CNTRL_DEFAULT (I2C2_CNTRL_MASTER|I2C2_CNTRL_REPSTARTEN|I2C2_CNTRL_RESET)

#define I2C2_IRQ_TXEMPTY      0x04
#define I2C2_IRQ_RXFULL       0x08
#define I2C2_IRQ_RXOVER       0x10
#define I2C2_IRQ_ACKTIMEO     0x80

#define REQ_UNFINISHED 0
#define REQ_FINISHED   1
#define REQ_RETRY      2

#ifdef DEBUG
#define IFDEBUG(x) x
#else
#define IFDEBUG(x)
#endif

#define ASCODEC_REQ_READ   0
#define ASCODEC_REQ_WRITE  1

/*
 * How many bytes we using in struct ascodec_request for the data buffer.
 * 4 fits the alignment best right now.
 * We don't actually use more than 3 at the moment (when reading interrupts)
 * Upper limit would be 255 since DACNT is 8 bits!
 */
#define ASCODEC_REQ_MAXLEN 4

struct ascodec_request;
typedef void (ascodec_cb_fn)(struct ascodec_request *req);

struct ascodec_request {
    /* standard request members */
    struct ll_node node;        /* request list link (first!) */
    unsigned char type;         /* reqest type (read or write) */
    unsigned char index;        /* initial i2c sub address */
    unsigned char cnt;          /* bytes remaining */
    unsigned char data[ASCODEC_REQ_MAXLEN]; /* actual I/O data */

    /* members relevant when a callback is specified (callback != NULL) */
    ascodec_cb_fn *callback;    /* pointer to callback function */
    intptr_t      cbdata;       /* data for callback function */
    int           len_done;     /* amount actually transferred */
};

/* I2C driver data */
static struct mutex as_mtx;
static struct ll_head req_list;
static unsigned char *req_data_ptr;
#define REQ_FIRST ((struct ascodec_request *)req_list.head)

/* INT_AUDIO interrupt data */
static void ascodec_int_audio_cb(struct ascodec_request *req);
void INT_I2C_AUDIO(void);
static struct ascodec_request as_audio_req;
static struct semaphore adc_done_sem;
static unsigned long ascodec_enrd0_shadow = 0;

static void ascodec_wait_cb(struct ascodec_request *req);

/** --debugging help-- **/

#ifdef DEBUG
/* counters for debugging INT_AUDIO */
static struct int_audio_counters {
    int int_audio;
    int int_chg_finished;
    int int_chg_insert;
    int int_chg_remove;
    int int_usb_insert;
    int int_usb_remove;
    int int_rtc;
    int int_adc;
} int_audio_counters;
#endif /* DEBUG */

#define COUNT_INT(x) IFDEBUG((int_audio_counters.int_##x)++)


/** --stock request and callback functionality -- **/

/* init for common request data (call before submitting) */
static inline void ascodec_req_init(struct ascodec_request *req, int type,
                                    unsigned int index, unsigned int cnt)
{
    req->type  = type;
    req->index = index;
    req->cnt   = cnt;
}

/* stock no-wait init for request (use any callback and data) */
static inline void ascodec_async_init(struct ascodec_request *req,
                                      ascodec_cb_fn *callback, intptr_t cbdata)
{
    /* cbdata is unused if no callback is used */
    if ((req->callback = callback))
        req->cbdata = cbdata;
}

/* initialize the stock completion callback */
static inline void ascodec_wait_init(struct ascodec_request *req,
                                     struct semaphore *completep)
{
    req->callback = ascodec_wait_cb;
    req->cbdata   = (intptr_t)completep;
    semaphore_init(completep, 1, 0);
}

/* caller waits here when using ascodec_wait_cb to do synchronous transfers */
static void ascodec_wait(struct ascodec_request *req)
{
    struct semaphore *completep = (struct semaphore *)req->cbdata;
    int timeout = TIMEOUT_BLOCK;

    if (!irq_enabled() || !is_thread_context()) {
        timeout = TIMEOUT_NOBLOCK; /* poll semaphore, no block */
    }

    while (semaphore_wait(completep, timeout) == OBJ_WAIT_TIMEDOUT) {
        /* pump the i2c interrupts ourselves (only waiting can do this!) */
        if (I2C2_MIS) {
            INT_I2C_AUDIO();
        }
    }
}

/* stock callback used in order to wait for a transfer to complete */
static void ascodec_wait_cb(struct ascodec_request *req)
{
    struct semaphore *completep = (struct semaphore *)req->cbdata;
    semaphore_release(completep);
}


/**-- I2C2 interrupt handling --**/

/* start the controller on the next transfer */
static void ascodec_start_req(struct ascodec_request *req)
{
    int unmask = 0;

    /* enable clock */
    bitset32(&CGU_PERI, CGU_I2C_AUDIO_MASTER_CLOCK_ENABLE);

    /* start transfer */
    I2C2_SADDR = req->index;

    if (req->type == ASCODEC_REQ_READ) {
        req_data_ptr = req->data;
        I2C2_CNTRL = I2C2_CNTRL_DEFAULT | I2C2_CNTRL_READ;
        unmask = I2C2_IRQ_RXFULL|I2C2_IRQ_RXOVER;
    } else {
        req_data_ptr = &req->data[1];
        I2C2_CNTRL = I2C2_CNTRL_DEFAULT | I2C2_CNTRL_WRITE;
        I2C2_DATA = req->data[0];
        unmask = I2C2_IRQ_TXEMPTY|I2C2_IRQ_ACKTIMEO;
    }

    I2C2_DACNT = req->cnt;
    I2C2_IMR |= unmask; /* enable interrupts */
}

/* send the next bytes or read bytes received */
static int ascodec_continue_req(struct ascodec_request *req, int irq_status)
{
    if ((irq_status & (I2C2_IRQ_RXOVER|I2C2_IRQ_ACKTIMEO)) > 0) {
        /* some error occured, restart the request */
        return REQ_RETRY;
    }

    if (req->type == ASCODEC_REQ_READ &&
        (irq_status & I2C2_IRQ_RXFULL) > 0) {
        *req_data_ptr++ = I2C2_DATA;
    } else {
        if (req->cnt > 1 &&
            (irq_status & I2C2_IRQ_TXEMPTY) > 0) {
            I2C2_DATA = *req_data_ptr++;
        }
    }

    req->index++;
    if (--req->cnt > 0)
        return REQ_UNFINISHED;

    return REQ_FINISHED;
}

/* complete the request and call the completion callback, if any */
static void ascodec_finish_req(struct ascodec_request *req)
{
    /*
     * Wait if still busy, unfortunately this happens since
     * the controller is running at a low divisor, so it's
     * still busy when we serviced the interrupt.
     * I tried upping the i2c speed to 4MHz which
     * made the number of busywait cycles much smaller
     * (none for reads and only a few for writes),
     * but who knows if it's reliable at that frequency. ;)
     * For one thing, 8MHz doesn't work, so 4MHz is likely
     * borderline.
     * In general writes need much more wait cycles than reads
     * for some reason, possibly because we read the data register
     * for reads, which will likely block the processor while
     * the i2c controller responds to the register read.
     */
    while (I2C2_SR & 1);

    if (req->callback) {
        req->len_done = req_data_ptr - req->data;
        req->callback(req);
    }
}

/* ISR for I2C2 */
void INT_I2C_AUDIO(void)
{
    struct ascodec_request *req = REQ_FIRST;

    int irq_status = I2C2_MIS;
    int status = ascodec_continue_req(req, irq_status);

    I2C2_INT_CLR = irq_status; /* clear interrupt status */

    if (status != REQ_UNFINISHED) {
        /* mask rx/tx interrupts */
        I2C2_IMR &= ~(I2C2_IRQ_TXEMPTY|I2C2_IRQ_RXFULL|
                      I2C2_IRQ_RXOVER|I2C2_IRQ_ACKTIMEO);

        if (status == REQ_FINISHED)
            ascodec_finish_req(req);

        int oldlevel = disable_irq_save(); /* IRQs are stacked */

        /*
         * If status == REQ_RETRY, this will restart the request from where
         * it left off because we didn't remove it from the request list
         */

        if (status == REQ_FINISHED) {
            ll_remove_next(&req_list, NULL);
        }

        req = REQ_FIRST;
        if (req == NULL) {
            /* disable clock */
            bitclr32(&CGU_PERI, CGU_I2C_AUDIO_MASTER_CLOCK_ENABLE);
        } else {
            ascodec_start_req(req);
        }

        restore_irq(oldlevel);
    }
}


/** --Routines for reading and writing data on the bus-- **/

/* add the request to the queue */
static void ascodec_submit(struct ascodec_request *req)
{
    int oldlevel = disable_irq_save();

    ll_insert_last(&req_list, &req->node);

    if (REQ_FIRST == req) {
        /* first on list? start driver */
        ascodec_start_req(req);
    }

    restore_irq(oldlevel);
}

/*
 * The request struct passed in must be allocated statically.
 * If you call ascodec_async_write from different places, each
 * call needs it's own request struct.
 */
static void ascodec_async_write(struct ascodec_request *req,
                                unsigned int index, unsigned int value)
{
#ifndef HAVE_AS3543
    if (index == AS3514_CVDD_DCDC3) /* prevent setting of the LREG_CP_not bit */
        value &= ~(1 << 5);
#endif

    ascodec_req_init(req, ASCODEC_REQ_WRITE, index, 1);
    req->data[0] = value;
    ascodec_submit(req);
}

void ascodec_write(unsigned int index, unsigned int value)
{
    struct ascodec_request req;
    struct semaphore complete;

    ascodec_wait_init(&req, &complete);
    ascodec_async_write(&req, index, value);
    ascodec_wait(&req);
}

/*
 * The request struct passed in must be allocated statically.
 * If you call ascodec_async_read from different places, each
 * call needs it's own request struct.
 * If len is bigger than ASCODEC_REQ_MAXLEN it will be
 * set to ASCODEC_REQ_MAXLEN.
 */
static void ascodec_async_read(struct ascodec_request *req,
                               unsigned int index, unsigned int len)
{
    if (len > ASCODEC_REQ_MAXLEN)
        len = ASCODEC_REQ_MAXLEN; /* can't fit more in one request */

    ascodec_req_init(req, ASCODEC_REQ_READ, index, len);
    ascodec_submit(req);
}

/* read data synchronously  */
int ascodec_read(unsigned int index)
{
    struct ascodec_request req;
    struct semaphore complete;

    ascodec_wait_init(&req, &complete);
    ascodec_async_read(&req, index, 1);
    ascodec_wait(&req);

    return req.data[0];
}

/* read an array of bytes */
void ascodec_readbytes(unsigned int index, unsigned int len, unsigned char *data)
{
    struct ascodec_request req;
    struct semaphore complete;

    ascodec_wait_init(&req, &complete);

    /* index and cnt will be filled in later, just use 0 */
    ascodec_req_init(&req, ASCODEC_REQ_READ, 0, 0);

    int i = 0;

    while (len > 0) {
        int cnt = MIN(len, ASCODEC_REQ_MAXLEN);

        req.index = index;
        req.cnt = cnt;

        ascodec_submit(&req);
        ascodec_wait(&req);

        for (int j = 0; j < cnt; j++)
            data[i++] = req.data[j];

        len   -= cnt;
        index += cnt;
    }
}

#if CONFIG_CPU == AS3525v2
/* write special PMU subregisters */
void ascodec_write_pmu(unsigned int index, unsigned int subreg,
                       unsigned int value)
{
    struct ascodec_request reqs[2];
    struct semaphore complete;

    ascodec_async_init(&reqs[0], NULL, 0);
    ascodec_wait_init(&reqs[1], &complete);

    int oldstatus = disable_irq_save();
    /* we submit consecutive requests to make sure no operations happen on the
     * i2c bus between selecting the sub register and writing to it */
    ascodec_async_write(&reqs[0], AS3543_PMU_ENABLE, 8 | subreg);
    ascodec_async_write(&reqs[1], index, value);
    restore_irq(oldstatus);

    /* Wait for second request to finish */
    ascodec_wait(&reqs[1]);
}

/* read special PMU subregisters */
int ascodec_read_pmu(unsigned int index, unsigned int subreg)
{
    struct ascodec_request reqs[2];
    struct semaphore complete;

    ascodec_async_init(&reqs[0], NULL, 0);
    ascodec_wait_init(&reqs[1], &complete);

    int oldstatus = disable_irq_save();
    /* we submit consecutive requests to make sure no operations happen on the
     * i2c bus between selecting the sub register and reading it */
    ascodec_async_write(&reqs[0], AS3543_PMU_ENABLE, subreg);
    ascodec_async_read(&reqs[1], index, 1);
    restore_irq(oldstatus);

    /* Wait for second request to finish */
    ascodec_wait(&reqs[1]);

    return reqs[1].data[0];
}
#endif /* CONFIG_CPU == AS3525v2 */

/* callback that receives results of reading INT_AUDIO status register */
static void ascodec_int_audio_cb(struct ascodec_request *req)
{
    unsigned char * const data = req->data;

    if (UNLIKELY(req->len_done != 3))  { /* some error happened? */
        panicf("INT_AUDIO callback got %d regs", req->len_done);
    }

    if (data[0] & CHG_ENDOFCH) { /* chg finished */
        COUNT_INT(chg_finished);
        ascodec_enrd0_shadow |= CHG_ENDOFCH;
    }

    if (data[0] & CHG_CHANGED) { /* chg status changed */
        if (data[0] & CHG_STATUS) {
            COUNT_INT(chg_insert);
            ascodec_enrd0_shadow |= CHG_STATUS;
        } else {
            COUNT_INT(chg_remove);
            ascodec_enrd0_shadow &= ~CHG_STATUS;
        }
    }

    if (data[0] & USB_CHANGED) { /* usb status changed */
        if (data[0] & USB_STATUS) {
            COUNT_INT(usb_insert);
            usb_insert_int();
        } else {
            COUNT_INT(usb_remove);
            usb_remove_int();
        }
    }

    if (data[2] & IRQ_RTC) { /* rtc irq */
        /*
         * Can be configured for once per second or once per minute,
         * default is once per second
         */
        COUNT_INT(rtc);
    }

    if (data[2] & IRQ_ADC) { /* adc finished */
        COUNT_INT(adc);
        semaphore_release(&adc_done_sem);
    }

    VIC_INT_ENABLE = INTERRUPT_AUDIO;
}

/* ISR for all various ascodec events */
void INT_AUDIO(void)
{
    VIC_INT_EN_CLEAR = INTERRUPT_AUDIO;
    COUNT_INT(audio);

    ascodec_async_read(&as_audio_req, AS3514_IRQ_ENRD0, 3);
}

/* wait for ADC to finish conversion */
void ascodec_wait_adc_finished(void)
{
    semaphore_wait(&adc_done_sem, TIMEOUT_BLOCK);
}

#if CONFIG_CHARGING
/* read sticky end-of-charge bit and clear it */
bool ascodec_endofch(void)
{
    int oldlevel = disable_irq_save();

    bool ret = ascodec_enrd0_shadow & CHG_ENDOFCH;
    ascodec_enrd0_shadow &= ~CHG_ENDOFCH; /* clear interrupt */

    restore_irq(oldlevel);

    return ret;
}

/* read the presence state of the charger */
bool ascodec_chg_status(void)
{
    return ascodec_enrd0_shadow & CHG_STATUS;
}

void ascodec_monitor_endofch(void)
{
    /* end of charge status interrupt already enabled */
}

/* write charger control register */
void ascodec_write_charger(int value)
{
#if CONFIG_CPU == AS3525
    ascodec_write(AS3514_CHARGER, value);
#else
    ascodec_write_pmu(AS3543_CHARGER, 1, value);
#endif
}

/* read charger control register */
int ascodec_read_charger(void)
{
#if CONFIG_CPU == AS3525
    return ascodec_read(AS3514_CHARGER);
#else
    return ascodec_read_pmu(AS3543_CHARGER, 1);
#endif
}
#endif /* CONFIG_CHARGING */

/*
 * NOTE:
 * After the conversion to interrupts, ascodec_(lock|unlock) are only used by
 * adc-as3514.c to protect against other threads corrupting the result by using
 * the ADC at the same time.
 * Concurrent ascodec_(async_)?(read|write) calls are instead protected
 * because ascodec_submit() is atomic and concurrent requests will wait
 * in the queue until the current request is finished.
 */
void ascodec_lock(void)
{
    mutex_lock(&as_mtx);
}

void ascodec_unlock(void)
{
    mutex_unlock(&as_mtx);
}


/** --Startup initialization-- **/

void i2c_init(void)
{
    /* required function but called too late for our needs */
}

/* initialises the internal i2c bus and prepares for transfers to the codec */
void ascodec_init(void)
{
    int prescaler;

    ll_init(&req_list);
    mutex_init(&as_mtx);
    ascodec_async_init(&as_audio_req, ascodec_int_audio_cb, 0);
    semaphore_init(&adc_done_sem, 1, 0);

    /* enable clock */
    bitset32(&CGU_PERI, CGU_I2C_AUDIO_MASTER_CLOCK_ENABLE);

    /* prescaler for i2c clock */
    prescaler = AS3525_I2C_PRESCALER;
    I2C2_CPSR0 = prescaler & 0xFF;          /* 8 lsb */
    I2C2_CPSR1 = (prescaler >> 8) & 0x3;    /* 2 msb */

    /* set i2c slave address of codec part */
    I2C2_SLAD0 = AS3514_I2C_ADDR << 1;

    I2C2_CNTRL = I2C2_CNTRL_DEFAULT;

    I2C2_IMR = 0x00;         /* disable interrupts */
    I2C2_INT_CLR = I2C2_RIS; /* clear interrupt status */
    VIC_INT_ENABLE = INTERRUPT_I2C_AUDIO;
    VIC_INT_ENABLE = INTERRUPT_AUDIO;

    /* detect if USB was connected at startup since there is no transition */
    ascodec_enrd0_shadow = ascodec_read(AS3514_IRQ_ENRD0);
    if(ascodec_enrd0_shadow & USB_STATUS)
        usb_insert_int();

    /* Generate irq for usb+charge status change */
    ascodec_write(AS3514_IRQ_ENRD0,
#if CONFIG_CHARGING /* m200v4 can't charge */
        IRQ_CHGSTAT | IRQ_ENDOFCH |
#endif
        IRQ_USBSTAT);

#if CONFIG_CPU == AS3525v2
    /* XIRQ = IRQ, active low reset signal, 6mA push-pull output */
    ascodec_write_pmu(0x1a, 3, (1<<2)|3); /* 1A-3 = Out_Cntr3 register */
    /* Generate irq on (rtc,) adc change */
    ascodec_write(AS3514_IRQ_ENRD2, /*IRQ_RTC |*/ IRQ_ADC);
#else
    /* Generate irq for push-pull, active high, irq on rtc+adc change */
    ascodec_write(AS3514_IRQ_ENRD2, IRQ_PUSHPULL | IRQ_HIGHACTIVE |
                                    /*IRQ_RTC |*/ IRQ_ADC);
#endif
}
