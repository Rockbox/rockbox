/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2021 by Tomasz Moń
 * Copied with minor modifications from Sansa Connect Linux driver
 * Copyright (c) 2005,2006 Zermatt Systems, Inc.
 * Written by: Ben Bostwick
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

#include <string.h>
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "panic.h"
#include "tnetv105_usb_drv.h"
#include "tnetv105_cppi.h"
#if USB_CPPI_LOGGING
#define LOGF_ENABLE
#endif
#include "logf.h"

/* This file is pretty much directly copied from the Sansa Connect
 * Linux kernel source code. This is because the functionality is
 * nicely separated from actual kernel specific code and CPPI seems
 * complex (atleast without access to the datasheet).
 *
 * The only non cosmetic change was changing the dynamic allocations
 * to static allocations.
 *
 * It seems that the only way to get interrupt on non-control endpoint
 * acticity is to use the CPPI. This sounds like a plausible explanation
 * for the fake DMA buffers mentioned in CPPI code.
 */

/* Translate Linux consistent_sync() to Rockbox functions */
#define DMA_TO_DEVICE commit_discard_dcache_range
#define DMA_FROM_DEVICE discard_dcache_range
#define consistent_sync(ptr, size, func) func(ptr, size)
/* Rockbox TMS320DM320 crt0.S maps everything to itself */
#define __virt_to_phys(ptr) ptr
#define __phys_to_virt(ptr) ptr

// CPPI functions
#define CB_SOF_BIT         (1<<31)
#define CB_EOF_BIT         (1<<30)
#define CB_OWNERSHIP_BIT   (1<<29)
#define CB_EOQ_BIT         (1<<28)
#define CB_ZLP_GARBAGE     (1<<23)
#define CB_SIZE_MASK       0x0000ffff
#define CB_OFFSET_MASK     0xffff0000
#define TEARDOWN_VAL       0xfffffffc

#define MAX_BUF_SIZE   512

#define CPPI_DMA_RX_BUF_SIZE  (MAX_BUF_SIZE * CPPI_RX_NUM_BUFS)

static uint8_t *dma_recv_buf[CPPI_NUM_CHANNELS];
static uint8_t ch0_rx_buf[CPPI_DMA_RX_BUF_SIZE];
static uint8_t ch1_rx_buf[CPPI_DMA_RX_BUF_SIZE];

#if USB_CPPI_LOGGING
#define cppi_log_event0(msg) logf(msg)
#define cppi_log_event1(msg, data0) logf(msg " %lx", (uint32_t)data0)
#define cppi_log_event2(msg, data0, data1) logf(msg " %lx %lx", (uint32_t)data0, (uint32_t)data1)
#define cppi_log_event3(msg, data0, data1, data2) logf(msg " %lx %lx %lx", (uint32_t)data0, (uint32_t)data1, (uint32_t)data2)
#define cppi_log_event4(msg, data0, data1, data2, data3) logf(msg " %lx %lx %lx %lx", (uint32_t)data0, (uint32_t)data1, (uint32_t)data2, (uint32_t)data3)
#else
#define cppi_log_event0(x)
#define cppi_log_event1(x, y)
#define cppi_log_event2(x, y, z)
#define cppi_log_event3(x, y, z, w)
#define cppi_log_event4(x, y, z, w, u)
#endif

/*
 *  This function processes transmit interrupts.  It traverses the
 *  transmit buffer queue, detecting sent data buffers
 *
 *  @return  0 if OK, non-zero otherwise.
 */
int tnetv_cppi_tx_int(struct cppi_info *cppi, int ch)
{
    cppi_tcb *CurrentTcb,*LastTcbProcessed;
    uint32_t TxFrameStatus;
    cppi_txcntl *pTxCtl = &cppi->tx_ctl[ch];
    int bytes_sent = 0;

    cppi_log_event1("[cppi]TxInt ch", ch);

    CurrentTcb = pTxCtl->TxActQueueHead;

    if (CurrentTcb == 0)
    {
        cppi_log_event0("[cppi] tx int: no current tcb");
        return -1;
    }

    // sync up the tcb from memory
    consistent_sync(CurrentTcb, sizeof(*CurrentTcb), DMA_FROM_DEVICE);

    TxFrameStatus = CurrentTcb->mode;
    LastTcbProcessed = NULL;

    cppi_log_event3("[cppi] int tcb status", (uint32_t) CurrentTcb, TxFrameStatus, CurrentTcb->Off_BLen);

    while(CurrentTcb && (TxFrameStatus & CB_OWNERSHIP_BIT) == 0)
    {
        cppi_log_event3("[cppi] tx int: tcb (mode) (len)", (uint32_t) CurrentTcb, CurrentTcb->mode, CurrentTcb->Off_BLen);

        // calculate the amount of bytes sent.
        // don't count the fake ZLP byte
        if (CurrentTcb->Off_BLen > 0x1)
        {
            bytes_sent += CurrentTcb->Off_BLen & 0xFFFF;
        }

        if (CurrentTcb->mode & CB_EOQ_BIT)
        {
            if (CurrentTcb->Next)
            {
                cppi_log_event0(" [cppi] misqueue!");

                // Misqueued packet
                tnetv_usb_reg_write(TNETV_DMA_TX_STATE(ch, TNETV_CPPI_TX_WORD_HDP), CurrentTcb->HNext);
            }
            else
            {
                cppi_log_event0("[cppi] eoq");

                /* Tx End of Queue */
                pTxCtl->TxActive = 0;
            }
        }

        cppi_log_event1("[cppi]SendComplete: ", CurrentTcb->Off_BLen & 0xFFFF);

        // Write the completion pointer
        tnetv_usb_reg_write(TNETV_DMA_TX_CMPL(ch), __dma_to_vlynq_phys(CurrentTcb->dma_handle));


        LastTcbProcessed = CurrentTcb;
        CurrentTcb = CurrentTcb->Next;

        // clean up TCB fields
        LastTcbProcessed->HNext = 0;
        LastTcbProcessed->Next = 0;
        LastTcbProcessed->BufPtr = 0;
        LastTcbProcessed->Off_BLen = 0;
        LastTcbProcessed->mode = 0;
        LastTcbProcessed->Eop = 0;

        /* Push Tcb(s) back onto the list */
        if (pTxCtl->TcbPool)
        {
            LastTcbProcessed->Next = pTxCtl->TcbPool->Next;
            pTxCtl->TcbPool->Next = LastTcbProcessed;
        }
        else
        {
            pTxCtl->TcbPool = LastTcbProcessed;
        }

        consistent_sync(LastTcbProcessed, sizeof(*LastTcbProcessed), DMA_TO_DEVICE);

        // get the status of the next packet
        if (CurrentTcb)
        {
            // sync up the tcb from memory
            consistent_sync(CurrentTcb, sizeof(*CurrentTcb), DMA_FROM_DEVICE);

            TxFrameStatus = CurrentTcb->mode;
        }


    }

    pTxCtl->TxActQueueHead = CurrentTcb;

    if (!LastTcbProcessed)
    {
        cppi_log_event1("  [cppi]No Tx packets serviced on int! ch", ch);
        return -1;
    }

    return bytes_sent;
}

