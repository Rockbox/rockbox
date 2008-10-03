/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Jens Arnold
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

/* Define how IRAM is used on the various targets.  Note that this file
 * is included by both .c and .S files so must not contain any C code. */

#ifndef _LIBDEMAC_IRAM_H
#define _LIBDEMAC_IRAM_H

#include "config.h"

/* On PP5002 code should go into IRAM. Otherwise put the insane
 * filter buffer into IRAM as long as there is no better use. */
#if CONFIG_CPU == PP5002
#define ICODE_SECTION_DEMAC_ARM   .icode
#define ICODE_ATTR_DEMAC          ICODE_ATTR
#define IBSS_ATTR_DEMAC_INSANEBUF
#else
#define ICODE_SECTION_DEMAC_ARM   .text
#define ICODE_ATTR_DEMAC
#define IBSS_ATTR_DEMAC_INSANEBUF IBSS_ATTR
#endif

#endif /* _LIBDEMAC_IRAM_H */
