
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "lcd.h"
#include "font.h"
#include "sprintf.h"
#include "string.h"
#include "settings.h"
#include "kernel.h"
#include "system.h"
#include "misc.h"
#include "screen_access.h"

/* return the number of text lines in the vp viewport */
int viewport_get_nb_lines(struct viewport *vp);

#define VP_ERROR              0
#define VP_DIMENSIONS       0x1
#define VP_COLORS           0x2
#define VP_SELECTIONCOLORS  0x4
/* load a viewport struct from a config string.
   returns a combination of the above to say which were loaded ok from the string */
int viewport_load_config(const char *config, struct viewport *vp);

void viewport_set_defaults(struct viewport *vp, enum screen_type screen);
