/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#ifndef __audio_target__
#define __audio_target__

#define IMX233_AUDIO_COUPLING_MODE ACM_CAPLESS

#define IMX233_AUDIO_SPKR_GATE_BANK 0
#define IMX233_AUDIO_SPKR_GATE_PIN  12

#define IMX233_AUDIO_MIC_SELECT 0 /* lradc0 */
#define IMX233_AUDIO_MIC_BIAS   0 /* 1.21V */
#define IMX233_AUDIO_MIC_RESISTOR   2KOhm

#endif /* __audio_target__ */
