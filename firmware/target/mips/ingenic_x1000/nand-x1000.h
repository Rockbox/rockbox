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

#ifndef __NAND_X1000_H__
#define __NAND_X1000_H__

/* NOTE: this is a very minimal API designed only to support a bootloader.
 * Not suitable for general data storage. It doesn't have proper support for
 * partial page writes, access to spare area, etc, which are all necessary
 * for an effective flash translation layer.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nand-x1000-err.h"

/* Chip supports quad I/O for page read/write */
#define NANDCHIP_FLAG_QUAD      0x01

/* Set/clear the BRWD bit when enabling/disabling write protection */
#define NANDCHIP_FLAG_USE_BRWD  0x02

typedef struct nand_drv nand_drv;

/* Defines some static information about a NAND chip */
typedef struct nand_chip_data {
    const char* name;       /* Name for debugging purposes */
    uint8_t  mf_id;         /* Manufacturer ID */
    uint8_t  dev_id;        /* Device ID */
    uint8_t  rowaddr_width; /* Number of bytes in row addresses */
    uint8_t  coladdr_width; /* Number of bytes in column addresses */
    uint32_t dev_conf;      /* Value to write to SFC_DEV_CONF register */
    uint32_t clock_freq;    /* Frequency to switch to after identification */
    uint32_t block_size;    /* Number of pages per eraseblock */
    uint32_t page_size;     /* Number of data bytes per page */
    uint32_t spare_size;    /* Number of spare bytes per page */
    int      flags;         /* Various flags */
} nand_chip_data;

/* Defines high-level operations used to implement the public API.
 * Chips may need to override operations if the default ones aren't suitable.
 *
 * Negative return codes return an error, while zero or positive codes are
 * considered successful. This allows a function to return meaningful data,
 * if applicable.
 */
typedef struct nand_chip_ops {
    /* Called once after identifying the chip */
    int(*open)(nand_drv* d);

    /* Called once when the driver is closed */
    void(*close)(nand_drv* d);

    /* Read or write a complete page including both main and spare areas. */
    int(*read_page)(nand_drv* d, uint32_t rowaddr, unsigned char* buf);
    int(*write_page)(nand_drv* d, uint32_t rowaddr, const unsigned char* buf);

    /* Erase a block. */
    int(*erase_block)(nand_drv* d, uint32_t blockaddr);

    /* Enable or disable the chip's write protection. */
    int(*set_wp_enable)(nand_drv* d, bool en);

    /* Perform error correction and detection on a raw page (main + spare).
     * Return the number of errors detected and corrected, or a negative value
     * if errors were detected but could not be corrected.
     */
    int(*ecc_read)(nand_drv* d, unsigned char* buf);

    /* Generate ECC data for a page. The buffer main area is already filled
     * and this function should write ECC data into the spare area.
     */
    void(*ecc_write)(nand_drv* d, unsigned char* buf);
} nand_chip_ops;

/* Struct used to list all supported NAND chips in an array */
typedef struct nand_chip_desc {
    const nand_chip_data* data;
    const nand_chip_ops* ops;
} nand_chip_desc;

/* NAND driver structure. It can be accessed by chip ops, but they must not
 * modify any fields except for "auxbuf", which is a small buffer that can
 * be used for commands that need to read/write small amounts of data: often
 * needed for polling status, etc.
 */
struct nand_drv {
    const nand_chip_ops* chip_ops;
    const nand_chip_data* chip_data;
    unsigned char* pagebuf;
    unsigned char* auxbuf;
    uint32_t raw_page_size;
    int flags;
};

/* Note: sfc_init() must be called prior to nand_open() */
extern int nand_open(void);
extern void nand_close(void);

/* Controls device-side write protection registers as well as software lock.
 * Erase and program operations will fail unless you first enable writes.
 */
extern int nand_enable_writes(bool en);

/* Byte-based NAND operations */
extern int nand_read_bytes(uint32_t byteaddr, int count, void* buf);
extern int nand_write_bytes(uint32_t byteaddr, int count, const void* buf);
extern int nand_erase_bytes(uint32_t byteaddr, int count);

