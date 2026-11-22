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

/*
 * NAND driver for the iPod Nano 4G (S5L8720 FMI).
 *
 * The register block is the same FMI aperture as the Nano 3G (0x38a00000).
 * Identify and the raw read follow that driver's NANDReadPage
 * sequence: command, five address cycles, then 512-byte chunks. A 4 KiB
 * page is eight chunks. The pair index that the 2 KiB driver stored in one
 * bit (FMADDR6 0x00/0x10, ECC kick bit 16) is the chunk number shifted
 * right by one, so chunks 0..3 issue the same writes as on the Nano 3G and
 * chunks 4..7 continue the same step (0x20/0x30, bits 17..18).
 *
 * wInd3x's Nano 4G NAND hooks are unimplemented, so this is that sequence
 * on the S5L8720 clock gates (NAND = 5, NANDECC = 9) rather than a second
 * disassembly. Pinmux is left to gpio_preinit(); the Nano 3G PCON values
 * are a different board.
 *
 * Page and boot-page reads, programs and erases run the sequencer
 * programs in fmss/. nand_write_pages() and nand_erase_block() refuse
 * everything until nand_set_writable(true), and block 0 of every bank
 * always: it holds the BootROM image and the Apple backup, which only the
 * bootloader's armed boot path writes.
 */

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "mv.h"
#include "storage.h"
#include "clocking-s5l8702.h"
#include "logf.h"
#include "nand-target.h"
#include "ftl-target.h"
#include <cpucache-arm.h>
#include <string.h>

#define NAND_CMD_READ           0x00
#define NAND_CMD_READ2          0x30
#define NAND_CMD_READSTATUS     0x70
#define NAND_CMD_READID         0x90
#define NAND_CMD_RESET          0xFF

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
    unsigned int blocks;
    unsigned int pagesperblock;
    unsigned int planes;
    unsigned int userblocks;
    unsigned int vflspares;
    bool twoplane;
};

static struct nand_chip_info nand_chip_decoded;
static const struct nand_chip_info *nand_chip;
static unsigned int nand_banks;
static uint32_t nand_id;
static bool nand_ready = false;
static long nand_last_activity_value = -1;
static struct nand_geometry nand_geo;

const struct nand_geometry *nand_get_geometry(void)
{
    return nand_ready ? &nand_geo : NULL;
}

static void nand_fill_geo(void)
{
    nand_geo.banks = nand_banks;
    nand_geo.blocks = nand_chip->blocks;
    nand_geo.pagesperblock = nand_chip->pagesperblock;
    nand_geo.planes = nand_chip->planes;
    nand_geo.userblocks = nand_chip->userblocks;
    nand_geo.vflspares = nand_chip->vflspares;
    nand_geo.twoplane = nand_chip->twoplane;
}

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
    clockgate_enable(CLOCKGATE_NAND, true);
    clockgate_enable(CLOCKGATE_NANDECC, true);

    nand_set_fmctrl0(bank);
    if (nand_send_cmd(NAND_CMD_RESET))
        return NAND_ERR_RESET_CMD;

    udelay(5000);
    return nand_wait_ready();
}

static int nand_identify(uint32_t *id)
{
    if (nand_send_cmd(NAND_CMD_READID))
        return -1;
    if (nand_send_addr_byte(0))
        return -1;

    FMDNUM = 7;
    FMADDR2 = 1;
    FMCSTAT = FMCSTAT_TRANSDONE;
    FMCTRL1 = FMCTRL1_DOREADDATA | FMCTRL1_CLEARWFIFO | FMCTRL1_CLEARRFIFO
            | (1 << 8);
    if (nand_wait_stat_noclear(FMCSTAT_TRANSDONE))
        return -1;
    FMADDR2 = 0x100;
    FMCTRL1 = FMCTRL1_CLEARWFIFO | (1 << 8) | (1 << 9);

    *id = FMFIFO;
    nand_clear_status();
    return 0;
}

/* JEDEC extended ID: page = 1024 << (id[3] & 3), erase = 64 KiB << ((id[3] >> 4) & 3).
 * id is the first four ID bytes, little endian, as READ ID returns them. */
