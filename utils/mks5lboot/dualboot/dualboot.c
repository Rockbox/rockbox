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
#include "button.h"

#include "s5l8702.h"
#include "clocking-s5l8702.h"
#include "spi-s5l8702.h"
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
 *             |   flsh DIR   |
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
 *             |              |                 |    Unused    |
 *             |              |                 |              |
 *             |    Unused    |      160KB+BLSZ |______________|
 *             |              |                 |              |
 *             |              |                 |   Original   |
 *             |              |                 |   NOR boot   |
 *       160KB |______________|                 |  (decrypted) |
 *             |              |                 |              |
 *             |              |       32KB+BLSZ |______________|
 *             |   Original   |                 |              |
 *             |   NOR boot   |                 |  Decrypted   |
 *             | (encrypted)  |                 |   Rockbox    |
 *             |              |                 |  Bootloader  |
 *        32KB |______________|            32KB |______________|
 *             |              |                 |              |
 *             |              |                 .              .
 *             |              |                 .              .
 *             |______________|
 *             |              |
 *             |    SysCfg    |
 *           0 |______________|
 *
 */

#define OF_LOADADDR     IRAM1_ORIG

/* tone sequences: period (uS), duration (ms), silence (ms) */
static uint16_t alive[] = { 500,100,0, 0 };
static uint16_t happy[] = { 1000,100,0, 500,150,0, 0 };
static uint16_t fatal[] = { 3000,500,500, 3000,500,500, 3000,500,0, 0 };
#define sad2 (&fatal[3])
#define sad  (&fatal[6])

/* iPod Classic: decrypted hashes for known OFs */
static unsigned char of_sha[][SIGN_SZ] = {
    "\x66\x66\x76\xDC\x1D\x32\xB2\x46\xA6\xC9\x7D\x5A\x61\xD3\x49\x4C", /* v1.1.2 */
    "\x1E\xF0\xD9\xDE\xC2\x7E\xEC\x02\x7C\x15\x76\xBB\x5C\x4F\x2D\x95", /* v2.0.1 */
    "\x06\x85\xDF\x28\xE4\xD7\xF4\x82\xC0\x73\xB0\x53\x26\xFC\xB0\xFE", /* v2.0.4 */
    "\x60\x80\x7D\x33\xA8\xDE\xF8\x49\xBB\xBE\x01\x45\xFF\x62\x40\x19"  /* v2.0.5 */
};
#define N_OF (int)(sizeof(of_sha)/SIGN_SZ)

/* we can assume that unknown FW is a RB bootloader */
#define FW_RB   N_OF

static int identify_fw(struct Im3Info *hinfo)
{
    unsigned char hash[SIGN_SZ];
    int of;

    /* decrypt hash to identify OF */
    memcpy(hash, hinfo->u.enc12.data_sign, SIGN_SZ);
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
    unsigned bl_nor_sz;

    usec_timer_init();
    piezo_seq(alive);
    spi_clkdiv(SPI_PORT, 4); /* SPI clock = 27/5 MHz. */

    hinfo = (struct Im3Info*)OF_LOADADDR;
    fw_addr = (void*)hinfo + IM3HDR_SZ;

    if (im3_read(NORBOOT_OFF, hinfo, NULL) != 0) {
        status = sad;
        goto bye; /* no FW found */
    }

    if (identify_fw(hinfo) != FW_RB) {
        status = happy;
        goto bye; /* RB bootloader not installed, nothing to do */
    }

    /* if found FW is a RB bootloader, OF should start just behind it */
    bl_nor_sz = im3_nor_sz(hinfo);
    if ((im3_read(NORBOOT_OFF + bl_nor_sz, hinfo, fw_addr) != 0)
                            || (identify_fw(hinfo) == FW_RB)) {
        status = sad;
        goto bye; /* OF not found */
    }

    /* decrypted OF correctly loaded, encrypt it before restoration */
    im3_crypt(HWKEYAES_ENCRYPT, hinfo, fw_addr);

    /* restore OF to it's original place */
    if (!im3_write(NORBOOT_OFF, hinfo)) {
        status = fatal;
        goto bye; /* corrupted NOR, use iTunes to restore */
    }

    /* erase freed NOR blocks */
    bootflash_init(SPI_PORT);
    bootflash_erase_blocks(SPI_PORT,
            (NORBOOT_OFF + im3_nor_sz(hinfo)) >> 12, bl_nor_sz >> 12);
    bootflash_close(SPI_PORT);

    status = happy;

bye:
    /* minimum time between the initial and the final beeps */
    while (USEC_TIMER < 2000000);
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

static uint32_t get_uint32le(unsigned char *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

void main(void)
{
    uint16_t *status = happy;
    int single_boot;
    struct Im3Info *hinfo;
    void *fw_addr;
    unsigned bl_nor_sz;

    usec_timer_init();
    piezo_seq(alive);
    spi_clkdiv(SPI_PORT, 4); /* SPI clock = 27/5 MHz. */

    /* check for single boot installation, is is configured when
       mks5lboot.exe builds the .dfu image */
    single_boot = bl_hinfo.info_sign[0];

    /* sign RB bootloader (data and header), but don't encrypt it,
       use current decrypted image for faster load */
    im3_sign(HWKEYAES_UKEY, (void*)&bl_hinfo + IM3HDR_SZ,
                get_uint32le(bl_hinfo.data_sz), bl_hinfo.u.enc12.data_sign);
    im3_sign(HWKEYAES_UKEY, &bl_hinfo, IM3INFOSIGN_SZ, bl_hinfo.info_sign);

    if (single_boot) {
        if (!im3_write(NORBOOT_OFF, &bl_hinfo))
            status = sad;
        goto bye;
    }

    hinfo = (struct Im3Info*)OF_LOADADDR;
    fw_addr = (void*)hinfo + IM3HDR_SZ;

    if (im3_read(NORBOOT_OFF, hinfo, fw_addr) != 0) {
        status = sad;
        goto bye; /* no FW found */
    }

    if (identify_fw(hinfo) == FW_RB) {
        /* FW found, but not OF, assume it is a RB bootloader,
           already decrypted OF should be located just behind */
        int nor_offset = NORBOOT_OFF + im3_nor_sz(hinfo);
        if ((im3_read(nor_offset, hinfo, fw_addr) != 0)
                                || (identify_fw(hinfo) == FW_RB)) {
            status = sad;
            goto bye; /* OF not found, use iTunes to restore */
        }
    }

    bl_nor_sz = im3_nor_sz(&bl_hinfo);
    /* safety check - verify we are not going to overwrite useful data */
    if (flsh_get_unused() < bl_nor_sz) {
        status = sad2;
        goto bye; /* no space if flash, use iTunes to restore */
    }

    /* write decrypted OF and RB bootloader, if any of these fails we
       will try to retore OF to its original place */
    if (!im3_write(NORBOOT_OFF + bl_nor_sz, hinfo)
                                || !im3_write(NORBOOT_OFF, &bl_hinfo)) {
        im3_crypt(HWKEYAES_ENCRYPT, hinfo, fw_addr);
        if (!im3_write(NORBOOT_OFF, hinfo)) {
            /* corrupted NOR, use iTunes to restore */
            status = fatal;
        }
        else {
            /* RB bootloader not succesfully intalled, but device
               was restored and should be working as before */
            status = sad;
        }
    }

bye:
    /* minimum time between the initial and the final beeps */
    while (USEC_TIMER < 2000000);
    piezo_seq(status);
    WDTCON = 0x100000; /* WDT reset */
    while (1);
}
#endif  /* DUALBOOT_UNINSTALL */