int tnetv_cppi_flush_tx_queue(struct cppi_info *cppi, int ch)
{
    cppi_txcntl *pTxCtl = &cppi->tx_ctl[ch];
    cppi_tcb *tcb, *next_tcb;

    tcb = pTxCtl->TxActQueueHead;

    cppi_log_event1("[cppi] flush TX ", (uint32_t) pTxCtl->TxActQueueHead);

    while (tcb)
    {
        tcb->mode = 0;
        tcb->BufPtr = 0;
        tcb->Off_BLen = 0;
        tcb->Eop = 0;
        tcb->HNext = 0;

        next_tcb = tcb->Next;

        tcb->Next = pTxCtl->TcbPool;
        pTxCtl->TcbPool = tcb;

        tcb = next_tcb;
    }

    pTxCtl->TxActQueueHead = 0;
    pTxCtl->TxActQueueTail = 0;
    pTxCtl->TxActive = 0;

    return 0;
}


/**
 *  @ingroup CPHAL_Functions
 *  This function transmits the data in FragList using available transmit
 *  buffer descriptors.  More information on the use of the Mode parameter
 *  is available in the module-specific appendices.  Note:  The OS should
 *  not call Send() for a channel that has been requested to be torndown.
 *
 */
int tnetv_cppi_send(struct cppi_info *cppi, int ch, dma_addr_t buf, unsigned length, int send_zlp)
{
    cppi_txcntl *pTxCtl;
    cppi_tcb *first_tcb;
    cppi_tcb *tcb;
    int queued_len;
    dma_addr_t buf_to_send;
    dma_addr_t buf_ptr;
    int total_len = length;
    int pktlen;

    pTxCtl = &cppi->tx_ctl[ch];

    if (length == 0)
    {
        cppi_log_event0("[cppi] len = 0, nothing to send");
        return -1;
    }

    // no send buffers.. try again later
    if (!pTxCtl->TcbPool)
    {
        cppi_log_event0("[cppi] out of cppi buffers");
        return -1;
    }

    // only send 1 packet at a time
    if (pTxCtl->TxActQueueHead || pTxCtl->TxActive)
    {
        cppi_log_event0("[cppi] already sending!");
        return -1;
    }

    buf_to_send = buf;

    // usb_requests can have a 32 bit length, but CPPI DMA fragments
    // have a (64k - 1) limit.  Split the usb_request up into fragments here.
    first_tcb = pTxCtl->TcbPool;
    tcb = first_tcb;

    cppi_log_event4("[cppi]cppi_send (buf) (len) (pool) (dma)", (uint32_t) buf_to_send, total_len, (uint32_t) first_tcb, first_tcb->dma_handle);

    queued_len = 0;

    do
    {
        buf_ptr = buf_to_send + queued_len;
        tcb->BufPtr = __dma_to_vlynq_phys(buf_ptr);
        tcb->HNext = 0;

        // can't transfer more that 64k-1 bytes in 1 CPPI transfer
        // need to queue up transfers if it's greater than that
        pktlen = ((total_len - queued_len) > CPPI_MAX_FRAG) ? CPPI_MAX_FRAG : (total_len - queued_len);
        tcb->Off_BLen = pktlen;
        tcb->mode = (CB_OWNERSHIP_BIT | CB_SOF_BIT | CB_EOF_BIT | pktlen);

        queued_len += pktlen;

        if (queued_len < total_len)
        {
            tcb->HNext = __dma_to_vlynq_phys(((cppi_tcb *) tcb->Next)->dma_handle);

            // write out the buffer to memory
            consistent_sync(tcb, sizeof(*tcb), DMA_TO_DEVICE);

            cppi_log_event4("[cppi] q tcb", (uint32_t) tcb, ((uint32_t *) tcb)[0], ((uint32_t *) tcb)[1], ((uint32_t *) tcb)[2]);
            cppi_log_event4("[cppi] ", ((uint32_t *) tcb)[3], ((uint32_t *) tcb)[4], ((uint32_t *) tcb)[5], ((uint32_t *) tcb)[6]);

            tcb = tcb->Next;
        }
    } while (queued_len < total_len);

    /* In the Tx Interrupt handler, we will need to know which TCB is EOP,
       so we can save that information in the SOP */
    first_tcb->Eop = tcb;

    // set the secret ZLP bit if necessary, this will be a completely separate packet
    if (send_zlp)
    {
#if defined(AUTO_ZLP) && AUTO_ZLP
        // add an extra buffer at the end to hold the ZLP
        tcb->HNext = __dma_to_vlynq_phys(((cppi_tcb *) tcb->Next)->dma_handle);

        // write out the buffer to memory
        consistent_sync(tcb, sizeof(*tcb), DMA_TO_DEVICE);

        tcb = tcb->Next;

        /* In the Tx Interrupt handler, we will need to know which TCB is EOP,
           so we can save that information in the SOP */
        first_tcb->Eop = tcb;
#endif

        buf_ptr = buf_to_send + queued_len;
        tcb->BufPtr = __dma_to_vlynq_phys(buf_ptr);  // not used, but can't be zero
        tcb->HNext = 0;
        tcb->Off_BLen = 0x1;  // device will send (((len - 1) / maxpacket) + 1) ZLPs
        tcb->mode = (CB_SOF_BIT | CB_EOF_BIT | CB_OWNERSHIP_BIT | CB_ZLP_GARBAGE | 0x1);  // send 1 ZLP
        tcb->Eop = tcb;

        cppi_log_event0("[cppi] Send ZLP!");
    }

    pTxCtl->TcbPool = tcb->Next;

    tcb->Next = 0;
    tcb->HNext = 0;

    // write out the buffer to memory
    consistent_sync(tcb, sizeof(*tcb), DMA_TO_DEVICE);

    cppi_log_event4("[cppi] q tcb", (uint32_t) tcb, ((uint32_t *) tcb)[0], ((uint32_t *) tcb)[1], ((uint32_t *) tcb)[2]);
    cppi_log_event4("[cppi] ", ((uint32_t *) tcb)[3], ((uint32_t *) tcb)[4], ((uint32_t *) tcb)[5], ((uint32_t *) tcb)[6]);

    cppi_log_event4("[cppi] send queued (ptr) (len) (ftcb, ltcb)", (uint32_t) tcb->BufPtr, tcb->Off_BLen, (uint32_t) first_tcb, (uint32_t) tcb);

    /* put it on the queue */
    pTxCtl->TxActQueueHead = first_tcb;
    pTxCtl->TxActQueueTail = tcb;

    cppi_log_event3("[cppi] setting state (head) (virt) (next)", (uint32_t) first_tcb, __dma_to_vlynq_phys(first_tcb->dma_handle), (uint32_t) first_tcb->HNext);

    /* write CPPI TX HDP - cache is cleaned above */
    tnetv_usb_reg_write(TNETV_DMA_TX_STATE(ch, TNETV_CPPI_TX_WORD_HDP), __dma_to_vlynq_phys(first_tcb->dma_handle));

    pTxCtl->TxActive = 1;

    return 0;
}

