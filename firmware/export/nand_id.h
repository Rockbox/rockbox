/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
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
#ifndef __NAND_ID_H__
#define __NAND_ID_H__

struct nand_info
{
    unsigned char     dev_id;
    unsigned char     dev_id2;
    unsigned short    pages_per_block;
    unsigned short    blocks_per_bank;
    unsigned short    page_size;
    unsigned short    spare_size;
    unsigned char     col_cycles;
    unsigned char     row_cycles;
    unsigned char     planes;
};

struct nand_info* nand_identify(unsigned char data[5]);

#endif /* __NAND_ID_H__ */
