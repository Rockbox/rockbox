/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Bertrik Sikken
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
 #include "nand-target.h"

#if 0
/* This is for documentation purpose as FTL has not been reverse engineered yet
 * Raw nand handling functions based on OF disassembly and partially inspired
 * by Rockchip patent
 */

#define MAX_FLASH_NUM 4
#define CMD_READ_STATUS 0x70
#define CMD_RESET       0xFF
#define CMD_READ_ID     0x90
#define READ_PAGE_CMD   0x30

/* this is the struct OF uses */
struct flashspec_t
{
    uint8_t  cache_prog;
    uint8_t  mul_plane;
    uint8_t  interleave;
    uint8_t  large;
    uint8_t  five;
    uint8_t  mlc;
    uint8_t  vendor;
    uint8_t  access_time;
    uint8_t  sec_per_page;
    uint8_t  sec_per_page_raw;
    uint16_t sec_per_block;
    uint16_t sec_per_block_raw;
    uint16_t page_per_block;
    uint16_t page_per_block_raw;

    uint32_t tot_logic_sec;
    uint32_t total_phy_sec;
    uint32_t total_bloks;

    uint32_t cmd;
    uint32_t addr;
    uint32_t data;
};

/* holds nand chips characteristics */
struct flashspec_t flash_spec[MAX_FLASH_NUM];

/* sum of all phy sectors in all chips */
uint32_t  total_phy_sec;

enum vendor_t {
    SAMSUNG,
    TOSHIBA,
    HYNIX,
    INFINEON,
    MICRON,
    RENESAS,
    ST
};

/* taken from OF */
const uint8_t device_code[] = {
    0x76,
    0x79,
    0xf1,
    0xda,
    0xdc,
    0xd3,
    0xd7
};

const uint8_t manufacture_id_tbl[] =
{
    0xec,     /* SAMSUNG  */
    0x98,     /* TOSHIBA  */
    0xad,     /* HYNIX    */
    0xc1,     /* INFINEON */
    0x2c,     /* MICRON   */
    0x07,     /* RENESAS  */
    0x20      /* ST       */
};

/* phisical sectors */
const uint32_t device_info[] =
{
    0x20000,      /*  64M, small page */
    0x40000,      /* 128M, small page */
    0x40000,      /* 128M, large page */
    0x80000,      /* 256M, large page */
    0x100000,     /* 512M, large page */
    0x200000,     /*   1G, large page */
    0x400000,     /*   2G, large page */
    0x800000      /*   4G, large page */
};

static int flash_delay(int n)
{
    volatile int cnt, i, j;

    for (j=0; j<n; j++)
        for (i=0; i<10000; i++)
            cnt++;

    return cnt;
}


static void flash_wait_busy(void)
{
    unsigned int i;

    for (i=0; i<0x2000; i++)
    {
        if (FMCTL & FM_RDY)
             break;

        flash_delay(1);
    }
}

void flash_chip_deselect(void)
{
    uint32_t tmp;
    tmp = FMCTL;
    tmp &= 0xf0;
    FMCTL = tmp;
}

void flash_chip_select(uint8_t chip)
{
    uint32_t tmp;

    /* Maybe we should handle IOMUX here as well?
     * for chip == 0,1 it is not needed
     */
    tmp = FMCTL;
    tmp &= 0xf0;
    tmp |= 1<<chip;
    FMCTL = tmp;
}

