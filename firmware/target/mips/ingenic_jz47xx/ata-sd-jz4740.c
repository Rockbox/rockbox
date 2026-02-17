/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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

#include "config.h"
#include "gcc_extensions.h"
#include "cpu.h"
#include "ata-sd-target.h"
#include "led.h"
#include "sdmmc.h"
#include "logf.h"
#include "storage.h"
#include "string.h"

static long               last_disk_activity = -1;
#if defined(CONFIG_STORAGE_MULTI) || defined(HAVE_HOTSWAP)
static int                sd_drive_nr = 0;
#else
#define                   sd_drive_nr 0
#endif
static tCardInfo          card;

static struct mutex       sd_mtx;
//static struct semaphore   sd_wakeup;

static int                use_4bit;
static int                use_sbc;

//#define SD_DMA_ENABLE
#define SD_DMA_INTERRUPT 0

//#define DEBUG(x...)         logf(x)
#define DEBUG(x, ...)

#define SD_INSERT_STATUS() __gpio_get_pin(MMC_CD_PIN)
#define SD_RESET()         __msc_reset()

#define SD_IRQ_MASK()              \
do {                               \
          REG_MSC_IMASK = 0xffff;  \
          REG_MSC_IREG = 0xffff;   \
} while (0)

/* Error codes */
enum sd_result_t
{
    SD_NO_RESPONSE        = -1,
    SD_NO_ERROR           = 0,
    SD_ERROR_OUT_OF_RANGE,
    SD_ERROR_ADDRESS,
    SD_ERROR_BLOCK_LEN,
    SD_ERROR_ERASE_SEQ,
    SD_ERROR_ERASE_PARAM,
    SD_ERROR_WP_VIOLATION,
    SD_ERROR_CARD_IS_LOCKED,
    SD_ERROR_LOCK_UNLOCK_FAILED,
    SD_ERROR_COM_CRC,
    SD_ERROR_ILLEGAL_COMMAND,
    SD_ERROR_CARD_ECC_FAILED,
    SD_ERROR_CC,
    SD_ERROR_GENERAL,
    SD_ERROR_UNDERRUN,
    SD_ERROR_OVERRUN,
    SD_ERROR_CID_CSD_OVERWRITE,
    SD_ERROR_STATE_MISMATCH,
    SD_ERROR_HEADER_MISMATCH,
    SD_ERROR_TIMEOUT,
    SD_ERROR_CRC,
    SD_ERROR_DRIVER_FAILURE,
    SD_ERROR_ACMD_REJECT,
};

/* Standard MMC/SD clock speeds */
#define MMC_CLOCK_SLOW    400000      /* 400 kHz for initial setup */
#define SD_CLOCK_FAST   24000000      /* 24 MHz for SD Cards */
#define SD_CLOCK_HIGH   48000000      /* 48 MHz for SD Cards */

/* Proprietary commands, illegal/reserved according to SD Specification 2.00 */
    /* class 1 */
#define SD_READ_DAT_UNTIL_STOP  11   /* adtc [31:0]  dadr       R1  */

    /* class 3 */
#define SD_WRITE_DAT_UNTIL_STOP 20   /* adtc [31:0]  data addr  R1  */

    /* class 4 */
#define SD_PROGRAM_CID          26   /* adtc                    R1  */
#define SD_PROGRAM_CSD          27   /* adtc                    R1  */

    /* class 9 */
#define SD_GO_IRQ_STATE         40   /* bcr                     R5  */

enum sd_rsp_t
{
    RESPONSE_NONE    = 0,
    RESPONSE_R1      = 1,
    RESPONSE_R1B     = 2,
    RESPONSE_R2      = 3,
    RESPONSE_R3      = 4,
    RESPONSE_R4      = 5,
    RESPONSE_R5      = 6,
    RESPONSE_R6      = 7,
    RESPONSE_R7      = 8,
};

/*
  MMC status in R1
  Type
    e : error bit
    s : status bit
    r : detected and set for the actual command response
    x : detected and set during command execution. the host must poll
        the card by sending status command in order to read these bits.
  Clear condition
    a : according to the card state
    b : always related to the previous command. Reception of
        a valid command will clear it (with a delay of one command)
    c : clear by read
 */

#define R1_OUT_OF_RANGE        (1 << 31)    /* er, c */
#define R1_ADDRESS_ERROR       (1 << 30)    /* erx, c */
#define R1_BLOCK_LEN_ERROR     (1 << 29)    /* er, c */
#define R1_ERASE_SEQ_ERROR     (1 << 28)    /* er, c */
#define R1_ERASE_PARAM         (1 << 27)    /* ex, c */
#define R1_WP_VIOLATION        (1 << 26)    /* erx, c */
#define R1_CARD_IS_LOCKED      (1 << 25)    /* sx, a */
#define R1_LOCK_UNLOCK_FAILED  (1 << 24)    /* erx, c */
#define R1_COM_CRC_ERROR       (1 << 23)    /* er, b */
#define R1_ILLEGAL_COMMAND     (1 << 22)    /* er, b */
#define R1_CARD_ECC_FAILED     (1 << 21)    /* ex, c */
#define R1_CC_ERROR            (1 << 20)    /* erx, c */
#define R1_ERROR               (1 << 19)    /* erx, c */
#define R1_UNDERRUN            (1 << 18)    /* ex, c */
#define R1_OVERRUN             (1 << 17)    /* ex, c */
#define R1_CID_CSD_OVERWRITE   (1 << 16)    /* erx, c, CID/CSD overwrite */
#define R1_WP_ERASE_SKIP       (1 << 15)    /* sx, c */
#define R1_CARD_ECC_DISABLED   (1 << 14)    /* sx, a */
#define R1_ERASE_RESET         (1 << 13)    /* sr, c */
#define R1_STATUS(x)           (x & 0xFFFFE000)
#define R1_CURRENT_STATE(x)    ((x & 0x00001E00) >> 9)    /* sx, b (4 bits) */
#define R1_READY_FOR_DATA      (1 << 8)     /* sx, a */
#define R1_APP_CMD             (1 << 7)     /* sr, c */