/* NAND command numbers */
#define NAND_CMD_READ_ID            0x9f
#define NAND_CMD_WRITE_ENABLE       0x06
#define NAND_CMD_GET_FEATURE        0x0f
#define NAND_CMD_SET_FEATURE        0x1f
#define NAND_CMD_PAGE_READ_TO_CACHE 0x13
#define NAND_CMD_READ_FROM_CACHE    0x0b
#define NAND_CMD_READ_FROM_CACHEx4  0x6b
#define NAND_CMD_PROGRAM_LOAD       0x02
#define NAND_CMD_PROGRAM_LOADx4     0x32
#define NAND_CMD_PROGRAM_EXECUTE    0x10
#define NAND_CMD_BLOCK_ERASE        0xd8

/* NAND device register addresses for GET_FEATURE / SET_FEATURE */
#define NAND_FREG_PROTECTION        0xa0
#define NAND_FREG_FEATURE           0xb0
#define NAND_FREG_STATUS            0xc0

/* Protection register bits */
#define NAND_FREG_PROTECTION_BRWD   0x80
#define NAND_FREG_PROTECTION_BP2    0x20
#define NAND_FREG_PROTECTION_BP1    0x10
#define NAND_FREG_PROTECTION_BP0    0x80
/* Mask of BP bits 0-2 */
#define NAND_FREG_PROTECTION_ALLBP  (0x38)

/* Feature register bits */
#define NAND_FREG_FEATURE_QE        0x01

/* Status register bits */
#define NAND_FREG_STATUS_OIP        0x01
#define NAND_FREG_STATUS_WEL        0x02
#define NAND_FREG_STATUS_E_FAIL     0x04
#define NAND_FREG_STATUS_P_FAIL     0x08

/* Standard implementations for low-level NAND commands. I'm not aware of any
 * actual standard governing these, but it seems many vendors follow the same
 * command numbering, status bits, and behavior so these implementations should
 * work across a wide variety of chips.
 *
 * If adding a new NAND chip which only has a minor deviation from these
 * standard implementations, consider adding a flag and modifying these
 * functions to change their behavior based on the flag, instead of writing
 * a whole new implementation.
 *
 * None of these functions are directly called by the high-level driver code,
 * except for nandcmd_std_read_id(). They can be used to implement higher-level
 * functions in a device's "nand_chip_ops".
 */
extern int nandcmd_read_id(nand_drv* d);
extern int nandcmd_write_enable(nand_drv* d);
extern int nandcmd_get_feature(nand_drv* d, int reg);
extern int nandcmd_set_feature(nand_drv* d, int reg, int val);
extern int nandcmd_page_read_to_cache(nand_drv* d, uint32_t rowaddr);
extern int nandcmd_read_from_cache(nand_drv* d, unsigned char* buf);
extern int nandcmd_program_load(nand_drv* d, const unsigned char* buf);
extern int nandcmd_program_execute(nand_drv* d, uint32_t rowaddr);
extern int nandcmd_block_erase(nand_drv* d, uint32_t blockaddr);

/* Table filled with all the standard operations for chips which don't
 * need to override any operations.
 */
extern const nand_chip_ops nand_chip_ops_std;

/* Standard NAND chip ops based on the standard "nandcmd" functions.
 *
 * Same advice applies here as there: if it's possible to support minor
 * chip variations with a flag, modify these functions to do so.
 */
extern int nandop_std_open(nand_drv* d);
extern void nandop_std_close(nand_drv* d);
extern int nandop_std_read_page(nand_drv* d, uint32_t rowaddr, unsigned char* buf);
extern int nandop_std_write_page(nand_drv* d, uint32_t rowaddr, const unsigned char* buf);
extern int nandop_std_erase_block(nand_drv* d, uint32_t blockaddr);
extern int nandop_std_set_wp_enable(nand_drv* d, bool en);

/* The default ECC implementation is a no-op: reads always succeed without
 * reporting errors and writes will fill the spare area with '0xff' bytes.
 *
 * For chips that support internal ECC, this often works because the chip will
 * ignore writes to the ECC areas.
 */
extern int nandop_ecc_none_read(nand_drv* d, unsigned char* buf);
extern void nandop_ecc_none_write(nand_drv* d, unsigned char* buf);

#endif /* __NAND_X1000_H__ */