static int nand_decode(uint32_t id, struct nand_chip_info *out)
{
    uint8_t dev = (id >> 8) & 0xff;
    uint8_t ext = (id >> 24) & 0xff;
    unsigned int pagesize = 1024u << (ext & 3);
    unsigned int erasesize = 65536u << ((ext >> 4) & 3);
    unsigned int mbytes;
    unsigned int blocks;
    unsigned int ppb;

    if (pagesize != NAND_PAGE_SIZE || erasesize < pagesize)
        return -1;
    switch (dev)
    {
        case 0xD3: mbytes = 1024; break;    /* 8 Gbit */
        case 0xD5: mbytes = 2048; break;    /* 16 Gbit */
        case 0xD7: mbytes = 4096; break;    /* 32 Gbit */
        case 0xDE: mbytes = 8192; break;    /* 64 Gbit, TH58NVG6 class */
        case 0x3A: mbytes = 16384; break;   /* 128 Gbit */
        case 0xDC: mbytes = 512; break;     /* 4 Gbit */
        default: return -1;
    }
    ppb = erasesize / pagesize;
    blocks = (mbytes * (1024u * 1024u / pagesize)) / ppb;
    if (!blocks || !ppb)
        return -1;

    out->id = id;
    out->blocks = blocks;
    out->pagesperblock = ppb;
    out->planes = 2;
    /* 23 spare superblocks, same allowance the Nano 3G VFL uses, plus a
     * small remap pool the context can replace once it is loaded. */
    out->vflspares = 16;
    if (blocks <= out->planes * (out->vflspares + 23))
        return -1;
    out->userblocks = blocks - out->planes * (out->vflspares + 23);
    out->twoplane = true;
    return 0;
}

/* S5L8720 ECC engine, from the BootROM page read at 0x200092b0. The
 * Nano 3G offsets (0x80c kick, 0x840 status) do not apply here. */
#define FMECC_KICK      (*(REG32_PTR_T)(FMC_BASE + 0x0804))
#define FMECC_RESULT    (*(REG32_PTR_T)(FMC_BASE + 0x080C))
#define FMECC_STATUS    (*(REG32_PTR_T)(FMC_BASE + 0x0810))

/* The BootROM's own ready wait: READ STATUS, then FMCTRL1 0x2a has the
 * controller poll until the part is ready and set FMCSTAT bit 23. */
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

/* READ, row address, READ2, wait, READ: the page is then streamed out
 * from column 0 */
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

/* No ECC: the next len bytes of the page stream through the FIFO, 32 at a
 * time, the way READ ID is read. The last word may be partly filled. */
static int nand_fifo_read(void *buf, unsigned int len)
{
    uint32_t *w = buf;
    unsigned int done, n, i;

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
        for (i = 0; i < (n + 3) / 4; i++)
            *w++ = FMFIFO;
    }
    return 0;
}

/* The Whimory page format, as Apple's EFI NAND driver reads it: chunk 0 is
 * 10 bytes of spare metadata, the parity and 512 data bytes, with one BCH
 * codeword over metadata and data; chunks 1 to 7 are parity + 512 data
 * bytes. meta[2] reads 0xffff above the byte-8 flag and the spare type.
 *
 * Most parts take 8-bit BCH, 13 parity bytes (decode modes 0xa01 and 1).
 * The Toshiba 98 D5 94 BA in an 8 GB player takes 16-bit, 26 parity bytes
 * (0xa05 and 5), the strength of the boot block: the 8-bit decode fails
 * every programmed page there and passes erased ones (measured). */
#define NAND_PAGE_ECCLEN    13
#define NAND_KICK_META      0xa01
#define NAND_KICK_DATA      0x001
#define NAND_PAGE_ECCLEN16  26
#define NAND_KICK_META16    0xa05
#define NAND_KICK_DATA16    0x005

static unsigned int nand_ecclen = 13;
static bool nand_ecclen_fixed;
#define NAND_KICK_BCH16 (nand_ecclen == 26 ? 4 : 0)

/* Apple's chip table for this model gives the spare size of each part,
 * which the ID does not encode: 218 bytes (10 metadata + 8 x 26 parity)
 * on the Toshiba, Intel and Micron parts, which Apple then reads with
 * 16-bit BCH, and 128 bytes on the Samsung and Hynix parts (8-bit BCH).
 * The ID is the first four ID bytes, little endian. */
static const uint32_t nand_bch16_ids[] =
{
    0xba94d598, 0xba95d798,                 /* Toshiba */
    0x3e94d589, 0x3ed5d789,                 /* Intel */
    0x3e94d52c, 0x3ed5d72c, 0x3e94d72c,     /* Micron */
};
static const uint32_t nand_bch8_ids[] =
{
    0xb614d5ec, 0xb655d7ec,                 /* Samsung */
    0xb614d5ad, 0xb655d7ad,                 /* Hynix */
};

static bool nand_id_listed(uint32_t id, const uint32_t *ids, unsigned int n)
{
    unsigned int i;

    for (i = 0; i < n; i++)
        if (ids[i] == id)
            return true;
    return false;
}

