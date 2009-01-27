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
#include "jz4740.h"
#include "ata.h"
#include "ata-sd-target.h"
#include "logf.h"
#include "sd.h"
#include "system.h"
#include "kernel.h"
#include "panic.h"
#include "debug.h"
#include "storage.h"

static struct wakeup sd_wakeup;

//#define MMC_DMA_ENABLE
#define MMC_DMA_INTERRUPT 0

#define DEBUG(x...) logf(x);

#define MMC_INSERT_STATUS() __gpio_get_pin(MMC_CD_PIN)

#define MMC_RESET()         __msc_reset()

#define MMC_IRQ_MASK()             \
do {                               \
          REG_MSC_IMASK = 0xffff;  \
          REG_MSC_IREG = 0xffff;   \
} while (0)

/* Error codes */
enum mmc_result_t
{
    MMC_NO_RESPONSE        = -1,
    MMC_NO_ERROR           = 0,
    MMC_ERROR_OUT_OF_RANGE,
    MMC_ERROR_ADDRESS,
    MMC_ERROR_BLOCK_LEN,
    MMC_ERROR_ERASE_SEQ,
    MMC_ERROR_ERASE_PARAM,
    MMC_ERROR_WP_VIOLATION,
    MMC_ERROR_CARD_IS_LOCKED,
    MMC_ERROR_LOCK_UNLOCK_FAILED,
    MMC_ERROR_COM_CRC,
    MMC_ERROR_ILLEGAL_COMMAND,
    MMC_ERROR_CARD_ECC_FAILED,
    MMC_ERROR_CC,
    MMC_ERROR_GENERAL,
    MMC_ERROR_UNDERRUN,
    MMC_ERROR_OVERRUN,
    MMC_ERROR_CID_CSD_OVERWRITE,
    MMC_ERROR_STATE_MISMATCH,
    MMC_ERROR_HEADER_MISMATCH,
    MMC_ERROR_TIMEOUT,
    MMC_ERROR_CRC,
    MMC_ERROR_DRIVER_FAILURE,
};

/* Standard MMC/SD clock speeds */
#define MMC_CLOCK_SLOW    400000      /* 400 kHz for initial setup */
#define MMC_CLOCK_FAST  20000000      /* 20 MHz for maximum for normal operation */
#define SD_CLOCK_FAST   24000000      /* 24 MHz for SD Cards */
#define SD_CLOCK_HIGH   48000000      /* 48 MHz for SD Cards */
 
/* Extra MMC commands for state control */
/* Use negative numbers to disambiguate */
#define MMC_CIM_RESET            -1

/* Standard MMC commands (3.1)                type  argument     response */
   /* class 1 */
#define MMC_GO_IDLE_STATE         0   /* bc                          */
#define MMC_SEND_OP_COND          1   /* bcr  [31:0] OCR         R3  */
#define MMC_ALL_SEND_CID          2   /* bcr                     R2  */
#define MMC_SET_RELATIVE_ADDR     3   /* ac   [31:16] RCA        R1  */
#define MMC_SET_DSR               4   /* bc   [31:16] RCA            */
#define MMC_SELECT_CARD           7   /* ac   [31:16] RCA        R1  */
#define MMC_SEND_CSD              9   /* ac   [31:16] RCA        R2  */
#define MMC_SEND_CID             10   /* ac   [31:16] RCA        R2  */
#define MMC_READ_DAT_UNTIL_STOP  11   /* adtc [31:0] dadr        R1  */
#define MMC_STOP_TRANSMISSION    12   /* ac                      R1b */
#define MMC_SEND_STATUS          13   /* ac   [31:16] RCA        R1  */
#define MMC_GO_INACTIVE_STATE    15   /* ac   [31:16] RCA            */

  /* class 2 */
#define MMC_SET_BLOCKLEN         16   /* ac   [31:0] block len   R1  */
#define MMC_READ_SINGLE_BLOCK    17   /* adtc [31:0] data addr   R1  */
#define MMC_READ_MULTIPLE_BLOCK  18   /* adtc [31:0] data addr   R1  */

  /* class 3 */
#define MMC_WRITE_DAT_UNTIL_STOP 20   /* adtc [31:0] data addr   R1  */

  /* class 4 */
