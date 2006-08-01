/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "ata.h"
#include <stdbool.h>
#include <string.h>

#define SECTOR_SIZE     (512)

static unsigned short identify_info[SECTOR_SIZE];
int ata_spinup_time = 0;
long last_disk_activity = -1;

void flash_select_chip(int no, int sel)
{

}

unsigned char flash_read_data(void)
{

}

void flash_write_data(unsigned char data)
{

}

void flash_write_cmd(unsigned char cmd)
{

}

void flash_write_addr(unsigned char addr)
{

}

void flash_wait_ready(void)
{

}

int flash_map_sector(int sector, int* chip, int* chip_sector)
{

}

int flash_read_id(int no)
{

}

int flash_read_sector(int sector, unsigned char* buf,
                      unsigned char* oob)
{

}

int flash_read_sector_oob(int sector, unsigned char* oob)
{

}

static int flash_get_n_segments(void)
{

}

static int flash_get_n_phblocks(void)
{

}

static int flash_get_n_sectors_in_block(void)
{

}

static int flash_phblock_to_sector(int segment, int block)
{

}

static int flash_is_bad_block(unsigned char* oob)
{

}

int flash_disk_scan(void)
{

}

int flash_disk_find_block(int block)
{

}

int flash_disk_read_sectors(unsigned long start,
                            int count,
                            void* buf)
{

}

int ata_read_sectors(IF_MV2(int drive,)
                     unsigned long start,
                     int incount,
                     void* inbuf)
{

}

int ata_write_sectors(IF_MV2(int drive,)
                      unsigned long start,
                      int count,
                      const void* buf)
{
    (void)start;
    (void)count;
    (void)buf;
    return -1;
}

/* schedule a single sector write, executed with the the next spinup 
   (volume 0 only, used for config sector) */
extern void ata_delayed_write(unsigned long sector, const void* buf)
{
    (void)sector;
    (void)buf;
}

/* write the delayed sector to volume 0 */
extern void ata_flush(void)
{

}

void ata_spindown(int seconds)
{
    (void)seconds;
}

bool ata_disk_is_active(void)
{
  return 0;
}

void ata_sleep(void)
{
}

void ata_spin(void)
{
}

/* Hardware reset protocol as specified in chapter 9.1, ATA spec draft v5 */
int ata_hard_reset(void)
{
  return 0;
}

int ata_soft_reset(void)
{
  return 0;
}

void ata_enable(bool on)
{
    (void)on;
}

unsigned short* ata_get_identify(void)
{

}

int ata_init(void)
{
    return 0;
}