/* These are unpacked versions of the actual responses */
struct sd_response_r1
{
    unsigned char  cmd;
    unsigned int   status;
};

struct sd_response_r3
{
    unsigned int ocr;
};

#define SD_CARD_BUSY    0x80000000    /* Card Power up status bit */

struct sd_request
{
    int               cmd;        /* Command to send */
    unsigned int      arg;        /* Argument to send */
    enum sd_rsp_t     rtype;      /* Response type expected */

    /* Data transfer (these may be modified at the low level) */
    unsigned short    nob;        /* Number of blocks to transfer*/
    unsigned short    block_len;  /* Block length */
    unsigned char     *buffer;    /* Data buffer */
    unsigned int      cnt;        /* Data length, for PIO */

    /* Results */
    unsigned char     response[18]; /* Buffer to store response - CRC is optional */
    enum sd_result_t result;
};

/**************************************************************************
 * Utility functions
 **************************************************************************/

#define PARSE_U32(_buf,_index) \
    (((unsigned int)_buf[_index]) << 24) | (((unsigned int)_buf[_index+1]) << 16) | \
        (((unsigned int)_buf[_index+2]) << 8) | ((unsigned int)_buf[_index+3]);

#define PARSE_U16(_buf,_index) \
    (((unsigned short)_buf[_index]) << 8) | ((unsigned short)_buf[_index+1]);

static int sd_unpack_r1(struct sd_request *request, struct sd_response_r1 *r1)
{
    unsigned char *buf = request->response;

    if (request->result)
        return request->result;

    r1->cmd    = buf[0];
    r1->status = PARSE_U32(buf,1);

    DEBUG("sd_unpack_r1: cmd=%d status=%08x", r1->cmd, r1->status);

    if (R1_STATUS(r1->status)) {
        if (r1->status & R1_OUT_OF_RANGE)       return SD_ERROR_OUT_OF_RANGE;
        if (r1->status & R1_ADDRESS_ERROR)      return SD_ERROR_ADDRESS;
        if (r1->status & R1_BLOCK_LEN_ERROR)    return SD_ERROR_BLOCK_LEN;
        if (r1->status & R1_ERASE_SEQ_ERROR)    return SD_ERROR_ERASE_SEQ;
        if (r1->status & R1_ERASE_PARAM)        return SD_ERROR_ERASE_PARAM;
        if (r1->status & R1_WP_VIOLATION)       return SD_ERROR_WP_VIOLATION;
        //if (r1->status & R1_CARD_IS_LOCKED)     return SD_ERROR_CARD_IS_LOCKED;
        if (r1->status & R1_LOCK_UNLOCK_FAILED) return SD_ERROR_LOCK_UNLOCK_FAILED;
        if (r1->status & R1_COM_CRC_ERROR)      return SD_ERROR_COM_CRC;
        if (r1->status & R1_ILLEGAL_COMMAND)    return SD_ERROR_ILLEGAL_COMMAND;
        if (r1->status & R1_CARD_ECC_FAILED)    return SD_ERROR_CARD_ECC_FAILED;
        if (r1->status & R1_CC_ERROR)           return SD_ERROR_CC;
        if (r1->status & R1_ERROR)              return SD_ERROR_GENERAL;
        if (r1->status & R1_UNDERRUN)           return SD_ERROR_UNDERRUN;
        if (r1->status & R1_OVERRUN)            return SD_ERROR_OVERRUN;
        if (r1->status & R1_CID_CSD_OVERWRITE)  return SD_ERROR_CID_CSD_OVERWRITE;
    }

    if (buf[0] != request->cmd)
        return SD_ERROR_HEADER_MISMATCH;

    /* This should be last - it's the least dangerous error */

    return 0;
}

static int sd_unpack_r6(struct sd_request *request, struct sd_response_r1 *r1, unsigned long *rca)
{
    unsigned char *buf = request->response;

    if (request->result)
        return request->result;

    if (buf[0] != 0x03)
        return SD_ERROR_HEADER_MISMATCH;

    *rca = PARSE_U16(buf,1);  /* Save RCA returned by the SD Card */

    *(buf+1) = 0;
    *(buf+2) = 0;

    return sd_unpack_r1(request, r1);
}

static int sd_unpack_r3(struct sd_request *request, struct sd_response_r3 *r3)
{
    unsigned char *buf = request->response;

    if (request->result) return request->result;

    r3->ocr = PARSE_U32(buf,1);
    DEBUG("sd_unpack_r3: ocr=%08x", r3->ocr);

    if (buf[0] != 0x3f)
        return SD_ERROR_HEADER_MISMATCH;

    return 0;
}

/* Stop the MMC clock and wait while it happens */
static inline int jz_sd_stop_clock(void)
{
    register int timeout = 1000;

    //DEBUG("stop MMC clock");
    REG_MSC_STRPCL = MSC_STRPCL_CLOCK_CONTROL_STOP;

    while (timeout && (REG_MSC_STAT & MSC_STAT_CLK_EN))
    {
        timeout--;
        if (timeout == 0)
        {
            logf("Timeout on stop clock waiting");
            return SD_ERROR_TIMEOUT;
        }
        udelay(1);
    }
    //DEBUG("clock off time is %d microsec", timeout);
    return SD_NO_ERROR;
}

/* Start the MMC clock and operation */
static inline int jz_sd_start_clock(void)
{
    REG_MSC_STRPCL = MSC_STRPCL_CLOCK_CONTROL_START | MSC_STRPCL_START_OP;
    return SD_NO_ERROR;
}

