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

#include "gitversion.h"

// PUREVERSION is needed to be able to just compare versions. It does not
// contain a build timestamp because it needs to be the same in different
// files
// VERSION is the plain version number, used for http User-Agent string.
// It is concatenated from separate digits to allow reusing for the Windows
// resource information
// BUILDID is an additional build string to handle package updates (i.e.
// rebuilds because of issues like dependency problems or library updates).
// Usually empty.
#define BUILDID ""
// Version string is constructed from parts, since the Windows rc file needs it
// combined differently.
#define VERSION_MAJOR 1
#define VERSION_MINOR 5
#define VERSION_MICRO 2
#define VERSION_PATCH 0
#define STR(x) #x
#define VERSIONSTRING(a, b, c) STR(a) "." STR(b) "." STR(c)
#define VERSION VERSIONSTRING(VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO) BUILDID
// PUREVERSION should identify the build uniquely. Use version string for now.
#define PUREVERSION GITHASH

#define FULLVERSION VERSION " (" GITHASH "), built " __DATE__ " " __TIME__

