/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#if defined(ARCHOS_PLAYER)
#include "config-player.h"
#elif defined(ARCHOS_RECORDER)
#include "config-recorder.h"
#elif defined(ARCHOS_FMRECORDER)
#include "config-fmrecorder.h"
#elif defined(ARCHOS_RECORDERV2)
#include "config-recorderv2.h"
#elif defined(NEO_35)
#include "config-neo35.h"
#elif defined(ARCHOS_ONDIOSP)
#include "config-ondiosp.h"
#elif defined(ARCHOS_ONDIOFM)
#include "config-ondiofm.h"
#elif defined(IRIVER_H100)
#include "config-h100.h"
#else
/* no known platform */
#endif

#endif