static void nand_select_ecc(uint32_t id)
{
    nand_ecclen = 13;
    nand_ecclen_fixed = true;
    if (nand_id_listed(id, nand_bch16_ids, ARRAYLEN(nand_bch16_ids)))
        nand_ecclen = 26;
    else if (!nand_id_listed(id, nand_bch8_ids, ARRAYLEN(nand_bch8_ids)))
        nand_ecclen_fixed = false;
}

/* The first len bytes (a multiple of 4) of a page, no ECC. Diagnostic. */
int nand_read_raw(uint32_t bank, uint32_t page, void *buf, unsigned int len)
{
    int rc;

    rc = nand_start_read(bank, page);

    if (!rc)
        rc = nand_fifo_read(buf, len);
    nand_end_read();
    return rc;
}

#include "fmss-nano4g.h"

#define FMISS_REG(off) (*(volatile uint32_t *)(FMC_BASE + (off)))
#define FMISS_RAM_ALIAS(p) ((uint32_t)(intptr_t)(p) | 0x80000000)

#define FMISS_POLL_SPINS    100000000
#define FMISS_WRITE_BYTES   0x600

/* FMISS register r6 after erase.s / program.s: bit 0 the chip reported a
 * failed operation, bit 27 a sequencer fault (aborted) */
#define FMISS_R6_FAILED     0x00000001
#define FMISS_R6_ABORTED    0x08000000

/* Parameter words of the fmss/ programs */
#define FMISS_P_ROWS        0x0d0c
#define FMISS_P_CEMASKS     0x0d10
#define FMISS_P_ECCBUF      0x0d14
#define FMISS_P_COUNT       0x0d18
#define FMISS_P_META        0x0d1c
#define FMISS_P_DMALIST     0x0d20
#define FMISS_P_STATS       0x0d24
#define FMISS_P_TIMING      0x0d4c

static uint8_t nand_fmiss_page[NAND_PAGE_SIZE] CACHEALIGN_ATTR;
static bool nand_writable;

static int nand_fmiss_idle(void)
{
    unsigned long spins;

    FMISS_REG(0x0c0c) = 0x0d;
    for (spins = FMISS_POLL_SPINS; FMISS_REG(0x0c0c) & 0x0d; )
        if (!--spins)
            return -1;
    return 0;
}

static int nand_fmiss_run(const void *program, size_t len)
{
    unsigned long spins;

    if (nand_fmiss_idle())
        return -2;
    commit_dcache_range(program, len);
    FMISS_REG(0x0c04) = (uint32_t)(intptr_t)program;
    FMISS_REG(0x0c0c) = 0xff;
    FMISS_REG(0x0c6c) = 0;
    FMISS_REG(0x0c60) = 1;
    for (spins = 100000; FMISS_REG(0x0c64) != 1; )
        if (!--spins)
            return -2;
    FMISS_REG(0x0c60) = 0;
    FMISS_REG(0x0c00) = 0xffb5;
    for (spins = FMISS_POLL_SPINS; !(FMISS_REG(0x0c0c) & 1); )
        if (!--spins)
            return nand_fmiss_idle() ? -3 : -1;
    FMISS_REG(0x0c0c) = 1;
    return 0;
}

static void nand_fmiss_cleanup(void)
{
    FMCTRL1 = 0x20;
    FMCTRL0 = 0;
    FMCSTAT = 0xffffffff;
}

static bool nand_fmiss_target_ok(uint32_t bank, uint32_t block)
{
    return nand_ready && nand_chip && bank < nand_banks
        && block < nand_chip->blocks;
}

/* The FMISS calls below return 0, the nand_fmiss_run() error, or -10 when
 * the program ran and the chip or the result check refused the operation */

#define NAND_WRITE_TIMING   0x00077000

/* fmss/erase.s: P_ROWS the first page of the block, P_CEMASKS 1 << bank */
static int nand_fmiss_erase(uint32_t bank, uint32_t block)
{
    uint32_t r6;
    int rc;

    FMISS_REG(FMISS_P_ROWS) = block * nand_chip->pagesperblock;
    FMISS_REG(FMISS_P_CEMASKS) = 1u << bank;
    FMISS_REG(FMISS_P_TIMING) = NAND_WRITE_TIMING;
    rc = nand_fmiss_run(fmss_prog_erase, sizeof(fmss_prog_erase));
    r6 = FMISS_REG(0x0c30);
    if (!rc && (r6 & FMISS_R6_ABORTED))
        rc = -4;
    else if (!rc && (r6 & FMISS_R6_FAILED))
        rc = -10;
    if (rc >= -1 || rc == -4 || rc == -10)
        nand_fmiss_cleanup();
    return rc;
}

