/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Maurus Cuelenaere
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
#ifndef __JZ4740_CODEC_H_
#define __JZ4740_CODEC_H_

#ifdef HAVE_SW_VOLUME_CONTROL
AUDIOHW_SETTING(VOLUME,     "dB", 0,  1, -74,   6, -25)
#else
AUDIOHW_SETTING(VOLUME,     "dB", 0,  1,   0,   6,   0)
#endif /* HAVE_SW_VOLUME_CONTROL */

#ifdef HAVE_RECORDING
AUDIOHW_SETTING(LEFT_GAIN,  "dB", 1,  1,   0,  31,  23)
AUDIOHW_SETTING(RIGHT_GAIN, "dB", 1,  1,   0,  31,  23)
AUDIOHW_SETTING(MIC_GAIN,   "dB", 1,  1,   0,   1,   1)
#endif /* HAVE_RECORDING */

#endif /* __JZ4740_CODEC_H_ */
