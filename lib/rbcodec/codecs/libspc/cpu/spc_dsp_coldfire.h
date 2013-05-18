/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Michael Sevakis (jhMikeS)
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

#define SPC_DSP_ECHO_APPLY

enum
{
    FIR_BUF_CNT   = FIR_BUF_HALF,
    FIR_BUF_SIZE  = FIR_BUF_CNT * sizeof ( int32_t ),
    FIR_BUF_ALIGN = FIR_BUF_SIZE * 2,
    FIR_BUF_MASK  = ~((FIR_BUF_ALIGN / 2) | (sizeof ( int32_t ) - 1))
};

/* Echo filter structure embedded in struct Spc_Dsp */
struct echo_filter
{
    /* FIR history is interleaved. Hardware handles wrapping by mask.
     * |LR|LR|LR|LR|LR|LR|LR|LR| */
    int32_t* ptr;
    /* wrapped address just behind current position -
       allows mac.w to increment and mask ptr */
    int32_t* hist_ptr;
    /* copy of echo FIR constants as int16_t for use with mac.w */
    int16_t coeff [VOICE_COUNT];
};
#endif /* !SPC_NOECHO */
