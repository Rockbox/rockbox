/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#ifndef DISABLE_DEMOS
#undef USE_DEMOS /* since older Makefiles set the define */
#define USE_DEMOS
#endif

/* disable demos until plugins are added, to stay below 200KB */
#ifdef USE_DEMOS
#undef USE_DEMOS
#endif

#ifndef DISABLE_GAMES
#undef USE_GAMES /* since older Makefiles set the define */
#define USE_GAMES
#endif

#endif /* End __OPTIONS_H__ */
