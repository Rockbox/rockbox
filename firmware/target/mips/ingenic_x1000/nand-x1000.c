/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "nand-x1000.h"
#include "sfc-x1000.h"
#include "system.h"
#include "logf.h"
#include <string.h>

static const struct nand_chip chip_ato25d1ga = {
    .log2_ppb = 6, /* 64 pages */
    .page_size = 2048,
    .oob_size = 64,
    .nr_blocks = 1024,
    .bbm_pos = 2048,
    .clock_freq = 150000000,
    .dev_conf = jz_orf(SFC_DEV_CONF,
                       CE_DL(1), HOLD_DL(1), WP_DL(1),
                       CPHA(0), CPOL(0),
                       TSH(7), TSETUP(0), THOLD(0),
                       STA_TYPE_V(1BYTE), CMD_TYPE_V(8BITS),
                       SMP_DELAY(1)),
    .flags = NAND_CHIPFLAG_QUAD | NAND_CHIPFLAG_HAS_QE_BIT,
    .cmd_page_read = NANDCMD_PAGE_READ,
    .cmd_program_execute = NANDCMD_PROGRAM_EXECUTE,
    .cmd_block_erase = NANDCMD_BLOCK_ERASE,
    .cmd_read_cache = NANDCMD_READ_CACHE_x4,
    .cmd_program_load = NANDCMD_PROGRAM_LOAD_x4,
};


const struct nand_chip_id supported_nand_chips[] = {
    NAND_CHIP_ID(&chip_ato25d1ga, NAND_READID_ADDR, 0x9b, 0x12),
};

const size_t nr_supported_nand_chips = ARRAYLEN(supported_nand_chips);

static struct nand_drv static_nand_drv;
static uint8_t static_scratch_buf[NAND_DRV_SCRATCHSIZE] CACHEALIGN_ATTR;
static uint8_t static_page_buf[NAND_DRV_MAXPAGESIZE] CACHEALIGN_ATTR;

struct nand_drv* nand_init(void)
{
    static bool inited = false;
    if(!inited) {
        mutex_init(&static_nand_drv.mutex);
        static_nand_drv.scratch_buf = static_scratch_buf;
        static_nand_drv.page_buf = static_page_buf;
        static_nand_drv.refcount = 0;
    }

    return &static_nand_drv;
}

static uint8_t nand_get_reg(struct nand_drv* drv, uint8_t reg)
{
    sfc_exec(NANDCMD_GET_FEATURE, reg, drv->scratch_buf, 1|SFC_READ);
    return drv->scratch_buf[0];
}

static void nand_set_reg(struct nand_drv* drv, uint8_t reg, uint8_t val)
{
    drv->scratch_buf[0] = val;
    sfc_exec(NANDCMD_SET_FEATURE, reg, drv->scratch_buf, 1|SFC_WRITE);
}

static void nand_upd_reg(struct nand_drv* drv, uint8_t reg, uint8_t msk, uint8_t val)
{
    uint8_t x = nand_get_reg(drv, reg);
    x &= ~msk;
    x |= val;
    nand_set_reg(drv, reg, x);
}

static const struct nand_chip* identify_chip_method(uint8_t method,
                                                    const uint8_t* id_buf)
{
    for (size_t i = 0; i < nr_supported_nand_chips; ++i) {
        const struct nand_chip_id* chip_id = &supported_nand_chips[i];
        if (chip_id->method == method &&
            !memcmp(chip_id->id_bytes, id_buf, chip_id->num_id_bytes))
            return chip_id->chip;
    }

    return NULL;
}

static bool identify_chip(struct nand_drv* drv)
{
    /* Read ID command has some variations; Linux handles these 3:
     * - no address or dummy bytes
     * - 1 byte address, no dummy byte
     * - no address byte, 1 byte dummy
     *
     * Currently we use the 2nd method, aka. address read ID, the
     * other methods can be added when needed.
     */
    sfc_exec(NANDCMD_READID_ADDR, 0, drv->scratch_buf, 4|SFC_READ);
    drv->chip = identify_chip_method(NAND_READID_ADDR, drv->scratch_buf);
    if (drv->chip)
        return true;

    return false;
}

static void setup_chip_data(struct nand_drv* drv)
{
    drv->ppb = 1 << drv->chip->log2_ppb;
    drv->fpage_size = drv->chip->page_size + drv->chip->oob_size;
}

static void setup_chip_registers(struct nand_drv* drv)
{
    /* Set chip registers to enter normal operation */
    if(drv->chip->flags & NAND_CHIPFLAG_HAS_QE_BIT) {
        bool en = (drv->chip->flags & NAND_CHIPFLAG_QUAD) != 0;
        nand_upd_reg(drv, FREG_CFG, FREG_CFG_QUAD_ENABLE,
                     en ? FREG_CFG_QUAD_ENABLE : 0);
    }

    if(drv->chip->flags & NAND_CHIPFLAG_ON_DIE_ECC) {
        /* Enable on-die ECC */
        nand_upd_reg(drv, FREG_CFG, FREG_CFG_ECC_ENABLE, FREG_CFG_ECC_ENABLE);
    }

    /* Clear OTP bit to access the main data array */
    nand_upd_reg(drv, FREG_CFG, FREG_CFG_OTP_ENABLE, 0);

    /* Clear write protection bits */
    nand_set_reg(drv, FREG_PROT, FREG_PROT_UNLOCK);

    /* Call any chip-specific hooks */
    if(drv->chip->setup_chip)
        drv->chip->setup_chip(drv);
}