static int jz_sd_check_status(struct sd_request *request)
{
    (void)request;
    unsigned int status = REG_MSC_STAT;

    /* Checking for response or data timeout */
    if (status & (MSC_STAT_TIME_OUT_RES | MSC_STAT_TIME_OUT_READ))
    {
        logf("SD timeout, MSC_STAT 0x%x CMD %d", status,
               request->cmd);
        return SD_ERROR_TIMEOUT;
    }

    /* Checking for CRC error */
    if (status &
        (MSC_STAT_CRC_READ_ERROR | MSC_STAT_CRC_WRITE_ERROR |
         MSC_STAT_CRC_RES_ERR))
    {
        logf("SD CRC error, MSC_STAT 0x%x", status);
        return SD_ERROR_CRC;

    }


    /* Checking for FIFO empty */
    /*if(status & MSC_STAT_DATA_FIFO_EMPTY && request->rtype != RESPONSE_NONE)
    {
        logf("SD FIFO empty, MSC_STAT 0x%x", status);
        return SD_ERROR_UNDERRUN;
    }*/

    return SD_NO_ERROR;
}

/* Obtain response to the command and store it to response buffer */
static void jz_sd_get_response(struct sd_request *request)
{
    int i;
    unsigned char *buf;
    unsigned int data;

    DEBUG("fetch response for request %d, cmd %d", request->rtype,
          request->cmd);
    buf = request->response;
    request->result = SD_NO_ERROR;

    switch (request->rtype) {
        case RESPONSE_R1:
        case RESPONSE_R1B:
        case RESPONSE_R3:
        case RESPONSE_R4:
        case RESPONSE_R5:
        case RESPONSE_R6:
        case RESPONSE_R7:
            data = REG_MSC_RES;
            buf[0] = (data >> 8) & 0xff;
            buf[1] = data & 0xff;
            data = REG_MSC_RES;
            buf[2] = (data >> 8) & 0xff;
            buf[3] = data & 0xff;
            data = REG_MSC_RES;
            buf[4] = data & 0xff;

            DEBUG("request %d, response [%02x %02x %02x %02x %02x]",
                  request->rtype, buf[0], buf[1], buf[2],
                  buf[3], buf[4]);
            break;
        case RESPONSE_R2:
            for (i = 0; i < 16; i += 2)
            {
                data = REG_MSC_RES;
                buf[i] = (data >> 8) & 0xff;
                buf[i + 1] = data & 0xff;
            }
            DEBUG("request %d, response []", request->rtype);
            break;
        case RESPONSE_NONE:
            DEBUG("No response");
            break;

        default:
            DEBUG("unhandled response type for request %d",
                  request->rtype);
            break;
    }
}

#ifdef SD_DMA_ENABLE
static void jz_sd_receive_data_dma(struct sd_request *req)
{
    unsigned int size = req->block_len * req->nob;
#if MMC_DMA_INTERRUPT
    unsigned char err = 0;
#endif

    /* flush dcache */
    discard_dcache_range(req->buffer, size);
    /* setup dma channel */
    REG_DMAC_DSAR(DMA_SD_RX_CHANNEL) = PHYSADDR(MSC_RXFIFO);    /* DMA source addr */
    REG_DMAC_DTAR(DMA_SD_RX_CHANNEL) = PHYSADDR((unsigned long) req->buffer);    /* DMA dest addr */
    REG_DMAC_DTCR(DMA_SD_RX_CHANNEL) = (size + 3) / 4;    /* DMA transfer count */
    REG_DMAC_DRSR(DMA_SD_RX_CHANNEL) = DMAC_DRSR_RS_MSCIN;    /* DMA request type */

#if SD_DMA_INTERRUPT
    REG_DMAC_DCMD(DMA_SD_RX_CHANNEL) =
        DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
        DMAC_DCMD_DS_32BIT | DMAC_DCMD_TIE;
    REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
    OSSemPend(sd_dma_rx_sem, 100, &err);
#else
    REG_DMAC_DCMD(DMA_SD_RX_CHANNEL) =
        DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
        DMAC_DCMD_DS_32BIT;
    REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;

    //while (REG_DMAC_DTCR(DMA_SD_RX_CHANNEL));
    while( !(REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL) & DMAC_DCCSR_TT) );
#endif

    /* clear status and disable channel */
    REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL) = 0;

    /* flush dcache again */
    discard_dcache_range(req->buffer, req->cnt);
}

static void jz_mmc_transmit_data_dma(struct mmc_request *req)
{
    unsigned int size = req->block_len * req->nob;
#if SD_DMA_INTERRUPT
    unsigned char err = 0;
#endif

    /* flush dcache */
    commit_discard_dcache_range(req->buffer, size);
    /* setup dma channel */
    REG_DMAC_DSAR(DMA_SD_TX_CHANNEL) = PHYSADDR((unsigned long) req->buffer);    /* DMA source addr */
    REG_DMAC_DTAR(DMA_SD_TX_CHANNEL) = PHYSADDR(MSC_TXFIFO);    /* DMA dest addr */
    REG_DMAC_DTCR(DMA_SD_TX_CHANNEL) = (size + 3) / 4;    /* DMA transfer count */
    REG_DMAC_DRSR(DMA_SD_TX_CHANNEL) = DMAC_DRSR_RS_MSCOUT;    /* DMA request type */

#if SD_DMA_INTERRUPT
    REG_DMAC_DCMD(DMA_SD_TX_CHANNEL) =
        DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
        DMAC_DCMD_DS_32BIT | DMAC_DCMD_TIE;
    REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
    OSSemPend(sd_dma_tx_sem, 100, &err);
#else
    REG_DMAC_DCMD(DMA_SD_TX_CHANNEL) =
        DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
        DMAC_DCMD_DS_32BIT;
    REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
    /* wait for dma completion */
    while( !(REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL) & DMAC_DCCSR_TT) );
#endif
    /* clear status and disable channel */

    REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL) = 0;
}

#else                /* SD_DMA_ENABLE */