/* fmss/program.s: one 4 KiB page from P_DMALIST with the 10 metadata bytes
 * at P_META, eight codewords. 0x0d2c/0x0d30 are the encode modes of
 * chunks 1..7 and of chunk 0 (with the metadata), 0x0d34/0x0d38 the bytes
 * before chunk 0's and chunks 1..7's data, less one. */
static uint32_t nand_fmiss_meta[4] CACHEALIGN_ATTR;

static int nand_fmiss_program_page(uint32_t bank, uint32_t page, const void *data,
                              const uint32_t *meta)
{
    uint32_t r6;
    int rc;

    memcpy(nand_fmiss_page, data, NAND_PAGE_SIZE);
    if (meta)
        memcpy(nand_fmiss_meta, meta, NAND_META_WORDS * 4);
    else
        memset(nand_fmiss_meta, 0xff, NAND_META_WORDS * 4);
    commit_dcache_range(nand_fmiss_page, sizeof(nand_fmiss_page));
    commit_dcache_range(nand_fmiss_meta, sizeof(nand_fmiss_meta));

    FMISS_REG(FMISS_P_ROWS) = page;
    FMISS_REG(FMISS_P_CEMASKS) = 1u << bank;
    FMISS_REG(FMISS_P_META) = FMISS_RAM_ALIAS(nand_fmiss_meta);
    FMISS_REG(FMISS_P_DMALIST) = FMISS_RAM_ALIAS(nand_fmiss_page);
    FMISS_REG(0x0d2c) = 0x002 | NAND_KICK_BCH16;
    FMISS_REG(0x0d30) = 0xa02 | NAND_KICK_BCH16;
    FMISS_REG(0x0d34) = 10 + nand_ecclen - 1;
    FMISS_REG(0x0d38) = nand_ecclen - 1;
    FMISS_REG(FMISS_P_TIMING) = NAND_WRITE_TIMING;
    rc = nand_fmiss_run(fmss_prog_program, sizeof(fmss_prog_program));
    r6 = FMISS_REG(0x0c30);
    if (!rc && (r6 & FMISS_R6_ABORTED))
        rc = -4;
    else if (!rc && (r6 & FMISS_R6_FAILED))
        rc = -10;
    if (rc >= -1 || rc == -4 || rc == -10)
        nand_fmiss_cleanup();
    return rc;
}

/* Keep the batched write API while the source program handles one page
 * per sequencer run. Report the exact bank of the first failed page. */
#define NAND_FMISS_PROGRAM_BATCH NAND_MAX_BANKS
static int nand_fmiss_program(const struct nand_write *w, unsigned int n,
                              uint32_t *failbank)
{
    for (unsigned int i = 0; i < n; i++)
    {
        int rc = nand_fmiss_program_page(w[i].bank, w[i].page,
                                        w[i].buf, w[i].meta);
        if (rc)
        {
            if (failbank)
                *failbank = w[i].bank;
            return rc;
        }
    }
    return 0;
}

/* fmss/read.s reads pages from the lists at P_ROWS and P_CEMASKS (P_COUNT
 * pages) to the 2 KiB DMA destinations listed at P_DMALIST, 12 metadata
 * bytes per page to P_META and one status byte per chunk to P_ECCBUF:
 * 0xff uncorrectable, 0xfe erased, else the corrected bits. r6 is the
 * summary: bit 30 a chunk was uncorrectable, bit n (0..16) a chunk had n
 * bits corrected, bit 27 a sequencer fault. The lists are read and
 * advanced in place by the program. Plain bus addresses, as the reads were
 * proven with. */
#define FMISS_R6_UECC       0x40000000
#define FMISS_R6_CORRECTED  0x0001fffe
#define NAND_READ_TIMING    ((NAND_TUNK1 << 16) | (NAND_TWP << 12))

static struct
{
    uint32_t row, cemask, dma[2], meta[NAND_META_WORDS];
    uint8_t stat[NAND_CHUNKS_PER_PAGE];
} nand_rd CACHEALIGN_ATTR;

