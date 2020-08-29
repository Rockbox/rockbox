/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Roman Stolyarov
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
#include "dma-target.h"
#include "led.h"
#include "sdmmc.h"
#include "logf.h"
#include "storage.h"
#include "string.h"
#include "panic.h"

#define SD_INTERRUPT 0   // COMPLETELY BROKEN!
#define SD_DMA_ENABLE 1
#define SD_DMA_INTERRUPT 1  // HANGS RANDOMLY!
#define SD_AUTO_CLOCK 1

#if NUM_DRIVES > 2
#error "JZ4760 SD driver supports NUM_DRIVES <= 2 only"
#endif

static long               last_disk_activity = -1;
static tCardInfo          card[NUM_DRIVES];
static char               active[NUM_DRIVES];

#if defined(CONFIG_STORAGE_MULTI) || defined(HAVE_HOTSWAP)
static int                sd_drive_nr = 0;
#else
#define                   sd_drive_nr 0
#endif

static struct mutex       sd_mtx[NUM_DRIVES];
#if SD_DMA_INTERRUPT || SD_INTERRUPT
static struct semaphore   sd_wakeup[NUM_DRIVES];
#endif

static int                use_4bit[NUM_DRIVES];
static int                num_6[NUM_DRIVES];
static int                sd2_0[NUM_DRIVES];


//#define DEBUG(x...)         logf(x)
#define DEBUG(x, ...)

/* volumes */
#define SD_SLOT_1  0       /* SD card 1 */
#define SD_SLOT_2  1       /* SD card 2 */

#define MSC_CHN(n) (2-n)

#define SD_IRQ_MASK(n)                \
do {                                  \
          REG_MSC_IMASK(n) = 0xffff;  \
          REG_MSC_IREG(n) = 0xffff;   \
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
};

/* Standard MMC/SD clock speeds */
#define MMC_CLOCK_SLOW    400000      /* 400 kHz for initial setup */
#define SD_CLOCK_FAST   24000000      /* 24 MHz for SD Cards */
#define SD_CLOCK_HIGH   48000000      /* 48 MHz for SD Cards */

/* Extra commands for state control */
/* Use negative numbers to disambiguate */
#define SD_CIM_RESET            -1

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

/* Don't change the order of these; they are used in dispatch tables */
enum sd_rsp_t
{
    RESPONSE_NONE    = 0,
    RESPONSE_R1      = 1,
    RESPONSE_R1B     = 2,
    RESPONSE_R2_CID  = 3,
    RESPONSE_R2_CSD  = 4,
    RESPONSE_R3      = 5,
    RESPONSE_R4      = 6,
    RESPONSE_R5      = 7,
    RESPONSE_R6      = 8,
    RESPONSE_R7      = 9,
};

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
    int               index;      /* Slot index - used for CS lines */
    int               cmd;        /* Command to send */
    unsigned int      arg;        /* Argument to send */
    enum sd_rsp_t    rtype;      /* Response type expected */

    /* Data transfer (these may be modified at the low level) */
    unsigned short    nob;        /* Number of blocks to transfer*/
    unsigned short    block_len;  /* Block length */
    unsigned char     *buffer;    /* Data buffer */
    unsigned int      cnt;        /* Data length, for PIO */

    /* Results */
    unsigned char     response[18]; /* Buffer to store response - CRC is optional */
    enum sd_result_t result;
};

#define SD_OCR_ARG             0x00ff8000  /* Argument of OCR */

/***********************************************************************
 *  SD Events
 */
#define SD_EVENT_NONE            0x00    /* No events */
#define SD_EVENT_RX_DATA_DONE    0x01    /* Rx data done */
#define SD_EVENT_TX_DATA_DONE    0x02    /* Tx data done */
#define SD_EVENT_PROG_DONE       0x04    /* Programming is done */

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

    if (SD_R1_STATUS(r1->status)) {
        if (r1->status & SD_R1_OUT_OF_RANGE)       return SD_ERROR_OUT_OF_RANGE;
        if (r1->status & SD_R1_ADDRESS_ERROR)      return SD_ERROR_ADDRESS;
        if (r1->status & SD_R1_BLOCK_LEN_ERROR)    return SD_ERROR_BLOCK_LEN;
        if (r1->status & SD_R1_ERASE_SEQ_ERROR)    return SD_ERROR_ERASE_SEQ;
        if (r1->status & SD_R1_ERASE_PARAM)        return SD_ERROR_ERASE_PARAM;
        if (r1->status & SD_R1_WP_VIOLATION)       return SD_ERROR_WP_VIOLATION;
        //if (r1->status & SD_R1_CARD_IS_LOCKED)     return SD_ERROR_CARD_IS_LOCKED;
        if (r1->status & SD_R1_LOCK_UNLOCK_FAILED) return SD_ERROR_LOCK_UNLOCK_FAILED;
        if (r1->status & SD_R1_COM_CRC_ERROR)      return SD_ERROR_COM_CRC;
        if (r1->status & SD_R1_ILLEGAL_COMMAND)    return SD_ERROR_ILLEGAL_COMMAND;
        if (r1->status & SD_R1_CARD_ECC_FAILED)    return SD_ERROR_CARD_ECC_FAILED;
        if (r1->status & SD_R1_CC_ERROR)           return SD_ERROR_CC;
        if (r1->status & SD_R1_ERROR)              return SD_ERROR_GENERAL;
        if (r1->status & SD_R1_UNDERRUN)           return SD_ERROR_UNDERRUN;
        if (r1->status & SD_R1_OVERRUN)            return SD_ERROR_OVERRUN;
        if (r1->status & SD_R1_CSD_OVERWRITE)      return SD_ERROR_CID_CSD_OVERWRITE;
    }

    if (buf[0] != request->cmd)
        return SD_ERROR_HEADER_MISMATCH;

    /* This should be last - it's the least dangerous error */

    return SD_NO_ERROR;
}

static int sd_unpack_r6(struct sd_request *request, struct sd_response_r1 *r1, unsigned long *rca)
{
    unsigned char *buf = request->response;

    if (request->result)
        return request->result;

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

    return SD_NO_ERROR;
}