/*
 *  This function allocates transmit buffer descriptors (internal CPHAL function).
 *  It creates a high priority transmit queue by default for a single Tx
 *  channel.  If QoS is enabled for the given CPHAL device, this function
 *  will also allocate a low priority transmit queue.
 *
 *  @return  0 OK, Non-Zero Not OK
 */
int tnetv_cppi_init_tcb(struct cppi_info *cppi, int ch)
{
    int i, num;
    cppi_tcb *pTcb = 0;
    char *AllTcb;
    int  tcbSize;
    cppi_txcntl *pTxCtl = &cppi->tx_ctl[ch];

    num = pTxCtl->TxNumBuffers;
    tcbSize = (sizeof(cppi_tcb) + 0xf) & ~0xf;

    cppi_log_event4("[cppi] init_tcb (ch) (num) (dma) (tcbsz)", ch, num, pTxCtl->tcb_start_dma_addr, tcbSize);

    if (pTxCtl->TxNumBuffers == 0)
    {
        return -1;
    }

    /* if the memory has already been allocated, simply reuse it! */
    AllTcb = pTxCtl->TcbStart;

    // now reinitialize the TCB pool
    pTxCtl->TcbPool = 0;
    for (i = 0; i < num; i++)
    {
        pTcb = (cppi_tcb *)(AllTcb + (i * tcbSize));
        pTcb->dma_handle = pTxCtl->tcb_start_dma_addr + (i * tcbSize);

        pTcb->BufPtr = 0;
        pTcb->mode = 0;
        pTcb->HNext = 0;
        pTcb->Off_BLen = 0;
        pTcb->Eop = 0;

        pTcb->Next = (void *) pTxCtl->TcbPool;

        pTxCtl->TcbPool = pTcb;
    }

    cppi_log_event2("  [cppi]TcbPool", (uint32_t) pTxCtl->TcbPool, pTxCtl->TcbPool->dma_handle);

#if USB_CPPI_LOGGING
    {
        // BEN DEBUG
        cppi_tcb *first_tcb = pTxCtl->TcbPool;
        cppi_log_event4("[cppi] init tcb", (uint32_t) first_tcb, ((uint32_t *) first_tcb)[0], ((uint32_t *) first_tcb)[1], ((uint32_t *) first_tcb)[2]);
        cppi_log_event4("[cppi] ", ((uint32_t *) first_tcb)[3], ((uint32_t *) first_tcb)[4], ((uint32_t *) first_tcb)[5], ((uint32_t *) first_tcb)[6]);
    }
#endif

    return 0;
}

