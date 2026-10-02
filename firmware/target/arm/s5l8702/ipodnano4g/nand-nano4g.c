/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2009 by Michael Sparmann
 * Copyright (C) 2026 by Andrew Rice
 * Copyright (C) 2026 by Robin Tschirschnitz
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
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "mv.h"
#include "storage.h"
#include "clocking-s5l8702.h"
#include "nand-target.h"
#include <cpucache-arm.h>
#include <string.h>

#define NAND_CMD_READ           0x00
#define NAND_CMD_READ2          0x30
#define NAND_CMD_READSTATUS     0x70
#define NAND_CMD_READID         0x90
#define NAND_CMD_RESET          0xff

#define NAND_STATUS_READY       (1 << 6)

#define NAND_CHUNK_SIZE         512
#define NAND_CHUNKS_PER_PAGE    (NAND_PAGE_SIZE / NAND_CHUNK_SIZE)
#define NAND_MAX_BANKS          4

/* Same timings the Nano 3G BootROM writes into FMCTRL0. */
#define NAND_TUNK1              4
#define NAND_TWP                3
#define NAND_POLL_SPINS         1000000

struct nand_chip_info
{
    uint32_t id;
    unsigned int banks;
    unsigned int blocks;
    unsigned int pagesperblock;
};

/*
 * A NAND ID alone does not identify the geometry used by Apple: the same
 * part may be wired with a different number of chip enables.  Only add
 * combinations which have been observed on hardware and whose page format
 * has been verified.
 */
static const struct nand_chip_info nand_chips[] =
{
    /* Samsung NAND observed on an 8 GB iPod nano 4G */
    { 0xb614d5ec, 4, 4096, 128 },
};

static const struct nand_chip_info *nand_chip;
static unsigned int nand_banks;
static uint32_t nand_id;
static bool nand_ready;
static long nand_last_activity_value = -1;
static struct nand_geometry nand_geo;

static void nand_touch(void)
{
    nand_last_activity_value = current_tick;
}

static void nand_set_fmctrl0(uint32_t bank)
{
    FMCTRL0 = (NAND_TUNK1 << 16) | (NAND_TWP << 12) | FMCTRL0_UNK1
            | 1 | (1 << (bank + 1));
}

static int nand_wait_reg(volatile uint32_t *reg, uint32_t bit)
{
    unsigned long spins = NAND_POLL_SPINS;

    while (spins--)
    {
        if (*reg & bit)
        {
            *reg = bit;
            return 0;
        }
    }
    return -1;
}

static int nand_wait_stat(uint32_t bit)
{
    return nand_wait_reg(&FMCSTAT, bit);
}

static int nand_wait_stat_noclear(uint32_t bit)
{
    unsigned long spins = NAND_POLL_SPINS;

    while (spins--)
        if (FMCSTAT & bit)
            return 0;
    return -1;
}

static void nand_clear_status(void)
{
    FMCSTAT = 0xffffffff;
}

static int nand_send_cmd(uint32_t cmd)
{
    FMCMD = cmd;
    return nand_wait_stat(FMCSTAT_CMDDONE);
}

static int nand_send_addr_byte(uint32_t addr)
{
    FMANUM = 0;
    FMADDR0 = addr;
    FMCTRL1 = FMCTRL1_DOTRANSADDR;
    return nand_wait_stat(FMCSTAT_ADDRDONE);
}

static int nand_read_status(void)
{
    unsigned long timeout = USEC_TIMER + 200000;
    unsigned long spins;
    uint32_t status = 0;

    FMADDR2 = 1;
    if (nand_send_cmd(NAND_CMD_READSTATUS))
        return -1;
    do
    {
        FMCTRL1 = 0xca;
        for (spins = 10000; spins; spins--)
        {
            status = FMSYND0;
            if (status & NAND_STATUS_READY)
                break;
        }
    } while (!(status & NAND_STATUS_READY)
             && TIME_BEFORE(USEC_TIMER, timeout));
    if (!(status & NAND_STATUS_READY))
        return -1;
    FMCSTAT = FMCSTAT_STATUSREADY;
    FMCTRL1 = 0x20;
    return status & 0xff;
}

static int nand_wait_ready(void)
{
    int status = nand_read_status();

    FMADDR2 = 0;
    FMCTRL1 = 0xe0;
    return status < 0 ? NAND_ERR_READY_CMD : 0;
}