#define MMC_SET_BLOCK_COUNT      23   /* adtc [31:0] data addr   R1  */
#define MMC_WRITE_BLOCK          24   /* adtc [31:0] data addr   R1  */
#define MMC_WRITE_MULTIPLE_BLOCK 25   /* adtc                    R1  */
#define MMC_PROGRAM_CID          26   /* adtc                    R1  */
#define MMC_PROGRAM_CSD          27   /* adtc                    R1  */

  /* class 6 */
#define MMC_SET_WRITE_PROT       28   /* ac   [31:0] data addr   R1b */
#define MMC_CLR_WRITE_PROT       29   /* ac   [31:0] data addr   R1b */
#define MMC_SEND_WRITE_PROT      30   /* adtc [31:0] wpdata addr R1  */

  /* class 5 */
#define MMC_ERASE_GROUP_START    35   /* ac   [31:0] data addr   R1  */
#define MMC_ERASE_GROUP_END      36   /* ac   [31:0] data addr   R1  */
#define MMC_ERASE                37   /* ac                      R1b */

  /* class 9 */
#define MMC_FAST_IO              39   /* ac   <Complex>          R4  */
#define MMC_GO_IRQ_STATE         40   /* bcr                     R5  */

  /* class 7 */
#define MMC_LOCK_UNLOCK          42   /* adtc                    R1b */

  /* class 8 */
#define MMC_APP_CMD              55   /* ac   [31:16] RCA        R1  */
#define MMC_GEN_CMD              56   /* adtc [0] RD/WR          R1b */

  /* SD class */
#define SD_SEND_OP_COND          41   /* bcr  [31:0] OCR         R3  */
#define SET_BUS_WIDTH            6    /* ac   [1:0] bus width    R1  */
#define SEND_SCR                 51   /* adtc [31:0] staff       R1  */

/* Don't change the order of these; they are used in dispatch tables */
enum mmc_rsp_t
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

enum card_state
{
    CARD_STATE_EMPTY = -1,
    CARD_STATE_IDLE  = 0,
    CARD_STATE_READY = 1,
    CARD_STATE_IDENT = 2,
    CARD_STATE_STBY  = 3,
    CARD_STATE_TRAN  = 4,
    CARD_STATE_DATA  = 5,
    CARD_STATE_RCV   = 6,
    CARD_STATE_PRG   = 7,
    CARD_STATE_DIS   = 8,
};

/* These are unpacked versions of the actual responses */
struct mmc_response_r1
{
    unsigned char  cmd;
    unsigned int   status;
};

struct mmc_cid
{
    unsigned char  mid;
    unsigned short oid;
    unsigned char  pnm[7];   /* Product name (we null-terminate) */
    unsigned char  prv;
    unsigned int   psn;
    unsigned char  mdt;
};

struct mmc_csd
{
    unsigned char  csd_structure;
    unsigned char  spec_vers;
    unsigned char  taac;
    unsigned char  nsac;
    unsigned char  tran_speed;
    unsigned short ccc;
    unsigned char  read_bl_len;
    unsigned char  read_bl_partial;
    unsigned char  write_blk_misalign;
    unsigned char  read_blk_misalign;
    unsigned char  dsr_imp;
    unsigned short c_size;
    unsigned char  vdd_r_curr_min;
    unsigned char  vdd_r_curr_max;
    unsigned char  vdd_w_curr_min;
    unsigned char  vdd_w_curr_max;
    unsigned char  c_size_mult;
    union
    {
        struct /* MMC system specification version 3.1 */
        {
            unsigned char  erase_grp_size;  
            unsigned char  erase_grp_mult; 
        } v31;
        struct /* MMC system specification version 2.2 */
        {
            unsigned char  sector_size;
            unsigned char  erase_grp_size;
        } v22;
    } erase;
    unsigned char  wp_grp_size;
    unsigned char  wp_grp_enable;
    unsigned char  default_ecc;
    unsigned char  r2w_factor;
    unsigned char  write_bl_len;
    unsigned char  write_bl_partial;
    unsigned char  file_format_grp;
    unsigned char  copy;
    unsigned char  perm_write_protect;
    unsigned char  tmp_write_protect;
    unsigned char  file_format;
    unsigned char  ecc;
};

