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
 * NAND driver for the iPod Nano 3G (S5L8702 FMC controller).
 *
 * The register sequences were worked out from the BootROM and the retail
 * firmware of the owner's own player: resetting a bank, waiting for it to
 * go ready, and reading a page with its per-chunk ECC. nand_identify()
 * follows wInd3x (freemyipod/wInd3x, pkg/exploit/wind3x_n3g.go).
 *
 * Limitations:
 *  - Only the first FMC controller is used. Banks (chip enables) 0 to
 *    NAND_MAX_BANKS-1 are probed at init; banks above 0 are reachable only
 *    because gpio_preinit() muxes all of PCON(9), which the BootROM does not.
 *  - Only chips validated on hardware are driven; nand_init() refuses any
 *    other chip (see nand_chip_table).
 *  - Storage goes through Apple's FTL (ftl-nano3g.c).
 *
 * A build with -DNAND_CHECK is the contributor check image: it serves the
 * raw NAND read-only instead of the FTL's disk, so the storage API below is
 * left to nand-check-nano3g.c and this file keeps only the chip.
 */

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "mv.h"
#include "storage.h"
#include "clocking-s5l8702.h"
#include "logf.h"
#include "panic.h"
#include "nand-target.h"
#include "ftl-target.h"
#include <cpucache-arm.h>
#include <stdbool.h>

#define NAND_CMD_READ       0x00
#define NAND_CMD_READ2      0x30
#define NAND_CMD_READSTATUS 0x70
#define NAND_CMD_READID     0x90
#define NAND_CMD_RESET      0xFF
#define NAND_CMD_PROGRAM    0x80
#define NAND_CMD_PROGRAM_PLANE1 0x81    /* plane 1 of a two-plane program */
#define NAND_CMD_PROGRAM_QUEUE  0x11    /* plane 0 queued, plane 1 to come */
#define NAND_CMD_PROGRAM2   0x10
#define NAND_CMD_ERASE      0x60
#define NAND_CMD_ERASE2     0xD0

/* READ STATUS result */
#define NAND_STATUS_FAIL    (1 << 0)
#define NAND_STATUS_READY   (1 << 6)

/* The controller moves a page in units of 2KiB, each of four 512-byte
 * chunks: one unit on 2KiB-page chips, two on 4KiB-page chips */
#define NAND_UNIT_SIZE      2048
#define NAND_CHUNK_SIZE     512

/* Bus timing: Apple's values, giving FMCTRL0 = 0x43803 for bank 0. Every
 * legal combination reads the ID correctly, so these are not critical. */
#define NAND_TUNK1          4 /* 3-bit field at bit 16 */
#define NAND_TWP            3 /* 3-bit field at bit 12 */

/* Apple's poll helper (0x20000e50) gives up after 100000 reads. */
#define NAND_POLL_SPINS     1000000

struct nand_chip_info
{
    uint32_t id;            /* first four ID bytes, little endian */
    uint8_t banks;          /* chip enables holding this part */
    uint8_t mode;           /* Apple's layout and programming mode */
    uint16_t blocks;        /* per chip enable */
    uint16_t pagesperblock;
    uint16_t pagesize;      /* bytes of data per page */
    uint16_t userblocks;    /* per chip enable, as Whimory counts them */
    bool validated;         /* read, written and remounted on hardware */
};

/* Apple's own chip table in the Nano 3G firmware (osos 1.1.3: 18 rows of 44
 * bytes holding the id, chip enables, blocks per enable, pages per block,
 * 512-byte sectors per page, spare bytes, timings, user blocks and mode).
 * Apple picks the row matching both the id and the number of chip enables
 * answering with it, and so does nand_init(): the same part can come in
 * a different mode with a different bank count.
 *
 * The mode decides how the VFL groups physical blocks (nand_mode_planes()
 * and nand_mode_layout()) and which program writes two planes at once. Only
 * validated rows are driven; nand_init() refuses any other chip, except in
 * the check image, which reads a chip without mounting it writable. */
