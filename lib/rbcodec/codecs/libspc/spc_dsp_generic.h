/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Adam Gashlin (hcs)
 * Copyright (C) 2004-2007 Shay Green (blargg)
 * Copyright (C) 2002 Brad Martin
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
#if !SPC_NOECHO

#ifndef SPC_DSP_ECHO_APPLY
enum
{
    FIR_BUF_CNT   = FIR_BUF_HALF * 2 * 2,
    FIR_BUF_SIZE  = FIR_BUF_CNT * sizeof ( int32_t ),
    FIR_BUF_ALIGN = FIR_BUF_SIZE,
    FIR_BUF_MASK  = ~((FIR_BUF_ALIGN / 2) | (sizeof ( int32_t ) * 2 - 1))
};

/* Echo filter structure embedded in struct Spc_Dsp */
struct echo_filter
{
    /* fir_buf [i + 8] == fir_buf [i], to avoid wrap checking in FIR code */
    int pos; /* (0 to 7) */
    int buf [FIR_BUF_HALF * 2] [2];
    /* copy of echo FIR constants as int, for faster access */
    int coeff [VOICE_COUNT]; 
};
#endif /* SPC_DSP_ECHO_APPLY */

#endif /* !SPC_NOECHO */
