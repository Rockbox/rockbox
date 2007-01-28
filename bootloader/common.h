/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: main.c 11997 2007-01-13 09:08:18Z miipekk $
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Error codes */
#define     EOK                      0
#define     EFILE_NOT_FOUND         -1
#define     EREAD_CHKSUM_FAILED     -2
#define     EREAD_MODEL_FAILED      -3
#define     EREAD_IMAGE_FAILED      -4
#define     EBAD_CHKSUM             -5
#define     EFILE_TOO_BIG           -6

/* Functions common to all bootloaders */
void reset_screen(void);
void printf(const char *format, ...);
char *strerror(int error);
int load_firmware(unsigned char* buf, char* firmware, int buffer_size);
int load_raw_firmware(unsigned char* buf, char* firmware, int buffer_size);
