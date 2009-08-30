/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2009 by Karl Kurbjun 
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdlib.h>
extern int decrypt_array[];
extern int encrypt_array[];

/* Notes about the header:
 *  The magic_name should always be "OIMCFWUP" 
 *  Header length is always 18 bytes
 *  The flags have the following mask:
 *      CHECK_CRC   0x01    : Tells the firmware whether or not to check CRC
 *      ENDIAN_MODE 0x02    : Tells the OF whether to re-order the bytes
 *      The rest are unknown
 *  The image length is in bytes and is always 0x007F0000
 */
struct olympus_header {
    int8_t          magic_name[8];
    uint32_t        unknown;
    uint16_t        header_length;
    uint16_t        flags;
    uint32_t        unknown_zeros;
    uint32_t        image_length;
} __attribute__((__packed__));

/* Patch entries should be saved in little endian format */
struct patch_single {
    off_t       offset;
    uint32_t    value;
};

int mr500_patch_file(char *, struct patch_single *, int);
int mr500_save_header(char *, struct olympus_header *);
int mr500_read_header(char *, struct olympus_header *);
int mr500_save_crc(char *, off_t, uint32_t *);
int mr500_read_crc(char *, off_t, uint32_t *);
int mr500_calculate_crc(char *, off_t, unsigned int, uint32_t *);
int mr500_save_data(char *, char *, off_t, unsigned int, int*);
int mr500_init(void);