void flash_init(void)
{
    uint8_t buff[5]; /* buff for CMD_READ_ID response */
    uint32_t i,j;

    mlc_refresh_row = 0xffffffff;
    total_phy_sec = 0;
    flash_pend_cmd.valid = 0;
    flash_read_status_cmd = CMD_READ_STATUS;

    FMWAIT = 0x1081;
    FLCTL = FL_RST;

    for (i=0; i< MAX_FLASH_NUM; i++)
    {
        /* Redundat - we will use special macros
         * just for reference what OF does
         */
        flash_spec[i].cmd = 0x180E8200 + (i<<9);
        flash_spec[i].addr = 0x180E204 + (i<<9);
        flash_spec[i].data = 0x180E208 + (i<<9);

        flash_chip_select(i);
        FLASH_CMD(i) = CMD_RESET; /* write cmd to flash chip */
        flash_delay(2);
        flash_wait_busy();
        FLASH_CMD(i) = CMD_READ_ID; /* write cmd to flash chip */
        FLASH_ADDR(i) = 0x00;

        /* read 5 bytes of CMD_READ_ID response */
        for (j=0; j<5; j++)
             buff[j] = FLASH_DATA(i);

        flash_chip_deselect();

        /* Get the vendor of the chip */
        for (j=0; j<sizeof(manufacture_id_tbl); j++)
        {
            /* store Manufacturer index */
            if (ManufactureIDTbl[j] == buff[0])
            {
                flash_spec[i].vendor = j;
            }
        }

        for (j=0; j<sizeof(device_code); j++)
        {
            /* look for matching device code
             * and store total phys sectors
             */
            if (DeviceCode[j] == buff[1])
            {
                flash_spec[i].total_phy_sec = device_info[j];
                break;
             }
        }

        /* div zero is fatal for us (not for OF :P) */
        if (flash_spec[i].total_phy_sec == 0)
            continue;

        /* loc_7e8 */
        flash_spec[i].mlc = 0;
        flash_spec[i].large = 0;
        flash_spec[i].five = 1;
        flash_spec[i].mul_plane = 1;
        flash_spec[i].interleave = 0;

        flash_spec[i].cache_prog = buff[2] & 0x80;

        /* flash access time (ns) */
        switch (buff[3] & 0x88)
        {
            case 0:
                flash_spec[i].access_time = 50;
                break;
            case 0x80:
                flash_spec[i].access_time = 25;
                break;
            case 0x08:
                flash_spec[i].access_time = 20;
                break;
            default:
                flash_spec[i].access_time = 60;
        }

        /* set_large
         * j is index in device_code table
         */
        if (j < 2)
        {
            /* small block */
            flash_spec[i].large = 0;
            flash_spec[i].sec_per_page_raw = 1;
            flash_spec[i].sec_per_block_raw = 32;
        }
        else
        {
            flash_spec[i].large = 1;
            if (j == 2)
                flash_spec[i].five = 0;


            /* cell type */
            flash_spec[i].mlc = (buff[2] >> 2) & 0x03;

            flash_spec[i].sec_per_page_raw = 2;   /* 1KB~8KB */

            /* set_sec_per_page_raw */
            flash_spec[i].sec_per_page_raw <<= (buff[3] & 3);

            flash_spec[i].sec_per_block_raw = 128;      /* 64KB~512KB */

            /* set_sec_per_block_raw */
            flash_spec[i].sec_per_block_raw <<= ((buff[3]>>4) & 3);

            /* simult_prog */
            if (buff[2] & 0x30)
            {
                /* buff4_mulplane */
                flash_spec[i].mul_plane <<= ((buff[4]>>2) & 3);
            }

            /* set_interleave */
            if (flash_spec[i].vendor == TOSHIBA)
            {
                flash_spec[i].mul_plane = 2;
                if (buff[2] & 3)
                    flash_spec[i].interleave = 1;
            }

        } /* large block */

        if (flash_spec[i].mul_plane > 2)
        {
            flash_spec[i].mul_plane = 2;
            flash_spec[i].interleave = 1;
        }

        flash_spec[i].page_per_block_raw = flash_spec[i].sec_per_block_raw/flash_spec[i].sec_per_page_raw;
        flash_spec[i].page_per_block = flash_spec[i].page_per_block_raw * flash_spec[i].mul_plane;
        flash_spec[i].sec_per_block = flash_spec[i].sec_per_block_raw * flash_spec[i].mul_plane;
        flash_spec[i].sec_per_page = flash_spec[i].sec_per_page_raw * flash_spec[i].mul_plane;
        flash_spec[i].total_bloks = flash_spec[i].total_phy_sec / flash_spec[i].sec_per_block;

        total_phy_sec += flash_spec[i].total_phy_sec;
    }

    /* read ID block and propagate SysDiskCapacity and SysResBlocks */
}

/* read single page in unbuffered mode */
void flash_read_page(int page, unsigned char *pgbuff)
{
    unsigned int i;

    flash_chip_select(0);
    flash_delay(2);
    flash_wait_busy();

    /* setup transfer */
    FLASH_CMD(0)  = 0x00;
    FLASH_ADDR(0) = 0x00;                /* column */
    FLASH_ADDR(0) = 0x00;                /* column */
    FLASH_ADDR(0) = page & 0xff;         /* row */
    FLASH_ADDR(0) = (page >> 8) & 0xff;  /* row */
    FLASH_ADDR(0) = (page >> 16) & 0xff; /* row */
    FLASH_CMD(0)  = READ_PAGE_CMD;

    /* wait for operation complete */
    flash_wait_busy();

    /* copy data from page register
     * WARNING flash page size can be different
     * for different chips. This value should be set
     * based on initialization.
     */
    for (i=0; i<(4096+218); i++)
        pgbuff[i] = FLASH_DATA(0);

    flash_chip_deselect();
}

