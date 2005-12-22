/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#ifndef PCF50606_H
#define PCF50606_H

#ifdef IRIVER_H300_SERIES
void pcf50606_init(void);
int pcf50606_write_multiple(int address, const unsigned char* buf, int count);
int pcf50606_write(int address, unsigned char val);
int pcf50606_read_multiple(int address, unsigned char* buf, int count);
int pcf50606_read(int address);
void pcf50606_set_bl_pwm(unsigned char ucVal);
#endif

#endif
