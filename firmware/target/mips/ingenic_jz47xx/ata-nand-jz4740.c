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
//#include "ata-nand-target.h" /* TODO */
#include "nand_id.h"
#include "system.h"
#include "panic.h"
#include "kernel.h"
#include "storage.h"
#include "buffer.h"
#include "string.h"

//#define USE_DMA

/*
 * Standard NAND flash commands
 */
#define NAND_CMD_READ0         0
#define NAND_CMD_READ1         1
#define NAND_CMD_RNDOUT        5
#define NAND_CMD_PAGEPROG      0x10
#define NAND_CMD_READOOB       0x50
#define NAND_CMD_ERASE1        0x60
#define NAND_CMD_STATUS        0x70
#define NAND_CMD_STATUS_MULTI  0x71
#define NAND_CMD_SEQIN         0x80
#define NAND_CMD_RNDIN         0x85
#define NAND_CMD_READID        0x90
#define NAND_CMD_ERASE2        0xd0
#define NAND_CMD_RESET         0xff

/* Extended commands for large page devices */
#define NAND_CMD_READSTART    0x30
#define NAND_CMD_RNDOUTSTART  0xE0
#define NAND_CMD_CACHEDPROG   0x15

/* Status bits */
#define NAND_STATUS_FAIL       0x01
#define NAND_STATUS_FAIL_N1    0x02
#define NAND_STATUS_TRUE_READY 0x20
#define NAND_STATUS_READY      0x40
#define NAND_STATUS_WP         0x80

/*
 * NAND parameter struct
 */
struct nand_param
{
    unsigned int bus_width;        /* data bus width: 8-bit/16-bit */
    unsigned int row_cycle;        /* row address cycles: 2/3 */
    unsigned int page_size;        /* page size in bytes: 512/2048 */
    unsigned int oob_size;         /* oob size in bytes: 16/64 */
    unsigned int page_per_block;   /* pages per block: 32/64/128 */
};

/*
 * jz4740_nand.c
 *
 * NAND read routine for JZ4740
 *
 * Copyright (c) 2005-2008 Ingenic Semiconductor Inc.
 *
 */

#define NAND_DATAPORT        0xb8000000
#define NAND_ADDRPORT        0xb8010000
#define NAND_COMMPORT        0xb8008000

#define ECC_BLOCK        512
#define ECC_POS          6
#define PAR_SIZE         9

#define __nand_cmd(n)      (REG8(NAND_COMMPORT) = (n))
#define __nand_addr(n)     (REG8(NAND_ADDRPORT) = (n))
#define __nand_data8()     REG8(NAND_DATAPORT)
#define __nand_data16()    REG16(NAND_DATAPORT)

#define __nand_enable()        (REG_EMC_NFCSR |= EMC_NFCSR_NFE1 | EMC_NFCSR_NFCE1)
#define __nand_disable()       (REG_EMC_NFCSR &= ~(EMC_NFCSR_NFCE1))
#define __nand_ecc_rs_encoding() \
    (REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST | EMC_NFECR_RS | EMC_NFECR_RS_ENCODING)
#define __nand_ecc_rs_decoding() \
    (REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST | EMC_NFECR_RS | EMC_NFECR_RS_DECODING)
#define __nand_ecc_disable()     (REG_EMC_NFECR &= ~EMC_NFECR_ECCE)
#define __nand_ecc_encode_sync() while (!(REG_EMC_NFINTS & EMC_NFINTS_ENCF))
#define __nand_ecc_decode_sync() while (!(REG_EMC_NFINTS & EMC_NFINTS_DECF))

/*--------------------------------------------------------------*/

static struct nand_info* chip_info = NULL;
static struct nand_param internal_param;
#ifdef USE_DMA
static struct mutex nand_mtx;
static struct wakeup nand_wkup;
#endif
static unsigned char temp_page[4096]; /* Max page size */

static inline void jz_nand_wait_ready(void)
{
    unsigned int timeout = 1000;
    while ((REG_GPIO_PXPIN(2) & 0x40000000) && timeout--);
    while (!(REG_GPIO_PXPIN(2) & 0x40000000));
}

#ifndef USE_DMA

static inline void jz_nand_read_buf16(void *buf, int count)
{
    int i;
    unsigned short *p = (unsigned short *)buf;

    for (i = 0; i < count; i += 2)
        *p++ = __nand_data16();
}

static inline void jz_nand_read_buf8(void *buf, int count)
{
    int i;
    unsigned char *p = (unsigned char *)buf;

    for (i = 0; i < count; i++)
        *p++ = __nand_data8();
}

#else

static void jz_nand_write_dma(void *source, unsigned int len, int bw)
{
    mutex_lock(&nand_mtx);
    
    if(((unsigned int)source < 0xa0000000) && len)
         dma_cache_wback_inv((unsigned long)source, len);
    
    dma_enable();

    REG_DMAC_DCCSR(DMA_NAND_CHANNEL) = DMAC_DCCSR_NDES;
    REG_DMAC_DSAR(DMA_NAND_CHANNEL) = PHYSADDR((unsigned long)source);       
    REG_DMAC_DTAR(DMA_NAND_CHANNEL) = PHYSADDR((unsigned long)NAND_DATAPORT);       
    REG_DMAC_DTCR(DMA_NAND_CHANNEL) = len / 16;                        
    REG_DMAC_DRSR(DMA_NAND_CHANNEL) = DMAC_DRSR_RS_AUTO;                
    REG_DMAC_DCMD(DMA_NAND_CHANNEL) = (DMAC_DCMD_SAI| DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DS_16BYTE |
                                       (bw == 8 ? DMAC_DCMD_DWDH_8 : DMAC_DCMD_DWDH_16));
    
    REG_DMAC_DCCSR(DMA_NAND_CHANNEL) |= DMAC_DCCSR_EN; /* Enable DMA channel */
#if 1
    while( REG_DMAC_DTCR(DMA_NAND_CHANNEL) )
        yield();
#else
    REG_DMAC_DCMD(DMA_NAND_CHANNEL) |= DMAC_DCMD_TIE;  /* Enable DMA interrupt */
    wakeup_wait(&nand_wkup, TIMEOUT_BLOCK);
#endif

    REG_DMAC_DCCSR(DMA_NAND_CHANNEL) &= ~DMAC_DCCSR_EN; /* Disable DMA channel */
    
    dma_disable();
    
    mutex_unlock(&nand_mtx);
}

static void jz_nand_read_dma(void *target, unsigned int len, int bw)
{
    mutex_lock(&nand_mtx);
    
    if(((unsigned int)target < 0xa0000000) && len)
        dma_cache_wback_inv((unsigned long)target, len);

    dma_enable();
    
    REG_DMAC_DCCSR(DMA_NAND_CHANNEL) = DMAC_DCCSR_NDES ;
    REG_DMAC_DSAR(DMA_NAND_CHANNEL) = PHYSADDR((unsigned long)NAND_DATAPORT);       
    REG_DMAC_DTAR(DMA_NAND_CHANNEL) = PHYSADDR((unsigned long)target);       
    REG_DMAC_DTCR(DMA_NAND_CHANNEL) = len / 4;                        
    REG_DMAC_DRSR(DMA_NAND_CHANNEL) = DMAC_DRSR_RS_AUTO;                
    REG_DMAC_DCMD(DMA_NAND_CHANNEL) = (DMAC_DCMD_SAI| DMAC_DCMD_DAI | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BIT |
                                       (bw == 8 ? DMAC_DCMD_SWDH_8 : DMAC_DCMD_SWDH_16));
    REG_DMAC_DCCSR(DMA_NAND_CHANNEL) |= DMAC_DCCSR_EN; /* Enable DMA channel */
#if 1
    while( REG_DMAC_DTCR(DMA_NAND_CHANNEL) )
        yield();
#else
    REG_DMAC_DCMD(DMA_NAND_CHANNEL) |= DMAC_DCMD_TIE;  /* Enable DMA interrupt */
    wakeup_wait(&nand_wkup, TIMEOUT_BLOCK);
#endif

    //REG_DMAC_DCCSR(DMA_NAND_CHANNEL) &= ~DMAC_DCCSR_EN; /* Disable DMA channel */
    
    dma_disable();
    
    mutex_unlock(&nand_mtx);
}

