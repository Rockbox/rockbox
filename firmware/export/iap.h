/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: iap.h 17400 2008-05-07 20:22:16Z xxxxxx $
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __IAP_H__
#define __IAP_H__

extern int iap_getc(unsigned char x);
extern void iap_write_pkt(unsigned char data, int len);
extern void iap_setup(int ratenum);
extern void iap_bitrate_set(int ratenum);
extern void iap_periodic(void);
extern void iap_handlepkt(void);
extern void iap_track_changed(void);

#endif
