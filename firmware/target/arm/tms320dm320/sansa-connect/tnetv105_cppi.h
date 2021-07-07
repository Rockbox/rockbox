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
 * Copyright (c) 2005 Zermatt Systems, Inc.
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

#ifndef TNETV105_CPPI_H
#define TNETV105_CPPI_H

#include <stdint.h>
#include "errno.h"

typedef uint32_t dma_addr_t;
#define USB_CPPI_LOGGING   0
#define CPPI_RX_NUM_BUFS   2
#define CPPI_INFO_MEM_SIZE (2 * CPPI_RX_NUM_BUFS * sizeof(cppi_rcb) + 4 * 2 * sizeof(cppi_tcb))

#define CPPI_NUM_CHANNELS    4
#define CPPI_MAX_FRAG        0xFE00

struct cppi_info;

typedef struct
{
    uint32_t HNext;      /*< Hardware's pointer to next buffer descriptor  */
    uint32_t BufPtr;     /*< Pointer to the data buffer                    */
    uint32_t Off_BLen;   /*< Contains buffer offset and buffer length      */
    uint32_t mode;       /*< SOP, EOP, Ownership, EOQ, Teardown, Q Starv, Length */
    void *Next;
    void *Eop;
    dma_addr_t dma_handle;
    uint32_t dummy;

} cppi_tcb;

typedef struct
{
    uint32_t HNext;      /*< Hardware's pointer to next buffer descriptor     */
    uint32_t BufPtr;     /*< Pointer to the data buffer                       */
    uint32_t Off_BLen;   /*< Contains buffer offset and buffer length         */
    uint32_t mode;       /*< SOP, EOP, Ownership, EOQ, Teardown Complete bits */
    void *Next;
    void *Eop;
    dma_addr_t dma_handle;
    uint32_t fake_bytes;

} cppi_rcb;

typedef struct cppi_txcntl
{
    cppi_tcb *TcbPool;
    cppi_tcb *TxActQueueHead;
    cppi_tcb *TxActQueueTail;
    uint32_t   TxActQueueCount;
    uint32_t   TxActive;
    cppi_tcb *LastTcbProcessed;
    char    *TcbStart;
    dma_addr_t tcb_start_dma_addr;
    int TxNumBuffers;

#ifdef _CPHAL_STATS
    uint32_t   TxMisQCnt;
    uint32_t   TxEOQCnt;
    uint32_t   TxPacketsServiced;
    uint32_t   TxMaxServiced;
    uint32_t  NumTxInt;
#endif
} cppi_txcntl;


typedef struct cppi_rxcntl
{
    cppi_rcb *RcbPool;
    cppi_rcb *RxActQueueHead;
    cppi_rcb *RxActQueueTail;
    uint32_t   RxActQueueCount;
    uint32_t   RxActive;
    char    *RcbStart;
    dma_addr_t rcb_start_dma_addr;
    int RxNumBuffers;

    cppi_rcb *RxFakeRcvHead;
    cppi_rcb *RxFakeRcvTail;

#ifdef _CPHAL_STATS
    uint32_t  RxMisQCnt;
    uint32_t  RxEOQCnt;
    uint32_t  RxMaxServiced;
    uint32_t  RxPacketsServiced;
    uint32_t  NumRxInt;
#endif
} cppi_rxcntl;

typedef struct cppi_info
{
    struct cppi_txcntl tx_ctl[CPPI_NUM_CHANNELS];
    struct cppi_rxcntl rx_ctl[CPPI_NUM_CHANNELS];

    uint8_t dma_mem[CPPI_INFO_MEM_SIZE];
    int dma_size;
    dma_addr_t dma_handle;

} cppi_info;

#define tnetv_cppi_rx_int_recv_check(cppi, ch) (((cppi)->rx_ctl[(ch)].RxFakeRcvHead) ? 1 : 0)

int tnetv_cppi_init_tcb(struct cppi_info *cppi, int ch);
int tnetv_cppi_flush_tx_queue(struct cppi_info *cppi, int ch);
int tnetv_cppi_send(struct cppi_info *cppi, int ch, dma_addr_t buf, unsigned length, int send_zlp);
int tnetv_cppi_tx_int(struct cppi_info *cppi, int ch);
void tnetv_cppi_free_tcb(struct cppi_info *cppi, int ch);

int tnetv_cppi_init_rcb(struct cppi_info *cppi, int ch);
int tnetv_cppi_flush_rx_queue(struct cppi_info *cppi, int ch);
int tnetv_cppi_rx_return(struct cppi_info *cppi, int ch, cppi_rcb *done_rcb);
int tnetv_cppi_rx_queue_add(struct cppi_info *cppi, int ch, dma_addr_t buf, unsigned length);
int tnetv_cppi_rx_int(struct cppi_info *cppi, int ch);
int tnetv_cppi_rx_int_recv(struct cppi_info *cppi, int ch, int *buf_size, void *buf, int maxpacket);
void tnetv_cppi_free_rcb(struct cppi_info *cppi, int ch);

void tnetv_cppi_init(struct cppi_info *cppi);
void tnetv_cppi_cleanup(struct cppi_info *cppi);

void tnetv_cppi_dump_info(struct cppi_info *cppi);

#endif