static int nand_read_page_fmt(uint32_t bank, uint32_t page, void *databuf,
                   uint32_t *meta)
{
    uint8_t *buf = databuf;
    unsigned int i, blank = 0, bad = 0;
    uint32_t r6;
    int rc;

    if ((uintptr_t)buf & (CACHEALIGN_SIZE - 1))
        buf = nand_fmiss_page;
    nand_rd.row = page;
    nand_rd.cemask = 1u << bank;
    nand_rd.dma[0] = (uint32_t)(intptr_t)buf;
    nand_rd.dma[1] = (uint32_t)(intptr_t)buf + NAND_PAGE_SIZE / 2;
    commit_discard_dcache_range(&nand_rd, sizeof(nand_rd));
    discard_dcache_range(buf, NAND_PAGE_SIZE);

    FMISS_REG(FMISS_P_ROWS) = (uint32_t)(intptr_t)&nand_rd.row;
    FMISS_REG(FMISS_P_CEMASKS) = (uint32_t)(intptr_t)&nand_rd.cemask;
    FMISS_REG(FMISS_P_COUNT) = 1;
    FMISS_REG(FMISS_P_META) = (uint32_t)(intptr_t)nand_rd.meta;
    FMISS_REG(FMISS_P_DMALIST) = (uint32_t)(intptr_t)nand_rd.dma;
    FMISS_REG(FMISS_P_ECCBUF) = (uint32_t)(intptr_t)nand_rd.stat;
    FMISS_REG(FMISS_P_STATS) = 0;
    FMISS_REG(0x0d2c) = NAND_KICK_DATA | NAND_KICK_BCH16;
    FMISS_REG(0x0d30) = NAND_KICK_META | NAND_KICK_BCH16;
    FMISS_REG(0x0d34) = 10 + nand_ecclen - 1;
    FMISS_REG(0x0d38) = nand_ecclen - 1;
    FMISS_REG(FMISS_P_TIMING) = NAND_READ_TIMING;
    rc = nand_fmiss_run(fmss_prog_read, sizeof(fmss_prog_read));
    r6 = FMISS_REG(0x0c30);
    if (!rc && (r6 & FMISS_R6_ABORTED))
        rc = -4;
    if (rc >= -1 || rc == -4)
        nand_fmiss_cleanup();
    nand_touch();
    if (rc)
        return rc;
    discard_dcache_range(&nand_rd, sizeof(nand_rd));
    discard_dcache_range(buf, NAND_PAGE_SIZE);

    /* An erased page reads as erased chunks, a few of which may decode as
     * uncorrectable from bits flipped in the blank area */
    for (i = 0; i < NAND_CHUNKS_PER_PAGE; i++)
    {
        if (nand_rd.stat[i] == 0xfe)
            blank++;
        else if (nand_rd.stat[i] == 0xff)
            bad++;
    }
    if (blank == NAND_CHUNKS_PER_PAGE
        || (bad < 3 && blank + bad == NAND_CHUNKS_PER_PAGE))
    {
        memset(buf, 0xff, NAND_PAGE_SIZE);
        memset(nand_rd.meta, 0xff, sizeof(nand_rd.meta));
        r6 = 0;
    }
    if (buf != databuf)
        memcpy(databuf, buf, NAND_PAGE_SIZE);
    if (meta)
        memcpy(meta, nand_rd.meta, sizeof(nand_rd.meta));
    if (r6 & FMISS_R6_UECC)
        return NAND_ECC_FAILED;
    return (r6 & FMISS_R6_CORRECTED) ? NAND_ECC_CORRECTED : NAND_ECC_CLEAN;
}

/* For a part outside Apple's table the strength is unknown, and erased
 * pages cannot select it. Retry an uncorrectable programmed page with
 * BCH16 and retain that format only after success. */
int nand_read_page(uint32_t bank, uint32_t page, void *databuf,
                   uint32_t *meta)
{
    int rc = nand_read_page_fmt(bank, page, databuf, meta);

    if (nand_ecclen_fixed || nand_ecclen == 26 || rc != NAND_ECC_FAILED)
        return rc;
    nand_ecclen = 26;
    rc = nand_read_page_fmt(bank, page, databuf, meta);
    if (rc != NAND_ECC_CLEAN && rc != NAND_ECC_CORRECTED)
        nand_ecclen = 13;
    return rc;
}

/* fmss/boot_read.s: the BootROM boot-block format, 0x600 bytes per page in
 * three BCH16 chunks with 26 parity bytes each. 0x0d00..0x0d08 return each
 * chunk's uncorrectable (bit 0) and erased (bit 6) flags. Bank 0 page 0
 * holds the IMG1 header. */
