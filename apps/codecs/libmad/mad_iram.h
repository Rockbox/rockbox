/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Define how IRAM is used on the various targets.  Note that this
   file is included by both .c and .S files so must not contain any C
   code. 
*/

#ifndef _LIBMAD_IRAM_H
#define _LIBMAD_IRAM_H

#include "config.h"

/* Code performs slower in IRAM on PP502x and there is no space in
   mpegplayer on the PP5002.  S3C2440 doesn't have any IRAM available for
   codecs */
#if defined(CPU_PP502x) || (CONFIG_CPU == PP5002 && defined(MPEGPLAYER)) || \
    (CONFIG_CPU==S3C2440) || (CONFIG_CPU==IMX31L)
#define ICODE_SECTION_MPA_ARM .text
#define ICODE_ATTR_MPA_SYNTH
#else
#define ICODE_SECTION_MPA_ARM .icode
#define ICODE_ATTR_MPA_SYNTH ICODE_ATTR
#endif

#if CONFIG_CPU == S3C2440 || CONFIG_CPU == IMX31L
#define IBSS_SECTION_MPA_ARM .bss
#else
#define IBSS_SECTION_MPA_ARM .ibss
#endif

#ifndef ICONST_ATTR_MPA_HUFFMAN
#define ICONST_ATTR_MPA_HUFFMAN ICONST_ATTR
#endif

#endif /* MAD_IRAM_H */
