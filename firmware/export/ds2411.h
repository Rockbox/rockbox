/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _DS2411_H_
#define _DS2411_H_

#include <stdbool.h>

/*
 * Byte 0:    8-bit family code (always 01h)
 * Bytes 1-6: 48-bit serial number
 * Byte 7:    8-bit CRC code
 */
struct ds2411_id
{
    unsigned char family_code;
    unsigned char uid[6];
    unsigned char crc;
} __attribute__((packed));

extern int ds2411_read_id(struct ds2411_id *id);

/* return values */
enum ds2411_id_return_codes
{
    DS2411_NO_PRESENCE = -3,
    DS2411_INVALID_FAMILY_CODE,
    DS2411_INVALID_CRC,
    DS2411_OK = 0,
};

#endif /* _DS2411_H_ */