static const struct nand_chip_info nand_chip_table[] =
{
    /* Hynix: the 4GB unit this port was developed on, and the 8GB one */
    { 0xA514D3AD, 4,  8, 4096, 128, 2048, 3872, true  },
    { 0xA555D5AD, 4,  8, 8192, 128, 2048, 7744, true  },
    /* Micron, 2 chip enables */
    { 0xA5D5D52C, 2,  4, 8192, 128, 2048, 7744, true  },
#ifdef NAND_CHECK
    /* The rest of the chips the original firmware knows. They are here for
     * the check image alone, which identifies a chip and reads it to
     * collect what validating it needs; a normal build does not carry them,
     * so Rockbox cannot drive a chip nobody has tested. A row moves above
     * this line, with its validated flag set, once someone has run the
     * check and a write test on that chip. A row tagged "reported" is
     * secondhand - someone's id and geometry, unconfirmed by us; one
     * tagged "checked" has a check archive that mounts and replays
     * correctly in the host FTL suite - real evidence, short only of the
     * write test. */
    /* Micronas (ITT Intermetall, acquired by TDK 2016): 0xEC, not Samsung */
    { 0xB614D5EC, 2,  8, 4096, 128, 4096, 3872, false },   /* checked */
    { 0xB614D5EC, 4,  1, 4096, 128, 4096, 3872, false },
    { 0x2555D5EC, 4,  9, 8192, 128, 2048, 7744, false },
    /* Hynix */
    { 0xB614D5AD, 4,  1, 4096, 128, 4096, 3872, false },
    /* Toshiba */
    { 0xA585D598, 2, 13, 8320, 128, 2048, 7744, false },
    { 0xA585D598, 4, 13, 8320, 128, 2048, 7744, false },
    { 0xBA94D598, 2, 12, 4096, 128, 4096, 3872, false },
    { 0xBA94D598, 4,  1, 4096, 128, 4096, 3872, false },
    /* Intel */
    { 0xA5D5D589, 2,  4, 8192, 128, 2048, 7744, false },   /* checked */
    { 0xA5D5D589, 4,  2, 8192, 128, 2048, 7744, false },   /* checked */
    { 0x3E94D589, 2,  3, 4096, 128, 4096, 3872, false },   /* checked */
    { 0x3ED5D789, 2,  2, 8192, 128, 4096, 7744, false },
    /* Micron */
    { 0xA5D5D52C, 4,  2, 8192, 128, 2048, 7744, false },
    { 0x3E94D52C, 2,  3, 4096, 128, 4096, 3872, false },
    { 0x3ED5D72C, 2,  2, 8192, 128, 4096, 7744, false },
#endif
};
/* Physical blocks per bank in one VFL block: the original firmware's mode
 * switch uses 1 for mode 1, 4 for mode 4, and 2 for every other mode */
static unsigned int nand_mode_planes(unsigned int mode)
{
    return mode == 1 ? 1 : mode == 4 ? 4 : 2;
}

/* How the planes of a VFL block are placed: a partner in the next block,
 * the other half of the chip, or both */
static unsigned int nand_mode_layout(unsigned int mode)
{
    switch (mode)
    {
    case 3: case 8:         return NAND_LAYOUT_ADJACENT;
    case 2: case 9: case 12: return NAND_LAYOUT_HALVES;
    case 4:                 return NAND_LAYOUT_BOTH;
    case 1:                 return NAND_LAYOUT_SINGLE;
    case 13:                return NAND_LAYOUT_SPLIT13;
    default:                return NAND_LAYOUT_UNKNOWN;
    }
}

static const struct nand_chip_info *nand_chip;
static uint32_t nand_id;        /* what bank 0 answered READ ID with */
static unsigned int nand_banks;
static bool nand_ready = false;
static long nand_last_activity_value = -1;
static struct nand_geometry nand_geo;

const struct nand_geometry *nand_get_geometry(void)
{
    if (!nand_ready)
        return NULL;
    nand_geo.banks = nand_banks;
    nand_geo.blocks = nand_chip->blocks;
    nand_geo.pagesperblock = nand_chip->pagesperblock;
    nand_geo.planes = nand_mode_planes(nand_chip->mode);
    nand_geo.userblocks = nand_chip->userblocks;
    /* Whimory reserves each bank's blocks beyond its user blocks, less 23
     * superblocks: 3 FTL control blocks and a pool of 20. 89 was measured
     * on the 4GB unit; the rest follow the same rule. */
    nand_geo.vflspares = (nand_chip->blocks - nand_chip->userblocks)
                       / nand_geo.planes - 23;
    /* Mode 8 is the mode whose two-plane program nand_write_pages()
     * reproduces */
    nand_geo.twoplane = nand_chip->mode == 8;
    nand_geo.pagesize = nand_chip->pagesize;
    nand_geo.mode = nand_chip->mode;
    nand_geo.layout = nand_mode_layout(nand_chip->mode);
    nand_geo.validated = nand_chip->validated;
#ifdef NAND_WRITABLE_ID
    /* Validation builds for testers: mount chips with this id writable,
     * whatever their bank count, e.g. -DNAND_WRITABLE_ID=0xA555D5AD */
    if (nand_chip->id == NAND_WRITABLE_ID)
        nand_geo.validated = true;
#endif
#ifdef NAND_TEST_READONLY
    /* Test builds: behave as on a chip that is not validated */
    nand_geo.validated = false;
#endif
    return &nand_geo;
}

/* ECC state gathered over the four chunks of a page, as the firmware's
 * page read does */
struct nand_ecc_acc
{
    bool flagged;           /* FMCSTAT_UNK27 was set on some chunk */
    uint32_t unk810;        /* FMUNK810 after each correction, ORed */
    uint32_t transstat;     /* FMTRANSSTAT after each correction, ORed */
};

static void nand_touch(void)
{
    nand_last_activity_value = current_tick;
}

static void nand_set_fmctrl0(uint32_t bank)
{
    FMCTRL0 = (NAND_TUNK1 << 16) | (NAND_TWP << 12) | FMCTRL0_UNK1
            | 1 | FMCTRL0_CE(bank);
}

/* Spin until bit is set in reg, then write 1 to clear it. Tight and
 * bounded, like Apple's poll helper: some of these flags assert briefly. */
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

/* As nand_wait_stat(), but leaves the bit set */
static int nand_wait_stat_noclear(uint32_t bit)
{
    unsigned long spins = NAND_POLL_SPINS;
    while (spins--)
        if (FMCSTAT & bit)
            return 0;
    return -1;
}

/* Clear every FMCSTAT bit. Needed after nand_identify() and after each page
 * read: stale bits let the stage waits in nand_transfer_chunk() pass before
 * the data has moved, which corrupts the next read. The original
 * firmware's page read ends the same way. */
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

