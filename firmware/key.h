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

#ifndef __key_h__
#define __key_h__

#include <sh7034.h>
#include <system.h>

#define key_released(key) \
  (key_released_##key ())
#define key_pressed(key) \
  (!(key_released_##key ()))

static inline int key_released_ON (void)
  { return (QI(PADR+1)&(1<<5)); }
static inline int key_released_STOP (void)
  { return (QI(PADR+0)&(1<<3)); }
static inline int key_released_MINUS (void)
  { return (QI(PCDR+0)&(1<<0)); }
static inline int key_released_MENU (void)
  { return (QI(PCDR+0)&(1<<1)); }
static inline int key_released_PLUS (void)
  { return (QI(PCDR+0)&(1<<2)); }
static inline int key_released_PLAY (void)
  { return (QI(PCDR+0)&(1<<3)); }

#endif