static int nand_reset(uint32_t bank)
{
    nand_set_fmctrl0(bank);
    if (nand_send_cmd(NAND_CMD_RESET))
        return NAND_ERR_RESET_CMD;

    udelay(5000);
    return nand_wait_ready();
}

static int nand_identify(uint32_t *id)
{
    if (nand_send_cmd(NAND_CMD_READID))
        return NAND_ERR_IDENTIFY;
    if (nand_send_addr_byte(0))
        return NAND_ERR_IDENTIFY;

    FMDNUM = 7;
    FMADDR2 = 1;
    FMCSTAT = FMCSTAT_TRANSDONE;
    FMCTRL1 = FMCTRL1_DOREADDATA | FMCTRL1_CLEARWFIFO | FMCTRL1_CLEARRFIFO
            | (1 << 8);
    if (nand_wait_stat_noclear(FMCSTAT_TRANSDONE))
        return NAND_ERR_IDENTIFY;
    FMADDR2 = 0x100;
    FMCTRL1 = FMCTRL1_CLEARWFIFO | (1 << 8) | (1 << 9);

    *id = FMFIFO;
    nand_clear_status();
    return 0;
}

static int nand_probe(void)
{
    bool gap = false;
    uint32_t id;
    int rc;

    rc = nand_reset(0);
    if (rc)
        return rc;
    rc = nand_identify(&nand_id);
    if (rc)
        return rc;

    nand_banks = 1;
    for (unsigned int bank = 1; bank < NAND_MAX_BANKS; bank++)
    {
        rc = nand_reset(bank);
        if (rc || nand_identify(&id))
        {
            gap = true;
            continue;
        }

        /* A later chip after a missing CE, or unlike chips, is unsafe. */
        if (gap || id != nand_id)
            return NAND_ERR_MIXED;
        nand_banks++;
    }

    for (unsigned int i = 0; i < ARRAYLEN(nand_chips); i++)
    {
        if (nand_chips[i].id == nand_id
                && nand_chips[i].banks == nand_banks)
        {
            nand_chip = &nand_chips[i];
            return 0;
        }
    }
    return NAND_ERR_UNSUPPORTED;
}

/* S5L8720 ECC engine, from the BootROM page read at 0x200092b0. */
#define FMECC_KICK      (*(REG32_PTR_T)(FMC_BASE + 0x0804))
#define FMECC_RESULT    (*(REG32_PTR_T)(FMC_BASE + 0x080c))
#define FMECC_STATUS    (*(REG32_PTR_T)(FMC_BASE + 0x0810))

static int nand_wait_ready_rom(void)
{
    int rc = 0;

    FMCTRL1 = 0x1000e0;
    FMCMD = NAND_CMD_READSTATUS;
    if (nand_wait_stat(FMCSTAT_CMDDONE))
        rc = -1;
    else
    {
        FMCTRL1 = 0x2a;
        if (nand_wait_stat(FMCSTAT_STATUSREADY))
            rc = -1;
    }
    FMADDR2 = 0;
    FMCTRL1 = 0xe0;
    return rc;
}

/* READ, row address, READ2, wait, READ: data then streams from column 0. */
static int nand_start_read(uint32_t bank, uint32_t page)
{
    FMCTRL1 = 0x1f0e0;
    nand_set_fmctrl0(bank);
    FMCMD = NAND_CMD_READ;
    if (nand_wait_stat(FMCSTAT_CMDDONE))
        return -1;
    FMANUM = 4;
    FMADDR0 = page << 16;
    FMADDR1 = (page >> 16) & 0xff;
    FMCTRL1 = 0xe1;
    if (nand_wait_stat(FMCSTAT_ADDRDONE))
        return -2;
    FMCMD = NAND_CMD_READ2;
    if (nand_wait_stat(FMCSTAT_CMDDONE))
        return -3;
    if (nand_wait_ready_rom())
        return -4;
    FMCMD = NAND_CMD_READ;
    if (nand_wait_stat(FMCSTAT_CMDDONE))
        return -5;
    return 0;
}

static void nand_end_read(void)
{
    FMCTRL0 = 0x43802;
    FMCTRL0 &= ~0x1ff;
    FMCSTAT = 0xffffffff;
    FMECC_STATUS = 0x1ff;
}

