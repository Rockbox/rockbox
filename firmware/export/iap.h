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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __IAP_H__
#define __IAP_H__

#include <stdbool.h>

/* This is just the payload size, without sync, length and checksum */
#define RX_BUFLEN (64*1024)
/* This is the entire frame length, sync, length, payload and checksum */
#define TX_BUFLEN 128

extern bool iap_getc(unsigned char x);
extern void iap_setup(int ratenum);
extern void iap_bitrate_set(int ratenum);
extern void iap_periodic(void);
extern void iap_handlepkt(void);
extern void iap_send_pkt(const unsigned char * data, int len);
const unsigned char *iap_get_serbuf(void);
#ifdef HAVE_LINE_REC
extern bool iap_record(bool onoff);
#endif

#endif
