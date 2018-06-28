/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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

#include <stdlib.h>
#include "nand_id.h"

struct nand_manufacturer
{
    unsigned char        id;
    struct nand_info*  info;
    unsigned short    total;
};

static const struct nand_info samsung[] =
{
/*
    id1, id2
    pages/block, blocks, page_size, spare_size, col_cycles, row_cycles, planes
*/      
    {0xDC, 0x10, /* K9F4G08UOM */
     64,         4096,   2048,      64,         2,          3,          1 },
                                              
    {0xD3, 0x51, /* K9K8G08UOM */
     64,         8192,   2048,      64,         2,          3,          1 },
                                              
    {0xD5, 0x14, /* K9GAG08UOM */
     128,        4096,   4096,      128,        2,          3,          2 },
                                              
    {0xD5, 0x55, /* K9LAG08UOM, K9HBG08U1M, K9MCG08U5M */
     128,        8192,   2048,      64,         2,          3,          4 },
                                 
    {0xD7, 0x55, /* K9LBG08UOM */
     128,        8192,   4096,      128,        2,          3,          4 },
};

static const struct nand_info gigadevice[] =
{
/*
    id1, id2
    pages/block, blocks, page_size, spare_size, col_cycles, row_cycles, planes
*/      
    {0xB1, 0x80, /* MD5N01G51MSD1B */
     64,         1024,   2048,      64,         2,          2,          1 },
};

#define NI(id, x) {id, (struct nand_info*)x, (sizeof(x)/sizeof(struct nand_info))}
static const struct nand_manufacturer all[] =
{
    NI(0xEC, samsung),
    NI(0x98, gigadevice),
};

// -----------------------------------------------------------------------------

struct nand_info* nand_identify(unsigned char data[5])
 {
    unsigned int i;
    int found = -1;
    
    for(i = 0; i < (sizeof(all)/sizeof(struct nand_manufacturer)); i++)
    {
        if(data[0] == all[i].id)
        {
            found = i;
            break;
        }
    }
    
    if(found < 0)
        return NULL;
    
    for(i = 0; i < all[found].total; i++)
    {
        if(data[1] == all[found].info[i].dev_id &&
           data[2] == all[found].info[i].dev_id2)
            return &all[found].info[i];
    }
    
    return NULL;
}