struct mmc_response_r3
{  
    unsigned int ocr;
}; 

#define MMC_VDD_145_150  0x00000001    /* VDD voltage 1.45 - 1.50 */
#define MMC_VDD_150_155  0x00000002    /* VDD voltage 1.50 - 1.55 */
#define MMC_VDD_155_160  0x00000004    /* VDD voltage 1.55 - 1.60 */
#define MMC_VDD_160_165  0x00000008    /* VDD voltage 1.60 - 1.65 */
#define MMC_VDD_165_170  0x00000010    /* VDD voltage 1.65 - 1.70 */
#define MMC_VDD_17_18    0x00000020    /* VDD voltage 1.7 - 1.8 */
#define MMC_VDD_18_19    0x00000040    /* VDD voltage 1.8 - 1.9 */
#define MMC_VDD_19_20    0x00000080    /* VDD voltage 1.9 - 2.0 */
#define MMC_VDD_20_21    0x00000100    /* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22    0x00000200    /* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23    0x00000400    /* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24    0x00000800    /* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25    0x00001000    /* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26    0x00002000    /* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27    0x00004000    /* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28    0x00008000    /* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29    0x00010000    /* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30    0x00020000    /* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31    0x00040000    /* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32    0x00080000    /* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33    0x00100000    /* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34    0x00200000    /* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35    0x00400000    /* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36    0x00800000    /* VDD voltage 3.5 ~ 3.6 */
#define MMC_CARD_BUSY    0x80000000    /* Card Power up status bit */


/* CSD field definitions */
#define CSD_STRUCT_VER_1_0  0          /* Valid for system specification 1.0 - 1.2 */
#define CSD_STRUCT_VER_1_1  1          /* Valid for system specification 1.4 - 2.2 */
#define CSD_STRUCT_VER_1_2  2          /* Valid for system specification 3.1       */

#define CSD_SPEC_VER_0      0          /* Implements system specification 1.0 - 1.2 */
#define CSD_SPEC_VER_1      1          /* Implements system specification 1.4 */
#define CSD_SPEC_VER_2      2          /* Implements system specification 2.0 - 2.2 */
#define CSD_SPEC_VER_3      3          /* Implements system specification 3.1 */

/* the information structure of MMC/SD Card */
typedef struct MMC_INFO
{
    int             id;     /* Card index */
    int             sd;     /* MMC or SD card */
    int             rca;    /* RCA */
    unsigned int    scr;    /* SCR 63:32*/        
    int             flags;  /* Ejected, inserted */
    enum card_state state;  /* empty, ident, ready, whatever */

    /* Card specific information */
    struct mmc_cid  cid;
    struct mmc_csd  csd;
    unsigned int    block_num;
    unsigned int    block_len;
    unsigned int    erase_unit;
} mmc_info;

static mmc_info mmcinfo;

struct mmc_request
{
    int               index;      /* Slot index - used for CS lines */
    int               cmd;        /* Command to send */
    unsigned int      arg;        /* Argument to send */
    enum mmc_rsp_t    rtype;      /* Response type expected */

    /* Data transfer (these may be modified at the low level) */
    unsigned short    nob;        /* Number of blocks to transfer*/
    unsigned short    block_len;  /* Block length */
    unsigned char     *buffer;    /* Data buffer */
    unsigned int      cnt;        /* Data length, for PIO */

    /* Results */
    unsigned char     response[18]; /* Buffer to store response - CRC is optional */
    enum mmc_result_t result;
};

#define MMC_OCR_ARG             0x00ff8000  /* Argument of OCR */

/***********************************************************************
 *  MMC Events
 */
#define MMC_EVENT_NONE            0x00    /* No events */
#define MMC_EVENT_RX_DATA_DONE    0x01    /* Rx data done */
#define MMC_EVENT_TX_DATA_DONE    0x02    /* Tx data done */
#define MMC_EVENT_PROG_DONE       0x04    /* Programming is done */

static int use_4bit;        /* Use 4-bit data bus */
static int num_6;
static int sd2_0 = 1;

