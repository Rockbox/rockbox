/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 by Amaury Pouly
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"

#ifndef IMX233_SUBTARGET
#error You must define IMX233_SUBTARGET for select register set
#endif

#if IMX233_SUBTARGET >= 3600 && IMX233_SUBTARGET < 3700
# ifdef STMP3600_INCLUDE
#  include STMP3600_INCLUDE
# endif
#elif IMX233_SUBTARGET >= 3700 && IMX233_SUBTARGET < 3780
# ifdef STMP3700_INCLUDE
#  include STMP3700_INCLUDE
# endif
#elif IMX233_SUBTARGET == 3780
# ifdef IMX233_INCLUDE
#  include IMX233_INCLUDE
# endif
#else
#error Unknown IMX233_SUBTARGET
#endif