// BEN DEBUG
void tnetv_cppi_dump_info(struct cppi_info *cppi)
{
    int ch;
    cppi_rxcntl *pRxCtl;
    cppi_txcntl *pTxCtl;
    cppi_tcb *tcb;
    cppi_rcb *rcb;

    logf("CPPI struct:\n");
    logf("Buf mem: %lx Buf size: %d int: %lx %lx\n\n", (uint32_t) cppi->dma_mem, cppi->dma_size, tnetv_usb_reg_read(TNETV_USB_RX_INT_STATUS), tnetv_usb_reg_read(VL_INTST));

    for (ch = 0; ch < CPPI_NUM_CHANNELS; ch++)
    {
        pRxCtl = &cppi->rx_ctl[ch];
        pTxCtl = &cppi->tx_ctl[ch];

        logf("ch: %d\n", ch);
        logf("  rx_numbufs: %d active %ld free_buf_cnt %ld\n", pRxCtl->RxNumBuffers, pRxCtl->RxActive, tnetv_usb_reg_read(TNETV_USB_RX_FREE_BUF_CNT(ch)));
        logf("  q_cnt %ld head %lx tail %lx\n", pRxCtl->RxActQueueCount, (uint32_t) pRxCtl->RxActQueueHead, (uint32_t) pRxCtl->RxActQueueTail);
        logf("  fake_head: %lx fake_tail: %lx\n", (uint32_t) pRxCtl->RxFakeRcvHead, (uint32_t) pRxCtl->RxFakeRcvTail);

        rcb = (cppi_rcb *) pRxCtl->RcbStart;
        do
        {
            if (!rcb)
                break;

            logf("   Rcb: %lx\n", (uint32_t) rcb);
            logf("      HNext %lx BufPtr %lx Off_BLen %lx mode %lx\n", rcb->HNext, rcb->BufPtr, rcb->Off_BLen, rcb->mode);
            logf("      Next %lx Eop %lx dma_handle %lx fake_bytes %lx\n", (uint32_t) rcb->Next, (uint32_t) rcb->Eop, rcb->dma_handle, rcb->fake_bytes);
            rcb = rcb->Next;

        } while (rcb && rcb != (cppi_rcb *) pRxCtl->RcbStart);

        logf("\n");
        logf("  tx_numbufs: %d active %ld\n", pTxCtl->TxNumBuffers, pTxCtl->TxActive);
        logf("  q_cnt %ld head %lx tail %lx\n", pTxCtl->TxActQueueCount, (uint32_t) pTxCtl->TxActQueueHead, (uint32_t) pTxCtl->TxActQueueTail);

        tcb = (cppi_tcb *) pTxCtl->TcbPool;
        do
        {
            if (!tcb)
                break;

            logf("   Tcb (pool): %lx\n", (uint32_t) tcb);
            logf("      HNext %lx BufPtr %lx Off_BLen %lx mode %lx\n", tcb->HNext, tcb->BufPtr, tcb->Off_BLen, tcb->mode);
            logf("      Next %lx Eop %lx dma_handle %lx\n", (uint32_t) tcb->Next, (uint32_t) tcb->Eop, tcb->dma_handle);
            tcb = tcb->Next;

        } while (tcb && tcb != (cppi_tcb *) pTxCtl->TcbPool);

        tcb = (cppi_tcb *) pTxCtl->TxActQueueHead;
        do
        {
            if (!tcb)
                break;

            logf("   Tcb (act): %lx\n", (uint32_t) tcb);
            logf("      HNext %lx BufPtr %lx Off_BLen %lx mode %lx\n", tcb->HNext, tcb->BufPtr, tcb->Off_BLen, tcb->mode);
            logf("      Next %lx Eop %lx dma_handle %lx\n", (uint32_t) tcb->Next, (uint32_t) tcb->Eop, tcb->dma_handle);
            tcb = tcb->Next;

        } while (tcb && tcb != (cppi_tcb *) pTxCtl->TxActQueueTail);

    }
}

/**
 *
 *  This function is called to indicate to the CPHAL that the upper layer
 *  software has finished processing the receive data (given to it by
 *  osReceive()).  The CPHAL will then return the appropriate receive buffers
 *  and buffer descriptors to the available pool.
 *
 */
int tnetv_cppi_rx_return(struct cppi_info *cppi, int ch, cppi_rcb *done_rcb)
{
    cppi_rxcntl *pRxCtl = &cppi->rx_ctl[ch];
    cppi_rcb *curRcb, *lastRcb, *endRcb;
    int num_bufs = 0;

    if (!done_rcb)
        return -1;

    //cppi_log_event3("[cppi] rx_return (last) (first) bufinq", (uint32_t) done_rcb, (uint32_t) done_rcb->Eop, tnetv_usb_reg_read(TNETV_USB_RX_FREE_BUF_CNT(ch)));

    curRcb = done_rcb;
    endRcb = done_rcb->Eop;
    do
    {
        curRcb->mode = CB_OWNERSHIP_BIT;
        curRcb->Off_BLen = MAX_BUF_SIZE;
        curRcb->Eop = 0;

        pRxCtl->RxActQueueCount++;
        num_bufs++;

        lastRcb = curRcb;
        curRcb = lastRcb->Next;

        consistent_sync(lastRcb, sizeof(*lastRcb), DMA_TO_DEVICE);

    } while (lastRcb != endRcb);

    cppi_log_event1("[cppi] rx_return done", num_bufs);

    // let the hardware know about the buffer(s)
    tnetv_usb_reg_write(TNETV_USB_RX_FREE_BUF_CNT(ch), num_bufs);

    return 0;
}

