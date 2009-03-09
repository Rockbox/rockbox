/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Michael Sevakis
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
#ifndef SDMA_IMX31_H
#define SDMA_IMX31_H

/* Much of the code in here is based upon the Linux BSP provided by Freescale
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved. */

/* Peripheral and transfer type - used to select the proper SDMA channel
 * script to execute. */
enum SDMA_PERIPHERAL_TYPE
{
    /* SHP = "Shared peripheral" where peripheral is mapped into SDMA
     * core memory via the SPBA */
    __SDMA_PER_FIRST = -1,
    SDMA_PER_MEMORY,
    SDMA_PER_DSP,
    SDMA_PER_FIRI,
    SDMA_PER_UART,
    SDMA_PER_UART_SHP,
    SDMA_PER_ATA,
    SDMA_PER_CSPI,
    SDMA_PER_EXT,
    SDMA_PER_SSI,
    SDMA_PER_SSI_SHP,
    SDMA_PER_MMC,
    SDMA_PER_SDHC,
    SDMA_PER_CSPI_SHP,
    SDMA_PER_MSHC,
    SDMA_PER_MSHC_SHP,
    SDMA_PER_CCM,
    SDMA_PER_ASRC,
    SDMA_PER_ESAI,
    SDMA_PER_SIM,
    SDMA_PER_SPDIF,
    SDMA_PER_IPU_MEMORY,
};

enum SDMA_TRANSFER_TYPE
{
    __SDMA_TRAN_FIRST = -1,
    SDMA_TRAN_INT_2_INT,
    SDMA_TRAN_EMI_2_INT,
    SDMA_TRAN_EMI_2_EMI,
    SDMA_TRAN_INT_2_EMI,

    SDMA_TRAN_INT_2_DSP,
    SDMA_TRAN_DSP_2_INT,
    SDMA_TRAN_DSP_2_DSP,
    SDMA_TRAN_DSP_2_PER,
    SDMA_TRAN_PER_2_DSP,
    SDMA_TRAN_EMI_2_DSP,
    SDMA_TRAN_DSP_2_EMI,
    SDMA_TRAN_DSP_2_EMI_LOOP,
    SDMA_TRAN_EMI_2_DSP_LOOP,

    SDMA_TRAN_PER_2_INT,
    SDMA_TRAN_PER_2_EMI,
    SDMA_TRAN_INT_2_PER,
    SDMA_TRAN_EMI_2_PER,
};

/* 2.3 - Smart Direct Memory Access (SDMA) Events, Table 2-5 */
/* These are indexes into the SDMA_CHNENBL register array (each a bitmask
 * determining which channels are triggered by requests). */