static int jz_sd_receive_data(struct sd_request *req)
{
    unsigned int nob = req->nob;
    unsigned int wblocklen = (unsigned int) (req->block_len + 3) >> 2;    /* length in word */
    unsigned char *buf = req->buffer;
    unsigned int *wbuf = (unsigned int *) buf;
    unsigned int waligned = (((unsigned int) buf & 0x3) == 0);    /* word aligned ? */
    unsigned int stat, timeout, data, cnt;

#ifdef SD_DMA_ENABLE
    if (waligned)
        jz_sd_reveive_data_dma(request);
    return SD_NO_ERROR;
#endif

    for (; nob >= 1; nob--)
    {
        timeout = 0x3FFFFFF;

        while (timeout)
        {
            timeout--;
            stat = REG_MSC_STAT;

            if (stat & MSC_STAT_TIME_OUT_READ)
                return SD_ERROR_TIMEOUT;
            else if (stat & MSC_STAT_CRC_READ_ERROR)
                return SD_ERROR_CRC;
            else if (!(stat & MSC_STAT_DATA_FIFO_EMPTY)
                 || (stat & MSC_STAT_DATA_FIFO_AFULL))
                /* Ready to read data */
                break;

            udelay(1);
        }

        if (!timeout)
            return SD_ERROR_TIMEOUT;

        /* Read data from RXFIFO. It could be FULL or PARTIAL FULL */
        DEBUG("Receive Data = %d", wblocklen);
        cnt = wblocklen;
        while (cnt)
        {
            data = REG_MSC_RXFIFO;
            if (waligned)
                *wbuf++ = data;
            else
            {
                *buf++ = (unsigned char) (data >> 0);
                *buf++ = (unsigned char) (data >> 8);
                *buf++ = (unsigned char) (data >> 16);
                *buf++ = (unsigned char) (data >> 24);
            }
            cnt--;
            while (cnt
                   && (REG_MSC_STAT &
                   MSC_STAT_DATA_FIFO_EMPTY));
        }
    }

    return SD_NO_ERROR;
}

static int jz_sd_transmit_data(struct sd_request *req)
{
    unsigned int nob = req->nob;
    unsigned int wblocklen = (unsigned int) (req->block_len + 3) >> 2;    /* length in word */
    unsigned char *buf = req->buffer;
    unsigned int *wbuf = (unsigned int *) buf;
    unsigned int waligned = (((unsigned int) buf & 0x3) == 0);    /* word aligned ? */
    unsigned int stat, timeout, data, cnt;

#ifdef SD_DMA_ENABLE
    if (waligned)
        jz_sd_transmit_data_dma(request);
    return SD_NO_ERROR;
#endif

    for (; nob >= 1; nob--)
    {
        timeout = 0x3FFFFFF;

        while (timeout)
        {
            timeout--;
            stat = REG_MSC_STAT;

            if (stat &
                (MSC_STAT_CRC_WRITE_ERROR |
                 MSC_STAT_CRC_WRITE_ERROR_NOSTS))
                return SD_ERROR_CRC;
            else if (!(stat & MSC_STAT_DATA_FIFO_FULL))
                /* Ready to write data */
                break;

            udelay(1);
        }

        if (!timeout)
            return SD_ERROR_TIMEOUT;

        /* Write data to TXFIFO */
        cnt = wblocklen;
        while (cnt)
        {
            while (REG_MSC_STAT & MSC_STAT_DATA_FIFO_FULL);

            if (waligned)
                REG_MSC_TXFIFO = *wbuf++;
            else
            {
                data = *buf++;
                data |= *buf++ << 8;
                data |= *buf++ << 16;
                data |= *buf++ << 24;
                REG_MSC_TXFIFO = data;
            }

            cnt--;
        }
    }

    return SD_NO_ERROR;
}
#endif

static inline unsigned int jz_sd_calc_clkrt(unsigned int rate)
{
    unsigned int clkrt;
    unsigned int clk_src = card.sd2plus ? SD_CLOCK_HIGH : SD_CLOCK_FAST;

    clkrt = 0;
    while (rate < clk_src)
    {
        clkrt++;
        clk_src >>= 1;
    }
    return clkrt;
}

static inline void cpm_select_msc_clk(unsigned int rate)
{
    unsigned int div = __cpm_get_pllout2() / rate;
    if (div == 0)
	    div = 1;

    REG_CPM_MSCCDR = (div - 1);
}

/* Set the MMC clock frequency */
static void jz_sd_set_clock(unsigned int rate)
{
    int clkrt;

    /* select clock source from CPM */
    cpm_select_msc_clk(rate);

    REG_CPM_CPCCR |= CPM_CPCCR_CE;
    clkrt = jz_sd_calc_clkrt(rate);
    REG_MSC_CLKRT = clkrt;

    DEBUG("set clock to %u Hz clkrt=%d", rate, clkrt);
}

