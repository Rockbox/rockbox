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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

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
int iriver_decode(char *infile, char *outfile, BOOL modify,
                  enum striptype stripmode );
int iriver_encode(char *infile_name, char *outfile_name, BOOL modify );