/* Stop the MMC clock and wait while it happens */
static inline int jz_sd_stop_clock(const int drive)
{
    register int timeout = 1000;

    //DEBUG("stop MMC clock");
#if SD_AUTO_CLOCK
    REG_MSC_LPM(drive) = 0; /* disable auto clock stop */
#endif

    /* only stop if necessary */
    if (!(REG_MSC_STAT(MSC_CHN(drive)) & MSC_STAT_CLK_EN))
       return SD_NO_ERROR;

    REG_MSC_STRPCL(MSC_CHN(drive)) = MSC_STRPCL_CLOCK_CONTROL_STOP;

    while (timeout && (REG_MSC_STAT(MSC_CHN(drive)) & MSC_STAT_CLK_EN))
    {
        timeout--;
        if (timeout == 0)
        {
            DEBUG("Timeout on stop clock waiting");
            return SD_ERROR_TIMEOUT;
        }
        udelay(1);
    }

    //DEBUG("clock off time is %d microsec", timeout);
    return SD_NO_ERROR;
}

/* Start the MMC clock and operation */
static inline int jz_sd_start_clock(const int drive)
{
    int reg = MSC_STRPCL_START_OP;
#if !SD_AUTO_CLOCK
    reg |= (REG_MSC_STAT(MSC_CHN(drive)) & MSC_STAT_CLK_EN) ? 0 : MSC_STRPCL_CLOCK_CONTROL_START;
#endif
    REG_MSC_STRPCL(MSC_CHN(drive)) = reg;
    return SD_NO_ERROR;
}

static int jz_sd_check_status(const int drive, struct sd_request *request)
{
    (void)request;
    unsigned int status = REG_MSC_STAT(MSC_CHN(drive));

    /* Checking for response or data timeout */
    if (status & (MSC_STAT_TIME_OUT_RES | MSC_STAT_TIME_OUT_READ))
    {
        DEBUG("SD timeout, MSC_STAT 0x%x CMD %d", status,
               request->cmd);
        return SD_ERROR_TIMEOUT;
    }

    /* Checking for CRC error */
    if (status &
        (MSC_STAT_CRC_READ_ERROR | MSC_STAT_CRC_WRITE_ERROR |
         MSC_STAT_CRC_RES_ERR))
    {
        DEBUG("SD CRC error, MSC_STAT 0x%x", status);
        return SD_ERROR_CRC;
    }

    /* Checking for FIFO empty */
    /*if(status & MSC_STAT_DATA_FIFO_EMPTY && request->rtype != RESPONSE_NONE)
    {
        DEBUG("SD FIFO empty, MSC_STAT 0x%x", status);
        return SD_ERROR_UNDERRUN;
    }*/

    return SD_NO_ERROR;
}

/* Obtain response to the command and store it to response buffer */
static void jz_sd_get_response(const int drive, struct sd_request *request)
{
    int i;
    unsigned char *buf;
    unsigned short data;

    if (request->result != SD_NO_RESPONSE)
        return;

    DEBUG("fetch response for request %d, cmd %d", request->rtype,
          request->cmd);
    buf = request->response;
    request->result = SD_NO_ERROR;

    switch (request->rtype)
    {
        case RESPONSE_R1:
        case RESPONSE_R1B:
        case RESPONSE_R7:
        case RESPONSE_R6:
        case RESPONSE_R3:
        case RESPONSE_R4:
        case RESPONSE_R5:
        {
            data = REG_MSC_RES(MSC_CHN(drive));
            buf[0] = (data >> 8) & 0xff;
            buf[1] = data & 0xff;
            data = REG_MSC_RES(MSC_CHN(drive));
            buf[2] = (data >> 8) & 0xff;
            buf[3] = data & 0xff;
            data = REG_MSC_RES(MSC_CHN(drive));
            buf[4] = data & 0xff;

            DEBUG("request %d, response [%02x %02x %02x %02x %02x]",
                  request->rtype, buf[0], buf[1], buf[2],
                  buf[3], buf[4]);
            break;
        }
        case RESPONSE_R2_CID:
        case RESPONSE_R2_CSD:
        {
            for (i = 0; i < 16; i += 2)
            {
                data = REG_MSC_RES(MSC_CHN(drive));
                buf[i] = (data >> 8) & 0xff;
                buf[i + 1] = data & 0xff;
            }
            DEBUG("request %d, response []", request->rtype);
            break;
        }
        case RESPONSE_NONE:
            DEBUG("No response");
            break;

        default:
            DEBUG("unhandled response type for request %d",
                  request->rtype);
            break;
    }
}

#if SD_DMA_ENABLE
static int jz_sd_transmit_data_dma(const int drive, struct sd_request *req);
static int jz_sd_receive_data_dma(const int drive, struct sd_request *req);
#endif

static int jz_sd_receive_data(const int drive, struct sd_request *req)
{
    unsigned int nob = req->nob;
    unsigned int wblocklen = (unsigned int) (req->block_len + 3) >> 2;    /* length in word */
    unsigned char *buf = req->buffer;
    unsigned int *wbuf = (unsigned int *) buf;
    unsigned int waligned = (((unsigned int) buf & 0x3) == 0);    /* word aligned ? */
    unsigned int stat, data, cnt;

#if SD_DMA_ENABLE
    /* Use DMA if we can */
    if ((int)req->buffer & 0x3 == 0)
        return jz_sd_receive_data_dma(drive, req);
#endif

    for (; nob >= 1; nob--)
    {
        long deadline = current_tick + (HZ * 65);

        do {
            stat = REG_MSC_STAT(MSC_CHN(drive));

            if (stat & MSC_STAT_TIME_OUT_READ)
                return SD_ERROR_TIMEOUT;
            else if (stat & MSC_STAT_CRC_READ_ERROR)
                return SD_ERROR_CRC;
            else if ((stat & MSC_STAT_DATA_FIFO_AFULL) ||
                     !(stat & MSC_STAT_DATA_FIFO_EMPTY))
                /* Ready to read data */
                break;

            yield();
        } while (TIME_BEFORE(current_tick, deadline));

        /* Read data from RXFIFO. It could be FULL or PARTIAL FULL */
        DEBUG("Receive Data = %d", wblocklen);
        cnt = wblocklen;
        while (cnt)
        {
            if (REG_MSC_STAT(MSC_CHN(drive)) & MSC_STAT_DATA_FIFO_EMPTY)
            {
                if (TIME_AFTER(current_tick, deadline))
                    return SD_ERROR_TIMEOUT;
                continue;
            }

            data = REG_MSC_RXFIFO(MSC_CHN(drive));
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
        }
    }

    return SD_NO_ERROR;
}