int nand_read_boot_page(uint32_t bank, uint32_t page, void *databuf)
{
    uint8_t *buf = databuf;
    int rc, i, c;

    if ((uintptr_t)buf & (CACHEALIGN_SIZE - 1))
        buf = nand_fmiss_page;
    discard_dcache_range(buf, FMISS_WRITE_BYTES);
    FMISS_REG(FMISS_P_ROWS) = page;
    FMISS_REG(FMISS_P_CEMASKS) = 1u << bank;
    FMISS_REG(FMISS_P_DMALIST) = (uint32_t)(intptr_t)buf;
    rc = nand_fmiss_run(fmss_prog_boot_read, sizeof(fmss_prog_boot_read));
    if (!rc && (FMISS_REG(0x0c30) & FMISS_R6_ABORTED))
        rc = -4;
    if (rc >= -1 || rc == -4)
        nand_fmiss_cleanup();
    if (rc)
        return rc;
    discard_dcache_range(buf, FMISS_WRITE_BYTES);
    if (buf != databuf)
        memcpy(databuf, buf, FMISS_WRITE_BYTES);
    for (c = 0; c < 3; c++)
    {
        uint32_t st = FMISS_REG(0x0d00 + 4 * c);

        if (!(st & 0x40))
        {
            if (st & 1)
                return NAND_ECC_FAILED;
            continue;
        }
        /* an erased chunk must read all 0xff */
        for (i = 0; i < 512; i++)
            if (buf[c * 512 + i] != 0xff)
                return NAND_ECC_FAILED;
    }
    return NAND_ECC_CLEAN;
}

void nand_set_writable(bool on)
{
    nand_writable = on;
}

/* Block 0 of every bank holds the BootROM image or its Apple backup, and
 * only the bootloader's armed boot path may touch it */
static bool nand_write_allowed(uint32_t bank, uint32_t block)
{
    return nand_writable && block != 0 && nand_fmiss_target_ok(bank, block);
}

#ifdef BOOTLOADER
static bool nand_boot_armed;

void nand_boot_arm_writes(void)
{
    nand_boot_armed = true;
}

/* Block 0 only, and only after the bootloader has matched the Apple image
 * and the install or restore buttons. */
static bool nand_boot_write_allowed(uint32_t bank, uint32_t block)
{
    return nand_boot_armed && block == 0 && nand_fmiss_target_ok(bank, block);
}

int nand_boot_erase_block(uint32_t bank, uint32_t block)
{
    if (!nand_boot_write_allowed(bank, block))
        return -2;
    return nand_fmiss_erase(bank, block);
}

int nand_boot_program_page(uint32_t bank, uint32_t page, const void *data)
{
    uint32_t r6;
    int rc;

    if (!data || !nand_chip || !nand_chip->pagesperblock
        || !nand_boot_write_allowed(bank, page / nand_chip->pagesperblock))
        return -2;
    memset(nand_fmiss_page, 0xff, sizeof(nand_fmiss_page));
    memcpy(nand_fmiss_page, data, FMISS_WRITE_BYTES);
    commit_dcache_range(nand_fmiss_page, sizeof(nand_fmiss_page));
    FMISS_REG(FMISS_P_ROWS) = page;
    FMISS_REG(FMISS_P_CEMASKS) = 1u << bank;
    FMISS_REG(FMISS_P_DMALIST) = FMISS_RAM_ALIAS(nand_fmiss_page);
    rc = nand_fmiss_run(fmss_prog_boot_program,
                        sizeof(fmss_prog_boot_program));
    r6 = FMISS_REG(0x0c30);
    if (!rc && (r6 & FMISS_R6_ABORTED))
        rc = -4;
    else if (!rc && (r6 & FMISS_R6_FAILED))
        rc = -10;
    if (rc >= -1 || rc == -4 || rc == -10)
        nand_fmiss_cleanup();
    return rc;
}

/* A user-format page into block 0, through the same program as
 * nand_write_pages(): the write test checks that path on a spare bank's
 * block 0, which is not FTL data */
int nand_boot_program_user_page(uint32_t bank, uint32_t page,
                                const void *data, const uint32_t *meta)
{
    struct nand_write w = { bank, page, data, meta };

    if (!data || !nand_chip || !nand_chip->pagesperblock
        || !nand_boot_write_allowed(bank, page / nand_chip->pagesperblock))
        return -2;
    return nand_fmiss_program(&w, 1, NULL);
}
#endif

int nand_read_pages(struct nand_read *r, unsigned int n)
{
    unsigned int i;

    if (!nand_ready || !n)
        return -1;
    for (i = 0; i < n; i++)
    {
        int ecc;

        if (r[i].bank >= nand_banks
            || r[i].page >= (uint32_t)nand_chip->blocks
                            * nand_chip->pagesperblock)
            return -1;
        ecc = nand_read_page(r[i].bank, r[i].page, r[i].buf, r[i].meta);
        if (ecc < 0)
            return -1;
        r[i].ecc = ecc;
    }
    return 0;
}