/********************************************************************************************************************
** Name:      int jz_sd_exec_cmd()
** Function:  send command to the card, and get a response
** Input:     struct sd_request *req: SD request
** Output:    0:  right        >0:  error code
********************************************************************************************************************/
static int jz_sd_exec_cmd(struct sd_request *request)
{
    unsigned int cmdat = 0;
    int retval, timeout = 0x3fffff;
    bool has_data = (request->buffer != NULL);

    /* Indicate we have no result yet */
    request->result = SD_NO_RESPONSE;

    /* Stop clock when programming things */
    jz_sd_stop_clock(); /* stop SD clock */

    /* Generic handling for all requests with data rx/tx */
    if (has_data) {
        cmdat |= MSC_CMDAT_DATA_EN;
//            if (request->nob > 1 && use_sbc)
//                cmdat |= MSC_CMDAT_SEND_AS_STOP;
#ifdef SD_DMA_ENABLE
        if (request->cnt >= 512)
            cmdat |= MSC_CMDAT_DMA_EN;
#endif
    }

    /* mask all interrupts */
    REG_MSC_IMASK = 0xffff;
    /* clear status */
    REG_MSC_IREG = 0xffff;
    /* unmask interrupts we care about */
    REG_MSC_IMASK = ~(MSC_IMASK_END_CMD_RES | MSC_IMASK_DATA_TRAN_DONE | MSC_IMASK_PRG_DONE);

    /* use 4-bit bus width when possible */
    if (use_4bit)
        cmdat |= MSC_CMDAT_BUS_WIDTH_4BIT;

    /* Per-command type handling */
    switch (request->cmd)
    {
            /* bc - broadcast - no response */
        case SD_GO_IDLE_STATE:
            cmdat |= MSC_CMDAT_INIT; /* Initialization sequence sent prior to command */
            /* Intentional Fallthrough */
        case SD_SET_DSR:
            break;

            /* bcr - broadcast with response */
        case SD_APP_OP_COND:
        case SD_ALL_SEND_CID:
        case SD_GO_IRQ_STATE:
            break;

            /* adtc - addressed with data transfer */
        case SD_READ_DAT_UNTIL_STOP:
        case SD_READ_SINGLE_BLOCK:
        case SD_READ_MULTIPLE_BLOCK:
        case SD_SEND_SCR:
        case SD_SWITCH_FUNC:
                /* These READ data */
            break;

        case SD_WRITE_DAT_UNTIL_STOP:
        case SD_WRITE_BLOCK:
        case SD_WRITE_MULTIPLE_BLOCK:
        case SD_PROGRAM_CID:
        case SD_PROGRAM_CSD:
                /* These WRITE data */
            cmdat |= MSC_CMDAT_WRITE;
            break;

//        case SD_LOCK_UNLOCK:  // can be a read or write, needs busy too.

        case SD_STOP_TRANSMISSION:
            // cmdat |= MSC_CMDAT_STOP_ABORT;
            break;

        /* ac - no data transfer */
        default:
            break;
    }

    /* Set response type */
    switch (request->rtype)
    {
        case RESPONSE_NONE:
            break;
        case RESPONSE_R1B:
            cmdat |= MSC_CMDAT_BUSY;
            /* FALLTHRU */
        case RESPONSE_R1:
            cmdat |= MSC_CMDAT_RESPONSE_R1;
            break;
        case RESPONSE_R2:
            cmdat |= MSC_CMDAT_RESPONSE_R2;
            break;
        case RESPONSE_R3:
            cmdat |= MSC_CMDAT_RESPONSE_R3;
            break;
        case RESPONSE_R4:
            cmdat |= MSC_CMDAT_RESPONSE_R4;
            break;
        case RESPONSE_R5:
            cmdat |= MSC_CMDAT_RESPONSE_R5;
            break;
        case RESPONSE_R6:
            cmdat |= MSC_CMDAT_RESPONSE_R6;
            break;
        case RESPONSE_R7:
            cmdat |= MSC_CMDAT_RESPONSE_R1;  /* 4740 lacks R7 */
            break;
        default:
            break;
    }

    /* Set command index */
    REG_MSC_CMD = request->cmd;

    /* Set argument */
    REG_MSC_ARG = request->arg;

    /* Set block length and nob if there is data */
    if (has_data) {
        REG_MSC_BLKLEN = request->block_len;
        REG_MSC_NOB = request->nob;
    }

    /* Set command */
    REG_MSC_CMDAT = cmdat;

    DEBUG("Send cmd %d cmdat: %x arg: %x resp %d", request->cmd,
          cmdat, request->arg, request->rtype);

    /* Start SD clock and send command to card */
    jz_sd_start_clock();

    /* Wait for command completion */
    //semaphore_wait(&sd_wakeup, 100);
    while (timeout-- && !(REG_MSC_STAT & MSC_STAT_END_CMD_RES));

    if (timeout == 0)
        return SD_ERROR_TIMEOUT;

    /* Check for status */
    retval = jz_sd_check_status(request);
    REG_MSC_IREG = MSC_IREG_END_CMD_RES;    /* clear flag */
    if (retval)
        return retval;

    /* Complete command with no response */
    if (request->rtype == RESPONSE_NONE)
        return SD_NO_ERROR;

    /* Get response */
    jz_sd_get_response(request);

    /* Start data operation */
    if (has_data) {
        if (cmdat & MSC_CMDAT_WRITE) {
            retval = jz_sd_transmit_data(request);
        } else {
            retval = jz_sd_receive_data(request);
        }
        if (retval)
            return retval;

        //semaphore_wait(&sd_wakeup, 100);
        /* Wait for Data Done */
        while (!(REG_MSC_IREG & MSC_IREG_DATA_TRAN_DONE))
            yield();
        REG_MSC_IREG = MSC_IREG_DATA_TRAN_DONE;    /* clear status */
    }

    /* Wait for Prog Done event if necessary */
    if (cmdat & (MSC_CMDAT_BUSY | MSC_CMDAT_WRITE))
    {
        //semaphore_wait(&sd_wakeup, 100);
        while (!(REG_MSC_IREG & MSC_IREG_PRG_DONE))
            yield();
        REG_MSC_IREG = MSC_IREG_PRG_DONE;    /* clear status */
    }

    /* Command completed */
    jz_sd_stop_clock();  /* Stop SD clock since we're done */

    return SD_NO_ERROR;    /* return successfully */
}

/*******************************************************************************************************************
** Name:      int sd_chkcard()
** Function:  check whether card is insert entirely
** Input:     NULL
** Output:    1: insert entirely    0: not insert entirely
********************************************************************************************************************/
static int jz_sd_chkcard(void)
{
    return (SD_INSERT_STATUS() == 0 ? 1 : 0);
}

#if SD_DMA_INTERRUPT
static void jz_sd_tx_handler(unsigned int arg)
{
    if (__dmac_channel_address_error_detected(arg))
    {
        logf("%s: DMAC address error.", __FUNCTION__);
        __dmac_channel_clear_address_error(arg);
    }
    if (__dmac_channel_transmit_end_detected(arg))
    {
        __dmac_channel_clear_transmit_end(arg);
        OSSemPost(sd_dma_tx_sem);
    }
}