static int jz_sd_transmit_data(const int drive, struct sd_request *req)
{
    unsigned int nob = req->nob;
    unsigned int wblocklen = (unsigned int) (req->block_len + 3) >> 2;    /* length in word */
    unsigned char *buf = req->buffer;
    unsigned int *wbuf = (unsigned int *) buf;
    unsigned int waligned = (((unsigned int) buf & 0x3) == 0);    /* word aligned ? */
    unsigned int stat, data, cnt;

#if SD_DMA_ENABLE
    /* Use DMA if we can */
    if ((int)req->buffer & 0x3 == 0)
        return jz_sd_transmit_data_dma(drive, req);
#endif

    for (; nob >= 1; nob--)
    {
        long deadline = current_tick + (HZ * 65);

        do {
            stat = REG_MSC_STAT(MSC_CHN(drive));

            if (stat &
                (MSC_STAT_CRC_WRITE_ERROR |
                 MSC_STAT_CRC_WRITE_ERROR_NOSTS))
                return SD_ERROR_CRC;
            else if (!(stat & MSC_STAT_DATA_FIFO_FULL))
                /* Ready to write data */
                break;

            yield();
        } while (TIME_BEFORE(current_tick, deadline));

        /* Write data to TXFIFO */
        cnt = wblocklen;
        while (cnt)
        {
            if (REG_MSC_STAT(MSC_CHN(drive)) & MSC_STAT_DATA_FIFO_FULL)
            {
                if (TIME_AFTER(current_tick, deadline))
                    return SD_ERROR_TIMEOUT;
                continue;
            }

            if (waligned)
                REG_MSC_TXFIFO(MSC_CHN(drive)) = *wbuf++;
            else
            {
                data = *buf++;
                data |= *buf++ << 8;
                data |= *buf++ << 16;
                data |= *buf++ << 24;
                REG_MSC_TXFIFO(MSC_CHN(drive)) = data;
            }

            cnt--;
        }
    }

    return SD_NO_ERROR;
}

#if SD_DMA_ENABLE
static int jz_sd_receive_data_dma(const int drive, struct sd_request *req)
{
    /* flush dcache */
    discard_dcache_range(req->buffer, req->cnt);

    /* setup dma channel */
    REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(drive)) = 0;
    REG_DMAC_DSAR(DMA_SD_RX_CHANNEL(drive)) = PHYSADDR(MSC_RXFIFO(MSC_CHN(drive))); /* DMA source addr */
    REG_DMAC_DTAR(DMA_SD_RX_CHANNEL(drive)) = PHYSADDR((unsigned long)req->buffer); /* DMA dest addr */
    REG_DMAC_DTCR(DMA_SD_RX_CHANNEL(drive)) = (req->cnt + 3) >> 2;                  /* DMA transfer count */
    REG_DMAC_DRSR(DMA_SD_RX_CHANNEL(drive)) = (drive == SD_SLOT_1) ? DMAC_DRSR_RS_MSC2IN : DMAC_DRSR_RS_MSC1IN;    /* DMA request type */

    REG_DMAC_DCMD(DMA_SD_RX_CHANNEL(drive)) =
#if SD_DMA_INTERRUPT
        DMAC_DCMD_TIE | /* Enable DMA interrupt */
#endif
        DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
        DMAC_DCMD_DS_32BIT;
    REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(drive)) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;

    /* wait for dma completion */
#if SD_DMA_INTERRUPT
    semaphore_wait(&sd_wakeup[drive], TIMEOUT_BLOCK);
#else
    while (REG_DMAC_DTCR(DMA_SD_RX_CHANNEL(drive)))
           yield();
#endif

    /* clear status and disable channel */
    REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(drive)) = 0;

    return SD_NO_ERROR;
}

static int jz_sd_transmit_data_dma(const int drive, struct sd_request *req)
{
    /* flush dcache */
    commit_discard_dcache_range(req->buffer, req->cnt);

    /* setup dma channel */
    REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(drive)) = 0;
    REG_DMAC_DSAR(DMA_SD_TX_CHANNEL(drive)) = PHYSADDR((unsigned long) req->buffer); /* DMA source addr */
    REG_DMAC_DTAR(DMA_SD_TX_CHANNEL(drive)) = PHYSADDR(MSC_TXFIFO(MSC_CHN(drive)));  /* DMA dest addr */
    REG_DMAC_DTCR(DMA_SD_TX_CHANNEL(drive)) = (req->cnt + 3) >> 2;                       /* DMA transfer count */
    REG_DMAC_DRSR(DMA_SD_TX_CHANNEL(drive)) = (drive == SD_SLOT_1) ? DMAC_DRSR_RS_MSC2OUT : DMAC_DRSR_RS_MSC1OUT;    /* DMA request type */

    REG_DMAC_DCMD(DMA_SD_TX_CHANNEL(drive)) =
#if SD_DMA_INTERRUPT
        DMAC_DCMD_TIE | /* Enable DMA interrupt */
#endif
        DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
        DMAC_DCMD_DS_32BIT;
    REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(drive)) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;

    /* wait for dma completion */
#if SD_DMA_INTERRUPT
    semaphore_wait(&sd_wakeup[drive], TIMEOUT_BLOCK);
#else
    while (REG_DMAC_DTCR(DMA_SD_TX_CHANNEL(drive)))
           yield();
#endif

    /* clear status and disable channel */
    REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(drive)) = 0;

    return SD_NO_ERROR;
}

#if SD_DMA_INTERRUPT
void DMA_CALLBACK(DMA_SD_RX_CHANNEL0)(void)
{
    if (REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(SD_SLOT_1)) & DMAC_DCCSR_AR)
    {
        REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(SD_SLOT_1)) &= ~DMAC_DCCSR_AR;
        panicf("SD RX DMA address error");
    }

    if (REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(SD_SLOT_1)) & DMAC_DCCSR_HLT)
    {
        REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(SD_SLOT_1)) &= ~DMAC_DCCSR_HLT;
        panicf("SD RX DMA halt");
    }

    if (REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(SD_SLOT_1)) & DMAC_DCCSR_TT)
    {
        REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(SD_SLOT_1)) &= ~DMAC_DCCSR_TT;
        //sd_rx_dma_callback();
        semaphore_release(&sd_wakeup[SD_SLOT_1]);
    }
}

