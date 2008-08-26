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
#include "nand_id.h"
#include "system.h"
#include "panic.h"

#define NAND_CMD_READ1_00                 0x00
#define NAND_CMD_READ1_01                 0x01
#define NAND_CMD_READ2                    0x50
#define NAND_CMD_READ_ID1                 0x90
#define NAND_CMD_READ_ID2                 0x91
#define NAND_CMD_RESET                    0xFF
#define NAND_CMD_PAGE_PROGRAM_START       0x80
#define NAND_CMD_PAGE_PROGRAM_STOP        0x10
#define NAND_CMD_BLOCK_ERASE_START        0x60
#define NAND_CMD_BLOCK_ERASE_CONFIRM      0xD0
#define NAND_CMD_READ_STATUS              0x70

#define NANDFLASH_CLE                     0x00008000 //PA[15]
#define NANDFLASH_ALE                     0x00010000 //PA[16]

#define NANDFLASH_BASE                    0xB8000000
#define REG_NAND_DATA8                    (*((volatile unsigned char *)NANDFLASH_BASE))
#define REG_NAND_DATA16                   (*((volatile unsigned short *)NANDFLASH_BASE))
#define REG_NAND_DATA                     REG_NAND_DATA8
#define REG_NAND_CMD                      (*((volatile unsigned char *)(NANDFLASH_BASE + NANDFLASH_CLE)))
#define REG_NAND_ADDR                     (*((volatile unsigned char *)(NANDFLASH_BASE + NANDFLASH_ALE)))

#define JZ_NAND_SELECT                    (REG_EMC_NFCSR |=  EMC_NFCSR_NFCE1 )
#define JZ_NAND_DESELECT                  (REG_EMC_NFCSR &= ~(EMC_NFCSR_NFCE1))

#define __nand_enable()                   (REG_EMC_NFCSR |= (EMC_NFCSR_NFE1 | EMC_NFCSR_NFCE1))
#define __nand_disable()                  (REG_EMC_NFCSR &= ~(EMC_NFCSR_NFCE1))
#define __nand_ecc_disable()              (REG_EMC_NFECR &= ~EMC_NFECR_ECCE)
#define __nand_ecc_encode_sync()          while (!(REG_EMC_NFINTS & EMC_NFINTS_ENCF))
#define __nand_ecc_decode_sync()          while (!(REG_EMC_NFINTS & EMC_NFINTS_DECF))
#define __nand_ecc_rs_encoding() \
    (REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST | EMC_NFECR_RS | EMC_NFECR_RS_ENCODING)
#define __nand_ecc_rs_decoding() \
    (REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST | EMC_NFECR_RS | EMC_NFECR_RS_DECODING)

#define my__gpio_as_nand()            \
do {                        \
    REG_GPIO_PXFUNS(1) = 0x1E018000;    \
    REG_GPIO_PXSELC(1) = 0x1E018000;    \
    REG_GPIO_PXFUNS(2) = 0x30000000;    \
    REG_GPIO_PXSELC(2) = 0x30000000;    \
    REG_GPIO_PXFUNC(2) = 0x40000000;    \
    REG_GPIO_PXSELC(2) = 0x40000000;    \
    REG_GPIO_PXDIRC(2) = 0x40000000;    \
    REG_GPIO_PXFUNS(1) = 0x00400000;    \
    REG_GPIO_PXSELC(1) = 0x00400000;    \
} while (0)

static struct nand_info* chip_info = NULL;
#define NAND_BLOCK_SIZE                   (chip_info->pages_per_block * chip_info->page_size)
#define NAND_OOB_SIZE                     (chip_info->page_size / 32)

struct nand_page_info_t
{
    unsigned char   block_status;
    unsigned int    reserved;
    unsigned short  block_addr_field;
    unsigned int    lifetime;
    unsigned char   ecc_field[50];//[NAND_OOB_SIZE - 11];    
} __attribute__ ((packed));

static void nand_wait_ready(void)
{
    int wait = 100;
    while(REG_GPIO_PXPIN(2) & (1 << 30) && wait--);
    while (!(REG_GPIO_PXPIN(2) & 0x40000000));
}

static inline void nand_read_memcpy(void *target, void *source, unsigned int len)
{
    int ch = 2;
	
	if(((unsigned int)source < 0xa0000000) && len)
		dma_cache_wback_inv((unsigned long)source, len);
	
	if(((unsigned int)target < 0xa0000000) && len)
		dma_cache_wback_inv((unsigned long)target, len);
	
	REG_DMAC_DSAR(ch) = PHYSADDR((unsigned long)source);       
	REG_DMAC_DTAR(ch) = PHYSADDR((unsigned long)target);       
	REG_DMAC_DTCR(ch) = len / 4;	            	    
	REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_AUTO;	        	
	REG_DMAC_DCMD(ch) = DMAC_DCMD_SAI | DMAC_DCMD_DAI | DMAC_DCMD_SWDH_8 | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BIT;
	REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
	while (	REG_DMAC_DTCR(ch) );
}