static void jz_sd_rx_handler(unsigned int arg)
{
    if (__dmac_channel_address_error_detected(arg))
    {
        logf("%s: DMAC address error.", __FUNCTION__);
        __dmac_channel_clear_address_error(arg);
    }
    if (__dmac_channel_transmit_end_detected(arg))
    {
        __dmac_channel_clear_transmit_end(arg);
        OSSemPost(sd_dma_rx_sem);
    }
}
#endif

/* MSC interrupt handler */
void MSC(void)
{
    //semaphore_release(&sd_wakeup);
    logf("MSC interrupt");
}

static void sd_gpio_setup_irq(bool inserted)
{
    if(inserted)
        __gpio_as_irq_rise_edge(MMC_CD_PIN);
    else
        __gpio_as_irq_fall_edge(MMC_CD_PIN);
}

/*******************************************************************************************************************
** Name:      void sd_hardware_init()
** Function:  initialize the hardware condiction that access sd card
** Input:     NULL
** Output:    NULL
********************************************************************************************************************/
static void jz_sd_hardware_init(void)
{
    __cpm_start_msc();   /* enable mmc clock */
    sd_init_gpio();     /* init GPIO */
    sd_gpio_setup_irq(jz_sd_chkcard());
#ifdef SD_POWER_ON
    SD_POWER_ON();      /* turn on power of card */
#endif
    SD_RESET();         /* reset mmc/sd controller */
    SD_IRQ_MASK();      /* mask all IRQs */
    jz_sd_stop_clock(); /* stop SD clock */
    jz_sd_set_clock(MMC_CLOCK_SLOW); /* Drop to lowest speed */
#ifdef SD_DMA_ENABLE
//    __cpm_start_dmac();
//    __dmac_enable_module();
//      REG_DMAC_DMACR = DMAC_DMACR_DME;
#if SD_DMA_INTERRUPT
    sd_dma_rx_sem = OSSemCreate(0);
    sd_dma_tx_sem = OSSemCreate(0);
    request_irq(IRQ_DMA_0 + RX_DMA_CHANNEL, jz_sd_rx_handler,
            RX_DMA_CHANNEL);
    request_irq(IRQ_DMA_0 + TX_DMA_CHANNEL, jz_sd_tx_handler,
            TX_DMA_CHANNEL);
#endif
#endif
}

static void sd_send_cmd(struct sd_request *request, int cmd, unsigned int arg,
                         unsigned short nob, unsigned short block_len,
                         enum sd_rsp_t rtype, unsigned char* buffer)
{
    int retval;

    request->cmd = cmd;
    request->arg = arg;
    request->rtype = rtype;
    request->nob = nob;
    request->block_len = block_len;
    request->buffer = buffer;
    request->cnt = nob * block_len;
    memset(&request->response, 0xa5, sizeof(request->response));
    request->result = SD_NO_RESPONSE;

    retval = jz_sd_exec_cmd(request);
    if (retval)
        request->result = retval;

}

static void sd_simple_cmd(struct sd_request *request, int cmd, unsigned int arg,
                           enum sd_rsp_t rtype)
{
    sd_send_cmd(request, cmd, arg, 0, 0, rtype, NULL);
}

static int sd_exec_acmd(struct sd_request *request, int cmd, unsigned int arg)
{
    struct sd_response_r1 r1;
    int retval;

    sd_simple_cmd(request, SD_APP_CMD, card.rca, RESPONSE_R1);
    retval = sd_unpack_r1(request, &r1);

    if (retval)
        return retval;

    if (!(r1.status & SD_R1_APP_CMD)) {
        logf("ACMD not accepted\n");
        return SD_ERROR_ACMD_REJECT;
    }

    sd_simple_cmd(request, cmd, arg, RESPONSE_R1);
    return sd_unpack_r1(request, &r1);
}

static int sd_exec_acmd_buf(struct sd_request *request, int cmd, unsigned int arg, enum sd_rsp_t rtype, unsigned short nob, unsigned short block_len, unsigned char *buffer)
{
    struct sd_response_r1 r1;
    int retval;

    sd_simple_cmd(request, SD_APP_CMD, card.rca, RESPONSE_R1);
    retval = sd_unpack_r1(request, &r1);
    if (retval)
        return retval;

    if (!(r1.status & SD_R1_APP_CMD)) {
        logf("ACMD not accepted\n");
        return SD_ERROR_ACMD_REJECT;
    }

    sd_send_cmd(request, cmd, arg,
                nob, block_len, rtype, buffer);
    return SD_NO_ERROR;
}

