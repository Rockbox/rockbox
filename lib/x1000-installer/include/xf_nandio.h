/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef _XF_NANDIO_H_
#define _XF_NANDIO_H_

#include "nand-x1000.h"
#include <stdint.h>
#include <stddef.h>

enum xf_nandio_mode {
    XF_NANDIO_READ = 0,
    XF_NANDIO_PROGRAM,
    XF_NANDIO_VERIFY,
};

struct xf_nandio {
    nand_drv* ndrv;
    int nand_err;       /* copy of the last NAND error code */
    int alloc_handle;
    enum xf_nandio_mode mode;

    size_t block_size;  /* size of a block, in bytes */
    size_t page_size;   /* size of a page, in bytes */

    uint8_t* old_buf;   /* contains the 'old' block data already on chip */
    uint8_t* new_buf;   /* contains the possibly modified 'new' block data */

    nand_block_t cur_block;     /* address of the current block */
    size_t offset_in_block;     /* byte offset in the current block */
    unsigned block_valid: 1;    /* 1 if buffered block data is valid */
};

int xf_nandio_init(struct xf_nandio* nio);
void xf_nandio_destroy(struct xf_nandio* nio);

/** Sets the operational mode, which determines read/write/flush semantics.
 *
 * - XF_NANDIO_READ: Accesses the chip in read-only mode. Writes are allowed,
 *   but should not be used. (Writes will modify a temporary buffer but this
 *   will not alter the flash contents.)
 *
 * - XF_NANDIO_PROGRAM: Writes are allowed to modify the flash contents.
 *   Writes within a block are accumulated in a temporary buffer. When
 *   crossing a block boundary, either by writing past the end the current
 *   block or by seeking to a new one, the data written to the temporary
 *   buffer is compared against the current flash contents. If the block
 *   has been modified, it is erased and any non-blank pages are programmed
 *   with the new data.
 *
 * - XF_NANDIO_VERIFY: This mode allows callers to easily check whether the
 *   flash contents match some expected contents. Callers "write" the expected
 *   contents as if programming it with XF_NANDIO_PROGRAM. When a block is
 *   flushed, if the written data doesn't match the block contents, an
 *   XF_E_VERIFY_FAILED error is returned. The flash contents will not be
 *   altered in this mode.
 *
 * \returns XF_E_SUCCESS or a negative error code on failure.
 */
int xf_nandio_set_mode(struct xf_nandio* nio, enum xf_nandio_mode mode);

/** Seek to a given byte offset in the NAND flash.
 *
 * If the new offset is outside the current block, the current block will
 * be automatically flushed. Note this can result in verification or program
 * failures as with any other flush.
 *
 * \returns XF_E_SUCCESS or a negative error code on failure.
 */
int xf_nandio_seek(struct xf_nandio* nio, size_t offset);

/** Read or write a contiguous sequence of bytes from flash.
 *
 * The read or write starts at the current position and continues for `count`
 * bytes. Both reads and writes may cross block boundaries. Modified blocks
 * will be flushed automatically if the operation crosses a block boundary.
 *
 * After a successful read or write, the current position is advanced by
 * exactly `count` bytes. After a failure, the position is indeterminate.
 *
 * \returns XF_E_SUCCESS or a negative error code on failure.
 */
int xf_nandio_read(struct xf_nandio* nio, void* buf, size_t count);
int xf_nandio_write(struct xf_nandio* nio, const void* buf, size_t count);

/** Get a pointer to the block buffer for direct read/write access.
 *
 * These functions can be used to read or write data without intermediate
 * buffers. The caller passes in the amount of data to be transferred in
 * `*count`. A pointer to part of the block buffer is returned in `*buf`
 * and the number of bytes available in `*buf` is returned in `*count`.
 *
 * Data at the current position can be read from the returned buffer and
 * it may be modified by writing to the buffer. The buffer is only valid
 * until the next call to an `xf_nandio` function.
 *
 * The read/write position is advanced by the returned `*count` on success,
 * and is unchanged on failure.
 *
 * \returns XF_E_SUCCESS or a negative error code on failure.
 */
int xf_nandio_get_buffer(struct xf_nandio* nio, void** buf, size_t* count);

/** Flush the buffered block to ensure all outstanding program or verification
 * operations have been performed. This should only be called to ensure the
 * final modified block is flushed after you have finished writing all data.
 *
 * \returns XF_E_SUCCESS or a negative error code on failure.
 */
int xf_nandio_flush(struct xf_nandio* nio);

#endif /* _XF_NANDIO_H_ */
