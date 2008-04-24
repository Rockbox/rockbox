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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef CREATIVE_H_
#define CREATIVE_H_

enum
{
    ZENVISIONM = 0,
    ZENVISIONM60 = 1,
    ZENVISION = 2,
	ZENV = 3,
    ZEN = 4
};

struct device_info
{
    const char* cinf; /*Must be Unicode encoded*/
    const unsigned int cinf_size;
    const char* null;
    const unsigned char* bootloader;
    const unsigned int bootloader_size;
    const unsigned int memory_address;
};

int zvm_encode(char *iname, char *oname, int device);

#endif /*CREATIVE_H_*/
