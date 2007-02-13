/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Miika Pekkarinen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _STRUCTEC_H
#define _STRUCTEC_H

#include <sys/types.h>
#include <stdbool.h>

void structec_convert(void *structure, const char *ecinst, 
                      long count, bool enable);
ssize_t ecread(int fd, void *buf, size_t scount, const char *ecinst, bool ec);
ssize_t ecwrite(int fd, const void *buf, size_t scount,  const char *ecinst, bool ec);
#endif

