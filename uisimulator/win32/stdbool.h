/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __STDBOOL_H__
#define __STDBOOL_H__   1

#ifndef __MINGW32__
typedef unsigned int bool;
#define __attribute__(s)

#define true        1
#define false       0
#else

typedef enum
{
  false = 0,
  true = 1
} bool;

#define false   false
#define true    true

/* Signal that all the definitions are present.  */
#define __bool_true_false_are_defined   1

#endif

#endif /* __STDBOOL_H__ */

