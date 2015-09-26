/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 by Cástor Muñoz
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
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "system.h"
#include "s5l8702.h"
#include "clocking-s5l8702.h"
#include "crypto-s5l8702.h"
#include "nor-target.h"
#include "piezo.h"

/* How it works:
 *
 * - dualboot-installer: installs or updates a RB bootloader, the bootloader
 *   to install/update is already included into dualboot-installer.dfu file,
 *   once it is executed by the iPod device:
 *
 *   1) locates an original NORBOOT (ONB): first it looks at offset=32KB, if
 *      a NORBOOT is found but it is not an ONB then it is supposed it is a
 *      RB bootloader (that should be updated), then the ONB is loaded from
 *      offset=32KB+old_BLSIZE).
 *   2) write ONB at 32KB+new_BLSIZE, if it fails then:
 *      2a) try to restore ONB to its 'pristine' place (offset=32KB), if it
 *          also fails then the NOR got corrupted (ONB probably destroyed)
 *          and iTunes should be used to restore the iPod.
 *   3) write new (included) RB bootloader at offset=32KB, it it fails then
 *      goto 2a)
 *   TODO: actual implementation is not exactly as described, needs small
 *         fixes.
 *
 * - dualboot-uninstaller: uninstall RB bootloader from NOR, leaving it at
 *   it's previous (pristine) state.
 *
 * See bootloader/ipod6g.c for notes on how the RB bootloader works.
 *
 *
 *               Pristine NOR                    Rockboxed NOR
 *         1MB  ______________
 *             |              |
 *             |     DIR      |
 *   1MB-0x200 |______________|
 *             |              |
 *             |    File 1    |
 *             |..............|
 *             |              |
 *             .              .
 *             .              .
 *             .              .
 *             |              |
 *             |..............|
 *             |              |                 .              .
 *             |    File N    |                 .              .
 *             |______________|                 |______________|
 *             |              |                 |              |
 *             |              |                 |              |
 *             |              |                 |     FREE     |
 *             |              |                 |              |
 *             |     FREE     |      160KB+BLSZ |______________|
 *             |              |                 |              |
 *             |              |                 |              |
 *             |              |                 |  Decrypted   |
 *       160KB |______________|                 |  OF NORBOOT  |
 *             |              |                 |              |
 *             |              |       32KB+BLSZ |______________|
 *             |  Encrypted   |                 |              |
 *             |  OF NORBOOT  |                 |  Decrypted   |
 *             |              |                 |   Rockbox    |
 *             |              |                 |  Bootloader  |
 *        32KB |______________|            32KB |______________|
 *             |              |                 |              |
 *             |              |                 .              .
 *             |              |                 .              .
 *             |______________|
 *             |              |
 *             | Device ident |
 *           0 |______________|
 *
 */

#define OF_LOADADDR     IRAM1_ORIG

/* tone sequences: period (uS), duration (ms), silence (ms) */
uint16_t alive[] = { 500,100,0, 0 };
uint16_t happy[] = { 1000,100,0, 500,150,0, 0 };
uint16_t sad[]   = { 3000,500,500, 0 };

/* iPod Classic: decrypted hashes for known OFs */
unsigned char of_sha[][SIGN_SZ] = {
    "\x66\x66\x76\xDC\x1D\x32\xB2\x46\xA6\xC9\x7D\x5A\x61\xD3\x49\x4C", /* v1.1.2 */
    "\x1E\xF0\xD9\xDE\xC2\x7E\xEC\x02\x7C\x15\x76\xBB\x5C\x4F\x2D\x95", /* v2.0.1 */
    "\x06\x85\xDF\x28\xE4\xD7\xF4\x82\xC0\x73\xB0\x53\x26\xFC\xB0\xFE"  /* v2.0.4 */
};
#define N_OF (int)(sizeof(of_sha)/SIGN_SZ)

/* we can assume that unknown FW is a RB bootloader */
#define FW_RB   N_OF

/* IM3 size aligned to NOR sector size */
#define GET_NOR_SZ(hinfo) \
    (ALIGN_UP(IM3HDR_SZ + get_uint32le((hinfo)->data_sz), 0x1000))

