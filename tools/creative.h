/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef CREATIVE_H_
#define CREATIVE_H_

enum
{
    ZENVISIONM = 0,
    ZENVISIONM60 = 1,
    ZENVISION = 2,
	ZENV = 3
};

static struct device_info
{
    const char* cinf; /*Must be Unicode encoded*/
    const int cinf_size;
    const char* null;
} device_info;

static const char null_key_v1[]  = "CTL:N0MAD|PDE0.SIGN.";
static const char null_key_v2[]  = "CTL:N0MAD|PDE0.DPMP.";
static const char null_key_v3[]  = "CTL:Z3N07|PDE0.DPMP.";
static const char null_key_v4[]  = "CTL:N0MAD|PDE0.DPFP.";

static const struct device_info devices[] =
{
    {"C\0r\0e\0a\0t\0i\0v\0e\0 \0Z\0e\0n\0 \0V\0i\0s\0i\0o\0n\0:\0M", 42, null_key_v2},
    {"C\0r\0e\0a\0t\0i\0v\0e\0 \0Z\0e\0n\0 \0V\0i\0s\0i\0o\0n\0:\0M\0 \0G\0o\0!", 50, null_key_v2},
    {"C\0r\0e\0a\0t\0i\0v\0e\0 \0Z\0e\0n\0 \0V\0i\0s\0i\0o\0n\0 \0©\0T\0L", 48, null_key_v2},
	{"C\0r\0e\0a\0t\0i\0v\0e\0 \0Z\0E\0N\0 \0V", 42, null_key_v4}
};

int zvm_encode(char *iname, char *oname, int device);

#endif /*CREATIVE_H_*/