/* Stop the MMC clock and wait while it happens */
static inline int jz_mmc_stop_clock(void)
{
    int timeout = 1000;

    DEBUG("stop MMC clock");
    REG_MSC_STRPCL = MSC_STRPCL_CLOCK_CONTROL_STOP;

    while (timeout && (REG_MSC_STAT & MSC_STAT_CLK_EN))
    {
        timeout--;
        if (timeout == 0)
        {
            DEBUG("Timeout on stop clock waiting");
            return MMC_ERROR_TIMEOUT;
        }
        udelay(1);
    }
    DEBUG("clock off time is %d microsec", timeout);
    return MMC_NO_ERROR;
}

/* Start the MMC clock and operation */
static inline int jz_mmc_start_clock(void)
{
    REG_MSC_STRPCL =
        MSC_STRPCL_CLOCK_CONTROL_START | MSC_STRPCL_START_OP;
    return MMC_NO_ERROR;
}

static inline unsigned int jz_mmc_calc_clkrt(int is_sd, unsigned int rate)
{
    unsigned int clkrt;
    unsigned int clk_src = is_sd ? 24000000 : 20000000;

    clkrt = 0;
    while (rate < clk_src)
    {
        clkrt++;
        clk_src >>= 1;
    }
    return clkrt;
}

static int jz_mmc_check_status(struct mmc_request *request)
{
    unsigned int status = REG_MSC_STAT;

    /* Checking for response or data timeout */
    if (status & (MSC_STAT_TIME_OUT_RES | MSC_STAT_TIME_OUT_READ))
    {
        DEBUG("MMC/SD timeout, MMC_STAT 0x%x CMD %d", status,
               request->cmd);
        return MMC_ERROR_TIMEOUT;
    }

    /* Checking for CRC error */
    if (status &
        (MSC_STAT_CRC_READ_ERROR | MSC_STAT_CRC_WRITE_ERROR |
         MSC_STAT_CRC_RES_ERR))
    {
        DEBUG("MMC/CD CRC error, MMC_STAT 0x%x", status);
        return MMC_ERROR_CRC;
    }

    return MMC_NO_ERROR;
}