int nand_open(struct nand_drv* drv)
{
    if(drv->refcount > 0) {
        drv->refcount++;
        return NAND_SUCCESS;
    }

    /* Initialize the controller */
    sfc_open();
    sfc_set_dev_conf(jz_orf(SFC_DEV_CONF,
                            CE_DL(1), HOLD_DL(1), WP_DL(1),
                            CPHA(0), CPOL(0),
                            TSH(15), TSETUP(0), THOLD(0),
                            STA_TYPE_V(1BYTE), CMD_TYPE_V(8BITS),
                            SMP_DELAY(0)));
    sfc_set_clock(X1000_EXCLK_FREQ);

    /* Send the software reset command */
    sfc_exec(NANDCMD_RESET, 0, NULL, 0);
    mdelay(10);

    /* Chip identification and setup */
    if(!identify_chip(drv))
        return NAND_ERR_UNKNOWN_CHIP;

    setup_chip_data(drv);

    /* Set new SFC parameters */
    sfc_set_dev_conf(drv->chip->dev_conf);
    sfc_set_clock(drv->chip->clock_freq);

    /* Enter normal operating mode */
    setup_chip_registers(drv);

    drv->refcount++;
    return NAND_SUCCESS;
}

void nand_close(struct nand_drv* drv)
{
    --drv->refcount;
    if(drv->refcount > 0)
        return;

    /* Let's reset the chip... the idea is to restore the registers
     * to whatever they should "normally" be */
    sfc_exec(NANDCMD_RESET, 0, NULL, 0);
    mdelay(10);

    sfc_close();
}

void nand_enable_otp(struct nand_drv* drv, bool enable)
{
    nand_upd_reg(drv, FREG_CFG, FREG_CFG_OTP_ENABLE,
                 enable ? FREG_CFG_OTP_ENABLE : 0);
}

static uint8_t nand_wait_busy(struct nand_drv* drv)
{
    uint8_t reg;
    do {
        reg = nand_get_reg(drv, FREG_STATUS);
    } while(reg & FREG_STATUS_BUSY);
    return reg;
}

int nand_block_erase(struct nand_drv* drv, nand_block_t block)
{
    sfc_exec(NANDCMD_WR_EN, 0, NULL, 0);
    sfc_exec(drv->chip->cmd_block_erase, block, NULL, 0);

    uint8_t status = nand_wait_busy(drv);
    if(status & FREG_STATUS_EFAIL)
        return NAND_ERR_ERASE_FAIL;
    else
        return NAND_SUCCESS;
}

int nand_page_program(struct nand_drv* drv, nand_page_t page, const void* buffer)
{
    sfc_exec(NANDCMD_WR_EN, 0, NULL, 0);
    sfc_exec(drv->chip->cmd_program_load,
             0, (void*)buffer, drv->fpage_size|SFC_WRITE);
    sfc_exec(drv->chip->cmd_program_execute, page, NULL, 0);

    uint8_t status = nand_wait_busy(drv);
    if(status & FREG_STATUS_PFAIL)
        return NAND_ERR_PROGRAM_FAIL;
    else
        return NAND_SUCCESS;
}

int nand_page_read(struct nand_drv* drv, nand_page_t page, void* buffer)
{
    sfc_exec(drv->chip->cmd_page_read, page, NULL, 0);
    nand_wait_busy(drv);
    sfc_exec(drv->chip->cmd_read_cache, 0, buffer, drv->fpage_size|SFC_READ);

    if(drv->chip->flags & NAND_CHIPFLAG_ON_DIE_ECC) {
        uint8_t status = nand_get_reg(drv, FREG_STATUS);

        if(status & FREG_STATUS_ECC_UNCOR_ERR) {
            logf("ecc uncorrectable error on page %08lx", (unsigned long)page);
            return NAND_ERR_ECC_FAIL;
        }

        if(status & FREG_STATUS_ECC_HAS_FLIPS) {
            logf("ecc corrected bitflips on page %08lx", (unsigned long)page);
        }
    }

    return NAND_SUCCESS;
}

