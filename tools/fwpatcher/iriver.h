/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Daniel Stenberg
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

#include <tchar.h>
#include <windows.h>

#define TRUE 1
#define FALSE 0

#define BOOL unsigned int

#define ESTF_SIZE 32

enum striptype
{
  STRIP_NONE,
  STRIP_HEADER_CHECKSUM,
  STRIP_HEADER_CHECKSUM_ESTF
};

/* protos for iriver.c */
int iriver_decode(TCHAR *infile, TCHAR *outfile, BOOL modify,
                  enum striptype stripmode );
int iriver_encode(TCHAR *infile_name, TCHAR *outfile_name, BOOL modify );