/* Obtain response to the command and store it to response buffer */
static void jz_mmc_get_response(struct mmc_request *request)
{
    int i;
    unsigned char *buf;
    unsigned int data;

    DEBUG("fetch response for request %d, cmd %d", request->rtype,
          request->cmd);
    buf = request->response;
    request->result = MMC_NO_ERROR;

    switch (request->rtype)
    {
        case RESPONSE_R1:
        case RESPONSE_R1B:
        case RESPONSE_R6:
        case RESPONSE_R3:
        case RESPONSE_R4:
        case RESPONSE_R5:
        {
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
        }
        case RESPONSE_R2_CID:
        case RESPONSE_R2_CSD:
        {
            for (i = 0; i < 16; i += 2)
            {
                data = REG_MSC_RES;
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

#ifdef MMC_DMA_ENABLE
static int jz_mmc_receive_data_dma(struct mmc_request *req)
{
    int ch = RX_DMA_CHANNEL;
    unsigned int size = req->block_len * req->nob;
    unsigned char err = 0;

    /* flush dcache */
    dma_cache_wback_inv((unsigned long) req->buffer, size);
    /* setup dma channel */
    REG_DMAC_DSAR(ch) = PHYSADDR(MSC_RXFIFO);    /* DMA source addr */
    REG_DMAC_DTAR(ch) = PHYSADDR((unsigned long) req->buffer);    /* DMA dest addr */
    REG_DMAC_DTCR(ch) = (size + 3) / 4;    /* DMA transfer count */
    REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_MSCIN;    /* DMA request type */

#if MMC_DMA_INTERRUPT
    REG_DMAC_DCMD(ch) =
        DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
        DMAC_DCMD_DS_32BIT | DMAC_DCMD_TIE;
    REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
    OSSemPend(mmc_dma_rx_sem, 100, &err);
#else
    REG_DMAC_DCMD(ch) =
        DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
        DMAC_DCMD_DS_32BIT;
    REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
    while (REG_DMAC_DTCR(ch));
#endif
/* clear status and disable channel */
    REG_DMAC_DCCSR(ch) = 0;
#if MMC_DMA_INTERRUPT
    return (err == OS_NO_ERR);
#else
    return 0;
#endif
}

static int jz_mmc_transmit_data_dma(struct mmc_request *req)
{
    int ch = TX_DMA_CHANNEL;
    unsigned int size = req->block_len * req->nob;
    unsigned char err = 0;

    /* flush dcache */
    dma_cache_wback_inv((unsigned long) req->buffer, size);
    /* setup dma channel */
    REG_DMAC_DSAR(ch) = PHYSADDR((unsigned long) req->buffer);    /* DMA source addr */
    REG_DMAC_DTAR(ch) = PHYSADDR(MSC_TXFIFO);    /* DMA dest addr */
    REG_DMAC_DTCR(ch) = (size + 3) / 4;    /* DMA transfer count */
    REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_MSCOUT;    /* DMA request type */

#if MMC_DMA_INTERRUPT
    REG_DMAC_DCMD(ch) =
        DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
        DMAC_DCMD_DS_32BIT | DMAC_DCMD_TIE;
    REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
    OSSemPend(mmc_dma_tx_sem, 100, &err);
#else
    REG_DMAC_DCMD(ch) =
        DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
        DMAC_DCMD_DS_32BIT;
    REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
    /* wait for dma completion */
    while (REG_DMAC_DTCR(ch));
#endif
    /* clear status and disable channel */
    REG_DMAC_DCCSR(ch) = 0;
#if MMC_DMA_INTERRUPT
    return (err == OS_NO_ERR);
#else
    return 0;
#endif
}

#endif                /* MMC_DMA_ENABLE */

static int jz_mmc_receive_data(struct mmc_request *req)
{
    unsigned int nob = req->nob;
    unsigned int wblocklen = (unsigned int) (req->block_len + 3) >> 2;    /* length in word */
    unsigned char *buf = req->buffer;
    unsigned int *wbuf = (unsigned int *) buf;
    unsigned int waligned = (((unsigned int) buf & 0x3) == 0);    /* word aligned ? */
    unsigned int stat, timeout, data, cnt;

    for (; nob >= 1; nob--)
    {
        timeout = 0x3FFFFFF;

        while (timeout)
        {
            timeout--;
            stat = REG_MSC_STAT;

            if (stat & MSC_STAT_TIME_OUT_READ)
                return MMC_ERROR_TIMEOUT;
            else if (stat & MSC_STAT_CRC_READ_ERROR)
                return MMC_ERROR_CRC;
            else if (!(stat & MSC_STAT_DATA_FIFO_EMPTY)
                 || (stat & MSC_STAT_DATA_FIFO_AFULL)) {
                /* Ready to read data */
                break;
            }

            udelay(1);
        }

        if (!timeout)
            return MMC_ERROR_TIMEOUT;

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

    return MMC_NO_ERROR;
}

static int jz_mmc_transmit_data(struct mmc_request *req)
{
    unsigned int nob = req->nob;
    unsigned int wblocklen = (unsigned int) (req->block_len + 3) >> 2;    /* length in word */
    unsigned char *buf = req->buffer;
    unsigned int *wbuf = (unsigned int *) buf;
    unsigned int waligned = (((unsigned int) buf & 0x3) == 0);    /* word aligned ? */
    unsigned int stat, timeout, data, cnt;

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
                return MMC_ERROR_CRC;
            else if (!(stat & MSC_STAT_DATA_FIFO_FULL))
            {
                /* Ready to write data */
                break;
            }

            udelay(1);
        }

        if (!timeout)
            return MMC_ERROR_TIMEOUT;

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

    return MMC_NO_ERROR;
}

/* Set the MMC clock frequency */
static void jz_mmc_set_clock(int sd, unsigned int rate)
{
    int clkrt = 0;

    sd = sd ? 1 : 0;

    jz_mmc_stop_clock();

    if(sd2_0)
    {
        __cpm_select_msc_hs_clk(sd);    /* select clock source from CPM */
        REG_CPM_CPCCR |= CPM_CPCCR_CE;
        REG_MSC_CLKRT = 0;
    }
    else
    {
        __cpm_select_msc_clk(sd);    /* select clock source from CPM */
        REG_CPM_CPCCR |= CPM_CPCCR_CE;
        clkrt = jz_mmc_calc_clkrt(sd, rate);
        REG_MSC_CLKRT = clkrt;
    }
    DEBUG("set clock to %u Hz is_sd=%d clkrt=%d", rate, sd, clkrt);
}

/********************************************************************************************************************
** Name:      int jz_mmc_exec_cmd()
** Function:  send command to the card, and get a response
** Input:     struct mmc_request *req: MMC/SD request
** Output:    0:  right        >0:  error code
********************************************************************************************************************/
static int jz_mmc_exec_cmd(struct mmc_request *request)
{
    unsigned int cmdat = 0, events = 0;
    int retval, timeout = 0x3fffff;

    /* Indicate we have no result yet */
    request->result = MMC_NO_RESPONSE;
    if (request->cmd == MMC_CIM_RESET)
    {
        /* On reset, 1-bit bus width */
        use_4bit = 0;

        /* Reset MMC/SD controller */
        __msc_reset();

        /* On reset, drop MMC clock down */
        jz_mmc_set_clock(0, MMC_CLOCK_SLOW);

        /* On reset, stop MMC clock */
        jz_mmc_stop_clock();
    }
#if 0
    if (request->cmd == MMC_SEND_OP_COND)
    {
        DEBUG("Have a MMC card");
        /* always use 1bit for MMC */
        use_4bit = 0;
    }
#endif
    if (request->cmd == SET_BUS_WIDTH)
    {
        if (request->arg == 0x2)
        {
            DEBUG("Use 4-bit bus width");
            use_4bit = 1;
        }
        else
        {
            DEBUG("Use 1-bit bus width");
            use_4bit = 0;
        }
    }

    /* stop clock */
    jz_mmc_stop_clock();

    /* mask all interrupts */
    REG_MSC_IMASK = 0xffff;
    /* clear status */
    REG_MSC_IREG = 0xffff;
    /*open interrupt */
    REG_MSC_IMASK = (~7);
    /* use 4-bit bus width when possible */
    if (use_4bit)
        cmdat |= MSC_CMDAT_BUS_WIDTH_4BIT;

    /* Set command type and events */
    switch (request->cmd)
    {
        /* MMC core extra command */
        case MMC_CIM_RESET:
            cmdat |= MSC_CMDAT_INIT;    /* Initialization sequence sent prior to command */
            break;

            /* bc - broadcast - no response */
        case MMC_GO_IDLE_STATE:
        case MMC_SET_DSR:
            break;

            /* bcr - broadcast with response */
        case MMC_SEND_OP_COND:
        case MMC_ALL_SEND_CID:

        case MMC_GO_IRQ_STATE:
            break;

            /* adtc - addressed with data transfer */
        case MMC_READ_DAT_UNTIL_STOP:
        case MMC_READ_SINGLE_BLOCK:
        case MMC_READ_MULTIPLE_BLOCK:
        case SEND_SCR:
#if defined(MMC_DMA_ENABLE)
            cmdat |=
                MSC_CMDAT_DATA_EN | MSC_CMDAT_READ | MSC_CMDAT_DMA_EN;
#else
            cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_READ;
#endif
            events = MMC_EVENT_RX_DATA_DONE;
            break;

        case 6:
            if (num_6 < 2)
            {
#if defined(MMC_DMA_ENABLE)
                cmdat |=
                    MSC_CMDAT_DATA_EN | MSC_CMDAT_READ |
                    MSC_CMDAT_DMA_EN;
#else
                cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_READ;
#endif
                events = MMC_EVENT_RX_DATA_DONE;
            }
            break;

        case MMC_WRITE_DAT_UNTIL_STOP:
        case MMC_WRITE_BLOCK:
        case MMC_WRITE_MULTIPLE_BLOCK:
        case MMC_PROGRAM_CID:
        case MMC_PROGRAM_CSD:
        case MMC_SEND_WRITE_PROT:
        case MMC_GEN_CMD:
        case MMC_LOCK_UNLOCK:
#if defined(MMC_DMA_ENABLE)
            cmdat |=
                MSC_CMDAT_DATA_EN | MSC_CMDAT_WRITE | MSC_CMDAT_DMA_EN;
#else
            cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_WRITE;
#endif
            events = MMC_EVENT_TX_DATA_DONE | MMC_EVENT_PROG_DONE;
            break;

        case MMC_STOP_TRANSMISSION:
            events = MMC_EVENT_PROG_DONE;
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
        /*FALLTHRU*/
        case RESPONSE_R1:
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

    /* Set command index */
    if (request->cmd == MMC_CIM_RESET)
        REG_MSC_CMD = MMC_GO_IDLE_STATE;
    else
        REG_MSC_CMD = request->cmd;

    /* Set argument */
    REG_MSC_ARG = request->arg;

    /* Set block length and nob */
    if (request->cmd == SEND_SCR)
    {    /* get SCR from DataFIFO */
        REG_MSC_BLKLEN = 8;
        REG_MSC_NOB = 1;
    }
    else
    {
        REG_MSC_BLKLEN = request->block_len;
        REG_MSC_NOB = request->nob;
    }

    /* Set command */
    REG_MSC_CMDAT = cmdat;

    DEBUG("Send cmd %d cmdat: %x arg: %x resp %d", request->cmd,
          cmdat, request->arg, request->rtype);

    /* Start MMC/SD clock and send command to card */
    jz_mmc_start_clock();

    /* Wait for command completion */
    //__intc_unmask_irq(IRQ_MSC);
    //wakeup_wait(&sd_wakeup, 100);
    while (timeout-- && !(REG_MSC_STAT & MSC_STAT_END_CMD_RES));


    if (timeout == 0)
        return MMC_ERROR_TIMEOUT;

    REG_MSC_IREG = MSC_IREG_END_CMD_RES;    /* clear flag */

    /* Check for status */
    retval = jz_mmc_check_status(request);
    if (retval)
        return retval;

    /* Complete command with no response */
    if (request->rtype == RESPONSE_NONE)
        return MMC_NO_ERROR;

    /* Get response */
    jz_mmc_get_response(request);

    /* Start data operation */
    if (events & (MMC_EVENT_RX_DATA_DONE | MMC_EVENT_TX_DATA_DONE))
    {
        if (events & MMC_EVENT_RX_DATA_DONE)
        {
            if (request->cmd == SEND_SCR)
            {
                /* SD card returns SCR register as data. 
                   MMC core expect it in the response buffer, 
                   after normal response. */
                request->buffer =
                    (unsigned char *) ((unsigned int) request->response + 5);
            }
#ifdef MMC_DMA_ENABLE
            jz_mmc_receive_data_dma(request);
#else
            jz_mmc_receive_data(request);
#endif
        }

        if (events & MMC_EVENT_TX_DATA_DONE)
        {
#ifdef MMC_DMA_ENABLE
            jz_mmc_transmit_data_dma(request);
#else
            jz_mmc_transmit_data(request);
#endif
        }
        //__intc_unmask_irq(IRQ_MSC);
        //wakeup_wait(&sd_wakeup, 100);
        /* Wait for Data Done */
        while (!(REG_MSC_IREG & MSC_IREG_DATA_TRAN_DONE));
        REG_MSC_IREG = MSC_IREG_DATA_TRAN_DONE;    /* clear status */

    }

    /* Wait for Prog Done event */
    if (events & MMC_EVENT_PROG_DONE)
    {
        //__intc_unmask_irq(IRQ_MSC);
        //wakeup_wait(&sd_wakeup, 100);
        while (!(REG_MSC_IREG & MSC_IREG_PRG_DONE));
        REG_MSC_IREG = MSC_IREG_PRG_DONE;    /* clear status */
    }

    /* Command completed */

    return MMC_NO_ERROR;    /* return successfully */
}

/*******************************************************************************************************************
** Name:      int mmc_chkcard()
** Function:  check whether card is insert entirely
** Input:     NULL
** Output:    1: insert entirely    0: not insert entirely
********************************************************************************************************************/
static int jz_mmc_chkcard(void)
{
    return (MMC_INSERT_STATUS() == 0 ? 1 : 0);
}

#if MMC_DMA_INTERRUPT
static void jz_mmc_tx_handler(unsigned int arg)
{
    if (__dmac_channel_address_error_detected(arg))
    {
        DEBUG("%s: DMAC address error.", __FUNCTION__);
        __dmac_channel_clear_address_error(arg);
    }
    if (__dmac_channel_transmit_end_detected(arg))
    {

        __dmac_channel_clear_transmit_end(arg);
        OSSemPost(mmc_dma_tx_sem);
    }
}

static void jz_mmc_rx_handler(unsigned int arg)
{
    if (__dmac_channel_address_error_detected(arg))
    {
        DEBUG("%s: DMAC address error.", __FUNCTION__);
        __dmac_channel_clear_address_error(arg);
    }
    if (__dmac_channel_transmit_end_detected(arg))
    {
        __dmac_channel_clear_transmit_end(arg);
        OSSemPost(mmc_dma_rx_sem);
    }
}
#endif

/* MSC interrupt handler */
void MSC(void)
{
    //wakeup_signal(&sd_wakeup);
}

/*******************************************************************************************************************
** Name:      void mmc_hardware_init()
** Function:  initialize the hardware condiction that access sd card
** Input:     NULL
** Output:    NULL
********************************************************************************************************************/
static void jz_mmc_hardware_init(void)
{
    mmc_init_gpio();     /* init GPIO */
    __cpm_start_msc();   /* enable mmc clock */
#ifdef MMC_POWER_ON
    MMC_POWER_ON();      /* turn on power of card */
#endif
    MMC_RESET();         /* reset mmc/sd controller */
    MMC_IRQ_MASK();      /* mask all IRQs */
    jz_mmc_stop_clock(); /* stop MMC/SD clock */
#ifdef MMC_DMA_ENABLE
//    __cpm_start_dmac();
//    __dmac_enable_module();
//      REG_DMAC_DMACR = DMAC_DMACR_DME;
#if MMC_DMA_INTERRUPT
    mmc_dma_rx_sem = OSSemCreate(0);
    mmc_dma_tx_sem = OSSemCreate(0);
    request_irq(IRQ_DMA_0 + RX_DMA_CHANNEL, jz_mmc_rx_handler,
            RX_DMA_CHANNEL);
    request_irq(IRQ_DMA_0 + TX_DMA_CHANNEL, jz_mmc_tx_handler,
            TX_DMA_CHANNEL);
#endif
#endif
}

static void mmc_send_cmd(struct mmc_request *request, int cmd, unsigned int arg,
                         unsigned short nob, unsigned short block_len,
                         enum mmc_rsp_t rtype, unsigned char* buffer)
{
    request->cmd = cmd;
    request->arg = arg;
    request->rtype = rtype;
    request->nob = nob;
    request->block_len = block_len;
    request->buffer = buffer;
    request->cnt = nob * block_len;
    logf("mmc_send_cmd: command = %d",cmd);
    jz_mmc_exec_cmd(request);
}

static bool inited = false;
int _sd_init(void)
{
    if(!inited)
    {
        jz_mmc_hardware_init();
        wakeup_init(&sd_wakeup);
        inited = true;
    }
    
    struct mmc_request test;
    //mmc_send_cmd(&test, MMC_CIM_RESET, 0, 0, 0, RESPONSE_NONE, NULL);
    mmc_send_cmd(&test, MMC_GO_IDLE_STATE, 0, 0, 0, RESPONSE_NONE, NULL);
    mmc_send_cmd(&test, SD_SEND_OP_COND, MMC_OCR_ARG, 0, 0, RESPONSE_R3, NULL);
    
    return 0;
}

bool card_detect_target(void)
{
    return (jz_mmc_chkcard() == 1);
}

#ifdef HAVE_HOTSWAP
void card_enable_monitoring_target(bool on)
{
    if(on)
    {
        
    }
    else
    {
        
    }
}
#endif

/* TODO */
tCardInfo* card_get_info_target(int card_no)
{
    static tCardInfo card;
    
    return &card;
}

/* TODO */
int _sd_read_sectors(unsigned long start, int count, void* buf)
{
    (void)start;
    (void)count;
    (void)buf;
    return -1;
}

/* TODO */
int _sd_write_sectors(unsigned long start, int count, const void* buf)
{
    (void)start;
    (void)count;
    (void)buf;
    return -1;
}

#ifdef STORAGE_GET_INFO
void sd_get_info(IF_MV2(int drive,) struct storage_info *info)
{
    (void)drive;
    /* firmware version */
    info->revision="0.00";

    info->vendor="Rockbox";
    info->product="SD Storage";

    /* blocks count */
    /* TODO: proper amount of sectors! */
    info->num_sectors = 0;
    info->sector_size = 512;
}
#endif
