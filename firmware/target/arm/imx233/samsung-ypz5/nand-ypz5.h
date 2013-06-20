/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Lorenzo Miori
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

/* COMMANDS & ADDRESSES
 * <First> refers to "1st Cycle" -> CLE high
 * <Second><Address> refers to "2nd Cycle" -> ALE high
 * Where address is omitted is 0x00
 */

#define NAND_READ                               (uint8_t) 0x00
#define NAND_READ_ADDRESS                       (uint8_t) 0x30
#define NAND_READ_COPYBACK                      (uint8_t) 0x00
#define NAND_READ_COPYBACK_ADDRESS              (uint8_t) 0x35 /* read for copy back */ 
#define NAND_READ_ID                            (uint8_t) 0x90
#define NAND_READ_ID_ADDRESS                    (uint8_t) 0x00
#define NAND_PAGE_PROGRAM                       (uint8_t) 0x80
#define NAND_PAGE_PROGRAM_ADDRESS               (uint8_t) 0x10
#define NAND_TWO_PLANE_PROGRAM                  (uint8_t) 0x80 /* aka CACHE page program */
#define NAND_TWO_PLANE_PROGRAM_ADDRESS          (uint8_t) 0x15
#define NAND_COPY_BACK_PROGRAM                  (uint8_t) 0x85
#define NAND_COPY_BACK_PROGRAM_ADDRESS          (uint8_t) 0x10
#define NAND_TWO_PLANE_COPY_BACK_PROGRAM        (uint8_t) 0x85
#define NAND_TWO_PLANE_COPY_BACK_PROGRAM_ADDRESS        (uint8_t) 0x10
#define NAND_BLOCK_ERASE                        (uint8_t) 0x60
#define NAND_BLOCK_ERASE_ADDRESS                (uint8_t) 0xD0
#define NAND_RANDOM_INPUT                       (uint8_t) 0x85
#define NAND_RANDOM_OUTPUT                      (uint8_t) 0x05
#define NAND_RANDOM_OUTPUT_ADDRESS              (uint8_t) 0xE0
#define NAND_READ_STATUS                        (uint8_t) 0x70 /* can be issued during BUSY state */
#define NAND_READ_EDC_STATUS                    (uint8_t) 0x7B /* can be issued during BUSY state. Only for Copy Back op. */
#define NAND_CHIP1_STATUS                       (uint8_t) 0xF1 /* can be issued during BUSY state */
#define NAND_CHIP2_STATUS                       (uint8_t) 0xF2 /* can be issued during BUSY state */
#define NAND_RESET                              (uint8_t) 0xFF /* can be issued during BUSY state */

/* Useful CMD / ADDR constants */

#define NAND_READ_GROUP1                        (uint8_t) 0x60
#define NAND_READ_GROUP1_CLE2                   (uint8_t) 0x60
#define NAND_READ_GROUP1_CLE3                   (uint8_t) 0x30

#define NAND_READ_GROUP2                        (uint8_t) 0x60
#define NAND_READ_GROUP2_CLE2                   (uint8_t) 0x60
#define NAND_READ_GROUP2_CLE3                   (uint8_t) 0x35 /* read for copy back */ 

#define NAND_PAGE_PROGRAM_GROUP                 (uint8_t) 0x80
#define NAND_PAGE_PROGRAM_GROUP_CLE2            (uint8_t) 0x11
#define NAND_PAGE_PROGRAM_GROUP_CLE3            (uint8_t) 0x81
#define NAND_PAGE_PROGRAM_GROUP_CLE4            (uint8_t) 0x10

#define NAND_COPY_BACK_GROUP                    (uint8_t) 0x85
#define NAND_COPY_BACK_GROUP_CLE2               (uint8_t) 0x11
#define NAND_COPY_BACK_GROUP_CLE3               (uint8_t) 0x81
#define NAND_COPY_BACK_GROUP_CLE4               (uint8_t) 0x10

#define NAND_AREA_A                             (uint8_t) 0x00
#define NAND_AREA_B                             (uint8_t) 0x01
#define NAND_AREA_C                             (uint8_t) 0x50

/* Flash chip information */

#define CHIP_ID_SIZE                            5
#define SECTOR_SIZE                             512


static const uint8_t samsung[4][CHIP_ID_SIZE] =
{
    {0xEC, 0xD3, 0x51, 0x95, 0x58}, /* K9K8G08UOM */
    {0xEC, 0xD3, 0x55, 0x25, 0x58}, /* K9L8G08U0M */
    {0xEC, 0xD3, 0x14, 0x25, 0x64}, /* K9G8G08B0M - K9G8G08U0M */
    {0xEC, 0xD5, 0x55, 0x25, 0x68},
};

/**
 * BCB - Boot Control Block
 * STMP3xxx chips are using this block to start execution on NAND
 * It is located in block 0 of the NAND
 */

#define BCB_SIGNATURE1    0x504d5453 /* STMP */
#define BCB_SIGNATURE2    0x32424342 /* BCB */
#define BCB_SIGNATURE3    0x41434143 /* CACA */

#define BCB_VERSION_MAJOR   0x0001
#define BCB_VERSION_MINOR   0x0000
#define BCB_VERSION_SUB     0x0000

/* From Sigmatel Datasheet
 *   Important that these structs have to be aligned otherwise
 *   "[...]the code is larger to handle these offsets[...]"
 */
typedef struct _bcb_struct {
        uint32_t m_u32Signature1;
        struct {
                uint16_t m_u16Major;
                uint16_t m_u16Minor;
                uint16_t m_u16Sub;
        } __attribute__ ((packed)) BCBVersion;
        uint32_t m_u32NANDBitmap;    /* bit 0 == NAND 0, bit 1 == NAND 1, bit 2 = NAND 2, bit 3 = NAND3 */
        uint32_t m_u32Signature2;
        uint32_t m_u32Firmware_startingNAND;
        uint32_t m_u32Firmware_startingSector;
        uint32_t m_uSectorsInFirmware;
        uint32_t m_uFirmwareBootTag;
        struct {
                uint16_t m_u16Major;
                uint16_t m_u16Minor;
                uint16_t m_u16Sub;
        } __attribute__ ((packed)) FirmwareVersion;
        uint32_t Rsvd[10];
        uint32_t m_u32Signature3;
} __attribute__ ((packed)) BCB_t;



//void nand_read_id(int bank, nand_chip_id_t *id, _nand_chip_info_t *info);
//void parse_nand_id(nand_chip_id_t *id, _nand_chip_info_t *info);

bool dbg_hw_info_nand(void);