static int nand_fifo_read(void *buf, unsigned int len)
{
    uint32_t *w = buf;
    unsigned int done;
    unsigned int n;

    for (done = 0; done < len; done += n)
    {
        n = MIN(len - done, 32u);
        FMDNUM = n - 1;
        FMADDR2 = 1;
        FMCSTAT = FMCSTAT_TRANSDONE;
        FMCTRL1 = FMCTRL1_DOREADDATA | FMCTRL1_CLEARWFIFO
                | FMCTRL1_CLEARRFIFO | (1 << 8);
        if (nand_wait_stat_noclear(FMCSTAT_TRANSDONE))
            return -6;
        FMADDR2 = 0x100;
        FMCTRL1 = FMCTRL1_CLEARWFIFO | (1 << 8) | (1 << 9);
        for (unsigned int i = 0; i < (n + 3) / 4; i++)
            *w++ = FMFIFO;
    }
    return 0;
}

static bool nand_page_valid(uint32_t bank, uint32_t page)
{
    return nand_ready && bank < nand_chip->banks
            && page < nand_chip->blocks * nand_chip->pagesperblock;
}

/* Boot-page reads are supplied by the later FMSS boot-page change. */
int nand_read_boot_page(uint32_t bank, uint32_t page, void *databuf)
{
    (void)bank;
    (void)page;
    (void)databuf;
    return -1;
}

/* Page reads are supplied by the later FMSS page-read change. */
int nand_read_page(uint32_t bank, uint32_t page, void *databuf,
                   uint32_t *meta)
{
    (void)bank;
    (void)page;
    (void)databuf;
    (void)meta;
    return -1;
}

/* Read the first len bytes without ECC.  Intended for diagnostics. */
int nand_read_raw(uint32_t bank, uint32_t page, void *buf, unsigned int len)
{
    int rc;

    if (!buf || ((uintptr_t)buf & 3) || (len & 3) || len > NAND_PAGE_SIZE
            || !nand_page_valid(bank, page))
        return -1;

    rc = nand_start_read(bank, page);
    if (!rc)
        rc = nand_fifo_read(buf, len);
    nand_end_read();
    nand_touch();
    return rc;
}

const struct nand_geometry *nand_get_geometry(void)
{
    return nand_ready ? &nand_geo : NULL;
}

unsigned int nand_get_bank_count(void)
{
    return nand_ready ? nand_banks : 0;
}

uint32_t nand_chip_id(void)
{
    return nand_ready ? nand_id : 0;
}

int nand_init(void)
{
    int rc;

    nand_ready = false;
    nand_chip = NULL;
    clockgate_enable(CLOCKGATE_NAND, true);
    clockgate_enable(CLOCKGATE_NANDECC, true);

    rc = nand_probe();
    if (rc)
        return rc;

    nand_geo.banks = nand_chip->banks;
    nand_geo.blocks = nand_chip->blocks;
    nand_geo.pagesperblock = nand_chip->pagesperblock;
    nand_geo.pagesize = NAND_PAGE_SIZE;
    nand_ready = true;
    nand_touch();
    return 0;
}

void nand_spindown(int seconds)
{
    (void)seconds;
}

void nand_spin(void)
{
    nand_touch();
}

#ifdef HAVE_STORAGE_FLUSH
int nand_flush(void)
{
    return 0;
}
#endif

/*
 * Logical-sector access belongs to the FTL driver.  Do not expose physical
 * NAND pages as a disk, and never report success for an unimplemented write.
 */
int nand_read_sectors(IF_MD(int drive,) sector_t start, int count, void *buf)
{
    IF_MD((void)drive);
    (void)start;
    (void)count;
    (void)buf;
    return -1;
}

int nand_write_sectors(IF_MD(int drive,) sector_t start, int count,
                       const void *buf)
{
    IF_MD((void)drive);
    (void)start;
    (void)count;
    (void)buf;
    return -1;
}

int nand_event(long id, intptr_t data)
{
    (void)id;
    (void)data;
    return 0;
}

#ifdef STORAGE_GET_INFO
void nand_get_info(IF_MD(int drive,) struct storage_info *info)
{
    IF_MD((void)drive);
    info->sector_size = SECTOR_SIZE;
    info->num_sectors = 0;
    info->vendor = "Samsung";
    info->product = "NAND";
    info->revision = "";
}
#endif

long nand_last_disk_activity(void)
{
    return nand_last_activity_value;
}