void DMA_CALLBACK(DMA_SD_RX_CHANNEL1)(void)
{
    if (REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(SD_SLOT_2)) & DMAC_DCCSR_AR)
    {
        REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(SD_SLOT_2)) &= ~DMAC_DCCSR_AR;
        panicf("SD RX DMA address error");
    }

    if (REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(SD_SLOT_2)) & DMAC_DCCSR_HLT)
    {
        REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(SD_SLOT_2)) &= ~DMAC_DCCSR_HLT;
        panicf("SD RX DMA halt");
    }

    if (REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(SD_SLOT_2)) & DMAC_DCCSR_TT)
    {
        REG_DMAC_DCCSR(DMA_SD_RX_CHANNEL(SD_SLOT_2)) &= ~DMAC_DCCSR_TT;
        //sd_rx_dma_callback();
        semaphore_release(&sd_wakeup[SD_SLOT_2]);
    }
}

void DMA_CALLBACK(DMA_SD_TX_CHANNEL0)(void)
{
    if (REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(SD_SLOT_1)) & DMAC_DCCSR_AR)
    {
        REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(SD_SLOT_1)) &= ~DMAC_DCCSR_AR;
        panicf("SD TX DMA address error");
    }

    if (REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(SD_SLOT_1)) & DMAC_DCCSR_HLT)
    {
        REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(SD_SLOT_1)) &= ~DMAC_DCCSR_HLT;
        panicf("SD TX DMA halt");
    }

    if (REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(SD_SLOT_1)) & DMAC_DCCSR_TT)
    {
        REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(SD_SLOT_1)) &= ~DMAC_DCCSR_TT;
        //sd_tx_dma_callback();
        semaphore_release(&sd_wakeup[SD_SLOT_1]);
    }
}

void DMA_CALLBACK(DMA_SD_TX_CHANNEL1)(void)
{
    if (REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(SD_SLOT_2)) & DMAC_DCCSR_AR)
    {
        REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(SD_SLOT_2)) &= ~DMAC_DCCSR_AR;
        panicf("SD TX DMA address error");
    }

    if (REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(SD_SLOT_2)) & DMAC_DCCSR_HLT)
    {
        REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(SD_SLOT_2)) &= ~DMAC_DCCSR_HLT;
        panicf("SD TX DMA halt");
    }

    if (REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(SD_SLOT_2)) & DMAC_DCCSR_TT)
    {
        REG_DMAC_DCCSR(DMA_SD_TX_CHANNEL(SD_SLOT_2)) &= ~DMAC_DCCSR_TT;
        //sd_tx_dma_callback();
	semaphore_release(&sd_wakeup[SD_SLOT_2]);
    }
}
#endif                /* SD_DMA_INTERRUPT */
#endif                /* SD_DMA_ENABLE */

#ifndef HAVE_ADJUSTABLE_CPU_FREQ
#define cpu_frequency __cpm_get_pllout2()
#endif

static inline unsigned int jz_sd_calc_clkrt(const int drive, unsigned int rate)
{
    unsigned int clkrt = 0;
    unsigned int clk_src = cpu_frequency / __cpm_get_mscdiv(); /* MSC_CLK */

    if (!sd2_0[drive] && rate > SD_CLOCK_FAST)
        rate = SD_CLOCK_FAST;

    while (rate < clk_src)
    {
        clkrt++;
        clk_src >>= 1;
    }
    return clkrt;
}

/* Set the MMC clock frequency */
static void jz_sd_set_clock(const int drive, unsigned int rate)
{
    int clkrt;

    clkrt = jz_sd_calc_clkrt(drive, rate);
    REG_MSC_CLKRT(MSC_CHN(drive)) = clkrt;

    DEBUG("set clock to %u Hz clkrt=%d", rate, clkrt);
}

