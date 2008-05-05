/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: jackpot.c 14034 2007-07-28 05:42:55Z kevin $
 *
 * Copyright (C) 2007 Copyright Kévin Ferrare
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#include "clock_bitmaps.h"

/* bitmaps */
#include "clock_binary.h"
#include "clock_digits.h"
#include "clock_smalldigits.h"
#include "clock_segments.h"
#include "clock_smallsegments.h"

#include "clock_logo.h"
#include "clock_messages.h"

#if NB_SCREENS==2
#include "clock_binary_remote.h"
#include "clock_digits_remote.h"
#include "clock_smalldigits_remote.h"
#include "clock_segments_remote.h"
#include "clock_smallsegments_remote.h"

#include "clock_logo_remote.h"
#include "clock_messages_remote.h"

#endif


const struct picture logos[]={
        {clock_logo, BMPWIDTH_clock_logo, BMPHEIGHT_clock_logo},
#if NB_SCREENS==2
        {clock_logo_remote,BMPWIDTH_clock_logo_remote,BMPHEIGHT_clock_logo_remote}
#endif
};

const struct picture messages[]={
    {clock_messages,BMPWIDTH_clock_messages,
    BMPHEIGHT_clock_messages/6},
#if NB_SCREENS==2
    {clock_messages_remote,BMPWIDTH_clock_messages_remote,
    BMPHEIGHT_clock_messages_remote/6}
#endif
};

const struct picture binary[]={
                {clock_binary,
                BMPWIDTH_clock_binary, BMPHEIGHT_clock_binary/2 },
#if NB_SCREENS==2
                {clock_binary_remote,
                BMPWIDTH_clock_binary_remote,BMPHEIGHT_clock_binary_remote/2}
#endif
};

const struct picture digits[]={
    {clock_digits,
    BMPWIDTH_clock_digits, BMPHEIGHT_clock_digits/13 },
#if NB_SCREENS==2
    {clock_digits_remote,
    BMPWIDTH_clock_digits_remote,BMPHEIGHT_clock_digits_remote/13}
#endif
};

const struct picture smalldigits[]={
    {clock_smalldigits,
    BMPWIDTH_clock_smalldigits, BMPHEIGHT_clock_smalldigits/13 },
#if NB_SCREENS==2
    {clock_smalldigits_remote,
    BMPWIDTH_clock_smalldigits_remote,BMPHEIGHT_clock_smalldigits_remote/13}
#endif
};

const struct picture segments[]={
    {clock_segments,
    BMPWIDTH_clock_segments, BMPHEIGHT_clock_segments/13 },
#if NB_SCREENS==2
    {clock_segments_remote,
    BMPWIDTH_clock_segments_remote,BMPHEIGHT_clock_segments_remote/13}
#endif
};

const struct picture smallsegments[]={
    {clock_smallsegments,
    BMPWIDTH_clock_smallsegments, BMPHEIGHT_clock_smallsegments/13 },
#if NB_SCREENS==2
    {clock_smallsegments_remote,
    BMPWIDTH_clock_smallsegments_remote,BMPHEIGHT_clock_smallsegments_remote/13}
#endif
};
