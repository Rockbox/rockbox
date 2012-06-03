/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
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

// PUREVERSION is needed to be able to just compare versions. It does not
// contain a build timestamp because it needs to be the same in different
// files
// VERSION is the plain version number, used for http User-Agent string.
// BUILDID is an additional build string to handle package updates (i.e.
// rebuilds because of issues like dependency problems or library updates).
// Usually empty.
#define BUILDID ""
#define VERSION "1.2.14" BUILDID
// PUREVERSION should identify the build uniquely. Use version string for now.
#define PUREVERSION "$Rev$"

#define FULLVERSION VERSION" ("PUREVERSION"), built "__DATE__" "__TIME__

