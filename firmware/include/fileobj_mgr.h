/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Michael Sevakis
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
#ifndef _FILEOBJ_MGR_H_
#define _FILEOBJ_MGR_H_

#include "file_internal.h"

void file_binding_insert_first(struct file_base_binding *bindp);
void file_binding_insert_last(struct file_base_binding *bindp);
void file_binding_remove(struct file_base_binding *bindp);
void file_binding_remove_next(struct file_base_binding *prevp,
                              struct file_base_binding *bindp);

void fileobj_fileop_open(struct filestr_base *stream,
                         const struct file_base_info *srcinfop,
                         unsigned int callflags);
void fileobj_fileop_close(struct filestr_base *stream);
void fileobj_fileop_create(struct filestr_base *stream,
                           const struct file_base_info *srcinfop,
                           unsigned int callflags);
void fileobj_fileop_rename(struct filestr_base *stream,
                           const struct file_base_info *oldinfop);
void fileobj_fileop_remove(struct filestr_base *stream,
                           const struct file_base_info *oldinfop);
void fileobj_fileop_sync(struct filestr_base *stream);

file_size_t * fileobj_get_sizep(const struct filestr_base *stream);
unsigned int fileobj_get_flags(const struct filestr_base *stream);
struct filestr_base * fileobj_get_next_stream(const struct filestr_base *stream,
                                              const struct filestr_base *s);
void fileobj_change_flags(struct filestr_base *stream,
                          unsigned int flags, unsigned int mask);
void fileobj_mgr_unmount(IF_MV_NONVOID(int volume));

void fileobj_mgr_init(void) INIT_ATTR;

#endif /* _FILEOBJ_MGR_H_ */