int nand_read_bytes(struct nand_drv* drv, uint32_t byte_addr, uint32_t byte_len, void* buffer)
{
    if(byte_len == 0)
        return NAND_SUCCESS;

    int rc;
    unsigned pg_size = drv->chip->page_size;
    nand_page_t page = byte_addr / pg_size;
    unsigned offset = byte_addr % pg_size;
    while(1) {
        rc = nand_page_read(drv, page, drv->page_buf);
        if(rc < 0)
            return rc;

        memcpy(buffer, &drv->page_buf[offset], MIN(pg_size - offset, byte_len));

        if(byte_len <= pg_size - offset)
            break;

        byte_len -= pg_size - offset;
        buffer += pg_size - offset;
        offset = 0;
        page++;
    }

    return NAND_SUCCESS;
}

int nand_write_bytes(struct nand_drv* drv, uint32_t byte_addr, uint32_t byte_len, const void* buffer)
{
    if(byte_len == 0)
        return NAND_SUCCESS;

    int rc;
    unsigned pg_size = drv->chip->page_size;
    unsigned blk_size = pg_size << drv->chip->log2_ppb;

    if(byte_addr % blk_size != 0)
        return NAND_ERR_UNALIGNED;
    if(byte_len % blk_size != 0)
        return NAND_ERR_UNALIGNED;

    nand_page_t page = byte_addr / pg_size;
    nand_page_t end_page = page + (byte_len / pg_size);

    for(nand_block_t blk = page; blk < end_page; blk += drv->ppb) {
        rc = nand_block_erase(drv, blk);
        if(rc < 0)
            return rc;
    }

    for(; page != end_page; ++page) {
        memcpy(drv->page_buf, buffer, pg_size);
        memset(&drv->page_buf[pg_size], 0xff, drv->chip->oob_size);
        buffer += pg_size;

        rc = nand_page_program(drv, page, drv->page_buf);
        if(rc < 0)
            return rc;
    }

    return NAND_SUCCESS;
}

/* TODO - NAND driver future improvements
 *
 * 1. Support sofware or on-die ECC transparently. Support debug ECC bypass.
 *
 * It's probably best to add an API call to turn ECC on or off. Software
 * ECC and most or all on-die ECC implementations require some OOB bytes
 * to function; which leads us to the next problem...
 *
 * 2. Allow safe access to OOB areas
 *
 * The OOB data area is not fully available to users; it is also occupied
 * by ECC data and bad block markings. The NAND driver needs to provide a
 * mapping which allows OOB data users to map around those reserved areas,
 * otherwise it's not really possible to use OOB data.
 *
 * 3. Support partial page programming.
 *
 * This might already work. My understanding of NAND flash is that bits are
 * represented by charge deposited on flash cells. In the case of SLC flash,
 * cells are one bit. For MLC flash, cells can store more than one bit; but
 * MLC flash is much less reliable than SLC. We probably don't have to be
 * concerned about MLC flash, and its does not support partial programming
 * anyway due to the cell characteristics, so I will only consider SLC here.
 *
 * For SLC there are two cell states -- an uncharged cell represents a "1"
 * and a charged cell represents "0". Programming can only deposit charge
 * on a cell and erasing can only remove charge. Therefore, "programming" a
 * cell to 1 is actually a no-op.
 *
 * So, there's no datasheet which spells this out, but I suspect you just
 * set the areas you're not interested in programming to 0xff. Programming
 * can never change a written 0 back to a 1, so programming a 1 bit works
 * more like a "don't care" (= keep whatever value is already there).
 *
 * What _is_ given by the datasheets is limits on how many times you can
 * reprogram the same page without erasing it. This is an overall limit
 * called NOP (number of programs) in many datasheets. In addition to this,
 * sub-regions of the page have further limits: it's common for a 2048+64
 * byte page to be split into 8 regions, with four 512-byte main areas and
 * four 16-byte OOB areas. Usually, each subregion can only be programmed
 * once. However, you can write multiple subregions with a single program.
 *
 * Violating programming constraints could cause data loss, so we need to
 * communicate to upper layers what the limitations are here if they want
 * to use partial programming safely.
 *
 * Programming the same page more than once increases the overall stress
 * on the flash cells and can cause bitflips. For this reason, it's best
 * to keep the number of programs as low as possible. Some sources suggest
 * that programming the pages in a block in linear order is also better to
 * reduce stress, although I don't know why this would be.
 *
 * These program/read stresses can flip bits, but it's only due to residual
 * charge building up on uncharged cells; cells are not permanently damaged
 * by these kind of stresses. Erasing the block will remove the charge and
 * restore all the cells to a clean state.
 *
 * These slides are fairly informative on this subject:
 * - https://cushychicken.github.io/assets/cooke_inconvenient_truths.pdf
 *
 * 4. Bad block management
 *
 * This probably doesn't belong in the NAND layer but it seems wise to keep
 * at least a bad block table at the level of the NAND driver. Factory bad
 * block marks are usually some non-0xFF byte in the OOB area, but bad blocks
 * which develop over the device lifetime usually won't be marked; after all
 * they are unreliable, so we can't program a marking on them and expect it
 * to stick. So, most FTL systems keep a bad block table somewhere in flash
 * and update it whenever a block goes bad.
 *
 * So, in addition to a bad block marker scan, we should try to gather bad
 * block information from such tables.
 */
