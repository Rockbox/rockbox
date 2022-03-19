/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2022 Aidan MacDonald
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

#include "linuxboot.h"
#include "system.h"
#include "core_alloc.h"
#include "crc32.h"
#include "inflate.h"
#include "file.h"
#include <string.h>

/* compression support options - can be decided per target if needed,
 * for now default to enabling everything */
#define HAVE_UIMAGE_COMP_NONE
#define HAVE_UIMAGE_COMP_GZIP

enum {
    E_OUT_OF_MEMORY = -1,
    E_BUFFER_OVERFLOW = -2,
    E_MAGIC_MISMATCH = -3,
    E_HCRC_MISMATCH = -4,
    E_DCRC_MISMATCH = -5,
    E_UNSUPPORTED_COMPRESSION = -6,
    E_READ = -7,
    E_INFLATE = -8,
    E_INFLATE_UNCONSUMED = -9,
};

uint32_t uimage_crc(uint32_t crc, const void* data, size_t size)
{
    return letoh32(crc_32r(data, size, htole32(crc ^ 0xffffffff))) ^ 0xffffffff;
}

uint32_t uimage_calc_hcrc(const struct uimage_header* uh)
{
    struct uimage_header h = *uh;
    uimage_set_hcrc(&h, 0);
    return uimage_crc(0, &h, sizeof(h));
}

static int uimage_check_header(const struct uimage_header* uh)
{
    if(uimage_get_magic(uh) != IH_MAGIC)
        return E_MAGIC_MISMATCH;

    if(uimage_get_hcrc(uh) != uimage_calc_hcrc(uh))
        return E_HCRC_MISMATCH;

    return 0;
}

static int uimage_alloc_state(const struct uimage_header* uh)
{
    size_t size;

    switch(uimage_get_comp(uh)) {
#ifdef HAVE_UIMAGE_COMP_NONE
    case IH_COMP_NONE:
        return 0;
#endif

#ifdef HAVE_UIMAGE_COMP_GZIP
    case IH_COMP_GZIP:
        size = inflate_size + inflate_align - 1;
        return core_alloc_ex("inflate", size, &buflib_ops_locked);
#endif

    default:
        return E_UNSUPPORTED_COMPRESSION;
    }
}

#ifdef HAVE_UIMAGE_COMP_GZIP
struct uimage_inflatectx
{
    uimage_reader reader;
    void* rctx;
    uint32_t dcrc;
    size_t remain;
    int err;
};

static uint32_t uimage_inflate_reader(void* block, uint32_t block_size, void* ctx)
{
    struct uimage_inflatectx* c = ctx;
    ssize_t len = c->reader(block, block_size, c->rctx);
    if(len < 0) {
        c->err = E_READ;
        return 0;
    }

    len = MIN(c->remain, (size_t)len);
    c->remain -= len;
    c->dcrc = uimage_crc(c->dcrc, block, len);
    return len;
}

static int uimage_decompress_gzip(const struct uimage_header* uh, int state_h,
                                  void* out, size_t* out_size,
                                  uimage_reader reader, void* rctx)
{
    size_t hbufsz = inflate_size + inflate_align - 1;
    void* hbuf = core_get_data(state_h);
    ALIGN_BUFFER(hbuf, hbufsz, inflate_align);

    struct uimage_inflatectx r_ctx;
    r_ctx.reader = reader;
    r_ctx.rctx = rctx;
    r_ctx.dcrc = 0;
    r_ctx.remain = uimage_get_size(uh);
    r_ctx.err = 0;

    struct inflate_bufferctx w_ctx;
    w_ctx.buf = out;
    w_ctx.end = out + *out_size;

    int ret = inflate(hbuf, INFLATE_GZIP,
                      uimage_inflate_reader, &r_ctx,
                      inflate_buffer_writer, &w_ctx);
    if(ret) {
        if(r_ctx.err)
            return r_ctx.err;
        else if(w_ctx.end == w_ctx.buf)
            return E_BUFFER_OVERFLOW;
        else
            /* note: this will likely mask DCRC_MISMATCH errors */
            return E_INFLATE;
    }

    if(r_ctx.remain > 0)
        return E_INFLATE_UNCONSUMED;
    if(r_ctx.dcrc != uimage_get_dcrc(uh))
        return E_DCRC_MISMATCH;

    *out_size = w_ctx.end - w_ctx.buf;
    return 0;
}
#endif /* HAVE_UIMAGE_COMP_GZIP */

static int uimage_decompress(const struct uimage_header* uh, int state_h,
                             void* out, size_t* out_size,
                             uimage_reader reader, void* rctx)
{
    size_t in_size = uimage_get_size(uh);
    ssize_t len;

    switch(uimage_get_comp(uh)) {
#ifdef HAVE_UIMAGE_COMP_NONE
    case IH_COMP_NONE:
        if(*out_size < in_size)
            return E_BUFFER_OVERFLOW;

        len = reader(out, in_size, rctx);
        if(len < 0 || (size_t)len != in_size)
            return E_READ;

        if(uimage_crc(0, out, in_size) != uimage_get_dcrc(uh))
            return E_DCRC_MISMATCH;

        *out_size = in_size;
        break;
#endif

#ifdef HAVE_UIMAGE_COMP_GZIP
    case IH_COMP_GZIP:
        return uimage_decompress_gzip(uh, state_h, out, out_size, reader, rctx);
#endif

    default:
        return E_UNSUPPORTED_COMPRESSION;
    }

    return 0;
}