int nand_erase_block(uint32_t bank, uint32_t block)
{
    int rc;

    if (!nand_write_allowed(bank, block))
        return -1;
    rc = nand_fmiss_erase(bank, block);
    nand_touch();
    if (rc)
        logf("nand: erase bank %lu block %lu: %d", (unsigned long)bank,
             (unsigned long)block, rc);
    return rc == -10 ? NAND_OP_FAILED : rc ? -1 : 0;
}

/* two_plane is accepted and ignored. Program at most one page per bank in
 * each FMISS run, preserving the order of pages sent to each bank. */
int nand_write_pages(const struct nand_write *w, unsigned int n,
                     bool two_plane, uint32_t *failbank)
{
    struct nand_write batch[NAND_FMISS_PROGRAM_BATCH];
    unsigned int i, bank, done = 0;
    unsigned int next[NAND_MAX_BANKS];
    int rc;

    (void)two_plane;
    if (!n)
        return -1;
    for (i = 0; i < n; i++)
    {
        if (!w[i].buf || !nand_chip
            || !nand_write_allowed(w[i].bank,
                                   w[i].page / nand_chip->pagesperblock))
        {
            if (failbank)
                *failbank = w[i].bank;
            return -1;
        }
    }
    for (bank = 0; bank < NAND_MAX_BANKS; bank++)
    {
        next[bank] = n;
        for (i = 0; i < n; i++)
            if (w[i].bank == bank)
            {
                next[bank] = i;
                break;
            }
    }
    while (done < n)
    {
        unsigned int count = 0;

        for (bank = 0; bank < nand_banks; bank++)
            if (next[bank] < n)
            {
                i = next[bank];
                batch[count++] = w[i];
                next[bank] = n;
                for (i++; i < n; i++)
                    if (w[i].bank == bank)
                    {
                        next[bank] = i;
                        break;
                    }
            }
        if (!count)
            return -1;
        rc = nand_fmiss_program(batch, count, failbank);
        if (rc)
        {
            uint32_t failed_bank = failbank ? *failbank : batch[0].bank;

            for (i = 0; i < count; i++)
                if (batch[i].bank == failed_bank)
                    break;
            if (i == count)
                i = 0;
            logf("nand: program bank %lu page %lu: %d",
                 (unsigned long)failed_bank,
                 (unsigned long)batch[i].page, rc);
            return rc == -10 ? NAND_OP_FAILED : -1;
        }
        done += count;
    }
    nand_touch();
    return 0;
}

int nand_write_page(uint32_t bank, uint32_t page, const void *databuf,
                    const uint32_t *meta)
{
    struct nand_write w = { bank, page, databuf, meta };

    return nand_write_pages(&w, 1, false, NULL);
}

unsigned int nand_get_bank_count(void)
{
    return nand_banks;
}

uint32_t nand_chip_id(void)
{
    return nand_id;
}

/* The FTL disk starts with Apple's own map: the firmware area (type 0x3f)
 * and a small 0x3e area, then the FAT partition. That partition is itself
 * a disk, with its own MBR ("Apple iPod") and the FAT32 volume at 63
 * (measured). Offer only it, as Apple's disk mode does, and only when a
 * FAT32 boot sector is where the inner MBR says. */
static uint32_t nand_user_start;
static uint32_t nand_user_sectors;
static uint8_t nand_user_buf[SECTOR_SIZE] STORAGE_ALIGN_ATTR;

static bool nand_is_fat32_bs(const uint8_t *s)
{
    return s[510] == 0x55 && s[511] == 0xaa
        && (s[11] | s[12] << 8) == SECTOR_SIZE
        && (s[0x24] | s[0x25] << 8 | s[0x26] << 16 | (uint32_t)s[0x27] << 24)
        && !memcmp(&s[0x52], "FAT32", 5);
}

static void nand_find_user_disk(void)
{
    uint32_t total = ftl_num_sectors();
    const uint8_t *e;
    uint32_t outer = 0, inner, i;

    nand_user_start = 0;
    nand_user_sectors = total;
    if (ftl_read(0, 1, nand_user_buf) || nand_user_buf[510] != 0x55
        || nand_user_buf[511] != 0xaa)
        return;
    for (i = 0; i < 4; i++)
    {
        e = &nand_user_buf[0x1be + 16 * i];
        if (e[4] == 0x0b || e[4] == 0x0c)
        {
            outer = e[8] | e[9] << 8 | e[10] << 16 | (uint32_t)e[11] << 24;
            break;
        }
    }
    if (!outer || outer >= total || ftl_read(outer, 1, nand_user_buf)
        || nand_is_fat32_bs(nand_user_buf)
        || nand_user_buf[510] != 0x55 || nand_user_buf[511] != 0xaa)
        return;
    e = &nand_user_buf[0x1be];
    if (e[4] != 0x0b && e[4] != 0x0c)
        return;
    inner = e[8] | e[9] << 8 | e[10] << 16 | (uint32_t)e[11] << 24;
    if (!inner || inner >= total - outer
        || ftl_read(outer + inner, 1, nand_user_buf)
        || !nand_is_fat32_bs(nand_user_buf))
        return;
    nand_user_start = outer;
    nand_user_sectors = total - outer;
    logf("nand: user disk at %lu", (unsigned long)outer);
}