void flash_read_sector(int page, unsigned char *secbuf, int nsec)
{
    int i = 0;
    int j = 0;

    /* WARNING this code assumes only one nand chip
     * it does not handle data split across different nand chips
     */
    flash_chip_select(0);
    flash_delay(2);
    flash_wait_busy();

    FLASH_CMD(0)  = 0x00;
    FLASH_ADDR(0) = 0x00;
    FLASH_ADDR(0) = 0x00;
    FLASH_ADDR(0) = page & 0xff;
    FLASH_ADDR(0) = (page >> 8) & 0xff;
    FLASH_ADDR(0) = (page >> 16) & 0xff;
    FLASH_CMD(0)  = READ_PAGE_CMD;

    flash_delay(1);

    /* wait for operation to complete */
    flash_wait_busy();

    /* enables hw checksum control most probably */
    BCHCTL = 1;

    /* This initializes the transfer from the nand to the buffer
     * There are 4 consecutive hw buffers 512 bytes long for data (PAGE_BUF)
     * and 4 16 bytes long for metadata (BCH code checksum) (SPARE_BUF)
     */
    FLCTL = 0xA24;

    /* This scheme utilizes some overlap in data transfers -
     * data are copied from buffer to the mem and from nand to the buf
     * at the same time.
     */
    while (++j < nsec)
    {
        /* wait for transfer to complete */
        while(! (FLCTL & FL_RDY));

        /* initialize next transfer to the next buffer */
        FLCTL = 0xA24 | (j&3)<<3;

        /* copy data chunk */
        memcpy(secbuf, (((unsigned char *)&PAGE_BUF)+((i&3)<<9)), 0x200);
        secbuf += 0x200;

        /* copy metadata chunk (BCH)
         * in real application this can be discarded
         */
        memcpy(secbuf, (((unsigned char *)&SPARE_BUF)+((i&3)<<4)), 0x10);
        secbuf += 0x10;
        i++;
    }

    /* wait for transfer to complete */
    while(! (FLCTL & FL_RDY));

    /* copy data chunk */
    memcpy(secbuf, (((unsigned char *)&PAGE_BUF)+((i&3)<<9)), 0x200);
    secbuf += 0x200;

    /* copy metadata chunk (BCH)
     * in real application this can be discarded
     */
    memcpy(secbuf, (((unsigned char *)&SPARE_BUF)+((i&3)<<4)), 0x10);
    secbuf += 0x10;

    flash_chip_deselect();
}

#endif
const struct nand_device_info_type* nand_get_device_type(uint32_t bank);


uint32_t nand_read_page(uint32_t bank, uint32_t page, void* databuffer,
                        void* sparebuffer, uint32_t doecc,
                        uint32_t checkempty)
{
    /* TODO implement */
    (void)bank;
    (void)page;
    (void)databuffer;
    (void)sparebuffer;
    (void)doecc;
    (void)checkempty;
    return 0;
}

uint32_t nand_write_page(uint32_t bank, uint32_t page, void* databuffer,
                         void* sparebuffer, uint32_t doecc)
{
    /* TODO implement */
    (void)bank;
    (void)page;
    (void)databuffer;
    (void)sparebuffer;
    (void)doecc;
    return 0;
}

uint32_t nand_block_erase(uint32_t bank, uint32_t page)
{
    /* TODO implement */
    (void)bank;
    (void)page;
    return 0;
}

uint32_t nand_reset(uint32_t bank)
{
    /* TODO implement */
    (void)bank;
    return 0;
}

uint32_t nand_device_init(void)
{
    /* TODO implement */
    return 0;
}

void nand_power_up(void)
{
    /* TODO implement */
}

void nand_power_down(void)
{
    /* TODO implement */
}

void nand_set_active(void)
{
    /* TODO implement */
}

long nand_last_activity(void)
{
    /* TODO implement */
    return 0;
}

int nand_spinup_time(void)
{
    return 0;
}