int tnetv_cppi_rx_int_recv(struct cppi_info *cppi, int ch, int *buf_size, void *buf, int maxpacket)
{
    cppi_rxcntl *pRxCtl = &cppi->rx_ctl[ch];
    cppi_rcb *CurrentRcb, *LastRcb = 0, *SopRcb;
    uint8_t *cur_buf_data_addr;
    int cur_buf_bytes;
    int copy_buf_size = *buf_size;
    int ret = -EAGAIN;

    *buf_size = 0;

    CurrentRcb = pRxCtl->RxFakeRcvHead;
    if (!CurrentRcb)
    {
        cppi_log_event2("[cppi] rx_int recv: nothing in q", tnetv_usb_reg_read(TNETV_USB_RX_INT_STATUS), tnetv_usb_reg_read(VL_INTST));
        return -1;
    }

    cppi_log_event1("[cppi] rx_int recv (ch)", ch);
    cppi_log_event4("  [cppi] recv - Processing SOP descriptor fb hd tl", (uint32_t) CurrentRcb, CurrentRcb->fake_bytes, (uint32_t) pRxCtl->RxFakeRcvHead, (uint32_t) pRxCtl->RxFakeRcvTail);

    SopRcb = CurrentRcb;
    LastRcb = 0;

    do
    {
        // convert from vlynq phys to virt
        cur_buf_data_addr = (uint8_t *) __vlynq_phys_to_dma(CurrentRcb->BufPtr);
        cur_buf_data_addr = (uint8_t *) __phys_to_virt(cur_buf_data_addr);
        cur_buf_bytes = (CurrentRcb->mode) & CB_SIZE_MASK;

        // make sure we don't overflow the buffer.
        if (cur_buf_bytes > copy_buf_size)
        {
            ret = 0;
            break;
        }

        // BEN - packet can be ZLP
        if (cur_buf_bytes)
        {
            consistent_sync(cur_buf_data_addr, MAX_BUF_SIZE, DMA_FROM_DEVICE);

            memcpy((buf + *buf_size), cur_buf_data_addr, cur_buf_bytes);

            copy_buf_size -= cur_buf_bytes;
            *buf_size += cur_buf_bytes;
            CurrentRcb->fake_bytes -= cur_buf_bytes;
        }
        else
        {
            CurrentRcb->fake_bytes = 0;
        }

        cppi_log_event4("  [cppi] bytes totrcvd amtleft fake", cur_buf_bytes, *buf_size, copy_buf_size, CurrentRcb->fake_bytes);

        LastRcb = CurrentRcb;
        CurrentRcb = LastRcb->Next;

        // sync out fake bytes info
        consistent_sync(LastRcb, sizeof(*LastRcb), DMA_TO_DEVICE);

        // make sure each packet processed individually
        if (cur_buf_bytes < maxpacket)
        {
            ret = 0;
            break;
        }

    } while (LastRcb != pRxCtl->RxFakeRcvTail && CurrentRcb->fake_bytes && copy_buf_size > 0);

    // make sure that the CurrentRcb isn't in the cache
    consistent_sync(CurrentRcb, sizeof(*CurrentRcb), DMA_FROM_DEVICE);

    if (copy_buf_size == 0)
    {
        ret = 0;
    }

    if (LastRcb)
    {
        SopRcb->Eop = LastRcb;

        cppi_log_event3("  [cppi] rcv end", *buf_size, (uint32_t) CurrentRcb, (uint32_t) SopRcb->Eop);

        if (LastRcb == pRxCtl->RxFakeRcvTail)
        {
            pRxCtl->RxFakeRcvHead = 0;
            pRxCtl->RxFakeRcvTail = 0;
        }
        else
        {
            pRxCtl->RxFakeRcvHead = CurrentRcb;
        }

        cppi_log_event1("  [cppi] st rx return", ch);
        cppi_log_event2("       rcv fake hd tl", (uint32_t) pRxCtl->RxFakeRcvHead, (uint32_t) pRxCtl->RxFakeRcvTail);

        // all done, clean up the RCBs
        tnetv_cppi_rx_return(cppi, ch, SopRcb);
    }

    return ret;
}

/*
 *  This function processes receive interrupts.  It traverses the receive
 *  buffer queue, extracting the data and passing it to the upper layer software via
 *  osReceive().  It handles all error conditions and fragments without valid data by
 *  immediately returning the RCB's to the RCB pool.
 */
