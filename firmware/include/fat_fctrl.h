/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 by Michael Sevakis
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
#ifndef _FAT_FCTRL_H_
#define _FAT_FCTRL_H_

#include "fat.h"
#include "file_internal.h"

/* File control */
static inline
int fctrl_open_at(struct file_ctrl_block *fctrl_dir,
                  const struct ioctrl_direntry_base *entry,
                  struct file_ctrl_block *fctrl)
{
    return fat_open(&fctrl_dir->fatfile, &entry->fctrl.fat_dirent.firstcluster,
                    &fctrl->fatfile);
} 

static inline
int fctrl_rename_at(struct file_ctrl_block *fctrl_dir,
                    struct file_ctrl_block *fctrl,
                    const unsigned char *newname)
{
    return fat_rename(&fctrl_dir->fatfile, &fctrl->fatfile, newname);
}

static inline
int fctrl_samefile(const struct file_ctrl_block *fctrl1,
                   const struct file_ctrl_block *fctrl2)
{
    return fat_file_is_same(&fctrl1->fatfile, &fctrl2->fatfile) ? 1 : 0;
}

static inline
int fctrl_parentdir(const struct file_ctrl_block *fctrl,
                    const struct file_ctrl_block *fctrl_dir)
{
    return fat_dir_is_parent(&fctrl_dir->fatfile, &fctrl->fatfile) ? 1 : 0;
}

static inline
int fctrl_remove(struct file_ctrl_block *fctrl, unsigned int flags)
{
    if (!flags) {
        return -1;
    }

    unsigned int what = 0;

    if (flags & FCTRL_RM_DIRENTRY) {
        what = FAT_RM_DIRENTRIES;
    }

    if (flags & FCTRL_RM_DATA) {
        what |= FAT_RM_DATA;
    }

    return fat_remove(&fctrl->fatfile, (enum fat_remove_op)what);
}

/* File I/O control */
static inline
unsigned long fctrl_io_query_sectornum(struct file_ioctrl_block *ioctrl)
{
    return fat_query_sectornum(&ioctrl->fatstr);
}

static inline
int fctrl_io_seek(struct file_ioctrl_block *ioctrl, unsigned long sector)
{
    return fat_seek(&ioctrl->fatstr, sector); 
}

static inline
long fctrl_io_readwrite(struct file_ioctrl_block *ioctrl,
                        unsigned long sectorcount,
                        void *buf, bool write)
{
    return fat_readwrite(&ioctrl->fatstr, sectorcount, buf, write);
}

static inline
int fctrl_io_seek_to_stream(struct file_ioctrl_block *ioctrl,
                            const struct file_ioctrl_block *ioctrl_to)
{
    fat_seek_to_stream(&ioctrl->fatstr, &ioctrl_to->fatstr);
    return 0;
}

static inline
int fctrl_io_truncate(struct file_ioctrl_block *ioctrl)
{
    return fat_truncate(&ioctrl->fatstr);
}

static inline
int fctrl_io_sync(struct file_ioctrl_block *ioctrl, file_size_t size)
{
    return fat_closewrite(&ioctrl->fatstr, size, get_dir_fatent_dircache());
}

static inline
int fctrl_io_close(struct file_ioctrl_block *ioctrl)
{
    return 0;
    (void)ioctrl;
}

static inline
int fctrl_io_init(struct file_ioctrl_block *ioctrl,
                  struct file_ctrl_block *fctrl)
{
    fat_filestr_init(&ioctrl->fatstr, &fctrl->fatfile);
    return 0;
}

static inline
int fctrl_io_rewind(struct file_ioctrl_block *ioctrl)
{
    fat_rewind(&ioctrl->fatstr);
    return 0;
}

static inline
int fctrl_io_assign_file(struct file_ioctrl_block *ioctrl,
                         struct file_ctrl_block *fctrl)
{
    ioctrl->fatstr.fatfilep = &fctrl->fatfile;
}

static inline
int fctrl_io_readdir(struct file_ioctrl_block *ioctrl,
                     struct file_control_block *fctrl_out,
                     struct ioctrl_direntry_base *entry)
{
####
    return fat_readdir(&ioctrl->fatstr, filestr_get_cache(stream),
                       &entry->fctrl.fat_dirent);
}

static inline
int fctrl_fill_dirinfo(struct dirinfo *info, struct dirinfo_native *din)
{
    if ((file_size_t)din->size > FILE_SIZE_MAX)
        return -EOVERFLOW;

    info->attribute = din->attr,
    info->size      = din->size,
    info->mtime     = fattime_mktime(din->wrtdate, din->wrttime);

    return 0;
}

static inline
void fctrl_reset_dirinfo(struct dirinfo *info)
{
    *info = (struct dirinfo){ .attribute = 0 };
}

#endif /* _FAT_FCTRL_H_ */