static int nand_send_addr_page(uint32_t page)
{
    FMANUM = 4;
    FMADDR0 = page << 16;
    FMADDR1 = (page >> 16) & 0xFF;
    FMCTRL1 = FMCTRL1_DOTRANSADDR;
    return nand_wait_stat(FMCSTAT_ADDRDONE);
}

/* Switch the NAND pins to their NAND function: Apple's 0x20001830 with
 * argument 0 (first controller). It leaves the other PCON(9) pins alone. */
static void nand_gpio_config(void)
{
    PCON(10) = (PCON(10) & 0xFFFF0000) | 0x00002222;
    PCON(9)  = (PCON(9)  & ~0x000F000F) | 0x00020002;
    PCON(8)  = 0x22222222;
}

/* Wait for the chip to go ready, as the original firmware does after every
 * operation: READ STATUS, then FMCTRL1 = 0xca has the controller read
 * the status byte into FMSYND0. Apple writes FMADDR2 = 1 whatever the bank.
 * Returns the status byte, or -1 on a timeout. It leaves the chip in status
 * output mode, so a reader must re-issue READ. */
static int nand_read_status(void)
{
    /* The microsecond timer, not current_tick: this runs at nand_init()
     * and must be bounded with interrupts off too */
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

/* Wait for a read or reset to finish. This used to follow Apple's ROM
 * (0x20009614): READ STATUS, FMCTRL1 = 0x2a, then wait for FMCSTAT bit 23.
 * That bit does set (128 reads of 128, measured), but nothing in it says
 * the chip is ready; nand_read_status() checks the ready bit, as the retail
 * firmware does. Both take ~300 us per page read. */
static int nand_wait_ready(void)
{
    int status = nand_read_status();

    FMADDR2 = 0;
    FMCTRL1 = 0xe0;
    return status < 0 ? NAND_ERR_READY_CMD : 0;
}

int nand_reset(uint32_t bank)
{
    clockgate_enable(CLOCKGATE_NAND, true);
    clockgate_enable(CLOCKGATE_NANDECC, true);
    nand_gpio_config();

    nand_set_fmctrl0(bank);
    if (nand_send_cmd(NAND_CMD_RESET))
        return NAND_ERR_RESET_CMD;

    /* Wait out tRST as well: the old ready wait could not tell when the
     * chip had left reset. */
    udelay(5000);
    return nand_wait_ready();
}

/* Read the first four ID bytes of the bank selected by the last
 * nand_reset(), following wInd3x. ext, if not NULL, gets
 * the next four of the eight the controller is asked for. */
int nand_identify(uint32_t *id, uint32_t *ext)
{
    if (nand_send_cmd(NAND_CMD_READID))
        return -1;
    if (nand_send_addr_byte(0))
        return -1;

    FMDNUM = 7;
    FMADDR2 = 1;
    FMCSTAT = FMCSTAT_TRANSDONE; /* nand_wait_ready() leaves it set */
    FMCTRL1 = FMCTRL1_DOREADDATA | FMCTRL1_CLEARWFIFO | FMCTRL1_CLEARRFIFO
            | (1 << 8);
    /* wInd3x leaves TRANSDONE set here; nand_clear_status() clears it */
    if (nand_wait_stat_noclear(FMCSTAT_TRANSDONE))
        return -1;
    FMADDR2 = 0x100;
    FMCTRL1 = FMCTRL1_CLEARWFIFO | (1 << 8) | (1 << 9);

    *id = FMFIFO;
    if (ext)
        *ext = FMFIFO;
    nand_clear_status();
    return 0;
}

/* Kick the controller's correction of the chunk just read and wait for it.
 * The register writes Apple's 0x2000ac1c makes, in the same order. */
static int nand_kick_chunk(uint32_t half, uint32_t sub, uint32_t upper)
{
    FMTRANSSTAT = 0x7f;
    FMTRANS1 = 0x01000180;
    FMTRANS0 = (1 << (sub + 8)) | (upper << 16) | (1 << (half + 8)) | 1;
    return nand_wait_reg(&FMTRANSSTAT, 1 << 2);
}

/* Move one 512-byte quarter (chunk 0..3) of the current page to dst, as
 * the firmware's page read does. Stages A and B read the chunk into the
 * controller,
 * which sets FMCSTAT_UNK27 if it finds ECC errors and corrects them when
 * kicked. Stage C then has the controller write the chunk to dst. */
static int nand_transfer_chunk(uint32_t chunk, void *dst,
                               struct nand_ecc_acc *ecc)
{
    uint32_t half  = chunk & 1;
    uint32_t upper = (chunk >> 1) & 1;
    uint32_t sub   = half + 4;

    /* Stage A: 16 units */
    FMADDR6 = upper ? 0x10 : 0;
    FMADDR2 = 1 << sub;
    FMDNUM = 0xf;
    FMCTRL1 = 0x32;
    if (nand_wait_stat(FMCSTAT_TRANSDONE))
        return -1;

    /* Stage B: 512 units */
    FMDNUM = 0x1ff;
    FMADDR2 = 1 << half;
    FMADDR6 = 0;
    FMCTRL1 = 0x22;
    if (nand_wait_stat(FMCSTAT_TRANSDONE))
        return -1;
    if (FMCSTAT & FMCSTAT_UNK27)
    {
        if (nand_kick_chunk(half, sub, upper))
            return -1;
        ecc->flagged = true;
        ecc->unk810 |= FMUNK810;
        ecc->transstat |= FMTRANSSTAT;
    }

    /* Stage C: the controller writes dst behind the cache's back */
    commit_discard_dcache_range(dst, NAND_CHUNK_SIZE);
    FMDATAW0 = (uint32_t)(intptr_t)dst;
    FMDATAW1 = 7;
    FMCTRL0 = (FMCTRL0 & ~FMCTRL0_ENABLEDMA) | FMCTRL0_AUTOXFER | 1;
    FMADDR2 = 1 << (half + 8);
    FMCTRL1 = 0x1a0;
    if (nand_wait_stat(FMCSTAT_UNK20))
        return -1;
    discard_dcache_range(dst, NAND_CHUNK_SIZE);
    return 0;
}

int nand_read_page(uint32_t bank, uint32_t page, void *databuf,
                   uint32_t *meta)
{
    struct nand_ecc_acc ecc = { false, 0, 0 };
    unsigned long spins;
    uint32_t chunk;

    /* That sequence only knows 2KiB pages. Larger pages take the firmware's
     * read sequence, which loops over the page's units. */
    if (nand_chip->pagesize != NAND_UNIT_SIZE)
    {
        struct nand_read r = { bank, page, databuf, meta, 0, 0 };

        return nand_read_pages(&r, 1) ? -1 : r.ecc;
    }

    nand_set_fmctrl0(bank);
    if (nand_send_cmd(NAND_CMD_READ))
        return -1;
    if (nand_send_addr_page(page))
        return -1;
    if (nand_send_cmd(NAND_CMD_READ2))
        return -1;
    if (nand_wait_ready())
        return -1;
    if (nand_send_cmd(NAND_CMD_READ)) /* back to data output */
        return -1;

    for (chunk = 0; chunk < 4; chunk++)
    {
        FMCSTAT = FMCSTAT_UNK27;
        if (nand_transfer_chunk(chunk,
                (uint8_t *)databuf + chunk * NAND_CHUNK_SIZE, &ecc))
            return -1;
    }

    /* Have the controller decode the spare metadata, as the firmware does
     * after its chunk loop. The result appears in FMSYND5..7. */
    FMUNK78 = 0x5140;
    FMUNK7C = 2;
    spins = NAND_POLL_SPINS;
    while (FMUNK7C & 2)
        if (!--spins)
            return -1;
    if (meta)
    {
        meta[0] = FMSYND5;
        meta[1] = FMSYND6;
        meta[2] = FMSYND7;
    }

    nand_clear_status();

    /* The same classification the firmware's page read returns */
    if (!ecc.flagged)
        return NAND_ECC_CLEAN;
    if (ecc.unk810 & 1)
        return NAND_ECC_FAILED;
    if (ecc.transstat & 0x20)
        return NAND_ECC_STATUS6;
    return NAND_ECC_CORRECTED;
}

/* Add one chunk's correction result to the bitmap used by Apple's firmware
 * read program: bit 30 is uncorrectable, and low bit n-1 says an n-bit
 * correction occurred. */
static void nand_read_ecc_result(uint32_t *result)
{
    uint32_t ecc = FMUNK810;

    if (ecc & 1)
        *result |= 1u << 30;
    else
    {
        uint32_t count = (ecc >> 16) & 0xf;
        if (count)
            *result |= 1u << (count - 1);
    }
}

/* Transfer one page after its NAND page load has completed. This is the
 * data and ECC path of the firmware's read program: four chunks per
 * 2KiB unit, the last chunk of every unit but the last going out as the
 * next unit's buffer is loaded, and the page's final chunk and erased-page
 * check once at the end. */
static int nand_read_loaded_page(struct nand_read *r, struct nand_read *next,
                                 bool primed, uint32_t *result)
{
    static const uint32_t bufsel[3] = { 0x102, 0x201, 0x102 };
    static const uint32_t correct[4] = { 0x1101, 0x11201, 0x21101, 0x31201 };
    uint32_t units = nand_chip->pagesize / NAND_UNIT_SIZE;
    uint32_t unit, chunk, stat;
    unsigned long spins;

    if (!primed)
    {
        FMCTRL1 = 0x100e0;
        if (nand_send_cmd(NAND_CMD_READ))
            return -1;

        /* Prime the first 16-byte and 512-byte controller stages. */
        FMDNUM = 0xf;
        FMADDR2 = 0x10;
        FMADDR6 = 0;
        FMCTRL1 = 0x32;
        if (nand_wait_stat(FMCSTAT_TRANSDONE))
            return -1;
        FMDNUM = 0x1ff;
        FMADDR2 = 1;
        FMCTRL1 = 0xe2;
        if (nand_wait_stat(FMCSTAT_TRANSDONE))
            return -1;

        FMCTRL0 |= FMCTRL0_AUTOXFER;
        FMDATAW0 = (uint32_t)(intptr_t)r->buf;
    }

    for (unit = 0; unit < units; unit++)
    {
        if (unit)
        {
            /* The previous unit's last chunk, and this unit's buffer */
            FMDNUM = 0xf;
            FMADDR2 = 0x10;
            FMADDR6 = 0;
            FMCTRL1 = 0x32;
            if (nand_wait_stat(FMCSTAT_TRANSDONE))
                return -1;
            FMADDR2 = 0x201;
            FMDNUM = 0x1ff;
            FMCTRL1 = 0xe2;
            FMTRANSSTAT = 0x1ff;
            FMTRANS1 = 0x180;
            FMTRANS0 = correct[3];
            if (nand_wait_reg(&FMTRANSSTAT, 1))
                return -1;
            nand_read_ecc_result(result);
            FMCTRL1 = 0x100;
            if (nand_wait_stat(FMCSTAT_UNK20))
                return -1;
            FMDATAW0 = (uint32_t)(intptr_t)r->buf + unit * NAND_UNIT_SIZE;
            if (nand_wait_stat(FMCSTAT_TRANSDONE))
                return -1;
        }

        for (chunk = 0; chunk < 3; chunk++)
        {
            FMDNUM = 0xf;
            FMADDR2 = 0x10;
            FMADDR6 = (chunk + 1) << 4;
            FMCTRL1 = 0x32;
            if (nand_wait_stat(FMCSTAT_TRANSDONE))
                return -1;
            FMADDR2 = bufsel[chunk];
            FMDNUM = 0x1ff;
            FMCTRL1 = 0xe2;

            FMTRANSSTAT = 0x1ff;
            FMTRANS1 = 0x180;
            FMTRANS0 = correct[chunk];
            if (nand_wait_reg(&FMTRANSSTAT, 1))
                return -1;
            nand_read_ecc_result(result);
            FMCTRL1 = 0x100;
            if (nand_wait_stat(FMCSTAT_UNK20)
                || nand_wait_stat(FMCSTAT_TRANSDONE))
                return -1;
        }
    }

    stat = FMCSTAT & FMCSTAT_UNK27;
    FMCSTAT = FMCSTAT_UNK27;
    if (!stat)
        *result |= 1u << 29;
    FMTRANSSTAT = 0x1ff;
    FMTRANS1 = 0x180;
    FMTRANS0 = correct[3];

    if (next)
    {
        FMCTRL0 = (FMCTRL0 & ~FMCTRL0_CE_MASK) | FMCTRL0_CE(next->bank);
        FMADDR2 = 1;
        if (nand_send_cmd(NAND_CMD_READSTATUS))
            return -1;
        FMCTRL1 = 0xca;
        if (nand_wait_stat(FMCSTAT_STATUSREADY))
            return -1;
        FMCTRL1 = 0x20;
    }
    if (nand_wait_reg(&FMTRANSSTAT, 1))
        return -1;
    nand_read_ecc_result(result);

    if (!next)
    {
        FMDATAW1 = 7;
        FMADDR2 = 0x200;
        FMDNUM = 0x1ff;
        FMCTRL1 = 0x1e0;
        if (nand_wait_stat(FMCSTAT_UNK20))
            return -1;
    }

    FMUNK78 = 0x3210;
    FMUNK7C = 2;
    spins = NAND_POLL_SPINS;
    while (FMUNK7C & 2)
        if (!--spins)
            return -1;
    if (r->meta)
    {
        r->meta[0] = FMSYND5;
        r->meta[1] = FMSYND6;
        r->meta[2] = FMSYND7;
    }

    if (next)
    {
        FMCTRL1 = 0x100e0;
        if (nand_send_cmd(NAND_CMD_READ))
            return -1;
        FMDNUM = 0xf;
        FMADDR2 = 0x10;
        FMADDR6 = 0;
        FMCTRL1 = 0x32;
        if (nand_wait_stat(FMCSTAT_TRANSDONE))
            return -1;
        FMADDR2 = 0x201;
        FMDNUM = 0x1ff;
        FMCTRL1 = 0xe2;
        FMCTRL1 = 0x100;
        if (nand_wait_stat(FMCSTAT_UNK20))
            return -1;
        FMDATAW0 = (uint32_t)(intptr_t)next->buf;
        if (nand_wait_stat(FMCSTAT_TRANSDONE))
            return -1;
    }
    return 0;
}

int nand_read_pages(struct nand_read *r, unsigned int n)
{
    unsigned int first, end, i;
    uint32_t used;
    int status;

    if (!nand_ready || !n)
        return -1;
    for (i = 0; i < n; i++)
    {
        if (r[i].bank >= nand_banks
            || r[i].page >= (uint32_t)nand_chip->blocks
                            * nand_chip->pagesperblock)
            return -1;
        commit_discard_dcache_range(r[i].buf, nand_chip->pagesize);
        r[i].ecc = NAND_ECC_FAILED;
    }

    FMCTRL1 = 0x0ff3f8e0;
    FMCSTAT = 0x0ff00ffe;
    FMCTRL0 = (NAND_TUNK1 << 16) | (NAND_TWP << 12) | FMCTRL0_UNK1 | 1;

    /* The FMSS program consumes the largest prefix containing no repeated
     * bank, starts every page load in it, transfers those pages, then repeats
     * with the remainder. */
    for (first = 0; first < n; first = end)
    {
        used = 0;
        for (end = first; end < n && !(used & (1u << r[end].bank)); end++)
            used |= 1u << r[end].bank;

        for (i = first; i < end; i++)
        {
            FMCTRL0 = (FMCTRL0 & ~FMCTRL0_CE_MASK) | FMCTRL0_CE(r[i].bank);
            if (nand_send_cmd(NAND_CMD_READ)
                || nand_send_addr_page(r[i].page)
                || nand_send_cmd(NAND_CMD_READ2))
                goto fail;
        }

        for (i = first; i < end; i++)
        {
            uint32_t result = 0;

            if (i == first)
                FMCTRL0 = (FMCTRL0 & ~FMCTRL0_CE_MASK)
                        | FMCTRL0_CE(r[i].bank);
            status = i == first ? nand_read_status() : NAND_STATUS_READY;
            if (status < 0
                || nand_read_loaded_page(&r[i], i + 1 < end ? &r[i + 1] : NULL,
                                         i != first, &result))
                goto fail;
            r[i].result = result;
            r[i].ecc = (result & (1u << 30)) ? NAND_ECC_FAILED
                     : (result & 0xff) ? NAND_ECC_CORRECTED
                     : NAND_ECC_CLEAN;
            discard_dcache_range(r[i].buf, nand_chip->pagesize);
        }
        FMCTRL0 &= ~FMCTRL0_AUTOXFER;
        FMCTRL0 &= ~FMCTRL0_CE_MASK;
    }
    return 0;

fail:
    FMCTRL0 &= ~FMCTRL0_AUTOXFER;
    nand_clear_status();
    return -1;
}

/*
 * Programming and erasing. The original firmware does these with programs
 * that the FMC's sequencer runs in hardware; this driver makes the register
 * writes those programs do for a single page or block from the ARM. Tested
 * on hardware: a page written, read back and erased again.
 */

static int nand_finish(int status)
{
    nand_clear_status();
    if (status < 0)
        return -1;
    return (status & NAND_STATUS_FAIL) ? NAND_OP_FAILED : 0;
}

int nand_erase_block(uint32_t bank, uint32_t block)
{
    if (!nand_ready || bank >= nand_banks || block >= nand_chip->blocks)
        return -1;

    FMCTRL1 = 0x0ff3f8e0;
    FMCSTAT = 0x0ff00ffe;
    nand_set_fmctrl0(bank);
    if (nand_send_cmd(NAND_CMD_ERASE))
        return nand_finish(-1);
    FMANUM = 2;                         /* row address only */
    FMADDR0 = block * nand_chip->pagesperblock;
    FMCTRL1 = FMCTRL1_DOTRANSADDR;
    if (nand_wait_stat(FMCSTAT_ADDRDONE))
        return nand_finish(-1);
    if (nand_send_cmd(NAND_CMD_ERASE2))
        return nand_finish(-1);
    return nand_finish(nand_read_status());
}

/* Load a page's first chunk into the controller and encode its spare
 * metadata. The rest of the page follows chunk by chunk as it goes out. */
static int nand_load_spare(const uint32_t *meta)
{
    unsigned long spins = NAND_POLL_SPINS;

    FMSYND5 = meta[0];
    FMSYND6 = meta[1];
    FMSYND7 = meta[2];
    FMUNK78 = 0x3210;
    FMUNK7C = 1;
    while (FMUNK7C & 1)
        if (!--spins)
            return -1;
    return nand_wait_stat(FMCSTAT_UNK20);
}

/* Start the ECC engine on a chunk */
static void nand_encode(uint32_t cmd)
{
    FMTRANSSTAT = 0x1ff;
    FMTRANS1 = 0x180;
    FMTRANS0 = cmd;
}

/* READ STATUS on a bank of the current run: the chip enable is switched
 * without touching the rest of FMCTRL0 */
static int nand_run_status(uint32_t bank)
{
    FMCTRL0 = (FMCTRL0 & ~FMCTRL0_CE_MASK) | FMCTRL0_CE(bank);
    return nand_read_status();
}

/* Program a run of pages, each with its 12 bytes of spare metadata, as the
 * firmware's sequencer programs do: one plane at a time, or with two_plane
 * two planes per command, as the chip's mode selects. The controller loads
 * each
 * 512-byte chunk from memory while the previous one goes out, a 4KiB page's
 * second 2KiB unit included, and loads a page's first chunk while the page
 * before it finishes, so the run never
 * waits for the host. A chip is not waited for until it is given its next
 * page: the others program in the meantime, and every chip used is checked
 * at the end. With two_plane the pages come in pairs on one bank, plane 0
 * then plane 1. On failure *failbank names the bank that reported it. */
int nand_write_pages(const struct nand_write *w, unsigned int n,
                     bool two_plane, uint32_t *failbank)
{
    /* Per chunk: the ECC engine's encode command, and which of the
     * controller's buffers goes out while the next chunk loads */
    static const uint32_t encode[4] = { 0x01102, 0x11202, 0x21102, 0x31202 };
    static const uint32_t bufsel[4] = { 0x102, 0x201, 0x102, 0x201 };
    uint32_t chunks = nand_chip->pagesize / NAND_CHUNK_SIZE;
    uint32_t used, bank, i, chunk;
    int status;

    if (!nand_ready || !n || (two_plane && (n & 1)))
        return -1;
    for (i = 0; i < n; i++)
    {
        if (w[i].bank >= nand_banks
            || w[i].page >= (uint32_t)nand_chip->blocks
                            * nand_chip->pagesperblock)
            return -1;
        /* The controller reads the data behind the cache's back */
        commit_dcache_range(w[i].buf, nand_chip->pagesize);
    }

    FMCTRL1 = 0x0ff3f8e0;
    FMCSTAT = 0x0ff00ffe;
    FMCTRL0 = (NAND_TUNK1 << 16) | (NAND_TWP << 12) | FMCTRL0_UNK1 | 1;
    FMCTRL0 = (FMCTRL0 & ~FMCTRL0_CE_MASK) | FMCTRL0_CE(w[0].bank);
    used = 1 << w[0].bank;

    /* Chunk 0 of the first page into the controller */
    FMDATAW0 = (uint32_t)(intptr_t)w[0].buf;
    FMCTRL0 |= FMCTRL0_AUTOXFER;
    FMDNUM = 0x1ff;
    FMADDR2 = 1;
    FMCTRL1 = 0x2e0;
    if (nand_load_spare(w[0].meta))
        goto fail;
    nand_encode(encode[0]);

    for (i = 0; i < n; i++)
    {
        bool plane1 = two_plane && (i & 1);
        bool last = i + 1 == n;

        if (nand_send_cmd(plane1 ? NAND_CMD_PROGRAM_PLANE1 : NAND_CMD_PROGRAM)
            || nand_send_addr_page(w[i].page)
            || nand_wait_reg(&FMTRANSSTAT, 1 << 0))
            goto fail;

        for (chunk = 0; chunk < chunks; chunk++)
        {
            bool pageend = chunk + 1 == chunks;

            if (pageend && last)
                FMCTRL0 &= ~FMCTRL0_AUTOXFER;

            /* 16 bytes of ECC and spare */
            FMDNUM = 0xf;
            FMADDR2 = 0x1000;
            FMADDR7 = (chunk % 4) << 4;
            FMCTRL1 = 0x34;
            if (pageend && !last)
                FMDATAW0 = (uint32_t)(intptr_t)w[i + 1].buf;
            else if (chunk % 4 == 3 && !pageend)  /* the next 2KiB unit */
                FMDATAW0 = (uint32_t)(intptr_t)w[i].buf
                         + (chunk + 1) * NAND_CHUNK_SIZE;
            if (nand_wait_stat(FMCSTAT_TRANSDONE))
                goto fail;

            /* 512 bytes of data, and the next chunk in behind them */
            if (pageend && last)
            {
                FMDATAW1 = 7;
                FMADDR2 = 0x200;
                FMDNUM = 0x1ff;
                FMCTRL1 = 0xe4;
                if (nand_wait_stat(FMCSTAT_TRANSDONE))
                    goto fail;
                break;
            }
            FMADDR2 = bufsel[chunk % 4];
            FMDNUM = 0x1ff;
            FMCTRL1 = 0x2e4;
            if (pageend)
            {
                if (nand_load_spare(w[i + 1].meta))
                    goto fail;
                nand_encode(encode[0]);
                if (nand_wait_stat(FMCSTAT_TRANSDONE))
                    goto fail;
                break;
            }
            if (nand_wait_stat(FMCSTAT_UNK20))
                goto fail;
            nand_encode(encode[(chunk + 1) % 4]);
            if (nand_wait_reg(&FMTRANSSTAT, 1 << 0)
                || nand_wait_stat(FMCSTAT_TRANSDONE))
                goto fail;
        }

        /* Plane 0 of a pair is queued; anything else starts programming */
        if (nand_send_cmd(two_plane && !plane1 ? NAND_CMD_PROGRAM_QUEUE
                                               : NAND_CMD_PROGRAM2))
            goto fail;
        if (last)
            break;

        /* On to the next page's chip, which must have finished whatever it
         * was given before */
        bank = w[i + 1].bank;
        FMCTRL0 = (FMCTRL0 & ~FMCTRL0_CE_MASK) | FMCTRL0_CE(bank);
        if (used & (1 << bank))
        {
            status = nand_read_status();
            if (status < 0)
                goto fail;
            if (status & NAND_STATUS_FAIL)
                goto failed;
        }
        used |= 1 << bank;
    }

    for (bank = 0; bank < NAND_MAX_BANKS; bank++)
    {
        if (!(used & (1 << bank)))
            continue;
        status = nand_run_status(bank);
        if (status < 0)
            goto fail;
        if (status & NAND_STATUS_FAIL)
            goto failed;
    }
    FMCTRL0 &= ~FMCTRL0_CE_MASK;
    nand_clear_status();
    return 0;

failed:
    if (failbank)
        *failbank = __builtin_ctz(FMCTRL0 & 0x1fe) - 1;
    FMCTRL0 &= ~FMCTRL0_AUTOXFER;
    nand_clear_status();
    return NAND_OP_FAILED;

fail:
    if (failbank)
        *failbank = __builtin_ctz(FMCTRL0 & 0x1fe) - 1;
    FMCTRL0 &= ~FMCTRL0_AUTOXFER;
    return nand_finish(-1);
}

int nand_write_page(uint32_t bank, uint32_t page, const void *databuf,
                    const uint32_t *meta)
{
    struct nand_write w = { bank, page, databuf, meta };

    return nand_write_pages(&w, 1, false, NULL);
}

uint32_t nand_get_id(void)
{
    return nand_id;
}

unsigned int nand_get_bank_count(void)
{
    return nand_banks;
}

#ifdef NAND_CHECK
/* Hooks for the contributor check image (nand-check-nano3g.c), which reads
 * the raw NAND itself and serves the storage API in its place */

int nand_get_chip_row(void)
{
    return nand_chip ? (int)(nand_chip - nand_chip_table) : -1;
}

const struct nand_geometry *nand_check_use_chip(unsigned int pagesperblock)
{
    /* Keep whatever was identified readable, mounted or not, but drop a
     * chip whose blocks do not hold the pages the check's layout expects */
    if (nand_chip && nand_chip->pagesperblock != pagesperblock)
        nand_chip = NULL;
    nand_ready = nand_chip != NULL;
    return nand_get_geometry();
}
#endif /* NAND_CHECK */

/* ---- Rockbox storage API ---- */

static int nand_init_chip(void)
{
    uint32_t bank, id;
    unsigned int i;
    int rc;

    nand_ready = false;
    nand_banks = 0;
    nand_chip = NULL;

    rc = nand_reset(0);
    if (rc)
    {
        logf("nand: reset failed (%d)", rc);
        return rc;
    }
    if (nand_identify(&id, NULL))
    {
        logf("nand: identify failed");
        return NAND_ERR_IDENTIFY;
    }
    nand_id = id;

    /* Further banks count if they hold the same chip, contiguously. Like
     * Apple's FIL, refuse a unit where another chip enable answers with a
     * different part, or where a chip follows a missing one: mounting it
     * with fewer banks would read the wrong blocks. */
    nand_banks = 1;
    for (bank = 1; bank < NAND_MAX_BANKS; bank++)
    {
        if (nand_reset(bank) || nand_identify(&id, NULL)
            || !id || id == 0xffffffff)
            continue;
        if (id != nand_id || bank != nand_banks)
        {
            logf("nand: bank %lu answers %08lx", (unsigned long)bank,
                 (unsigned long)id);
            return NAND_ERR_MIXED;
        }
        nand_banks++;
    }

    for (i = 0; i < ARRAYLEN(nand_chip_table); i++)
        if (nand_chip_table[i].id == nand_id
            && nand_chip_table[i].banks == nand_banks)
            nand_chip = &nand_chip_table[i];
    if (!nand_chip)
    {
        /* A normal build carries only the chips Rockbox has been tested on,
         * so a chip missing from the table is one nobody has validated */
        logf("nand: chip %08lx x %u is not supported", (unsigned long)nand_id,
             nand_banks);
        return NAND_ERR_UNSUPPORTED;
    }

    nand_ready = true;
    nand_touch();

#ifndef NAND_CHECK
    /* The table can still hold a row that has not been through a write test
     * (a NAND_TEST_READONLY build makes every row look that way): leave that
     * chip alone too. The check image reads any chip it identifies. */
    if (!nand_get_geometry()->validated)
    {
        logf("nand: chip %08lx x %u is not validated",
             (unsigned long)nand_id, nand_banks);
        nand_ready = false;
        return NAND_ERR_UNSUPPORTED;
    }
#endif

    rc = ftl_init();
    if (rc)
    {
        logf("nand: FTL mount failed (%d)", rc);
        nand_ready = false;
        return NAND_ERR_FTL + rc;
    }
    return 0;
}

int nand_init(void)
{
    int rc = nand_init_chip();

#ifdef NAND_CHECK
    /* The check image reports whatever happened and serves the raw NAND of
     * any chip it identified, mounted or not, so init always succeeds */
    nand_check_init(rc);
    rc = 0;
#endif
    return rc;
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
    int rc;

    if (!nand_ready)
        return -1;
    rc = ftl_sync();
    if (rc)
        panicf("Failed to unmount flash: %d", rc);
    return rc;
}
#endif

/* ---- the FTL's disk. A NAND_CHECK build serves the raw NAND instead, from
 * nand-check-nano3g.c, which defines these five entry points in place of
 * the ones below ---- */
#ifndef NAND_CHECK
#ifdef HAVE_STORAGE_READONLY
bool nand_readonly(IF_MD_NONVOID(int drive))
{
    IF_MD((void)drive);
    return ftl_readonly_mount();
}
#endif

int nand_read_sectors(IF_MD(int drive,) sector_t start, int incount,
                     void* inbuf)
{
    IF_MD((void)drive);
    int rc;

    if (!nand_ready)
        return -1;
    rc = ftl_read((uint32_t)start, incount, inbuf);
    if (!rc)
        nand_touch();
    return rc;
}

int nand_write_sectors(IF_MD(int drive,) sector_t start, int count,
                      const void* outbuf)
{
    IF_MD((void)drive);
    int rc;

    if (!nand_ready)
        return -1;
    rc = ftl_write((uint32_t)start, count, outbuf);
    if (!rc)
        nand_touch();
    return rc;
}

int nand_event(long id, intptr_t data)
{
    (void) id;
    (void) data;
    return 0;
}

#ifdef STORAGE_GET_INFO
void nand_get_info(IF_MD(int drive,) struct storage_info *info)
{
    IF_MD((void)drive);
    info->sector_size = SECTOR_SIZE;
    info->num_sectors = ftl_num_sectors();
    info->vendor = "Apple";
    info->product = "iPod Nano 3G";
    info->revision = "1.0";
}
#endif
#endif /* !NAND_CHECK */

long nand_last_disk_activity(void)
{
    return nand_last_activity_value;
}