#define SD_INIT_DOING   0
#define SD_INIT_PASSED  1
#define SD_INIT_FAILED  2
static int sd_init_card_state(struct sd_request *request)
{
    struct sd_response_r1 r1;
    struct sd_response_r3 r3;
    int retval, i, ocr = 0x40300000 /* SDXC, 3.2-3.4v */;

#ifdef HAVE_SDUC
    ocr |= 0x08000000; /* Add on SDUC */
#endif

    switch (request->cmd)
    {
        case SD_GO_IDLE_STATE: /* No response to parse */
            sd_simple_cmd(request, SD_SEND_IF_COND, 0x1AA, RESPONSE_R1);
            break;
        case SD_SEND_IF_COND:
            retval = sd_unpack_r1(request, &r1);
//            if (retval)
//                return SD_INIT_FAILED;

            retval = sd_exec_acmd_buf(request, SD_APP_OP_COND, ocr,
                                      RESPONSE_R3, 0, 0, NULL);
            if (retval)
                return SD_INIT_FAILED;
            break;
        case SD_APP_OP_COND:
            retval = sd_unpack_r3(request, &r3);
            if (retval)
                return SD_INIT_FAILED;

            DEBUG("read ocr value = 0x%08x", r3.ocr);
            card.ocr = r3.ocr;

            // XXX for SDUC, check that B27+B30 of OCR are set.
            if(!(r3.ocr & SD_CARD_BUSY || ocr == 0))
            {
                /* Re-issue AP_OP_COND */
                sleep(HZ / 100);
                retval = sd_exec_acmd_buf(request, SD_APP_OP_COND, ocr,
                                          RESPONSE_R3, 0, 0, NULL);
                if (retval)
                    return SD_INIT_FAILED;
            }
            else
            {
                sd_simple_cmd(request, SD_ALL_SEND_CID, 0, RESPONSE_R2);
            }
            break;
        case SD_ALL_SEND_CID:
            for(i=0; i<4; i++)
                card.cid[i] = ((request->response[1+i*4]<<24) | (request->response[2+i*4]<<16) |
                               (request->response[3+i*4]<< 8) | request->response[4+i*4]);

            logf("CID: %08lx%08lx%08lx%08lx", card.cid[0], card.cid[1], card.cid[2], card.cid[3]);
            sd_simple_cmd(request, SD_SEND_RELATIVE_ADDR, 0, RESPONSE_R6);
            break;
        case SD_SEND_RELATIVE_ADDR:
            retval = sd_unpack_r6(request, &r1, &card.rca);
            DEBUG("sd_init_card_state: Get RCA from SD: 0x%04lx Status: %x", card.rca, r1.status);
            card.rca = card.rca << 16;
            if (retval)
            {
                logf("sd_init_card_state: unable to SET_RELATIVE_ADDR error=%d",
                      retval);
                return SD_INIT_FAILED;
            }

            sd_simple_cmd(request, SD_SEND_CSD, card.rca, RESPONSE_R2);
            break;

        case SD_SEND_CSD:
            for(i=0; i<4; i++)
                card.csd[i] = ((request->response[1+i*4]<<24) | (request->response[2+i*4]<<16) |
                               (request->response[3+i*4]<< 8) | request->response[4+i*4]);

            sd_parse_csd(&card);

            logf("CSD: %08lx%08lx%08lx%08lx", card.csd[0], card.csd[1], card.csd[2], card.csd[3]);

#if 0  // XXX SCR is not working yet
            retval = sd_exec_acmd_buf(request, SD_SEND_SCR, 0, RESPONSE_R1,
                                      1, 8, &request->response[5]);
            if (!retval)
                retval = sd_unpack_r1(request, &r1);
            // XXX check retval?
            break;
        case SD_SEND_SCR:
//            if (request->result)
//                return SD_INIT_FAILED;
            for(i=0; i<2; i++)
                card.scr[i] = ((request->buffer[0+i*4]<<24) | (request->buffer[1+i*4]<<16) |
                               (request->buffer[2+i*4]<< 8) | request->buffer[3+i*4]);

            logf("SCR: %08lx%08lx", card.scr[0], card.scr[1]);

            use_sbc = card.scr[0] & 2;
#endif
            DEBUG("SD card is ready");
            jz_sd_set_clock(SD_CLOCK_FAST);
            return SD_INIT_PASSED;

        default:
            logf("sd_init_card_state: error!  Illegal last cmd %d", request->cmd);
            return SD_INIT_FAILED;
    }

    return SD_INIT_DOING;
}

static int sd_switch(struct sd_request *request, int mode, int group,
              unsigned char value, unsigned char * resp)
{
    unsigned int arg;

    mode = !!mode;
    value &= 0xF;
    arg = (mode << 31 | 0x00FFFFFF);
    if (mode) {
        group--;
        arg &= ~(0xF << (group * 4));
        arg |= value << (group * 4);
    }
    sd_send_cmd(request, 6, arg, 1, 64, RESPONSE_R1, resp);

    return 0;
}

/*
 * Fetches and decodes switch information
 */
static int sd_read_switch(struct sd_request *request)
{
    unsigned int status[64 / 4];

    memset((unsigned char *)status, 0, 64);
    sd_switch(request, 0, 0, 1, (unsigned char*) status);

    if (((unsigned char *)status)[13] & 0x02)
        return 0;
    else
        return 1;
}

/*
 * Test if the card supports high-speed mode and, if so, switch to it.
 */
static int sd_switch_hs(struct sd_request *request)
{
    unsigned int status[64 / 4];

    sd_switch(request, 1, 1, 1, (unsigned char*) status);
    return 0;
}

static int sd_select_card(void)
{
    struct sd_request request;
    struct sd_response_r1 r1;
    int retval;

    sd_simple_cmd(&request, SD_SELECT_CARD, card.rca,
               RESPONSE_R1B);
    retval = sd_unpack_r1(&request, &r1);
    if (retval)
        return retval;

    if (card.sd2plus)
    {
        retval = sd_read_switch(&request);
        if (!retval)
        {
            sd_switch_hs(&request);
            jz_sd_set_clock(SD_CLOCK_HIGH);
        }
    }

    bool sup_4bit = 1; //  XXX check against SCR

    if (sup_4bit) {
        retval = sd_exec_acmd(&request, SD_SET_BUS_WIDTH, 2);
        if (retval)
            return retval;
        use_4bit = 2;
        DEBUG("Use 4-bit bus width");
    }
    retval = sd_exec_acmd(&request, SD_SET_CLR_CARD_DETECT, 0);
    if (retval)
        return retval;

    card.initialized = 1;

    return 0;
}

static inline bool card_detect_target(void)
{
    return (jz_sd_chkcard() == 1);
}

static int __sd_init_device(void)
{
    int retval;
    struct sd_request init_req;

    /* Initialise card data as blank */
    memset(&card, 0, sizeof(tCardInfo));

    use_4bit = 0;
    use_sbc = 0;

    /* reset mmc/sd controller */
    jz_sd_hardware_init();

    if (!card_detect_target())
        return 0;

    sd_simple_cmd(&init_req, SD_GO_IDLE_STATE, 0,     RESPONSE_NONE);

    sleep(HZ/2); /* Give the card/controller some rest */

    while((retval = sd_init_card_state(&init_req)) == SD_INIT_DOING);
    retval = (retval == SD_INIT_PASSED ? sd_select_card() : -1);

    __cpm_stop_msc(); /* disable SD clock */

    return retval;
}

