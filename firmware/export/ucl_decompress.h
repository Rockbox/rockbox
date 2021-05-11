/* Standalone UCL decompressor and uclpack-compatible unpacker,
 * adapted for Rockbox from the libucl code.
 *
 * Copyright (C) 1996-2002 Markus Franz Xaver Johannes Oberhumer
 * All Rights Reserved.
 *
 * See firmware/common/ucl_decompress.c for full copyright notice
 */
#ifndef UCL_DECOMPRESS_H
#define UCL_DECOMPRESS_H

#include <stdint.h>
#include <stdlib.h>

#define UCL_E_OK                 0
#define UCL_E_INPUT_OVERRUN      1
#define UCL_E_OUTPUT_OVERRUN     2
#define UCL_E_LOOKBEHIND_OVERRUN 3
#define UCL_E_INPUT_NOT_CONSUMED 4
#define UCL_E_BAD_MAGIC          5
#define UCL_E_BAD_BLOCK_SIZE     6
#define UCL_E_UNSUPPORTED_METHOD 7
#define UCL_E_TRUNCATED          8
#define UCL_E_CORRUPTED          9

extern int ucl_nrv2e_decompress_8(const uint8_t* src, uint32_t src_len,
                                  uint8_t* dst, uint32_t* dst_len);

extern int ucl_unpack(const uint8_t* src, uint32_t src_len,
                      uint8_t* dst, uint32_t* dst_len);

#endif /* UCL_DECOMPRESS_H */