static void rs_correct(unsigned char *buf, int idx, int mask)
{
	int i, j;
	unsigned short d, d1, dm;

	i = (idx * 9) >> 3;
	j = (idx * 9) & 0x7;

	i = (j == 0) ? (i - 1) : i;
	j = (j == 0) ? 7 : (j - 1);
    if(i >= 512)
		return;
	
	d = (buf[i] << 8) | buf[i - 1];

	d1 = (d >> j) & 0x1ff;
	d1 ^= mask;

	dm = ~(0x1ff << j);
	d = (d & dm) | (d1 << j);
    
	buf[i - 1] = d & 0xff;
	buf[i] = (d >> 8) & 0xff;
	
	
}

static inline int nand_rs_correct(unsigned char *data)
{
	unsigned int stat = REG_EMC_NFINTS;
	if (stat & EMC_NFINTS_ERR) {
		if (stat & EMC_NFINTS_UNCOR)
        {
            panicf("Uncorrectable ECC error occurred!\n stat = 0x%x", stat);
			return -1;
		}
		else
        {
			unsigned int errcnt = (stat & EMC_NFINTS_ERRCNT_MASK) >> EMC_NFINTS_ERRCNT_BIT;
			switch (errcnt)
            {
    			case 4:
    				rs_correct(data, (REG_EMC_NFERR3 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR3 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
    				break;
    				
    			case 3:
    				rs_correct(data, (REG_EMC_NFERR2 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR2 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
    				break;
    				
    			case 2:
    				rs_correct(data, (REG_EMC_NFERR1 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR1 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
    				break;
    				
    			case 1:
    				rs_correct(data, (REG_EMC_NFERR0 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR0 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
    				break;
			}
		}
		
	}
	return 0;	
}

static inline void nand_send_readaddr(unsigned int pageaddr, unsigned int offset)
{
    int i;
    
    /* Read command */
    REG_NAND_CMD = 0x00;
    
    /* Write column address */
    for (i = 0; i < chip_info->col_cycles; i++)
    {
        REG_NAND_ADDR = offset & 0xFF;
        offset = offset >> 8;
    }

    /* Write row address */
    for (i = 0; i < chip_info->row_cycles; i++)
    {
        REG_NAND_ADDR = pageaddr & 0xFF;
        pageaddr = pageaddr >> 8;
    }
}

static inline void nand_send_readcacheaddr(unsigned short offset)
{
    REG_NAND_CMD = 0x05;
    REG_NAND_ADDR = (unsigned char)((offset & 0x000000FF) >> 0);
    REG_NAND_ADDR = (unsigned char)((offset & 0x0000FF00) >> 8);
    REG_NAND_CMD = 0xe0;
}

static inline int nand_read_oob(int page, unsigned char *data)
{
    unsigned short i;
    
    nand_send_readaddr(page, chip_info->page_size);
        
    REG_NAND_CMD = 0x30;
    
    nand_wait_ready();
       
    for ( i = 0; i < NAND_OOB_SIZE; i++) 
        *data++ = REG_NAND_ADDR;
    
    return 0;
}

static int nand_read_page_info(int page, struct nand_page_info_t *info)
{
    int ret;
    
    JZ_NAND_SELECT;
    ret = nand_read_oob(page, (unsigned char*)info);
    JZ_NAND_DESELECT;
    
    return ret;
}

static struct nand_page_info_t page_info;

int jz_nand_read_page (int page, unsigned char *data)
{
    int ret, i;
    
    JZ_NAND_SELECT;
    ret = nand_read_oob(page, &page_info);
    
    nand_send_readcacheaddr(0);
    
    /* TODO: use information from page_info */
    
    for ( i = 0; i < chip_info->page_size; i++) 
        *data++ = REG_NAND_ADDR;
    
    JZ_NAND_DESELECT;
    
    return ret;
}

int ata_read_sectors(IF_MV2(int drive,) unsigned long start, int count, void* buf)
{
    (void)start;
    (void)count;
    (void)buf;
    return 0;
}

int ata_write_sectors(IF_MV2(int drive,) unsigned long start, int count, const void* buf)
{
    (void)start;
    (void)count;
    (void)buf;
    return 0;
}

int ata_init(void)
{   
    /* Read/Write timings */
#define SET_STANDARD_TIMING(x) x = (((x) & ~0xFF) | 0x4621200)
    SET_STANDARD_TIMING(REG_EMC_SMCR1);
    SET_STANDARD_TIMING(REG_EMC_SMCR2);
    SET_STANDARD_TIMING(REG_EMC_SMCR3);
    SET_STANDARD_TIMING(REG_EMC_SMCR4);
    
    /* Set NFE bit */
    REG_EMC_NFCSR = EMC_NFCSR_NFE1;
    
    __nand_ecc_disable();
    
    unsigned char cData[5];
    JZ_NAND_SELECT;
    REG_NAND_CMD = NAND_CMD_READ_ID1;
    REG_NAND_ADDR = NAND_CMD_READ1_00;
    cData[0] = REG_NAND_DATA;
    cData[1] = REG_NAND_DATA;
    cData[2] = REG_NAND_DATA;
    cData[3] = REG_NAND_DATA;
    cData[4] = REG_NAND_DATA;
    JZ_NAND_DESELECT;
    
    chip_info = nand_identify(cData);
    if(chip_info == NULL)
    {
        panicf("Unknown NAND flash chip: 0x%x 0x%x 0x%x 0x%x 0x%x", cData[0],
                                      cData[1], cData[2], cData[3], cData[4]);
        return -1;
    }
    
    /* Set timings */
    
    return 0;
}