int tnetv_cppi_rx_int(struct cppi_info *cppi, int ch)
{
    cppi_rxcntl *pRxCtl = &cppi->rx_ctl[ch];
    cppi_rcb *CurrentRcb, *LastRcb = 0, *SopRcb;
    uint32_t RxBufStatus,PacketsServiced;
    int TotalFrags;

    cppi_log_event1("[cppi] rx_int (ch)", ch);

    CurrentRcb = pRxCtl->RxActQueueHead;

    if (!CurrentRcb)
    {
        cppi_log_event1("[cppi] rx_int no bufs!", (uint32_t) CurrentRcb);
        return -1;
    }

    // make sure that all of the buffers get an invalidated cache
    consistent_sync(pRxCtl->RcbStart, sizeof(cppi_rcb) * CPPI_RX_NUM_BUFS, DMA_FROM_DEVICE);

    RxBufStatus = CurrentRcb->mode;
    PacketsServiced = 0;

    cppi_log_event4("[cppi] currentrcb, mode numleft fake", (uint32_t) CurrentRcb, CurrentRcb->mode, pRxCtl->RxActQueueCount, CurrentRcb->fake_bytes);
    cppi_log_event4("[cppi]", ((uint32_t *) CurrentRcb)[0], ((uint32_t *) CurrentRcb)[1], ((uint32_t *) CurrentRcb)[2], ((uint32_t *) CurrentRcb)[3]);

    while(((RxBufStatus & CB_OWNERSHIP_BIT) == 0) && (pRxCtl->RxActQueueCount > 0))
    {
        cppi_log_event2("  [cppi]Processing SOP descriptor st", (uint32_t) CurrentRcb, RxBufStatus);

        SopRcb = CurrentRcb;

        TotalFrags = 0;

        do
        {
            TotalFrags++;
            PacketsServiced++;

            // Write the completion pointer
            tnetv_usb_reg_write(TNETV_DMA_RX_CMPL(ch), __dma_to_vlynq_phys(CurrentRcb->dma_handle));

            CurrentRcb->fake_bytes = (CurrentRcb->mode) & 0xFFFF;

            // BEN - make sure this gets marked!
            if (!CurrentRcb->fake_bytes || (CurrentRcb->mode & CB_ZLP_GARBAGE))
            {
                CurrentRcb->mode &= 0xFFFF0000;
                CurrentRcb->fake_bytes = 0x10000;
            }

            cppi_log_event1("     fake_bytes:", CurrentRcb->fake_bytes);

            RxBufStatus = CurrentRcb->mode;
            LastRcb = CurrentRcb;
            CurrentRcb = LastRcb->Next;

            // sync the fake_bytes value back to mem
            consistent_sync(LastRcb, sizeof(*LastRcb), DMA_TO_DEVICE);

        } while (((CurrentRcb->mode & CB_OWNERSHIP_BIT) == 0) && ((RxBufStatus & CB_EOF_BIT) == 0));

        SopRcb->Eop = LastRcb;

        pRxCtl->RxActQueueHead = CurrentRcb;
        pRxCtl->RxActQueueCount -= TotalFrags;

        if (LastRcb->mode & CB_EOQ_BIT)
        {
            if (CurrentRcb)
            {
                cppi_log_event1("  [cppi] rcv done q next", LastRcb->HNext);
                tnetv_usb_reg_write(TNETV_DMA_RX_STATE(ch, TNETV_CPPI_RX_WORD_HDP), LastRcb->HNext);
            }
            else
            {
                cppi_log_event0("  [cppi] rcv done");

                pRxCtl->RxActive = 0;
            }
        }

        // BEN - add to the list of buffers we need to deal with
        if (!pRxCtl->RxFakeRcvHead)
        {
            pRxCtl->RxFakeRcvHead = SopRcb;
            pRxCtl->RxFakeRcvTail = SopRcb->Eop;
        }
        else
        {
            pRxCtl->RxFakeRcvTail = SopRcb->Eop;
        }

        // make sure we have enough buffers
        cppi_log_event1("        nextrcb", CurrentRcb->mode);

        if (CurrentRcb)
        {
            // continue the loop
            RxBufStatus = CurrentRcb->mode;
        }

    } /* while */

    cppi_log_event2("[cppi] fake hd tl", (uint32_t) pRxCtl->RxFakeRcvHead, (uint32_t) pRxCtl->RxFakeRcvTail);

    // sync out all buffers before leaving
    consistent_sync(pRxCtl->RcbStart, (CPPI_RX_NUM_BUFS * sizeof(cppi_rcb)), DMA_FROM_DEVICE);

    return PacketsServiced;
}