int uimage_load(struct uimage_header* uh, size_t* out_size,
                uimage_reader reader, void* rctx)
{
    if(reader(uh, sizeof(*uh), rctx) != (ssize_t)sizeof(*uh))
        return E_READ;

    int ret = uimage_check_header(uh);
    if(ret)
        return ret;

    int state_h = uimage_alloc_state(uh);
    if(state_h < 0)
        return E_OUT_OF_MEMORY;

    *out_size = 0;
    int out_h = core_alloc_maximum("uimage", out_size, &buflib_ops_locked);
    if(out_h <= 0) {
        ret = E_OUT_OF_MEMORY;
        goto err;
    }

    ret = uimage_decompress(uh, state_h, core_get_data(out_h), out_size,
                            reader, rctx);
    if(ret)
        goto err;

    core_shrink(out_h, core_get_data(out_h), *out_size);
    ret = 0;

  err:
    if(state_h > 0)
        core_free(state_h);
    if(out_h > 0) {
        if(ret == 0)
            ret = out_h;
        else
            core_free(out_h);
    }

    return ret;
}

ssize_t uimage_fd_reader(void* buf, size_t size, void* ctx)
{
    int fd = (intptr_t)ctx;
    return read(fd, buf, size);
}

/* Linux's self-extracting kernels are broken on MIPS. The decompressor stub
 * doesn't flush caches after extracting the kernel code which can cause the
 * boot to fail horribly. This has been true since at least 2009 and at the
 * time of writing (2022) it's *still* broken.
 *
 * The FiiO M3K and Shanling Q1 both have broken kernels of this type, so we
 * work around this by replacing the direct call to the kernel entry point with
 * a thunk that adds the necessary cache flush.
 */
uint32_t mips_linux_stub_get_entry(void** code_start, size_t code_size)
{
    /* The jump to the kernel entry point looks like this:
     *
     *   move a0, s0
     *   move a1, s1
     *   move a2, s2
     *   move a3, s3
     *   ...
     *   la   k0, KERNEL_ENTRY
     *   jr   k0
     *   --- or in kernels since 2021: ---
     *   la   t9, KERNEL_ENTRY
     *   jalr t9
     *
     * We're trying to identify this code and decode the kernel entry
     * point address, and return a suitable address where we can patch
     * in a call to our thunk.
     */

    /* We should only need to scan within the first 128 bytes
     * but do up to 256 just in case. */
    uint32_t* start = *code_start;
    uint32_t* end = start + (MIN(code_size, 256) + 3) / 4;

    /* Scan for the "move aN, sN" sequence */
    uint32_t* move_instr = start;
    for(move_instr += 4; move_instr < end; ++move_instr) {
        if(move_instr[-4] == 0x02002021 &&  /* move a0, s0 */
           move_instr[-3] == 0x02202821 &&  /* move a1, s1 */
           move_instr[-2] == 0x02403021 &&  /* move a2, s2 */
           move_instr[-1] == 0x02603821)    /* move a3, s3 */
            break;
    }

    if(move_instr == end)
        return 0;

    /* Now search forward for the next jr/jalr instruction */
    int jreg = 0;
    uint32_t* jump_instr = move_instr;
    for(; jump_instr != end; ++jump_instr) {
        if((jump_instr[0] & 0xfc1ff83f) == 0xf809 ||
           (jump_instr[0] & 0xfc00003f) == 0x8) {
            /* jalr rN */
            jreg = (jump_instr[0] >> 21) & 0x1f;
            break;
        }
    }

    /* Need room here for 4 instructions. Assume everything between the
     * moves and the jump is safe to overwrite; otherwise, we'll need to
     * take a different approach.
     *
     * Count +1 instruction for the branch delay slot and another +1 because
     * "move_instr" points to the instruction following the last move. */
    if(jump_instr - move_instr + 2 < 4)
        return 0;
    if(!jreg)
        return 0;

    /* Now scan from the end of the move sequence until the jump instruction
     * and try to reconstruct the entry address. We check for lui/ori/addiu. */
    const uint32_t lui_mask   = 0xffff0000;
    const uint32_t lui        = 0x3c000000 | (jreg << 16);
    const uint32_t ori_mask   = 0xffff0000;
    const uint32_t ori        = 0x34000000 | (jreg << 21) | (jreg << 16);
    const uint32_t addiu_mask = 0xffff0000;
    const uint32_t addiu      = 0x24000000 | (jreg << 21) | (jreg << 16);

    /* Can use any initial value here */
    uint32_t jreg_val = 0xdeadbeef;

    for(uint32_t* instr = move_instr; instr != jump_instr; ++instr) {
        if((instr[0] & lui_mask) == lui)
            jreg_val = (instr[0] & 0xffff) << 16;
        else if((instr[0] & ori_mask) == ori)
            jreg_val |= instr[0] & 0xffff;
        else if((instr[0] & addiu_mask) == addiu)
            jreg_val += instr[0] & 0xffff;
    }

    /* Success! Probably! */
    *code_start = move_instr;
    return jreg_val;
}