/********************************************************************************************************************
** Name:      int jz_sd_exec_cmd()
** Function:  send command to the card, and get a response
** Input:     struct sd_request *req: SD request
** Output:    0:  right        >0:  error code
********************************************************************************************************************/
static int jz_sd_exec_cmd(const int drive, struct sd_request *request)
{
    unsigned int cmdat = 0, events = 0;
    int retval;
#if !SD_INTERRUPT
    long deadline = current_tick + (HZ * 5);
#endif

    /* Indicate we have no result yet */
    request->result = SD_NO_RESPONSE;

    if (request->cmd == SD_CIM_RESET) {
        /* On reset, 1-bit bus width */
        use_4bit[drive] = 0;

        /* On reset, stop SD clock */
        jz_sd_stop_clock(drive);

        /* Reset MMC/SD controller */
        __msc_reset(MSC_CHN(drive));

        /* Drop SD clock down to lowest speed */
        jz_sd_set_clock(drive, MMC_CLOCK_SLOW);

#if SD_AUTO_CLOCK
	/* Re-enable clocks */
        REG_MSC_STRPCL(MSC_CHN(drive)) = MSC_STRPCL_CLOCK_CONTROL_START;
        REG_MSC_LPM(drive) = MSC_SET_LPM;
#endif
    }

    /* mask all interrupts and clear status */
    SD_IRQ_MASK(MSC_CHN(drive));

    /* open interrupt */
    REG_MSC_IMASK(MSC_CHN(drive)) = ~(MSC_IMASK_END_CMD_RES | MSC_IMASK_DATA_TRAN_DONE | MSC_IMASK_PRG_DONE);

    /* Set command type and events */
    switch (request->cmd)
    {
        /* SD core extra command */
        case SD_CIM_RESET:
            cmdat |= MSC_CMDAT_INIT; /* Initialization sequence sent prior to command */
            break;
            /* bc - broadcast - no response */
        case SD_GO_IDLE_STATE:
        case SD_SET_DSR:
            break;

            /* bcr - broadcast with response */
        case SD_APP_OP_COND:
        case SD_ALL_SEND_CID:
        case SD_GO_IRQ_STATE:
            break;

            /* adtc - addressed with data transfer */
        case SD_SEND_SCR:
            /* SD card returns SCR register as data.
               SD core expect it in the response buffer,
               after normal response. */
            request->buffer =
              (unsigned char *) ((unsigned int) request->response + 5);
            request->block_len = 8;
            request->nob = 1;

        case SD_READ_DAT_UNTIL_STOP:
        case SD_READ_SINGLE_BLOCK:
        case SD_READ_MULTIPLE_BLOCK:
#if SD_DMA_ENABLE
            cmdat |=
                MSC_CMDAT_DATA_EN | MSC_CMDAT_READ | MSC_CMDAT_DMA_EN;
#else
            cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_READ;
#endif
            events = SD_EVENT_RX_DATA_DONE;
            break;

        case SD_SWITCH_FUNC:
            if (request->arg == 0x2)
            {
                DEBUG("Use 4-bit bus width");
                use_4bit[drive] = 1;
            }
            else
            {
                DEBUG("Use 1-bit bus width");
                use_4bit[drive] = 0;
            }
            if (num_6[drive] < 2)
            {
#if SD_DMA_ENABLE
                cmdat |=
                    MSC_CMDAT_DATA_EN | MSC_CMDAT_READ |
                    MSC_CMDAT_DMA_EN;
#else
                cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_READ;
#endif
                events = SD_EVENT_RX_DATA_DONE;
            }
            break;

        case SD_WRITE_DAT_UNTIL_STOP:
        case SD_WRITE_BLOCK:
        case SD_WRITE_MULTIPLE_BLOCK:
        case SD_PROGRAM_CID:
        case SD_PROGRAM_CSD:
//        case SD_LOCK_UNLOCK:
#if SD_DMA_ENABLE
            cmdat |=
                MSC_CMDAT_DATA_EN | MSC_CMDAT_WRITE | MSC_CMDAT_DMA_EN;
#else
            cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_WRITE;
#endif
            events = SD_EVENT_TX_DATA_DONE | SD_EVENT_PROG_DONE;
            break;

        case SD_STOP_TRANSMISSION:
            events = SD_EVENT_PROG_DONE;
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
        case RESPONSE_R7:
            cmdat |= MSC_CMDAT_RESPONSE_R1;
            break;
        case RESPONSE_R2_CID:
        case RESPONSE_R2_CSD:
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
        default:
            break;
    }

    /* use 4-bit bus width when possible */
    if (use_4bit[drive])
        cmdat |= MSC_CMDAT_BUS_WIDTH_4BIT;

    /* Set command index */
    if (request->cmd == SD_CIM_RESET)
        REG_MSC_CMD(MSC_CHN(drive)) = SD_GO_IDLE_STATE;
    else
        REG_MSC_CMD(MSC_CHN(drive)) = request->cmd;

    /* Set argument */
    REG_MSC_ARG(MSC_CHN(drive)) = request->arg;

    /* Set block length and nob */
    REG_MSC_BLKLEN(MSC_CHN(drive)) = request->block_len;
    REG_MSC_NOB(MSC_CHN(drive)) = request->nob;

    /* Set command */
    REG_MSC_CMDAT(MSC_CHN(drive)) = cmdat;

    DEBUG("Send cmd %d cmdat: %x arg: %x resp %d", request->cmd,
          cmdat, request->arg, request->rtype);

    /* Start SD clock and send command to card */
    jz_sd_start_clock(drive);

    /* Wait for command completion */
#if SD_INTERRUPT
    semaphore_wait(&sd_wakeup[drive], HZ * 5);
#else
    while (!(REG_MSC_IREG(MSC_CHN(drive)) & MSC_IREG_END_CMD_RES))
    {
        if (TIME_AFTER(current_tick, deadline))
            return SD_ERROR_TIMEOUT;
        yield();
    }
    REG_MSC_IREG(MSC_CHN(drive)) = MSC_IREG_END_CMD_RES;    /* clear flag */
#endif

    /* Check for status */
    retval = jz_sd_check_status(drive, request);
    if (retval)
        return retval;

    /* Complete command with no response */
    if (request->rtype == RESPONSE_NONE)
        return SD_NO_ERROR;

    /* Get response */
    jz_sd_get_response(drive, request);

    /* Start data operation */
    if (events & (SD_EVENT_RX_DATA_DONE | SD_EVENT_TX_DATA_DONE))
    {
        if (events & SD_EVENT_RX_DATA_DONE)
        {
            retval = jz_sd_receive_data(drive, request);
        }
        if (retval)
            return retval;

        if (events & SD_EVENT_TX_DATA_DONE)
        {
            retval = jz_sd_transmit_data(drive, request);
        }
        if (retval)
            return retval;

#if SD_INTERRUPT
        semaphore_wait(&sd_wakeup[drive], HZ * 5);
#else
        /* Wait for Data Done */
        while (!(REG_MSC_IREG(MSC_CHN(drive)) & MSC_IREG_DATA_TRAN_DONE))
            yield();
        REG_MSC_IREG(MSC_CHN(drive)) = MSC_IREG_DATA_TRAN_DONE;    /* clear status */
#endif
    }

    /* Wait for Prog Done event */
    if (events & SD_EVENT_PROG_DONE)
    {
#if SD_INTERRUPT
        semaphore_wait(&sd_wakeup[drive], HZ * 5);
#else
        while (!(REG_MSC_IREG(MSC_CHN(drive)) & MSC_IREG_PRG_DONE))
            yield();
        REG_MSC_IREG(MSC_CHN(drive)) = MSC_IREG_PRG_DONE;    /* clear status */
#endif
    }

    /* Command completed */
#if !SD_AUTO_CLOCK
    jz_sd_stop_clock(drive); /* stop SD clock */
#endif

    return SD_NO_ERROR;    /* return successfully */
}

/*******************************************************************************************************************
** Name:      int sd_chkcard()
** Function:  check whether card is insert entirely
** Input:     NULL
** Output:    1: insert entirely    0: not insert entirely
********************************************************************************************************************/
static int jz_sd_chkcard(const int drive)
{
    return (__gpio_get_pin((drive == SD_SLOT_1) ? PIN_SD1_CD : PIN_SD2_CD) == 0 ? 1 : 0);
}

