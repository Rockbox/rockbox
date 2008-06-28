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
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define BOOL unsigned int

#define ESTF_SIZE 32

#ifdef __cplusplus
extern "C" {
#endif

enum striptype
{
  STRIP_NONE,
  STRIP_HEADER_CHECKSUM,
  STRIP_HEADER_CHECKSUM_ESTF
};

/* protos for iriver.c */
int iriver_decode(const char *infile, const char *outfile, BOOL modify,
                  enum striptype stripmode );
int iriver_encode(const char *infile_name, const char *outfile_name, BOOL modify);

#ifdef __cplusplus
}
#endif