uint32_t nand_user_disk_start(void)
{
    return nand_user_start;
}

int nand_init(void)
{
    uint32_t bank, id;
    int rc;

    nand_ready = false;
    nand_banks = 0;
    nand_chip = NULL;
    nand_id = 0;
    nand_ecclen = 13;

    /* NAND pinmux, as the BootROM's NAND boot path (0x20001b70) sets it */
    PCON6 = (PCON6 & 0x0000000f) | 0x22222220;
    PCON7 = 0x22222222;
    PCON(8) = (PCON(8) & 0xff0000f0) | 0x00333302;

    rc = nand_reset(0);
    if (rc)
    {
        logf("nand: reset failed (%d)", rc);
        return rc;
    }
    if (nand_identify(&id))
    {
        logf("nand: identify failed");
        return NAND_ERR_IDENTIFY;
    }
    /* Keep the ID even for an unknown chip, so the storage error on screen
     * can show which chip it is. */
    nand_id = id;
    nand_select_ecc(id);
    if (!nand_decode(id, &nand_chip_decoded))
        nand_chip = &nand_chip_decoded;

    for (bank = 1; bank < NAND_MAX_BANKS; bank++)
        if (nand_reset(bank) || nand_identify(&id) || id != nand_id)
            break;
    nand_banks = bank;
    if (!nand_chip)
    {
        logf("nand: unknown chip %08lx x %u", (unsigned long)nand_id,
             nand_banks);
        return NAND_ERR_UNKNOWN;
    }
    /* Apple's populations for this model: two or four chips of one part on
     * the first chip enables, the others empty */
    for (; bank < NAND_MAX_BANKS; bank++)
        if (!nand_reset(bank) && !nand_identify(&id)
            && id != 0 && id != 0xffffffff)
            return NAND_ERR_UNKNOWN;
    if (nand_banks != 2 && nand_banks != NAND_MAX_BANKS)
        return NAND_ERR_UNKNOWN;

    nand_ready = true;
    nand_fill_geo();
    nand_touch();

    rc = ftl_init();
    if (rc)
    {
        logf("nand: FTL mount failed (%d)", rc);
        return NAND_ERR_FTL + rc;
    }
    nand_find_user_disk();
    return 0;
}

void nand_enable(bool on)
{
    (void)on;
}

void nand_spindown(int seconds)
{
    (void)seconds;
}

void nand_sleepnow(void)
{
}

void nand_close(void)
{
}

void nand_spin(void)
{
    nand_touch();
}

int nand_spinup_time(void)
{
    return 0;
}

#ifdef HAVE_STORAGE_FLUSH
int nand_flush(void)
{
    if (!nand_ready)
        return -1;
    return ftl_sync();
}
#endif

int nand_read_sectors(IF_MD(int drive,) sector_t start, int incount,
                     void* inbuf)
{
#ifdef HAVE_MULTIDRIVE
    (void)drive;
#endif
    int rc;

    if (!nand_ready)
        return -1;
    if (start + incount > nand_user_sectors)
        return -2;
    rc = ftl_read((uint32_t)(start + nand_user_start), incount, inbuf);
    if (!rc)
        nand_touch();
    return rc;
}

int nand_write_sectors(IF_MD(int drive,) sector_t start, int count,
                      const void* outbuf)
{
#ifdef HAVE_MULTIDRIVE
    (void)drive;
#endif
    int rc;

    if (!nand_ready)
        return -1;
    if (start + count > nand_user_sectors)
        return -2;
    rc = ftl_write((uint32_t)(start + nand_user_start), count, outbuf);
    if (!rc)
        nand_touch();
    return rc;
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
    info->num_sectors = nand_user_sectors;
    info->vendor = "Apple";
    info->product = "iPod Nano 4G";
    info->revision = "1.0";
}
#endif

long nand_last_disk_activity(void)
{
    return nand_last_activity_value;
}

#ifdef CONFIG_STORAGE_MULTI
int nand_num_drives(int first_drive)
{
    (void)first_drive;
    return 1;
}
#endif
