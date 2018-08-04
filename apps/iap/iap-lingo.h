/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr & Nick Robinson
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

void iap_handlepkt_mode0(const unsigned int len, const unsigned char *buf);
#ifdef HAVE_LINE_REC
void iap_handlepkt_mode1(const unsigned int len, const unsigned char *buf);
#endif
void iap_handlepkt_mode2(const unsigned int len, const unsigned char *buf);
void iap_handlepkt_mode3(const unsigned int len, const unsigned char *buf);
void iap_handlepkt_mode4(const unsigned int len, const unsigned char *buf);
#if CONFIG_TUNER
void iap_handlepkt_mode7(const unsigned int len, const unsigned char *buf);
#endif
