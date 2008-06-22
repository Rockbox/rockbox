/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Andree Buschmann
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _mpc_config_h_
#define _mpc_config_h_

#include "config.h"

/* choose fixed point or floating point */
#define MPC_FIXED_POINT

#ifndef MPC_FIXED_POINT
#error FIXME, mpc will not with floating point now
#endif

/* choose speed vs. accuracy for MPC_FIXED_POINT
 * speed-setting will increase decoding speed on ARM only (+20%), loss of accuracy 
 * equals about 5 dB SNR (15bit output precision) to not use the speed-optimization 
 * -> comment OPTIMIZE_FOR_SPEED here for desired target */
#if defined(MPC_FIXED_POINT)
   #if defined(CPU_COLDFIRE)
      // do nothing
   #elif defined(CPU_ARM)
      //#define OPTIMIZE_FOR_SPEED
   #else
      #define OPTIMIZE_FOR_SPEED
   #endif
#else
    // do nothing
#endif

#endif