/* MSC interrupt handlers */
#if SD_INTERRUPT
void MSC2(void)  /* SD_SLOT_1 */
{
    logf("MSC2 interrupt");

    if (REG_MSC_IREG(MSC_CHN(SD_SLOT_1)) & MSC_IREG_END_CMD_RES) {
        REG_MSC_IREG(MSC_CHN(SD_SLOT_1)) = MSC_IREG_END_CMD_RES;    /* clear flag */
        semaphore_release(&sd_wakeup[SD_SLOT_1]);
    }
    if (REG_MSC_IREG(MSC_CHN(SD_SLOT_1)) & MSC_IREG_PRG_DONE) {
        REG_MSC_IREG(MSC_CHN(SD_SLOT_1)) = MSC_IREG_PRG_DONE;    /* clear flag */
        semaphore_release(&sd_wakeup[SD_SLOT_1]);
    }
    if (REG_MSC_IREG(MSC_CHN(SD_SLOT_1)) & MSC_IREG_DATA_TRAN_DONE) {
        REG_MSC_IREG(MSC_CHN(SD_SLOT_1)) = MSC_IREG_DATA_TRAN_DONE;    /* clear flag */
        semaphore_release(&sd_wakeup[SD_SLOT_1]);
    }
}

/* MSC interrupt handlers */
void MSC1(void) /* SD_SLOT_2 */
{
    logf("MSC1 interrupt");
    if (REG_MSC_IREG(MSC_CHN(SD_SLOT_2)) & MSC_IREG_END_CMD_RES) {
        REG_MSC_IREG(MSC_CHN(SD_SLOT_2)) = MSC_IREG_END_CMD_RES;    /* clear flag */
        semaphore_release(&sd_wakeup[SD_SLOT_2]);
    }
    if (REG_MSC_IREG(MSC_CHN(SD_SLOT_2)) & MSC_IREG_PRG_DONE) {
        REG_MSC_IREG(MSC_CHN(SD_SLOT_2)) = MSC_IREG_PRG_DONE;    /* clear flag */
        semaphore_release(&sd_wakeup[SD_SLOT_2]);
    }
    if (REG_MSC_IREG(MSC_CHN(SD_SLOT_2)) & MSC_IREG_DATA_TRAN_DONE) {
        REG_MSC_IREG(MSC_CHN(SD_SLOT_2)) = MSC_IREG_DATA_TRAN_DONE;    /* clear flag */
        semaphore_release(&sd_wakeup[SD_SLOT_2]);
    }
}
#endif

#ifdef HAVE_HOTSWAP
static void sd_gpio_setup_irq(const int drive, bool inserted)
{
    int pin = (drive == SD_SLOT_1) ? PIN_SD1_CD : PIN_SD2_CD;
    int irq = (drive == SD_SLOT_1) ? IRQ_SD1_CD : IRQ_SD2_CD;
    if(inserted)
        __gpio_as_irq_rise_edge(pin);
    else
        __gpio_as_irq_fall_edge(pin);
    system_enable_irq(irq);
}
#endif

/*******************************************************************************************************************
** Name:      void sd_hardware_init()
** Function:  initialize the hardware condiction that access sd card
** Input:     NULL
** Output:    NULL
********************************************************************************************************************/
static void jz_sd_hardware_init(const int drive)
{
    if (drive == SD_SLOT_1)
        __cpm_start_msc2();   /* enable mmc2 clock */
    else
        __cpm_start_msc1();   /* enable mmc1 clock */
#ifdef HAVE_HOTSWAP
    sd_gpio_setup_irq(drive, jz_sd_chkcard(drive));
#endif
    __msc_reset(MSC_CHN(drive));  /* reset mmc/sd controller */
    SD_IRQ_MASK(MSC_CHN(drive));  /* mask all IRQs */
#if SD_AUTO_CLOCK
    REG_MSC_STRPCL(MSC_CHN(drive)) = MSC_STRPCL_CLOCK_CONTROL_START; /* Enable clocks */
    REG_MSC_LPM(drive) = MSC_SET_LPM; /* enable auto clock stop */
#else
    jz_sd_stop_clock(drive); /* stop SD clock */
#endif
}

static void sd_send_cmd(const int drive, struct sd_request *request, int cmd, unsigned int arg,
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

    retval = jz_sd_exec_cmd(drive, request);
    if (retval)
            request->result = retval;
}

static void sd_simple_cmd(const int drive, struct sd_request *request, int cmd, unsigned int arg,
                           enum sd_rsp_t rtype)
{
    sd_send_cmd(drive, request, cmd, arg, 0, 0, rtype, NULL);
}

static int sd_exec_acmd(const int drive, struct sd_request *request, int cmd, unsigned int arg)
{
    struct sd_response_r1 r1;
    int retval;

    sd_simple_cmd(drive, request, SD_APP_CMD, card[drive].rca, RESPONSE_R1);
    retval = sd_unpack_r1(request, &r1);

    if (!retval)
    {
        sd_simple_cmd(drive, request, cmd, arg, RESPONSE_R1);
        retval = sd_unpack_r1(request, &r1);
    }

    return retval;
}

