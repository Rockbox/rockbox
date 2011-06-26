/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

#ifndef BBFUNCS_H
#define BBFUNCS_H

#include "mbreader.h"

void error_die(const char* msg);
void error_msg(const char* msg);
size_t safe_read(struct mbreader_t *md, void *buf, size_t count);
ssize_t full_read(struct mbreader_t *md, void *buf, size_t len);
void xread(struct mbreader_t *md, void *buf, ssize_t count);
unsigned char xread_char(struct mbreader_t *md);
void check_header_gzip(struct mbreader_t *md);

#endif