void DMA_CALLBACK(DMA_NAND_CHANNEL)(void)
{
    if (REG_DMAC_DCCSR(DMA_NAND_CHANNEL) & DMAC_DCCSR_HLT)
        REG_DMAC_DCCSR(DMA_NAND_CHANNEL) &= ~DMAC_DCCSR_HLT;

    if (REG_DMAC_DCCSR(DMA_NAND_CHANNEL) & DMAC_DCCSR_AR)
        REG_DMAC_DCCSR(DMA_NAND_CHANNEL) &= ~DMAC_DCCSR_AR;

    if (REG_DMAC_DCCSR(DMA_NAND_CHANNEL) & DMAC_DCCSR_CT)
        REG_DMAC_DCCSR(DMA_NAND_CHANNEL) &= ~DMAC_DCCSR_CT;

    if (REG_DMAC_DCCSR(DMA_NAND_CHANNEL) & DMAC_DCCSR_TT)
        REG_DMAC_DCCSR(DMA_NAND_CHANNEL) &= ~DMAC_DCCSR_TT;
    
    wakeup_signal(&nand_wkup);
}

#endif

static inline void jz_nand_read_buf(void *buf, int count, int bw)
{
#ifdef USE_DMA
    if (bw == 8)
        jz_nand_read_dma(buf, count, 8);
    else
        jz_nand_read_dma(buf, count, 16);
#else
    if (bw == 8)
        jz_nand_read_buf8(buf, count);
    else
        jz_nand_read_buf16(buf, count);
#endif
}

/*
 * Correct 1~9-bit errors in 512-bytes data 
 */
static void jz_rs_correct(unsigned char *dat, int idx, int mask)
{
    int i, j;
    unsigned short d, d1, dm;

    i = (idx * 9) >> 3;
    j = (idx * 9) & 0x7;

    i = (j == 0) ? (i - 1) : i;
    j = (j == 0) ? 7 : (j - 1);

    if (i > 512) return;

    if (i == 512)
        d = dat[i - 1];
    else
        d = (dat[i] << 8) | dat[i - 1];

    d1 = (d >> j) & 0x1ff;
    d1 ^= mask;

    dm = ~(0x1ff << j);
    d = (d & dm) | (d1 << j);

    dat[i - 1] = d & 0xff;
    if (i < 512)
        dat[i] = (d >> 8) & 0xff;
}

/*
 * Read oob
 */
static int jz_nand_read_oob(int page_addr, unsigned char *buf, int size)
{
    struct nand_param *nandp = &internal_param;
    int page_size, row_cycle, bus_width;
    int col_addr;

    page_size = nandp->page_size;
    row_cycle = nandp->row_cycle;
    bus_width = nandp->bus_width;

    if (page_size >= 2048)
        col_addr = page_size;
    else
        col_addr = 0;

    if (page_size >= 2048)
        /* Send READ0 command */
        __nand_cmd(NAND_CMD_READ0);
    else
        /* Send READOOB command */
        __nand_cmd(NAND_CMD_READOOB);

    /* Send column address */
    __nand_addr(col_addr & 0xff);
    if (page_size >= 2048)
        __nand_addr((col_addr >> 8) & 0xff);

    /* Send page address */
    __nand_addr(page_addr & 0xff);
    __nand_addr((page_addr >> 8) & 0xff);
    if (row_cycle == 3)
        __nand_addr((page_addr >> 16) & 0xff);

    /* Send READSTART command for 2048 ps NAND */
    if (page_size >= 2048)
        __nand_cmd(NAND_CMD_READSTART);

    /* Wait for device ready */
    jz_nand_wait_ready();

    /* Read oob data */
    jz_nand_read_buf(buf, size, bus_width);

    return 0;
}