#define SD_INIT_DOING   0
#define SD_INIT_PASSED  1
#define SD_INIT_FAILED  2
static int sd_init_card_state(const int drive, struct sd_request *request)
{
    struct sd_response_r1 r1;
    struct sd_response_r3 r3;
    int retval, i, ocr = 0x40300000;

    switch (request->cmd)
    {
        case SD_GO_IDLE_STATE: /* No response to parse */
            sd_simple_cmd(drive, request, SD_SEND_IF_COND, 0x1AA, RESPONSE_R1);
            break;

        case SD_SEND_IF_COND:
            retval = sd_unpack_r1(request, &r1);
            sd_simple_cmd(drive, request, SD_APP_CMD,  0, RESPONSE_R1);
            break;

        case SD_APP_CMD:
            if (sd_unpack_r1(request, &r1))
                return SD_INIT_FAILED;
            sd_simple_cmd(drive, request, SD_APP_OP_COND, ocr, RESPONSE_R3);
            break;

        case SD_APP_OP_COND:
            retval = sd_unpack_r3(request, &r3);
            if (retval)
                return SD_INIT_FAILED;

            DEBUG("sd_init_card_state: read ocr value = 0x%08x", r3.ocr);
            card[drive].ocr = r3.ocr;

            if(!(r3.ocr & SD_CARD_BUSY || ocr == 0))
            {
                sleep(HZ / 100);
                sd_simple_cmd(drive, request, SD_APP_CMD, 0, RESPONSE_R1);
            }
            else
            {
                /* Set the data bus width to 4 bits */
                use_4bit[drive] = 1;
                sd_simple_cmd(drive, request, SD_ALL_SEND_CID, 0, RESPONSE_R2_CID);
            }
            break;

        case SD_ALL_SEND_CID:
            if (request->result)
                return SD_INIT_FAILED;

            for(i=0; i<4; i++)
                card[drive].cid[i] = ((request->response[1+i*4]<<24) | (request->response[2+i*4]<<16) |
                               (request->response[3+i*4]<< 8) | request->response[4+i*4]);

            logf("CID: %08lx%08lx%08lx%08lx", card[drive].cid[0], card[drive].cid[1], card[drive].cid[2], card[drive].cid[3]);
            sd_simple_cmd(drive, request, SD_SEND_RELATIVE_ADDR, 0, RESPONSE_R6);
            break;

        case SD_SEND_RELATIVE_ADDR:
            retval = sd_unpack_r6(request, &r1, &card[drive].rca);
            card[drive].rca = card[drive].rca << 16;
            DEBUG("sd_init_card_state: Get RCA from SD: 0x%04lx Status: %x", card[drive].rca, r1.status);
            if (retval)
            {
                DEBUG("sd_init_card_state: unable to SET_RELATIVE_ADDR error=%d",
                      retval);
                return SD_INIT_FAILED;
            }

            sd_simple_cmd(drive, request, SD_SEND_CSD, card[drive].rca, RESPONSE_R2_CSD);
            break;

        case SD_SEND_CSD:
            if (request->result)
                return SD_INIT_FAILED;
            for(i=0; i<4; i++)
                card[drive].csd[i] = ((request->response[1+i*4]<<24) | (request->response[2+i*4]<<16) |
                               (request->response[3+i*4]<< 8) | request->response[4+i*4]);

            sd_parse_csd(&card[drive]);
            sd2_0[drive] = (card_extract_bits(card[drive].csd, 127, 2) == 1);

            logf("CSD: %08lx%08lx%08lx%08lx", card[drive].csd[0], card[drive].csd[1], card[drive].csd[2], card[drive].csd[3]);
            DEBUG("SD card is ready");
            jz_sd_set_clock(drive, SD_CLOCK_FAST);
            return SD_INIT_PASSED;

        default:
            DEBUG("sd_init_card_state: error!  Illegal last cmd %d", request->cmd);
            return SD_INIT_FAILED;
    }

    return SD_INIT_DOING;
}

static int sd_switch(const int drive, struct sd_request *request, int mode, int group,
              unsigned char value, unsigned char * resp)
{
    unsigned int arg;

    mode = !!mode;
    value &= 0xF;
    arg = (mode << 31 | 0x00FFFFFF);
    arg &= ~(0xF << (group * 4));
    arg |= value << (group * 4);
    sd_send_cmd(drive, request, SD_SWITCH_FUNC, arg, 1, 64, RESPONSE_R1, resp);

    return 0;
}

/*
 * Fetches and decodes switch information
 */
static int sd_read_switch(const int drive, struct sd_request *request)
{
    unsigned int status[64 / 4];

    memset((unsigned char *)status, 0, 64);
    sd_switch(drive, request, 0, 0, 1, (unsigned char*) status);

    if (((unsigned char *)status)[13] & 0x02)
        return 0;
    else
        return 1;
}

/*
 * Test if the card supports high-speed mode and, if so, switch to it.
 */
static int sd_switch_hs(const int drive, struct sd_request *request)
{
    unsigned int status[64 / 4];

    sd_switch(drive, request, 1, 0, 1, (unsigned char*) status);
    return 0;
}

static int sd_select_card(const int drive)
{
    struct sd_request request;
    struct sd_response_r1 r1;
    int retval;

    sd_simple_cmd(drive, &request, SD_SELECT_CARD, card[drive].rca,
               RESPONSE_R1B);
    retval = sd_unpack_r1(&request, &r1);
    if (retval)
        return retval;

    if (sd2_0[drive])
    {
        retval = sd_read_switch(drive, &request);
        if (!retval)
        {
            sd_switch_hs(drive, &request);
            jz_sd_set_clock(drive, SD_CLOCK_HIGH);
        }
    }
    num_6[drive] = 3;
    retval = sd_exec_acmd(drive, &request, SD_SET_BUS_WIDTH, 2);
    if (retval)
        return retval;
    retval = sd_exec_acmd(drive, &request, SD_SET_CLR_CARD_DETECT, 0);
    if (retval)
        return retval;

    card[drive].initialized = 1;

    return 0;
}

static int __sd_init_device(const int drive)
{
    int retval = 0;
    long deadline;
    struct sd_request init_req;

    /* Initialise card data as blank */
    memset(&card[drive], 0, sizeof(tCardInfo));

    sd2_0[drive] = 0;
    num_6[drive] = 0;
    use_4bit[drive] = 0;
    active[drive] = 0;

    /* reset mmc/sd controller */
    jz_sd_hardware_init(drive);

    sd_simple_cmd(drive, &init_req, SD_CIM_RESET,     0,     RESPONSE_NONE);
    sd_simple_cmd(drive, &init_req, SD_GO_IDLE_STATE, 0,     RESPONSE_NONE);

    sleep(HZ/2); /* Give the card/controller some rest */

    deadline = current_tick + HZ;
    do {
       retval = sd_init_card_state(drive, &init_req);
    } while (TIME_BEFORE(current_tick, deadline) && (retval == SD_INIT_DOING));

    retval = (retval == SD_INIT_PASSED ? sd_select_card(drive) : -1);

    if (drive == SD_SLOT_1)
        __cpm_stop_msc2(); /* disable SD1 clock */
    else
        __cpm_stop_msc1(); /* disable SD2 clock */

    return retval;
}