static uint32_t get_uint32le(unsigned char *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

int identify_fw(struct Im3Info *hinfo)
{
    unsigned char hash[SIGN_SZ];
    int of;

    /* decrypt hash to identify OF */
    memcpy(hash, hinfo->enc12.data_sign, SIGN_SZ);
    hwkeyaes(HWKEYAES_DECRYPT, HWKEYAES_UKEY, hash, SIGN_SZ);

    for (of = 0; of < N_OF; of++)
        if (memcmp(hash, of_sha[of], SIGN_SZ) == 0)
            break;

    return of;
}

#ifdef DUALBOOT_UNINSTALL
/* Uninstall RB bootloader */
void main(void)
{
    struct Im3Info *hinfo;
    void *fw_addr;
    uint16_t *status;
    uint32_t bl_nor_sz;

    usec_timer_init();

    piezo_seq(alive);

    hinfo = (struct Im3Info*)OF_LOADADDR;
    fw_addr = (void*)hinfo + IM3HDR_SZ;

    if (!read_im3(NORBOOT_OFF, hinfo, NULL)) {
        status = sad;
        goto bye; /* no FW found */
    }

    if (identify_fw(hinfo) != FW_RB) {
        status = happy;
        goto bye; /* RB bootloader not installed, nothing to do */
    }

    /* if found FW is RB bootloader, OF should start just behind it */
    bl_nor_sz = GET_NOR_SZ(hinfo);
    if (!read_im3(NORBOOT_OFF + bl_nor_sz, hinfo, fw_addr)
                            || (identify_fw(hinfo) == FW_RB)) {
        status = sad;
        goto bye; /* OF not found */
    }

    /* OF correctly loaded */
    #if 1
    crypt_fw(HWKEYAES_ENCRYPT, hinfo, fw_addr);
    #else
    /* use current decrypted imagen (faster load) */
    #endif

    /* restore OF to it's original place */
    if (!write_im3(NORBOOT_OFF, hinfo)) {
        /* fatal: corrupted NOR, use iTunes to restore */
        while (true)
            piezo_seq(sad);
    }

    /* erase freed NOR blocks */
    bootflash_init(SPI_PORT);
    bootflash_erase_blocks(SPI_PORT,
            (NORBOOT_OFF + GET_NOR_SZ(hinfo)) >> 12, bl_nor_sz >> 12);
    bootflash_close(SPI_PORT);

    status = happy;

bye:
    udelay(300000);
    piezo_seq(status);
    WDTCON = 0x100000; /* WDT reset */
    while (1);
}

#else
/* Install RB bootloader */
struct Im3Info bl_hinfo __attribute__((section(".im3info.data"))) =
{
    .ident = IM3_IDENT,
    .version = IM3_VERSION,
    .enc_type = 2,
};

void main(void)
{
    struct Im3Info *hinfo;
    void *fw_addr;
    uint16_t *status;

    usec_timer_init();

    piezo_seq(alive);

    hinfo = (struct Im3Info*)OF_LOADADDR;
    fw_addr = (void*)hinfo + IM3HDR_SZ;

    if (!read_im3(NORBOOT_OFF, hinfo, fw_addr)) {
        status = sad;
        goto bye; /* no FW found */
    }

    if (identify_fw(hinfo) == FW_RB) {
        /* FW found, but not OF, assume it is a RB bootloader,
           already decrypted OF should be located just behind */
        int nor_offset = NORBOOT_OFF + GET_NOR_SZ(hinfo);
        if (!read_im3(nor_offset, hinfo, fw_addr) ||
                                (identify_fw(hinfo) == FW_RB)) {
            status = sad;
            goto bye; /* OF not found, use iTunes to restore */
        }
    }

    /* safety check, verify we are not going to overwrite useful data */
    if (bootflash_fs_tail(SPI_PORT) <
                NORBOOT_OFF + GET_NOR_SZ(&bl_hinfo) + GET_NOR_SZ(hinfo)) {
        /* TODO?: probably this should not happen, realloc files in
           flash to get free space if so */
        piezo_seq(sad);
        status = sad;
        goto bye; /* no space if flash, try to use iTunes to restore */
    }

    /* write decrypted OF, if OF is corretly written then prepare RB
       bootloader to be written to NOR, else we will try to restore OF
       to its original place */
    if (write_im3(NORBOOT_OFF + GET_NOR_SZ(&bl_hinfo), hinfo)) {
        hinfo = &bl_hinfo;
        fw_addr = (void*)hinfo + IM3HDR_SZ;
        /* sign RB bootloader */
        mksign(HWKEYAES_UKEY, fw_addr,
                get_uint32le(hinfo->data_sz), hinfo->enc12.data_sign);
        mksign(HWKEYAES_UKEY, hinfo, IM3INFOSIGN_SZ, hinfo->info_sign);
    }

    #if 0
    crypt_fw(HWKEYAES_ENCRYPT, hinfo, fw_addr);
    #else
    /* use current decrypted imagen (faster load) */
    #endif

    /* write RB bootloader (or restore OF to is original place) */
    if (!write_im3(NORBOOT_OFF, hinfo)) {
        /* fatal: corrupted NOR, use iTunes to restore */
        while (true)
            piezo_seq(sad);
    }

    /* RB bootloader not succesfully intalled, but device
       was restored and should be working as before */
    if (hinfo != &bl_hinfo) {
        status = sad;
        goto bye;
    }

    /* enjoy RB! */
    status = happy;

bye:
    udelay(300000);
    piezo_seq(status);
    WDTCON = 0x100000; /* WDT reset */
    while (1);
}
#endif  /* DUALBOOT_UNINSTALL */