/*
 * nand_read_page()
 *
 * Input:
 *
 *    block - block number: 0, 1, 2, ...
 *    page - page number within a block: 0, 1, 2, ...
 *    dst - pointer to target buffer
 */
static int jz_nand_read_page(int block, int page, unsigned char *dst)
{
    struct nand_param *nandp = &internal_param;
    int page_size, oob_size, page_per_block;
    int row_cycle, bus_width, ecc_count;
    int page_addr, i, j;
    unsigned char *data_buf;
    unsigned char oob_buf[128];

    page_size = nandp->page_size;
    oob_size = nandp->oob_size;
    page_per_block = nandp->page_per_block;
    row_cycle = nandp->row_cycle;
    bus_width = nandp->bus_width;

    page_addr = page + block * page_per_block;

    /*
     * Read oob data
     */
    jz_nand_read_oob(page_addr, oob_buf, oob_size);

    /*
     * Read page data
     */

    /* Send READ0 command */
    __nand_cmd(NAND_CMD_READ0);

    /* Send column address */
    __nand_addr(0);
    if (page_size >= 2048)
        __nand_addr(0);

    /* Send page address */
    __nand_addr(page_addr & 0xff);
    __nand_addr((page_addr >> 8) & 0xff);
    if (row_cycle == 3)
        __nand_addr((page_addr >> 16) & 0xff);

    /* Send READSTART command for 2048 ps NAND */
    if (page_size >= 2048)
        __nand_cmd(NAND_CMD_READSTART);

    /* Wait for device ready */
    jz_nand_wait_ready();

    /* Read page data */
    data_buf = dst;

    ecc_count = page_size / ECC_BLOCK;

    for (i = 0; i < ecc_count; i++)
    {
        volatile unsigned char *paraddr = (volatile unsigned char *)EMC_NFPAR0;
        unsigned int stat;

        /* Enable RS decoding */
        REG_EMC_NFINTS = 0x0;
        __nand_ecc_rs_decoding();

        /* Read data */
        jz_nand_read_buf((void *)data_buf, ECC_BLOCK, bus_width);

        /* Set PAR values */
        for (j = 0; j < PAR_SIZE; j++)
            *paraddr++ = oob_buf[ECC_POS + i*PAR_SIZE + j];

        /* Set PRDY */
        REG_EMC_NFECR |= EMC_NFECR_PRDY;

        /* Wait for completion */
        __nand_ecc_decode_sync();

        /* Disable decoding */
        __nand_ecc_disable();

        /* Check result of decoding */
        stat = REG_EMC_NFINTS;
        if (stat & EMC_NFINTS_ERR)
        {
            /* Error occurred */
            if (stat & EMC_NFINTS_UNCOR)
            {
                /* Uncorrectable error occurred */
                panicf("Uncorrectable ECC error at NAND page 0x%x block 0x%x", page, block);
                return -1;
            }
            else
            {
                unsigned int errcnt, index, mask;

                errcnt = (stat & EMC_NFINTS_ERRCNT_MASK) >> EMC_NFINTS_ERRCNT_BIT;
                switch (errcnt)
                {
                case 4:
                    index = (REG_EMC_NFERR3 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT;
                    mask = (REG_EMC_NFERR3 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT;
                    jz_rs_correct(data_buf, index, mask);
                case 3:
                    index = (REG_EMC_NFERR2 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT;
                    mask = (REG_EMC_NFERR2 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT;
                    jz_rs_correct(data_buf, index, mask);
                case 2:
                    index = (REG_EMC_NFERR1 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT;
                    mask = (REG_EMC_NFERR1 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT;
                    jz_rs_correct(data_buf, index, mask);
                case 1:
                    index = (REG_EMC_NFERR0 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT;
                    mask = (REG_EMC_NFERR0 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT;
                    jz_rs_correct(data_buf, index, mask);
                    break;
                default:
                    break;
                }
            }
        }

        data_buf += ECC_BLOCK;
    }

    return 0;
}

/*
 * Enable NAND controller
 */
static void jz_nand_enable(void)
{
    __nand_enable();

    REG_EMC_SMCR1 = 0x04444400;
}

/*
 * Disable NAND controller
 */
static void jz_nand_disable(void)
{
    __nand_disable();
}

static int jz_nand_init(void)
{
    unsigned char cData[5];
    
    jz_nand_enable();
    
    __nand_cmd(NAND_CMD_READID);
    __nand_addr(NAND_CMD_READ0);
    cData[0] = __nand_data8();
    cData[1] = __nand_data8();
    cData[2] = __nand_data8();
    cData[3] = __nand_data8();
    cData[4] = __nand_data8();
    
    chip_info = nand_identify(cData);
    if(chip_info == NULL)
    {
        panicf("Unknown NAND flash chip: 0x%x 0x%x 0x%x 0x%x 0x%x", cData[0],
                                      cData[1], cData[2], cData[3], cData[4]);
        return -1; /* panicf() doesn't return though */
    }
    
    internal_param.bus_width = 8;
    internal_param.row_cycle = chip_info->row_cycles;
    internal_param.page_size = chip_info->page_size;
    internal_param.oob_size = chip_info->page_size/32;
    internal_param.page_per_block = chip_info->pages_per_block;
    
    return 0;
}

static bool inited = false;
int nand_init(void)
{
    int res = 0;
    
    if(!inited)
    {
        res = jz_nand_init();
#ifdef USE_DMA
        mutex_init(&nand_mtx);
        wakeup_init(&nand_wkup);
        system_enable_irq(DMA_IRQ(DMA_NAND_CHANNEL));
#endif
        
        inited = true;
    }

    return res;
}

int nand_read_sectors(IF_MV2(int drive,) unsigned long start, int count, void* buf)
{
    int i, ret = 0;
    
    start *= 512;
    count *= 512;
    
    if(count <= chip_info->page_size)
    {
        ret = jz_nand_read_page(start % (chip_info->page_size * chip_info->pages_per_block), start % chip_info->page_size, temp_page);
        memcpy(buf, temp_page, count);
        return ret;
    }
    else
    {
        for(i=0; i<count && ret == 0; i+=chip_info->page_size)
        {
            ret = jz_nand_read_page((start+i) % (chip_info->page_size * chip_info->pages_per_block), (start+i) % chip_info->page_size, temp_page);
            memcpy(buf+i, temp_page, (count-i < chip_info->page_size ? count-i : chip_info->page_size));
        }
        return ret;
    }
}

/* TODO */
int nand_write_sectors(IF_MV2(int drive,) unsigned long start, int count, const void* buf)
{
    (void)start;
    (void)count;
    (void)buf;
    return -1;
}

void nand_spindown(int seconds)
{
    /* null */
    (void)seconds;
}

bool nand_disk_is_active(void)
{
    /* null */
    return false;
}

void nand_sleep(void)
{
    /* null */
}

void nand_spin(void)
{
    /* null */
}

int nand_soft_reset(void)
{
    /* null */
    return 0;
}

void nand_enable(bool on)
{
    /* null - flash controller is enabled/disabled as needed. */
    (void)on;
}

#ifdef STORAGE_GET_INFO
void nand_get_info(IF_MV2(int drive,) struct storage_info *info)
{
    (void)drive;
    /* firmware version */
    info->revision="0.00";

    info->vendor="Rockbox";
    info->product="NAND Storage";

    /* blocks count */
    /* TODO: proper amount of sectors! */
    info->num_sectors = (chip_info->page_size / 512) * chip_info->pages_per_block * chip_info->blocks_per_bank;
    info->sector_size = 512;
}
#endif