int sd_init(void)
{
    static bool inited = false;

    sd_init_gpio();     /* init GPIO */

#if SD_DMA_ENABLE
    __dmac_channel_enable_clk(DMA_SD_RX_CHANNEL(SD_SLOT_1));
    __dmac_channel_enable_clk(DMA_SD_RX_CHANNEL(SD_SLOT_2));
    __dmac_channel_enable_clk(DMA_SD_TX_CHANNEL(SD_SLOT_1));
    __dmac_channel_enable_clk(DMA_SD_TX_CHANNEL(SD_SLOT_2));
#endif

    if(!inited)
    {
        __cpm_stop_msc0();   /* We don't use MSC0 */

#if SD_DMA_INTERRUPT || SD_INTERRUPT
        semaphore_init(&sd_wakeup[SD_SLOT_1], 1, 0);
        semaphore_init(&sd_wakeup[SD_SLOT_2], 1, 0);
#endif
        mutex_init(&sd_mtx[SD_SLOT_1]);
        mutex_init(&sd_mtx[SD_SLOT_2]);

#if SD_INTERRUPT
        system_enable_irq(IRQ_MSC2);
        system_enable_irq(IRQ_MSC1);
#endif

#if SD_DMA_ENABLE && SD_DMA_INTERRUPT
        system_enable_irq(DMA_IRQ(DMA_SD_RX_CHANNEL(SD_SLOT_1)));
        system_enable_irq(DMA_IRQ(DMA_SD_RX_CHANNEL(SD_SLOT_2)));
        system_enable_irq(DMA_IRQ(DMA_SD_TX_CHANNEL(SD_SLOT_1)));
        system_enable_irq(DMA_IRQ(DMA_SD_TX_CHANNEL(SD_SLOT_2)));
#endif

        inited = true;
    }

    for (int drive = 0; drive < NUM_DRIVES; drive++) {
        mutex_lock(&sd_mtx[drive]);
        __sd_init_device(drive);
        mutex_unlock(&sd_mtx[drive]);
    }

    return 0;
}

static inline bool card_detect_target(const int drive)
{
    return (jz_sd_chkcard(drive) == 1);
}

tCardInfo* card_get_info_target(const int drive)
{
    return &card[drive];
}

static inline void sd_start_transfer(const int drive)
{
    mutex_lock(&sd_mtx[drive]);
    if (drive == SD_SLOT_1)
        __cpm_start_msc2();
    else
        __cpm_start_msc1();

    active[drive] = 1;
    led(active[SD_SLOT_1] || active[SD_SLOT_2]);
}

static inline void sd_stop_transfer(const int drive)
{
    active[drive] = 0;
    led(active[SD_SLOT_1] || active[SD_SLOT_2]);

    if (drive == SD_SLOT_1)
        __cpm_stop_msc2();
    else
        __cpm_stop_msc1();
    mutex_unlock(&sd_mtx[drive]);
}

int sd_transfer_sectors(IF_MD(const int drive,) unsigned long start, int count, void* buf, bool write)
{
    struct sd_request request;
    struct sd_response_r1 r1;
    int retval = -1;
#ifndef HAVE_MULTIDRIVE
    const int drive = 0;
#endif

    sd_start_transfer(drive);

    if (!card_detect_target(drive) || count < 1 || (start + count) > card[drive].numblocks)
        goto err;

    if(card[drive].initialized == 0 && !__sd_init_device(drive))
        goto err;

    sd_simple_cmd(drive, &request, SD_SEND_STATUS, card[drive].rca, RESPONSE_R1);
    if ((retval = sd_unpack_r1(&request, &r1)))
        goto err;

    sd_simple_cmd(drive, &request, SD_SET_BLOCKLEN, SD_BLOCK_SIZE, RESPONSE_R1);
    if ((retval = sd_unpack_r1(&request, &r1)))
        goto err;

    sd_send_cmd(drive, &request,
                (count > 1) ?
                (write ? SD_WRITE_MULTIPLE_BLOCK : SD_READ_MULTIPLE_BLOCK) :
                (write ? SD_WRITE_BLOCK : SD_READ_SINGLE_BLOCK),
                sd2_0[drive] ? start : (start * SD_BLOCK_SIZE),
                count, SD_BLOCK_SIZE, RESPONSE_R1, buf);
    if ((retval = sd_unpack_r1(&request, &r1)))
        goto err;

    if (count > 1)
    {
        sd_simple_cmd(drive, &request, SD_STOP_TRANSMISSION, 0, RESPONSE_R1B);
        retval = sd_unpack_r1(&request, &r1);
        if (!write && retval == SD_ERROR_OUT_OF_RANGE)
            retval = 0;
    }

err:
    last_disk_activity = current_tick;
    sd_stop_transfer(drive);

    return retval;
}

int sd_read_sectors(IF_MD(int drive,) unsigned long start, int count, void* buf)
{
  return sd_transfer_sectors(IF_MD(drive,) start, count, buf, false);
}

int sd_write_sectors(IF_MD(int drive,) unsigned long start, int count, const void* buf)
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
bool sd_removable(IF_MD_NONVOID(const int drive))
{
#ifdef HAVE_MULTIDRIVE
     (void)drive;
#endif
    return true;
}

static int sd_oneshot_callback(struct timeout *tmo)
{
    int slot = (int) tmo->data;
    int state = card_detect_target(slot);

    /* This is called only if the state was stable for 300ms - check state
     * and post appropriate event. */
    queue_broadcast(state ? SYS_HOTSWAP_INSERTED : SYS_HOTSWAP_EXTRACTED,
                    sd_drive_nr + slot);

    sd_gpio_setup_irq(slot, state);

    return 0;
}

/* called on insertion/removal interrupt */
void GPIO_SD1_CD(void)
{
    static struct timeout sd1_oneshot;
    timeout_register(&sd1_oneshot, sd_oneshot_callback, (3*HZ/10), SD_SLOT_1);
}

void GPIO_SD2_CD(void)
{
    static struct timeout sd2_oneshot;
    timeout_register(&sd2_oneshot, sd_oneshot_callback, (3*HZ/10), SD_SLOT_2);
}

bool sd_present(IF_MD_NONVOID(const int drive))
{
#ifndef HAVE_MULTIDRIVE
    const int drive = 0;
#endif
    return card_detect_target(drive);
}
#endif

#ifdef CONFIG_STORAGE_MULTI
int sd_num_drives(int first_drive)
{
    sd_drive_nr = first_drive;
    return NUM_DRIVES;
}
#endif /* CONFIG_STORAGE_MULTI */

int sd_event(long id, intptr_t data)
{
    int rc = 0;

    switch (id)
    {
#ifdef HAVE_HOTSWAP
    case SYS_HOTSWAP_INSERTED:
    case SYS_HOTSWAP_EXTRACTED:
        /* Force card init for new card, re-init for re-inserted one or
         * clear if the last attempt to init failed with an error. */
        mutex_lock(&sd_mtx[data]); /* lock-out card activity */
        card[data].initialized = 0;
	if (id == SYS_HOTSWAP_INSERTED)
            __sd_init_device(data);
        mutex_unlock(&sd_mtx[data]);
        break;
#endif /* HAVE_HOTSWAP */
    default:
        rc = storage_event_default_handler(id, data, last_disk_activity,
                                           STORAGE_SD);
        break;
    }

    return rc;
}
