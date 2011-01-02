/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
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
#ifndef __ATA_H__
#define __ATA_H__

#include <stdbool.h>
#include "config.h" /* for HAVE_MULTIVOLUME or not */
#include "mv.h" /* for IF_MV() and friends */

#ifndef ATA_OUT8
#define ATA_OUT8(reg, data) (reg) = (data)
#endif
#ifndef ATA_OUT16
#define ATA_OUT16(reg, data) (reg) = (data)
#endif
#ifndef ATA_IN8
#define ATA_IN8(reg) (reg)
#endif
#ifndef ATA_IN16
#define ATA_IN16(reg) (reg)
#endif
#ifndef ATA_SWAP_IDENTIFY
#define ATA_SWAP_IDENTIFY(word) (word)
#endif

#define STATUS_BSY      0x80
#define STATUS_RDY      0x40
#define STATUS_DRQ      0x08
#define STATUS_ERR      0x01
#define STATUS_DF       0x20
#define ERROR_IDNF      0x10
#define ERROR_ABRT      0x04

#define TEST_PATTERN1   0xa5
#define TEST_PATTERN2   0x5a
#define TEST_PATTERN3   0xaa
#define TEST_PATTERN4   0x55

#define ATA_FEATURE     ATA_ERROR

#define ATA_STATUS      ATA_COMMAND
#define ATA_ALT_STATUS  ATA_CONTROL

struct storage_info;

void ata_enable(bool on);
void ata_spindown(int seconds);
void ata_sleep(void);
void ata_sleepnow(void);
/* NOTE: DO NOT use this to poll for disk activity.
         If you are waiting for the disk to become active before
         doing something use ata_idle_notify.h
 */
bool ata_disk_is_active(void);
int ata_soft_reset(void);
int ata_init(void);
void ata_close(void);
int ata_read_sectors(IF_MD2(int drive,) unsigned long start, int count, void* buf);
int ata_write_sectors(IF_MD2(int drive,) unsigned long start, int count, const void* buf);
void ata_spin(void);
#if (CONFIG_LED == LED_REAL)
void ata_set_led_enabled(bool enabled);
#endif
unsigned short* ata_get_identify(void);

#ifdef STORAGE_GET_INFO
void ata_get_info(IF_MD2(int drive,) struct storage_info *info);
#endif
#ifdef HAVE_HOTSWAP
bool ata_removable(IF_MD_NONVOID(int drive));
bool ata_present(IF_MD_NONVOID(int drive));
#endif


#ifdef CONFIG_STORAGE_MULTI
int ata_num_drives(int first_drive);
#endif


long ata_last_disk_activity(void);
int ata_spinup_time(void); /* ticks */

#ifdef HAVE_ATA_DMA
/* Needed to allow updating while waiting for DMA to complete */
void ata_keep_active(void);
/* Returns current DMA mode */
int ata_get_dma_mode(void);
/* Set DMA mode for ATA interface */
void ata_dma_set_mode(unsigned char mode);
/* Sets up DMA transfer */
bool ata_dma_setup(void *addr, unsigned long bytes, bool write);
/* Waits for DMA transfer completion */
bool ata_dma_finish(void);
#endif /* HAVE_ATA_DMA */

#endif /* __ATA_H__ */