static void tnetv_cppi_rx_queue_init(struct cppi_info *cppi, int ch, dma_addr_t buf, unsigned length)
{
    cppi_rxcntl *pRxCtl = &cppi->rx_ctl[ch];
    cppi_rcb *rcb, *first_rcb;
    unsigned int queued_len = 0;
    int rcblen;
    int num_frags = 0;
    dma_addr_t buf_ptr;

    if (length == 0)
    {
        cppi_log_event0("[cppi] len = 0, nothing to recv");
        return;
    }

    // usb_requests can have a 32 bit length, but CPPI DMA fragments
    // have a 64k limit.  Split the usb_request up into fragments here.
    first_rcb = pRxCtl->RcbPool;
    rcb = first_rcb;

    cppi_log_event2("[cppi] Rx queue add: head len", (uint32_t) first_rcb, length);

    while (queued_len < length)
    {
        buf_ptr = buf + queued_len;
        rcb->BufPtr = __dma_to_vlynq_phys(buf_ptr);

        rcb->HNext = 0;
        rcb->mode = CB_OWNERSHIP_BIT;

        rcblen = ((length - queued_len) > MAX_BUF_SIZE) ? MAX_BUF_SIZE : (length - queued_len);
        rcb->Off_BLen = rcblen;

        queued_len += rcblen;
        if (queued_len < length)
        {
            rcb->HNext = __dma_to_vlynq_phys(((cppi_rcb *) (rcb->Next))->dma_handle);
            rcb = rcb->Next;
        }

        num_frags++;
    }

    pRxCtl->RcbPool = rcb->Next;
    rcb->Next = 0;

    cppi_log_event4("[cppi] Adding Rcb (dma) (paddr) (buf)", (uint32_t) rcb, rcb->dma_handle, __dma_to_vlynq_phys(rcb->dma_handle), (uint32_t) rcb->BufPtr);
    cppi_log_event4("[cppi]  Next HNext (len) of (total)", (uint32_t) rcb->Next, rcb->HNext, queued_len, length);

    pRxCtl->RxActQueueCount += num_frags;

    cppi_log_event4("[cppi] rx queued (ptr) (len) (ftcb, ltcb)", (uint32_t) rcb->BufPtr, rcb->Off_BLen, (uint32_t) first_rcb, (uint32_t) rcb);
    cppi_log_event2("  [cppi] mode num_frags", rcb->mode, num_frags);

    pRxCtl->RxActQueueHead = first_rcb;
    pRxCtl->RxActQueueTail = rcb;

    cppi_log_event2("[cppi] setting rx (head) (virt)", (uint32_t) first_rcb, __dma_to_vlynq_phys(first_rcb->dma_handle));
    cppi_log_event4("[cppi] ", ((uint32_t *) first_rcb)[0], ((uint32_t *) first_rcb)[1], ((uint32_t *) first_rcb)[2], ((uint32_t *) first_rcb)[3]);

	// make this into a circular buffer so we never get caught with
	// no free buffers left
	rcb->Next = pRxCtl->RxActQueueHead;
	rcb->HNext = (uint32_t) (__dma_to_vlynq_phys(pRxCtl->RxActQueueHead->dma_handle));
}

int tnetv_cppi_rx_queue_add(struct cppi_info *cppi, int ch, dma_addr_t buf, unsigned length)
{
    (void)buf;
    (void)length;
    cppi_rxcntl *pRxCtl = &cppi->rx_ctl[ch];
    unsigned int cur_bufs;

    cur_bufs = tnetv_usb_reg_read(TNETV_USB_RX_FREE_BUF_CNT(ch));

    if (!pRxCtl->RxActive)
    {
        cppi_log_event0("[cppi] queue add - not active");

        pRxCtl->RcbPool = (cppi_rcb *) pRxCtl->RcbStart;

        // add all the buffers to the active (circular) queue
        tnetv_cppi_rx_queue_init(cppi, ch, (dma_addr_t) __virt_to_phys(dma_recv_buf[ch]), (MAX_BUF_SIZE * pRxCtl->RxNumBuffers));

        /* write Rx Queue Head Descriptor Pointer */
        tnetv_usb_reg_write(TNETV_DMA_RX_STATE(ch, TNETV_CPPI_RX_WORD_HDP), __dma_to_vlynq_phys(pRxCtl->RxActQueueHead->dma_handle));

        pRxCtl->RxActive = 1;

        // sync out all buffers before starting
        consistent_sync(pRxCtl->RcbStart, (CPPI_RX_NUM_BUFS * sizeof(cppi_rcb)), DMA_TO_DEVICE);

        // sync out temp rx buffer
        consistent_sync(dma_recv_buf[ch], CPPI_DMA_RX_BUF_SIZE, DMA_FROM_DEVICE);

        if (cur_bufs < pRxCtl->RxActQueueCount)
        {
            // let the hardware know about the buffer(s)
            tnetv_usb_reg_write(TNETV_USB_RX_FREE_BUF_CNT(ch), pRxCtl->RxActQueueCount - cur_bufs);
        }
    }

    cppi_log_event3("[cppi] rx add: (cur_bufs) (avail_bufs) (now)", cur_bufs, pRxCtl->RxActQueueCount, tnetv_usb_reg_read(TNETV_USB_RX_FREE_BUF_CNT(ch)));

    return 0;
}

int tnetv_cppi_flush_rx_queue(struct cppi_info *cppi, int ch)
{
    cppi_rxcntl *pRxCtl = &cppi->rx_ctl[ch];
    cppi_rcb *rcb;
    int num_bufs;

    cppi_log_event1("[cppi] flush RX ", (uint32_t) pRxCtl->RxActQueueHead);

    // flush out any pending receives
    tnetv_cppi_rx_int(cppi, ch);

    // now discard all received data
    rcb = pRxCtl->RxFakeRcvHead;

    if (rcb)
    {
        rcb->Eop = pRxCtl->RxFakeRcvTail;

        // clean up any unreceived RCBs
        tnetv_cppi_rx_return(cppi, ch, rcb);
    }

    pRxCtl->RxFakeRcvHead = 0;
    pRxCtl->RxFakeRcvTail = 0;

    pRxCtl->RxActive = 0;

    // drain the HW free buffer count
    num_bufs = tnetv_usb_reg_read(TNETV_USB_RX_FREE_BUF_CNT(ch));
    tnetv_usb_reg_write(TNETV_USB_RX_FREE_BUF_CNT(ch), -num_bufs);

    cppi_log_event2("[cppi] flush RX queue done (freed) act: ", num_bufs, (uint32_t) pRxCtl->RxActQueueCount);

    return 0;
}


/*
 *  This function allocates receive buffer descriptors (internal CPHAL function).
 *  After allocation, the function 'queues' (gives to the hardware) the newly
 *  created receive buffers to enable packet reception.
 *
 *  @param   ch    Channel number.
 *
 *  @return  0 OK, Non-Zero Not OK
 */