enum SDMA_REQUEST_TYPE
{
    SDMA_REQ_EXT0       =  0, /* Extern DMA request from MCU1_0 */
    SDMA_REQ_CCM        =  1, /* DVFS/DPTC event (ccm_dvfs_sdma_int) */
    SDMA_REQ_ATA_TX_END =  2, /* ata_txfer_end_alarm (event_id) */
    SDMA_REQ_ATA_TX     =  3, /* ata_tx_fifo_alarm (event_id2) */
    SDMA_REQ_ATA_RX     =  4, /* ata_rcv_fifo_alarm (event_id2) */
    SDMA_REQ_SIM        =  5, /* */
    SDMA_REQ_CSPI2_RX   =  6, /* DMA Rx request */
    SDMA_REQ_CSPI2_TX   =  7, /* DMA Tx request */
    SDMA_REQ_CSPI1_RX   =  8, /* DMA Rx request of CSPI */
    SDMA_REQ_UART3_RX   =  8, /* DMA Rx request RxFIFO of UART3 */
    SDMA_REQ_CSPI1_TX   =  9, /* DMA Tx request of CSPI */
    SDMA_REQ_UART3_TX   =  9, /* DMA Tx request TxFIFO of UART3 */
    SDMA_REQ_CSPI3_RX   = 10, /* RxFIFO or CSPI3 Rx request */
    SDMA_REQ_UART5_RX   = 10, /* RxFIFO or CSPI3 Rx request */
    SDMA_REQ_CSPI3_TX   = 11, /* TxFIFO or CSPI3 Tx request */
    SDMA_REQ_UART5_TX   = 11, /* TxFIFO or CSPI3 Tx request */
    SDMA_REQ_UART4_RX   = 12, /* RxFIFO */
    SDMA_REQ_UART4_TX   = 13, /* TxFIFO */
    SDMA_REQ_EXT2       = 14, /* External DMA request from MCU1_2 or from
                                MBX (Graphic accelerator) */
    SDMA_REQ_EXT1       = 15, /* External request from MCU1_1 */
    SDMA_REQ_FIRI_RX    = 16, /* DMA request of FIR's receiver FIFO
                                controlled by the pgp_firi signal
                                from the IOMUXC PGP register */
    SDMA_REQ_UART2_RX   = 16, /* RxFIFO of UART2 */
    SDMA_REQ_FIRI_TX    = 17, /* DMA request of FIR's transmitter 
                                FIFO controled by the pgp_firi signal
                                the IOMUXC PGP register */
    SDMA_REQ_UART2_TX   = 17, /* TxFIFO of UART2 */
    SDMA_REQ_UART1_RX   = 18, /* RxFIFO */
    SDMA_REQ_UART1_TX   = 19, /* TxFIFO */
    SDMA_REQ_MMC1       = 20, /* MMC DMA request */
    SDMA_REQ_SDHC1      = 20, /* SDHC1 DMA request */
    SDMA_REQ_MSHC1      = 20, /* MSHC1 DMA request */
    SDMA_REQ_MMC2       = 21, /* MMC DMA request */
    SDMA_REQ_SDHC2      = 21, /* SDHC2 DMA request */
    SDMA_REQ_MSHC2      = 21, /* MSHC2 DMA request */
    SDMA_REQ_SSI2_RX2   = 22, /* SSI #2 receive 2 DMA request (SRX1_2) */
    SDMA_REQ_SSI2_TX2   = 23, /* SSI #2 transmit 2 DMA request  (STX1_2) */
    SDMA_REQ_SSI2_RX1   = 24, /* SSI #2 receive 1 DMA request (SRX0_2) */
    SDMA_REQ_SSI2_TX1   = 25, /* SSI #2 transmit 1 DMA request (STX0_2) */
    SDMA_REQ_SSI1_RX2   = 26, /* SSI #1 receive 2 DMA request (SRX1_1) */
    SDMA_REQ_SSI1_TX2   = 27, /* SSI #1 transmit 2 DMA request (STX1_1) */
    SDMA_REQ_SSI1_RX1   = 28, /* SSI #1 receive 1 DMA request (SRX1_0) */
    SDMA_REQ_SSI1_TX1   = 29, /* SSI #1 transmit 1 DMA request (STX1_0) */
    SDMA_REQ_NFC        = 30, /* NAND-flash controller */
    SDMA_REQ_IPU        = 31, /* IPU source (defaults to IPU at reset) */
    SDMA_REQ_ECT        = 31, /* ECT source */
};