int sd_init(void)
{
    static bool inited = false;
    if(!inited)
    {
//        semaphore_init(&sd_wakeup, 1, 0);
        //__intc_unmask_irq(IRQ_MSC);
        mutex_init(&sd_mtx);
        inited = true;
    }

    mutex_lock(&sd_mtx);
    int ret = __sd_init_device();
    mutex_unlock(&sd_mtx);

    return ret;
}

tCardInfo* card_get_info_target(int card_no)
{
    (void)card_no;
    return &card;
}

static inline void sd_start_transfer(void)
{
    mutex_lock(&sd_mtx);
    __cpm_start_msc();
    led(true);
}

static inline void sd_stop_transfer(void)
{
    led(false);
    __cpm_stop_msc();
    mutex_unlock(&sd_mtx);
}

int sd_transfer_sectors(IF_MD(int drive,) sector_t start, int count, void* buf, bool write)
{
#ifdef HAVE_MULTIDRIVE
    (void)drive;
#endif
    sd_start_transfer();

    struct sd_request request;
    struct sd_response_r1 r1;
    int retval = -1;

    if (!card_detect_target() || count == 0 || start > card.numblocks)
        goto err;

    if(card.initialized == 0 && !__sd_init_device())
        goto err;

    sd_simple_cmd(&request, SD_SEND_STATUS, card.rca, RESPONSE_R1);
    retval = sd_unpack_r1(&request, &r1);
    if (retval && (retval != SD_ERROR_STATE_MISMATCH))
        goto err;

    sd_simple_cmd(&request, SD_SET_BLOCKLEN, SD_BLOCK_SIZE, RESPONSE_R1);
    if ((retval = sd_unpack_r1(&request, &r1)))
        goto err;

    if (use_sbc && count > 1) {
        sd_simple_cmd(&request, SD_SET_BLOCK_COUNT, count, RESPONSE_R1);
        if ((retval = sd_unpack_r1(&request, &r1)))
            goto err;
    }

#ifdef STORAGE_64BIT_SECTOR
    // XXX check card[drive].ocr, b27+b30 == 11
    if (card.numblocks > 0xffffffffULL) {
        uint32_t upper = start >> 32;
        sd_simple_cmd(&request, SD_UC_ADDRESS_EXTENSION, upper, RESPONSE_R1);
        if ((retval = sd_unpack_r1(&request, &r1)))
            goto err;
    }
#endif

    sd_send_cmd(&request,
                (count > 1) ?
                (write ? SD_WRITE_MULTIPLE_BLOCK : SD_READ_MULTIPLE_BLOCK) :
                (write ? SD_WRITE_BLOCK : SD_READ_SINGLE_BLOCK),
                card.sd2plus ? start : (start * SD_BLOCK_SIZE),
                count, SD_BLOCK_SIZE, RESPONSE_R1, buf);
    retval = sd_unpack_r1(&request, &r1);

    if (retval || (!use_sbc && count > 1)) {
        sd_simple_cmd(&request, SD_STOP_TRANSMISSION, 0, RESPONSE_R1B);
        if ((retval = sd_unpack_r1(&request, &r1)))
            goto err;
    }

err:
    last_disk_activity = current_tick;
    sd_stop_transfer();

    return retval;
}

int sd_read_sectors(IF_MD(int drive,) sector_t start, int count, void* buf)
{
  return sd_transfer_sectors(IF_MD(drive,) start, count, buf, false);
}

int sd_write_sectors(IF_MD(int drive,) sector_t start, int count, const void* buf)
{
  return sd_transfer_sectors(IF_MD(drive,) start, count, (void*)buf, true);
}

long sd_last_disk_activity(void)
{
    return last_disk_activity;
}

int sd_spinup_time(void)
{
    return 0;
}

void sd_enable(bool on)
{
    (void)on;
}

bool sd_disk_is_active(void)
{
    return false;
}

int sd_soft_reset(void)
{
    return 0;
}

#ifdef HAVE_HOTSWAP
bool sd_removable(IF_MD_NONVOID(int drive))
{
#ifdef HAVE_MULTIDRIVE
    (void)drive;
#endif
    return true;
}

static int sd_oneshot_callback(struct timeout *tmo)
{
    int state = card_detect_target();

    /* This is called only if the state was stable for 300ms - check state
     * and post appropriate event. */
    queue_broadcast(state ? SYS_HOTSWAP_INSERTED : SYS_HOTSWAP_EXTRACTED,
                    sd_drive_nr);

    sd_gpio_setup_irq(state);
    return 0;
    (void)tmo;
}

/* called on insertion/removal interrupt */
void MMC_CD_IRQ(void)
{
    static struct timeout sd_oneshot;
    timeout_register(&sd_oneshot, sd_oneshot_callback, (3*HZ/10), 0);
}
#endif

bool sd_present(IF_MD_NONVOID(int drive))
{
#ifdef HAVE_MULTIDRIVE
    (void)drive;
#endif
    return card_detect_target();
}

#ifdef CONFIG_STORAGE_MULTI
int sd_num_drives(int first_drive)
{
    sd_drive_nr = first_drive;
    return 1;
}
#endif

int sd_event(long id, intptr_t data)
{
    int rc = 0;

    switch (id)
    {
#ifdef HAVE_HOTSWAP
    case SYS_HOTSWAP_INSERTED:
    case SYS_HOTSWAP_EXTRACTED:
        mutex_lock(&sd_mtx); /* lock-out card activity */
        /* Force card init for new card, re-init for re-inserted one or
         * clear if the last attempt to init failed with an error. */
        card.initialized = 0;
        mutex_unlock(&sd_mtx);
        break;
#endif /* HAVE_HOTSWAP */
    default:
        rc = storage_event_default_handler(id, data, last_disk_activity,
                                           STORAGE_SD);
        break;
    }

    return rc;
}
