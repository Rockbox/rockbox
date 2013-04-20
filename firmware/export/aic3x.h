/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011 by Tomasz Moń
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

#ifndef _AIC3X_H_
#define _AIC3X_H_

AUDIOHW_SETTING(VOLUME, "dB",  0,  1, -64,   0, -25)

/*** definitions ***/
void aic3x_switch_output(bool stereo);

/* Page 0 registers */
#define AIC3X_PAGE_SELECT 0
#define AIC3X_SOFT_RESET  1
#define AIC3X_SMPL_RATE   2
#define AIC3X_PLL_REG_A   3
#define AIC3X_PLL_REG_B   4
#define AIC3X_PLL_REG_C   5
#define AIC3X_PLL_REG_D   6
#define AIC3X_DATAPATH    7
#define AIC3X_DATA_REG_A  8
#define AIC3X_DATA_REG_B  9
#define AIC3X_DATA_REG_C  10
#define AIC3X_OVERFLOW    11

#define AIC3X_LINE1L_LEFTADC 19

#define AIC3X_LINE1R_RIGHTADC 22

#define AIC3X_DAC_POWER   37
#define AIC3X_HIGH_POWER  38

#define AIC3X_POWER_OUT   40

#define AIC3X_POP_REDUCT  42
#define AIC3X_LEFT_VOL    43
#define AIC3X_RIGHT_VOL   44

#define AIC3X_DAC_L1_VOL  47
#define AIC3X_HPLOUT_LVL  51

#define AIC3X_HPLCOM_LVL  58

#define AIC3X_DAC_R1_VOL  64
#define AIC3X_HPROUT_LVL  65

#define AIC3X_DAC_L1_MONO_LOP_M_VOL 75

#define AIC3X_LINE2R_MONO_LOP_M_VOL 76
#define AIC3X_PGA_R_MONO_LOP_M_VOL  77
#define AIC3X_DAC_R1_MONO_LOP_M_VOL 78

#define AIC3X_MONO_LOP_M_LVL 79

#define AIC3X_DAC_L1_LEFT_LOP_M_VOL 82

#define AIC3X_LEFT_LOP_M_LVL 86

#define AIC3X_DAC_R1_RIGHT_LOP_M_VOL 92
#define AIC3X_RIGHT_LOP_M_LVL 93
#define AIC3X_MOD_POWER   94

#define AIC3X_GPIO1_CTRL  98

#endif /*_AIC3X_H_*/