/* Addresses for peripheral DMA transfers */
enum SDMA_PER_ADDR
{
    SDMA_PER_ADDR_SDRAM     = SDRAM_BASE_ADDR,      /* memory */
    SDMA_PER_ADDR_CCM       = CCM_BASE_ADDR+0x00,   /* CCMR */
    /* ATA */
    SDMA_PER_ADDR_ATA_TX    = ATA_DMA_BASE_ADDR+0x18,
    SDMA_PER_ADDR_ATA_RX    = ATA_DMA_BASE_ADDR,
#if 0
    SDMA_PER_ADDR_ATA_TX16  =
    SDMA_PER_ADDR_ATA_RX16  =
#endif
#if 0
    SDMA_PER_ADDR_SIM       =
#endif
    /* CSPI2 */
    SDMA_PER_ADDR_CSPI2_RX  = CSPI2_BASE_ADDR+0x00, /* RXDATA2 */
    SDMA_PER_ADDR_CSPI2_TX  = CSPI2_BASE_ADDR+0x04, /* TXDATA2 */
    /* CSPI1 */
    SDMA_PER_ADDR_CSPI1_RX  = CSPI1_BASE_ADDR+0x00, /* RXDATA1 */
    SDMA_PER_ADDR_CSPI1_TX  = CSPI1_BASE_ADDR+0x04, /* TXDATA1 */
    /* UART3 */
    SDMA_PER_ADDR_UART3_RX  = UART3_BASE_ADDR+0x00, /* URXD3 */
    SDMA_PER_ADDR_UART3_TX  = UART3_BASE_ADDR+0x40, /* UTXD3 */
    /* CSPI3 */
    SDMA_PER_ADDR_CSPI3_RX  = CSPI3_BASE_ADDR+0x00, /* RXDATA3 */
    SDMA_PER_ADDR_CSPI3_TX  = CSPI3_BASE_ADDR+0x04, /* TXDATA3 */
    /* UART5 */
    SDMA_PER_ADDR_UART5_RX  = UART5_BASE_ADDR+0x00, /* URXD5 */
    SDMA_PER_ADDR_UART5_TX  = UART5_BASE_ADDR+0x40,  /* UTXD5 */
    /* UART4 */
    SDMA_PER_ADDR_UART4_RX  = UART4_BASE_ADDR+0x00, /* URXD4 */
    SDMA_PER_ADDR_UART4_TX  = UART4_BASE_ADDR+0x40, /* UTXD4 */
    /* FIRI */
    SDMA_PER_ADDR_FIRI_RX   = FIRI_BASE_ADDR+0x18,  /* Receiver FIFO */
    SDMA_PER_ADDR_FIRI_TX   = FIRI_BASE_ADDR+0x14,  /* Transmitter FIFO */
    /* UART2 */
    SDMA_PER_ADDR_UART2_RX  = UART2_BASE_ADDR+0x00, /* URXD2 */
    SDMA_PER_ADDR_UART2_TX  = UART2_BASE_ADDR+0x40, /* UTXD2 */
    /* UART1 */
    SDMA_PER_ADDR_UART1_RX  = UART1_BASE_ADDR+0x00, /* URXD1 */
    SDMA_PER_ADDR_UART1_TX  = UART1_BASE_ADDR+0x40, /* UTXD1 */
    SDMA_PER_ADDR_MMC_SDHC1 = MMC_SDHC1_BASE_ADDR+0x38, /* BUFFER_ACCESS */
    SDMA_PER_ADDR_MMC_SDHC2 = MMC_SDHC2_BASE_ADDR+0x38, /* BUFFER_ACCESS */
#if 0
    SDMA_PER_ADDR_MSHC1     =
    SDMA_PER_ADDR_MSHC2     =
#endif
    /* SSI2 */
    SDMA_PER_ADDR_SSI2_RX2  = SSI2_BASE_ADDR+0x0C, /* SRX1_2 */
    SDMA_PER_ADDR_SSI2_TX2  = SSI2_BASE_ADDR+0x04, /* STX1_2 */
    SDMA_PER_ADDR_SSI2_RX1  = SSI2_BASE_ADDR+0x08, /* SRX0_2 */
    SDMA_PER_ADDR_SSI2_TX1  = SSI2_BASE_ADDR+0x00, /* STX0_2 */
    /* SSI1 */
    SDMA_PER_ADDR_SSI1_RX2  = SSI1_BASE_ADDR+0x0C, /* SRX1_1 */
    SDMA_PER_ADDR_SSI1_TX2  = SSI1_BASE_ADDR+0x04, /* STX1_1 */
    SDMA_PER_ADDR_SSI1_RX1  = SSI1_BASE_ADDR+0x08, /* SRX0_1 */
    SDMA_PER_ADDR_SSI1_TX1  = SSI1_BASE_ADDR+0x00, /* STX0_1 */
#if 0
    SDMA_PER_ADDR_NFC       =
    SDMA_PER_ADDR_IPU       =
    SDMA_PER_ADDR_ECT       =
#endif
};

/* DMA driver defines */
#define SDMA_SDHC_MMC_WML    16
#define SDMA_SDHC_SD_WML     64
#define SDMA_SSI_TXFIFO_WML   4 /* Four samples written per channel activation */
#define SDMA_SSI_RXFIFO_WML   6 /* Six samples read per channel activation */
#define SDMA_FIRI_WML        16

#define SDMA_ATA_WML         32  /* DMA watermark level in bytes */
#define SDMA_ATA_BD_NR       (512/3/4) /* Number of BDs per channel */

#include "sdma_struct.h"

void sdma_init(void);
void sdma_read_words(unsigned long *buf, unsigned long start, int count);
void sdma_write_words(const unsigned long *buf, unsigned long start, int count);
void sdma_channel_set_priority(unsigned int channel, unsigned int priority);
bool sdma_channel_reset(unsigned int channel);
void sdma_channel_run(unsigned int channel);
void sdma_channel_pause(unsigned int channel);
void sdma_channel_stop(unsigned int channel);
void sdma_channel_wait_nonblocking(unsigned int channel);
bool sdma_channel_init(unsigned int channel,
                       struct channel_descriptor *cd_p,
                       struct buffer_descriptor *base_bd_p);
void sdma_channel_close(unsigned int channel);

#endif /* SDMA_IMX31_H */