int tnetv_cppi_init_rcb(struct cppi_info *cppi, int ch)
{
    int i, num;
    cppi_rcb *pRcb;
    char *AllRcb;
    int  rcbSize;
    cppi_rxcntl *pRxCtl = &cppi->rx_ctl[ch];

    num = pRxCtl->RxNumBuffers;
    rcbSize = (sizeof(cppi_rcb) + 0xf) & ~0xf;

    cppi_log_event2("[cppi] init_rcb ch num", ch, num);

    if (pRxCtl->RxNumBuffers == 0)
    {
        return -1;
    }

    /* if the memory has already been allocated, simply reuse it! */
    AllRcb = pRxCtl->RcbStart;

    // now reinitialize the RCB pool
    pRxCtl->RcbPool = 0;
    for (i = (num - 1); i >= 0; i--)
    {
        pRcb = (cppi_rcb *)(AllRcb + (i * rcbSize));

        pRcb->dma_handle = pRxCtl->rcb_start_dma_addr + (i * rcbSize);

        pRcb->BufPtr = 0;
        pRcb->mode = 0;
        pRcb->HNext = 0;
        pRcb->Next = (void *) pRxCtl->RcbPool;
        pRcb->Off_BLen = 0;
        pRcb->Eop = 0;
        pRcb->fake_bytes = 0;

        pRxCtl->RcbPool = pRcb;
    }

    cppi_log_event2("  [cppi]RcbPool (dma)", (uint32_t) pRxCtl->RcbPool, pRxCtl->RcbPool->dma_handle);

    pRxCtl->RxActQueueCount = 0;
    pRxCtl->RxActQueueHead = 0;
    pRxCtl->RxActive = 0;

    pRxCtl->RxFakeRcvHead = 0;
    pRxCtl->RxFakeRcvTail = 0;

    return 0;
}

static uint8_t ch_buf_cnt[][2] = {
    {CPPI_RX_NUM_BUFS, 2},  // ch0: bulk out/in
    {CPPI_RX_NUM_BUFS, 2},  // ch1: bulk out/in
    {0, 2},    // ch2: interrupt
    {0, 2}     // ch3: interrupt
};

void tnetv_cppi_init(struct cppi_info *cppi)
{
    int ch;
    uint8_t *alloc_ptr;
    int ch_mem_size[CPPI_NUM_CHANNELS];

    // wipe cppi memory
    memset(cppi, 0, sizeof(*cppi));

    // find out how much memory we need to allocate
    cppi->dma_size = 0;
    for (ch = 0; ch < CPPI_NUM_CHANNELS; ch++)
    {
        ch_mem_size[ch] = (ch_buf_cnt[ch][0] * sizeof(cppi_rcb)) + (ch_buf_cnt[ch][1] * sizeof(cppi_tcb));
        cppi->dma_size += ch_mem_size[ch];
    }

    // allocate DMA-able memory
    if (cppi->dma_size != CPPI_INFO_MEM_SIZE)
    {
        panicf("Invalid dma size expected %d got %d", cppi->dma_size, CPPI_INFO_MEM_SIZE);
    }
    cppi->dma_handle = (dma_addr_t)  __virt_to_phys(cppi->dma_mem);

    memset(cppi->dma_mem, 0, cppi->dma_size);

    cppi_log_event2("[cppi] all CBs sz mem", cppi->dma_size, (uint32_t) cppi->dma_mem);

    // now set up the pointers
    alloc_ptr = cppi->dma_mem;
    for (ch = 0; ch < CPPI_NUM_CHANNELS; ch++)
    {
        cppi->rx_ctl[ch].RxNumBuffers = ch_buf_cnt[ch][0];
        cppi->rx_ctl[ch].RcbStart = alloc_ptr;
        cppi->rx_ctl[ch].rcb_start_dma_addr = (dma_addr_t)  __virt_to_phys(alloc_ptr);
        alloc_ptr += (ch_buf_cnt[ch][0] * sizeof(cppi_rcb));

        cppi->tx_ctl[ch].TxNumBuffers = ch_buf_cnt[ch][1];
        cppi->tx_ctl[ch].TcbStart = alloc_ptr;
        cppi->tx_ctl[ch].tcb_start_dma_addr = (dma_addr_t)  __virt_to_phys(alloc_ptr);
        alloc_ptr += (ch_buf_cnt[ch][1] * sizeof(cppi_tcb));

        cppi_log_event3("[cppi] alloc bufs: ch dmarcb dmatcb", ch, cppi->rx_ctl[ch].rcb_start_dma_addr, cppi->tx_ctl[ch].tcb_start_dma_addr);

        // set up receive buffer
        if (ch_buf_cnt[ch][0])
        {
            dma_recv_buf[ch] = (ch == 0) ? ch0_rx_buf : ((ch == 1) ? ch1_rx_buf : 0);
            cppi_log_event3("[cppi] Alloc fake DMA buf ch", ch, (uint32_t) dma_recv_buf[ch], (uint32_t) __virt_to_phys(dma_recv_buf[ch]));
        }
        else
        {
            dma_recv_buf[ch] = 0;
        }
   }

}

void tnetv_cppi_cleanup(struct cppi_info *cppi)
{
    cppi_log_event0("wipe cppi mem");

    // wipe cppi memory
    memset(cppi, 0, sizeof(*cppi));
}
