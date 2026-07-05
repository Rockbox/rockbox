/* ucl_asm.h -- assembler prototypes for the UCL data compression library

   This file is part of the UCL data compression library.

   Copyright (C) 2004 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2003 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2002 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2001 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2000 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1999 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1998 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1997 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The UCL library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The UCL library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the UCL library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/ucl/
 */


#ifndef __UCL_ASM_H_INCLUDED
#define __UCL_ASM_H_INCLUDED

#ifndef __UCLCONF_H_INCLUDED
#include <ucl/uclconf.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
// NRV2B assembly decompressors
************************************************************************/

UCL_EXTERN(int) ucl_nrv2b_decompress_asm_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_safe_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_fast_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_fast_safe_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_small_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_small_safe_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);


UCL_EXTERN(int) ucl_nrv2b_decompress_asm_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_safe_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_fast_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_fast_safe_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_small_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_small_safe_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);


UCL_EXTERN(int) ucl_nrv2b_decompress_asm_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_safe_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_fast_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_fast_safe_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_small_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_asm_small_safe_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);


/***********************************************************************
// NRV2D assembly decompressors
************************************************************************/

UCL_EXTERN(int) ucl_nrv2d_decompress_asm_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_safe_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_fast_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_fast_safe_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_small_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_small_safe_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);


UCL_EXTERN(int) ucl_nrv2d_decompress_asm_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_safe_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_fast_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_fast_safe_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_small_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_small_safe_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);


UCL_EXTERN(int) ucl_nrv2d_decompress_asm_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_safe_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_fast_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_fast_safe_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_small_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_asm_small_safe_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);


/***********************************************************************
// NRV2E assembly decompressors
************************************************************************/

UCL_EXTERN(int) ucl_nrv2e_decompress_asm_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_safe_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_fast_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_fast_safe_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_small_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_small_safe_8
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);


UCL_EXTERN(int) ucl_nrv2e_decompress_asm_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_safe_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_fast_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_fast_safe_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_small_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_small_safe_le16
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);


UCL_EXTERN(int) ucl_nrv2e_decompress_asm_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_safe_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_fast_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_fast_safe_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_small_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_asm_small_safe_le32
                                (const ucl_bytep src, ucl_uint  src_len,
                                       ucl_bytep dst, ucl_uintp dst_len,
                                       ucl_voidp wrkmem);


/***********************************************************************
// checksum and misc functions
************************************************************************/

UCL_EXTERN(ucl_uint32)
ucl_crc32_asm(ucl_uint32 _c, const ucl_bytep _buf, ucl_uint _len,
              const ucl_uint32p _crc_table);

UCL_EXTERN(ucl_uint32)
ucl_crc32_asm_small(ucl_uint32 _c, const ucl_bytep _buf, ucl_uint _len);

UCL_EXTERN(int)
ucl_cpuid_asm(ucl_uint32p /* ucl_uint32 info[16] */ );

UCL_EXTERN(unsigned)
ucl_rdtsc_asm(ucl_uint32p /* ucl_uint32 ticks[2] */ );
UCL_EXTERN(unsigned)
ucl_rdtsc_add_asm(ucl_uint32p /* ucl_uint32 ticks[2] */ );


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */

